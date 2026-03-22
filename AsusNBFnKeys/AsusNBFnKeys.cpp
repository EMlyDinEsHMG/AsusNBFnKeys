/*
 *  Copyright (c) EMlyDinEsH (mg-dinesh@live.com) 2012-2018. All rights reserved.
 *
 *  Asus Notebooks Fn keys driver v2.6 for Mac OSX
 *
 *  Credits: Hotkoffy(insanelymac) for WMI source and ideas
 *
 *  AsusNBFnKeys.cpp
 *  
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "AsusNBFnKeys.h"

#define ENABLE_DEBUG 0

#if ENABLE_DEBUG
#define DEBUG_LOG(fmt, args...) IOLog(fmt, ## args)
#else
#define DEBUG_LOG(fmt, args...)
#endif


#define super IOService

OSDefineMetaClassAndStructors(AsusNBFnKeys, IOService)

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Define device power states for System power management
// by Overriding IOService function which sets the device power state.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

enum
{
	kPowerStateOff = 0,
	kPowerStateOn,
	kPowerStateCount
};

static IOPMPowerState powerStateArray[ kPowerStateCount ] =
{
	{ 1,0,0,0,0,0,0,0,0,0,0,0 },
	{ 1,IOPMDeviceUsable,IOPMPowerOn,IOPMPowerOn,0,0,0,0,0,0,0,0 }
};

IOReturn AsusNBFnKeys::setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker)
{
	if (kPowerStateOff == powerStateOrdinal)
	{
		DEBUG_LOG("%s: Device power state set to OFF\n", this->getName());
        
        // Disable ALS timer
        if(_als.enabled) {
            CancelTimerTimeout(_als.timer);
            _als.processing = false;
        }
	}
	else if (kPowerStateOn == powerStateOrdinal)
	{
        DEBUG_LOG("%s: Device power state set to ON\n", this->getName());
        
        //
        // Set the keyboard string which is displayed on Keyboard settings
        //
        
        IOSleep(1000); // wait a sec after power state change
        
		_keyboardDevice->keyPressed(0);
        
        //
        // Restore keyboard backlight on power transitions
        //
        
        if(_kBLight.exists && _kBLight.level >= 0) {
            
            SetkeyboardBackLight(_kBLight.level);
            
            // Trigger timeout
            TriggerAutoAsusBacklightOff();
        }
        
        // Enable ALS timer back
        if(_als.enabled) {
            SetTimerTimeout(_als.timer, _als.timerInterval);
        }
	}
	
	return IOPMAckImplied;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit function which initializes the driver
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool AsusNBFnKeys::init(OSDictionary *dict)
{
    DEBUG_LOG("AsusNBFnKeys: Init\n");

    _keyboardDevice             = NULL;
    
    _display.deviceEntry        = NULL;
    _display.asusBrightnessLvl  = 0;
    _display.brighntessLevel    = 16;
    
    _kBLight.level          = -1;
    _kBLight.offTimeout     = 10000;
    _kBLight.has16Lvls      = false;
    _kBLight.exists         = false;
    _kBLight.idleOff        = true;
    
    _display.brightness         = 0x400;
    _display.backlightOn        = true;
    _display.dimOnBatAC         = false;
    
    _als.enabled                = false;
    _als.disableDispBritCtrl    = false;
    
    _touchPadEnabled    = true;
    _hasMediaButtons    = true;

	return super::init(dict);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit function which finds the device needed to attach
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOService * AsusNBFnKeys::probe(IOService *provider, SInt32 *score)
{
    DEBUG_LOG("%s: Probe\n", this->getName());

	IOService *result = super::probe(provider, score);
    
    //
    // Search the ATKD device in DSDT to attach driver
    //
    
    if(IOACPIPlatformDevice *devATK = OSDynamicCast(IOACPIPlatformDevice, provider))
    {
        OSObject * obj;
        OSString * devUID;
        
        // Get the UID for the attaching device
        devATK->evaluateObject("_UID", &obj);
        
        devUID = OSDynamicCast(OSString, obj);
        
        DEBUG_LOG("%s: Checking Asus UID ...\n", this->getName());
        
        // UID is NULL (or) not an ATK?
        if (devUID == NULL || (devUID && !(devUID->isEqualTo("ATK"))))
        {
            //
            // check for the presence of ATKD device
            //
            
            if(!strncmp(devATK->getName(),"ATKD",strlen(devATK->getName()))
               && kIOReturnSuccess == devATK->validateObject("INIT"))
            {
                DEBUG_LOG("%s: Found Asus Hotkey device ATKD.\n", this->getName());
            }
            else {
                DEBUG_LOG("%s: Not an Asus Hotkey device.\n", this->getName());
                result = NULL;
            }
        }
        else
            DEBUG_LOG("%s: Found Asus Hotkey device.\n", this->getName());
        
        OSSafeRelease(devUID);
    }
    else {
        IOLog("%s: Asus Hotkey device not found.\n", this->getName());
        return NULL;
    }

    //
    // Read preferences configured in the plist from IOReg
    //
    
    if(result != NULL) {
        
        if (OSDictionary *confg = OSDynamicCast(OSDictionary, getProperty("Preferences")))
        {
            if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, confg->getObject(HasMediaFnKeys))) {
                _hasMediaButtons = kBoolVal->getValue();
            }
            if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, confg->getObject(MediaKeysDoFastRewind))) {
                _mediaDoesFastRewind = kBoolVal->getValue();
            }
            if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, confg->getObject(SoftDisplayBacklightOff))) {
                _display.softBackLightOff = kBoolVal->getValue();
            }
            if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, confg->getObject(DimBacklightOnACDC))) {
                _display.dimOnBatAC = kBoolVal->getValue();
            }
            if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, confg->getObject(IdleKBacklightAutoOff))) {
                _kBLight.hasAutoShutOff = kBoolVal->getValue();
            }
            if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, confg->getObject(FinerBrightnessControl))) {
                _useFinerBrightnessControl = kBoolVal->getValue();
            }
            if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, confg->getObject(FinerVolumeControl))) {
                _useFinerVolumeControl = kBoolVal->getValue();
            }
            if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, confg->getObject(MakeFnKeysAsFunction)))
            {
                _workAsFunctionKeys = kBoolVal->getValue();
            }
            
            if(OSNumber *kNumVal = OSDynamicCast(OSNumber, confg->getObject(KBackLightlvlAtBoot))) {
                _kBLight.level = kNumVal->unsigned8BitValue();
            }
            if(OSNumber *kNumVal = OSDynamicCast(OSNumber, confg->getObject(F3KeyFunction))) {
                _f3KeyAction = kNumVal->unsigned8BitValue();
            }
            if(OSNumber *kNumVal = OSDynamicCast(OSNumber, confg->getObject(F4KeyFunction))) {
                _f4KeyAction = kNumVal->unsigned8BitValue();
            }
            if(OSNumber *kNumVal = OSDynamicCast(OSNumber, confg->getObject(InstantKeyFunction))) {
                _instantKeyAction = kNumVal->unsigned8BitValue();
            }
            if(OSNumber *kNumVal = OSDynamicCast(OSNumber, confg->getObject(IdleKBacklightAutoOffTimeout)))
            {
                _kBLight.offTimeout = kNumVal->unsigned32BitValue();
            }
            if(OSNumber *kNumVal = OSDynamicCast(OSNumber, confg->getObject(DimBrightnessByLevels))) {
                _display.dimBLevels = kNumVal->unsigned8BitValue();
            }
            
            // Read ALS Configuration
            if (OSDictionary *ALSconfg = OSDynamicCast(OSDictionary, confg->getObject(ConfigForALS)))
            {
                if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, ALSconfg->getObject(EnableALSLogs)))
                {
                    _als.logsEnabled = kBoolVal->getValue();
                }
                if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, ALSconfg->getObject(EnableAtBoot)))
                {
                    _als.atBoot = kBoolVal->getValue();
                }
                if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, ALSconfg->getObject(DisableOnAC)))
                {
                    _als.disableOnAC = kBoolVal->getValue();
                }
                if(OSBoolean *kBoolVal = OSDynamicCast(OSBoolean, ALSconfg->getObject(DisableOnFnControl)))
                {
                    _als.disableOnFnCtrl = kBoolVal->getValue();
                }
                
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(TimerInterval)))
                {
                    _als.timerInterval = kNumVal->unsigned32BitValue();
                }
                
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level1RangeStart)))
                {
                    _als.levelRStart[0] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level2RangeStart)))
                {
                    _als.levelRStart[1] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level3RangeStart)))
                {
                    _als.levelRStart[2] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level4RangeStart)))
                {
                    _als.levelRStart[3] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level5RangeStart)))
                {
                    _als.levelRStart[4] = kNumVal->unsigned32BitValue();
                }
                
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level1RangeEnd)))
                {
                    _als.levelREnd[0] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level2RangeEnd)))
                {
                    _als.levelREnd[1] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level3RangeEnd)))
                {
                    _als.levelREnd[2] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level4RangeEnd)))
                {
                    _als.levelREnd[3] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level5RangeEnd)))
                {
                    _als.levelREnd[4] = kNumVal->unsigned32BitValue();
                }
                
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level1Brightness)))
                {
                    _als.brightnessForLevel[0] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level2Brightness)))
                {
                    _als.brightnessForLevel[1] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level3Brightness)))
                {
                    _als.brightnessForLevel[2] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level4Brightness)))
                {
                    _als.brightnessForLevel[3] = kNumVal->unsigned32BitValue();
                }
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(Level5Brightness)))
                {
                    _als.brightnessForLevel[4] = kNumVal->unsigned32BitValue();
                }
                
                if(OSNumber *kNumVal = OSDynamicCast(OSNumber, ALSconfg->getObject(SamplesToProcess)))
                {
                    _als.samples = kNumVal->unsigned8BitValue();
                }
            }
        }
    }
    
    return result;
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOKit functions which starts and stops the driver
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool AsusNBFnKeys::start(IOService *provider)
{
    IOLog("Asus Notebooks Fn Keys v2.6 Copyright (c) EMlyDinEsH <www.osxlatitude.com> 2012-2018.\n");

	if(!provider || !super::start( provider ))
	{
		IOLog("%s: Error loading kext\n", this->getName());
		return false;
	}
	
    DEBUG_LOG("%s: Start\n", this->getName());

	_wmiDevice = (IOACPIPlatformDevice *) provider;
	
	//
    // Get the Darwin Kernel version for detecting OSX version
    //
    
    IORegistryEntry *ioregRoot = IORegistryEntry::getRegistryRoot();
    OSString *iokitbuildObj = 0;
    const char * iokitbuildStr = nullptr;
    char buildVersString[8] = {NULL};
    int index = 0, strCount = 0;
    
    if (ioregRoot != NULL) {
        
        iokitbuildObj = OSDynamicCast(OSString, ioregRoot->getProperty("IOKitBuildVersion"));
        
        if(iokitbuildObj != NULL) {
            
            DEBUG_LOG("%s: IOKitBuildVersion %s\n", this->getName() ,iokitbuildObj->getCStringNoCopy());
            iokitbuildStr = iokitbuildObj->getCStringNoCopy();
            
            do {
                if(*iokitbuildStr == ' ' ||
                   *iokitbuildStr == '\0' ||
                   *iokitbuildStr == ':')
                {
                    
                    if(*iokitbuildStr == ':')
                    {
                        buildVersString[index] = '\0';
                    }
                    
                    index = 0;
                    strCount++;
                    
                    if(strCount == 4)
                    {
                        DEBUG_LOG("%s: OSX Darwin Kernel version - %s\n", this->getName(), buildVersString);
                        break;
                    }
                    continue;
                }
                
                buildVersString[index] = *iokitbuildStr;
                index++;
                
            } while (*iokitbuildStr++);
            
            //
            // Set the ioreg PCI Bridge name
            // because 10.9.3 (Darwin 13.2.0) or later use new name
            //
            
            if(strncmp(buildVersString, "13.2.0", strlen(buildVersString)) >= 0) {
                snprintf(_pciBridgeName, sizeof(_pciBridgeName),"IOPP");
            }
            else {
                snprintf(_pciBridgeName, sizeof(_pciBridgeName),"IOPCI2PCIBridge");
            }
        }
        else
            IOLog("%s: Failed to get IOKitBuildVersion\n", this->getName());
        
        // ioregRoot->release();
    }
    else
    {
        IOLog("%s: Failed to get object for IOReg root\n", this->getName());
        return false;
    }
    
    //
	// Parse the WMI properties and add them to IOReg (needed for EEEPC)
    //
    
	parse_wdg();
	
    //
    // Enable ATK events
    //
    
	if(!enableATKEvents())
        return false;
    
    //
    // Setup workloop for timers
    //
    
    _bWorkLoop = getWorkLoop();

    if(!_bWorkLoop)
        return false;
    
    //
    // Setup Keyboard backlight timer
    //
    
    if(_kBLight.hasAutoShutOff && _kBLight.exists) {
        
        _kBLight.timer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &AsusNBFnKeys::ControlfKeybrdBackLight));
                
        if(!(_bWorkLoop->addEventSource(_kBLight.timer) == kIOReturnSuccess))
            return false;
    }
    
    //
    // Setup ALS timer
    //
    
    if(_als.exists) {
        
        _als.timer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &AsusNBFnKeys::ProcessALS));
        
        if(!(_bWorkLoop->addEventSource(_als.timer) == kIOReturnSuccess))
            return false;
        
        if(_als.enabled) {
            SetTimerTimeout(_als.timer, _als.timerInterval);
        }
    }
    
    //
    // Setup screen backlight timer
    //
    
    _brightnessTimer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &AsusNBFnKeys::ControlScreenBrightness));
    
    if(!(_bWorkLoop->addEventSource(_brightnessTimer) == kIOReturnSuccess))
        return false;
    
    //
    // Work as Function keys ?
    //
    
    if(_workAsFunctionKeys)
        setProperty("FunctionKeysMode", true);
    else
        setProperty("FunctionKeysMode", false);
    
	PMinit();
    registerPowerDriver(this, powerStateArray, 2);
	provider->joinPMtree(this);
	
	registerService();

	return true;
}


void AsusNBFnKeys::stop(IOService *provider)
{
	DEBUG_LOG("%s: Stop\n", this->getName());

    if(_isEEEpcWMI)
        enable_EEEPC_Events(false);

	if (_keyboardDevice)
	{
		_keyboardDevice->release();
		_keyboardDevice = NULL;
	}
    
    if(_kBLight.timer)
    {
        CancelTimerTimeout(_kBLight.timer);
        _bWorkLoop->removeEventSource(_kBLight.timer);
        _kBLight.timer->release();
        _kBLight.timer = 0;
    }
    
    if(_als.timer)
    {
        CancelTimerTimeout(_als.timer);
        _bWorkLoop->removeEventSource(_als.timer);
        _als.timer->release();
        _als.timer = 0;
    }
    
    if(_brightnessTimer)
    {
        CancelTimerTimeout(_brightnessTimer);
        _bWorkLoop->removeEventSource(_brightnessTimer);
        _brightnessTimer->release();
        _brightnessTimer = 0;
    }
    
    OSSafeReleaseNULL(_bWorkLoop);
    
	PMstop();
	
	super::stop(provider);
    
	return;
}

void AsusNBFnKeys::free(void)
{
	DEBUG_LOG("%s: Free\n", this->getName());
	super::free();
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Override IOService function which handles the message
// sent by the attached device (provider).
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

IOReturn AsusNBFnKeys::message( UInt32 type, IOService * provider, void * argument)
{
    DEBUG_LOG("%s: ACPI Message, Type %x Provider %s, Event %d \n",
              this->getName(), uint(type), provider->getName(), *((UInt32 *) argument));

	if (type == kIOACPIMessageDeviceNotification)
	{
		UInt32 event = *((UInt32 *) argument);
		OSObject * wed;
		
        //
        // Need to parse _WED to get the event value
        //
        
        if(kIOReturnSuccess == _wmiDevice->validateObject("_WED"))
        {
            OSNumber * number = OSNumber::withNumber(event,32);
            _wmiDevice->evaluateObject("_WED", &wed, (OSObject**)&number,1);
            number->release();
            number = OSDynamicCast(OSNumber, wed);
            if (NULL == number)
            {
                // try a package
                OSArray * array = OSDynamicCast(OSArray, wed);
                if (NULL == array)
                {
                    // try a buffer
                    OSData * data = OSDynamicCast(OSData, wed);
                    if ( (NULL == data) || (data->getLength() == 0))
                    {
                        DEBUG_LOG("%s: Fail to cast _WED returned objet %s\n", this->getName(), wed->getMetaClass()->getClassName());
                        return kIOReturnError;
                    }
                    const char * bytes = (const char *) data->getBytesNoCopy();
                    number = OSNumber::withNumber(bytes[0],32);
                }
                else
                {
                    number = OSDynamicCast(OSNumber, array->getObject(0));
                    if (NULL == number)
                    {
                        DEBUG_LOG("%s: Fail to cast _WED returned 1st objet in array %s\n", this->getName(), array->getObject(0)->getMetaClass()->getClassName());
                        return kIOReturnError;
                    }
                }
            }
            
            event = number->unsigned32BitValue();
            
            DEBUG_LOG("%s: ACPI Message, Event %d \n", this->getName(),  event);
        }
        else {
            //
            // Could be event values are received directly
            // which happens in some old models (or) something else.
            //
        }
        
        handleMessage(event);
	}
	else
	{	// Someone unexpected is sending us messages!
		DEBUG_LOG("%s: Unexpected message, Type %x Provider %s \n", this->getName(), uint(type), provider->getName());
	}
	
	return kIOReturnSuccess;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Enable and initialize Asus ATK Events
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

bool AsusNBFnKeys::enableATKEvents()
{
    //
    // Call Asus WMI method 'INIT' from DSDT to enable ATK events
    //
    
     if(_wmiDevice->evaluateObject("INIT", NULL, NULL, NULL) != kIOReturnSuccess)
         return false;
    
    //
    // Check Asus WMI method 'IANE' for detecting EEEPC and older models,
    // because this method is not present in old and EEEPC models.
    //
    
    if(kIOReturnSuccess != _wmiDevice->validateObject("IANE"))
        _isEEEpcWMI = true;
    else
        _isEEEpcWMI = false;
    
    //
    // Call additional Asus WMI Method to enable ATK events in old notebooks
    //
    
    if(_isEEEpcWMI) {
        if(!enable_EEEPC_Events(true))
            return false;
    }
    
    //
    // Create HIKeyboardDevice for Apple Special functions
    //
    
    _keyboardDevice = new FnKeysHIKeyboardDevice;
    
    if (!_keyboardDevice              ||
        !_keyboardDevice->init()      ||
        !_keyboardDevice->attach(this) )
    {
        OSSafeRelease(_keyboardDevice);
        IOLog("%s: Error creating keyboardDevice\n", this->getName());
        return false;
    }
    else
    {        
        _keyboardDevice->registerService();
        
        // Set the touchpad to enabled state in ioreg
        setProperty("TouchpadEnabled", true);
        
        //
        // Check for the presence of keyboard backlight
        // by looking for the method 'SKBL' in DSDT
        //
        
        if(kIOReturnSuccess == _wmiDevice->validateObject("SKBL"))
        {
            _kBLight.exists = true;
            
            DEBUG_LOG("%s: Keyboard backlight level from plist %d\n", this->getName(), _kBLight.level);

            // Load Keyboard backlight value from NVRAM if the plist value is -ve/0xFF
            if(_kBLight.level < 0 || _kBLight.level == 0xFF)
            {
                LoadValuesFromNVRAM();
                
                // invalid NVRAM value ? set the level to 1
                if(_kBLight.level < 0 && _kBLight.level > 15)
                    _kBLight.level = 1;
            }
            
            DEBUG_LOG("%s: Keyboard backlight level set to %d at boot\n", this->getName(), _kBLight.level);
            SetkeyboardBackLight(_kBLight.level);
        }
        else {
            DEBUG_LOG("%s: Keyboard backlight method 'SKBL' not found, support disabled.\n", this->getName());
            _kBLight.exists = false;
        }
        
        //
        // Check for the presence of 16 keyboard backlight levels
        // by looking for the buffer 'KBPW' in DSDT
        //
        
        if(kIOReturnSuccess == _wmiDevice->validateObject("KBPW"))
        {
            _kBLight.has16Lvls = true;
        }
        else {
            _kBLight.has16Lvls = false;
        }
        
        //
        // Check for the presence of ALS sensor
        // by looking for the method 'ALSC' in DSDT
        //
        
        if(kIOReturnSuccess == _wmiDevice->validateObject("ALSC"))
        {
            _als.exists = true;
            
            // Enable ALS at boot?
            if(_als.atBoot)
                enableALS(true);
        }
        else {
            _als.exists = false;
            DEBUG_LOG("%s: ALS sensor method 'ALSC' not found, support disabled.\n", this->getName());
        }
        
        IOLog("%s: Fn Hotkey events Enabled.\n", this->getName());

    }
    
	return true;
}

bool AsusNBFnKeys::enable_EEEPC_Events(bool enable)
{
    bool result = true;

    char method[5];
    OSNumber *num;
    OSDictionary *dict = getDictByUUID(ASUS_EEEPC_WMI_EVENT_GUID);
    if (NULL == dict) {
        DEBUG_LOG("%s: Failed to find WMI Event GUID, Assuming ATKD reporting events directly.\n", this->getName());
        return true;
    }
    
    num = OSDynamicCast(OSNumber, dict->getObject("notify_value"));
    if (NULL == num) {
        IOLog("%s: Failed to find WMI Event notifier value.\n", this->getName());
        return false;
    }
    
    snprintf(method, 5, "WE%02X", num->unsigned8BitValue());
    
    DEBUG_LOG("%s: WE%02X\n", this->getName(), num->unsigned8BitValue());

    OSObject * res;
    OSNumber * number = OSNumber::withNumber(enable, 32);
    if(_wmiDevice->evaluateObject(method, &res, (OSObject**)&number,1) != kIOReturnSuccess)
    {
        result = false;
        DEBUG_LOG("%s: Failed to enable EEEPC WMI events\n", this->getName());
    }
    else {
        DEBUG_LOG("%s: Sucessfully enbled EEEPC WMI events.\n", this->getName());
    }
    
    number->release();
    
    return result;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Process and Dispatch Fn key events reported by the device to OSX
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::handleMessage(int fnKeyCode)
{
    _display.blvlsToMove =  0;

    DEBUG_LOG("%s: Received Asus WMI Fn Key code %d(0x%x)\n",this->getName(), fnKeyCode, fnKeyCode);
    
    // Process the fnKeyCode
	switch (fnKeyCode)
    {
        /* 
         * _Q0A Method in DSDT for Sleep F1 which is not sent here
         * _Q0B Method in DSDT for WiFi/BT F2
         */
        case 0x5E: // WiFI on
        case 0x5F: // WiFI off
            // Make this Function key F1 based on my patch
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x7a;
                DEBUG_LOG("%s: Dispatched F1\n",this->getName());
            }
            // Skip further processing
            else {
                fnKeyCode = 0;
                DEBUG_LOG("%s: Ignoring WiFi %s\n",this->getName(), (fnKeyCode == 0x5E)?"On":"Off");
            }
            break;
            
        case 0x7D: // BT on
        case 0x7E: // BT off
            
            // Make this Function key F2 based on my patch
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x78;
                DEBUG_LOG("%s: Dispatched F2\n",this->getName());
            }
            // Skip further processing
            else {
                fnKeyCode = 0;
                DEBUG_LOG("%s: Ignoring Bluetooth %s\n",this->getName(), (fnKeyCode == 0x5E)?"On":"Off");
            }
            break;
           
        /* _Q0C Method in DSDT for F3 */
        case 0x50: // Does nothing, but we map this to OSX Function
        case 0xC5: // Decreases Keyboard Backlight
            
            // Make this Function key
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x63;
                DEBUG_LOG("%s: Dispatched F3\n",this->getName());
            }
            // Control Keyboard backlight
            else if(_kBLight.exists)
            {
                if(ReadkeyboardBacklight(0) == kIOReturnSuccess)
                {
                    if(_kBLight.level > 0)
                        _kBLight.level--;
                    else
                        _kBLight.level = 0;
                    
                    // Disable ALS timer
                    if(_als.exists && _als.enabled)  {
                        CancelTimerTimeout(_als.timer);
                        
                        if(_als.disableOnFnCtrl)
                            enableALS(false);
                    }
                    
                    SetkeyboardBackLight(_kBLight.level);
                    
                    // Enable ALS timer back
                    if(_als.exists && _als.enabled && !_als.disableOnFnCtrl)
                    {
                        SetTimerTimeout(_als.timer, _als.timerInterval);
                    }
                    
                    DEBUG_LOG("%s: Dispatched Keyboard backlight level decrease to %d\n", this->getName(), _kBLight.level);
                    
                    // Store value in ioregistry
                    setProperty("KeyboardBLightLevel", _kBLight.level, 32);
                }                
                
                fnKeyCode = 0; // skip further processing
            }
            // Work as OSX Function
            else {
                switch (_f3KeyAction) {
                    case 0:
                    case 2:
                        fnKeyCode = NOTIFY_MISSON_CNTL;
                        break;
                    case 1:
                        fnKeyCode = NOTIFY_LAUNCHPAD;
                        break;
                    case 3:
                        fnKeyCode = NOTIFY_DASHBOARD;
                        break;
                    case 4:
                        fnKeyCode = NOTIFY_DESKTOP;
                        break;
                    case 5:
                        fnKeyCode = NOTIFY_APPWINDS;
                        break;
                    case 6:
                        fnKeyCode = NOTIFY_EJECT;
                        break;
                        
                    default:
                        break;
                }
                
                if(_mediaDoesFastRewind)
                    fnKeyCode = NOTIFY_DASHBOARD;
                
                DEBUG_LOG("%s: Dispatch OSX Function %d\n", this->getName(), _f3KeyAction);
            }
            break;
            
        /* _Q0D Method in DSDT for F4 */
        case 0x51: // Does nothing, but we map it to OSX Function
        case 0xC4: // Increases Keyboard Backlight
            
            // Make this Function key?
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x76;
                DEBUG_LOG("%s: Dispatched F4\n",this->getName());
            }
            // Control keyboard backlight
            else if(_kBLight.exists)
            {
                if(ReadkeyboardBacklight(0) == kIOReturnSuccess)
                {
                    
                    if(_kBLight.has16Lvls && _kBLight.level == 15) // 16 levels
                        _kBLight.level = 15;
                    else if(!_kBLight.has16Lvls && _kBLight.level == 3) // 4 levels
                        _kBLight.level = 3;
                    else
                        _kBLight.level++;
                    
                    // Disable ALS timer
                    if(_als.exists && _als.enabled)  {
                        CancelTimerTimeout(_als.timer);
                        
                        if(_als.disableOnFnCtrl)
                            enableALS(false);
                    }
                    
                    SetkeyboardBackLight(_kBLight.level);
                    
                    // Enable ALS timer back
                    if(_als.exists && _als.enabled && !_als.disableOnFnCtrl)
                    {
                        SetTimerTimeout(_als.timer, _als.timerInterval);
                    }
                    
                    DEBUG_LOG("%s: Dispatched Keyboard backlight level increase to %d\n", this->getName(), _kBLight.level);
                    
                    // Store value in ioregistry
                    setProperty("KeyboardBLightLevel", _kBLight.level,32);
                }                
                
                fnKeyCode = 0; // skip further processing
            }
            // Work as OSX Function
            else {
                switch (_f4KeyAction) {
                    case 0:
                    case 1:
                        fnKeyCode = NOTIFY_LAUNCHPAD;
                        break;
                    case 2:
                        fnKeyCode = NOTIFY_MISSON_CNTL;
                        break;
                    case 3:
                        fnKeyCode = NOTIFY_DASHBOARD;
                        break;
                    case 4:
                        fnKeyCode = NOTIFY_DESKTOP;
                        break;
                    case 5:
                        fnKeyCode = NOTIFY_APPWINDS;
                        break;
                    case 6:
                        fnKeyCode = NOTIFY_EJECT;
                        break;
                        
                    default:
                        break;
                }
                
                if(_mediaDoesFastRewind)
                    fnKeyCode = NOTIFY_DESKTOP;
                
                DEBUG_LOG("%s: Dispatch OSX Function %d\n", this->getName(), _f4KeyAction);
            }
            break;
            
        /* _Q10 Method in DSDT for F7 */
        case 0x33: // Hardwired backlight On
        case 0x34: // Hardwired backlight Off
        case 0x35: // Soft Event backlight
            
            // Using Asus backlight driver?
            if(!_display.usingAsusBackLight)
                CheckAsusBacklight();
            
            // Make this Function key?
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x62;
                DEBUG_LOG("%s: Dispatched F7\n",this->getName());
            }
            
            //
            // Skip  processing
            // because it has native backlight on/off support
            //

            else if(_display.usingAsusBackLight) {
                
                fnKeyCode = 0; 
                
                if(fnKeyCode == 0x33)
                    _display.backlightOn = true;
                
                else if(fnKeyCode == 0x34)
                    _display.backlightOn = false;
            }
            
            //
            // Process soft backlight on/off events
            //
            
            else if(_display.softBackLightOff)
            {
                if(_display.backlightOn)
                {
                    fnKeyCode = NOTIFY_BRIGHTNESS_DOWN;
                    _display.blvlsToMove = 16;
                    
                    // Read Panel brigthness value to restore on next toggle
                    ReadApplePanelBrightnessValue();
                    
                    _display.backlightOn = false;
                }
                else
                {
                    if(_useFinerBrightnessControl)
                        fnKeyCode = NOTIFY_BRIGHTNESS_UP_MIN;
                    else
                        fnKeyCode = NOTIFY_BRIGHTNESS_UP;
                    
                    // Got panel brightenss level ?
                    if(_display.brighntessLevel > 0) {
                        _display.blvlsToMove = _display.brighntessLevel;
                    }
                    else {
                        // set brightnesss to middle
                        _display.brighntessLevel = _display.blvlsToMove = 8; 
                    }
                    _display.backlightOn = true;
                }
                DEBUG_LOG("%s: Dispatch Soft backlight %s\n", this->getName(), _display.backlightOn?"On":"Off");
                
                _fnKeyAction = fnKeyCode;
                CancelTimerTimeout(_brightnessTimer);
                SetTimerTimeout(_brightnessTimer, 0);
            }
            fnKeyCode = 0; // skip further processing
			break;
          
        /* _Q11 Method in DSDT for F8 */
        case 0x61:// Video Mirror
            
            // Make this Function key?
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x64;
            }
            else {
                fnKeyCode = NOTIFY_VID_MIRROR;
            }
            DEBUG_LOG("%s: Dispatched %s\n",this->getName(), _workAsFunctionKeys?"F8":"Video mirror");
            break;
            
            
        /* _Q12 Method in DSDT for F9 */
		case 0x6B: // Tochpad On/Off
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x65;
                DEBUG_LOG("%s: Dispatched F9\n",this->getName());
            }
            else {
                _touchPadEnabled = !_touchPadEnabled;
                if(_touchPadEnabled)
                {
                    setProperty("TouchpadEnabled", true);
                    removeProperty("TouchpadDisabled");
                }
                else
                {
                    removeProperty("TouchpadEnabled");
                    setProperty("TouchpadDisabled", true);
                }
                
                DEBUG_LOG("%s: Dispatched Touchpad %s state\n",this->getName(), _touchPadEnabled?"Enabled":"Disabled");
                
                fnKeyCode = 0; // skip further processing
            }
			break;
          
        /* _Q13 Method in DSDT for F10 */
        case 0x32:// Volume Mute
           
            // Make this Function key
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x6d;
            }
            else {
                fnKeyCode = NOTIFY_VOL_MUTE;
            }
            DEBUG_LOG("%s: Dispatched %s\n",this->getName(), _workAsFunctionKeys?"F10":"Volume mute");
            break;
            
        /* _Q14 Method in DSDT for F11 */
        case 0x31:// Volume Down
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x67;
                DEBUG_LOG("%s: Dispatched F3\n",this->getName());
            }
            else if(_useFinerVolumeControl)
                fnKeyCode = NOTIFY_VOL_DOWN_MIN;
            else {
                fnKeyCode = NOTIFY_VOL_DOWN;
                // little delay for key hold to get smooth slow transition
                IOSleep(50); 
            }
            DEBUG_LOG("%s: Dispatched %s\n",this->getName(), _workAsFunctionKeys?"F11":"Volume down");
            break;
            
        /* _Q15 Method in DSDT for F12 */   
        case 0x30: // Volume Up
           
            // Make this Function key
            if(_workAsFunctionKeys) {
                fnKeyCode = 0x6f;
            }
            else if(_useFinerVolumeControl)
                fnKeyCode = NOTIFY_VOL_UP_MIN;
            else {
                fnKeyCode = NOTIFY_VOL_UP;
                // little delay for key hold to get smooth slow transition
                IOSleep(50); 
            }
            DEBUG_LOG("%s: Dispatched %s\n",this->getName(), _workAsFunctionKeys?"F12":"Volume up");
            break;
            
        /* _Q73 and _Q6D Method in DSDT for C and Right Arrow */
        case 0x82: // C (for Ultrabooks)
        case 0x41: // Next media
            
            //
            // Control fast/rewind between C and Right Arrow
            //
            
            if((_mediaDoesFastRewind && fnKeyCode == 0x41) ||
               (_mediaDoesFastRewind && fnKeyCode == 0x82 && !_hasMediaButtons) ||
               (!_mediaDoesFastRewind && fnKeyCode == 0x82 && _hasMediaButtons))
            {
                if(_didMediaForward) {
                    fnKeyCode = NOTIFY_FAST_REL;
                    _didMediaForward = false;
                } else {
                    fnKeyCode = NOTIFY_MEDIA_FAST;
                    _didMediaForward = true;
                }
            }
            else
                fnKeyCode = NOTIFY_MEDIA_NEXT;
            
            DEBUG_LOG("%s: Dispatch Media %s\n", this->getName(), (fnKeyCode == NOTIFY_MEDIA_NEXT)?"Next":"Fast");
            break;
           
        /* _Q72 and _Q6C Method in DSDT for V and Left Arrow */
        case 0x8A:// V (for Ultrabooks)
        case 0x40:// Previous media
            
            //
            // Control fast/rewind between V and Left Arrow
            //
            
            if((_mediaDoesFastRewind && fnKeyCode == 0x40) ||
               (_mediaDoesFastRewind && fnKeyCode == 0x8A && !_hasMediaButtons) ||
               (!_mediaDoesFastRewind && fnKeyCode == 0x8A && _hasMediaButtons))
            {
                if(_didMediaRewind) {
                    fnKeyCode = NOTIFY_REWIND_REL;
                    _didMediaRewind = false;
                } else {
                    fnKeyCode = NOTIFY_MEDIA_REWIND;
                    _didMediaRewind = true;
                }
            }
            else
                fnKeyCode = NOTIFY_MEDIA_PREV;
            
            DEBUG_LOG("%s: Dispatch Media %s\n", this->getName(), (fnKeyCode == NOTIFY_MEDIA_PREV)?"Previous":"Rewind");
            break;
         
        /* _Q71 and _Q6F Method in DSDT for Space and Down Arrow */
        case 0x5C: // Space (for Ultrabooks)
        case 0x45: // Play media
            
            if(!_didMediaRewind && !_didMediaForward)
                fnKeyCode = NOTIFY_MEDIA_PLAY;
            else
            {
                if(_didMediaRewind){
                    _keyboardDevice->keyPressed(NOTIFY_REWIND_REL);
                    _didMediaRewind = false;
                }
                if(_didMediaForward) {
                    _keyboardDevice->keyPressed(NOTIFY_FAST_REL);
                    _didMediaForward = false;
                }
            }
            
            DEBUG_LOG("%s: Dispatch Media Play up\n", this->getName());
            break;
          
        /* 
         * _Q6E Method in DSDT for Up Arrow
         * Toggles Fast/Rewind and Prev/Next actions for the arrow left, right keys and C, V keys
         * Toggles F3 and F4 keys functions
         * Toggles Finger brightness and Volume controls
         */
        case 0x43: // Stop media
            _mediaDoesFastRewind = !_mediaDoesFastRewind;
            _useFinerBrightnessControl = !_useFinerBrightnessControl;
            _useFinerVolumeControl = !_useFinerVolumeControl;
            fnKeyCode = 0; // skip further processing
            DEBUG_LOG("%s: Dispatched Media Toggle\n", this->getName());
            break;
		
        /* _Q76 Method in DSDT for A */
        case 0x7A: // ALS Sensor toggle
            
            _als.enabled = !_als.enabled;

            if(_als.exists)
                enableALS(_als.enabled);
            
            fnKeyCode = 0; // skip further processing
            break;
            
        /* _QEC Method in DSDT for Asus Instant key in Ultra books */
        case 0x71: // Customizable Asus Instant key
           
            switch (_instantKeyAction) {
                case 0:
                case 1:
                    fnKeyCode = NOTIFY_LAUNCHPAD;
                    break;
                case 2:
                    fnKeyCode = NOTIFY_MISSON_CNTL;
                    break;
                case 3:
                    fnKeyCode = NOTIFY_DASHBOARD;
                    break;
                case 4:
                    fnKeyCode = NOTIFY_DESKTOP;
                    break;
                case 5:
                    fnKeyCode = NOTIFY_APPWINDS;
                    break;
                case 6:
                    fnKeyCode = NOTIFY_EJECT;
                    break;
                    
                default:
                    break;
            }
            
            DEBUG_LOG("%s: Dispatch OSX Function %d for Instant Key\n", this->getName(), _instantKeyAction);
            break;
            
        /* _QA0 Method in DSDT */
        case 0x57: // AC disconnected
            
            // Enable ALS if its disabled on AC
            if(_als.exists && _als.disabled) {                
                _als.disabled = false;
                
                if(!_als.enabled)
                    enableALS(true);
            }
            
            // Using Asus backlight driver?
            if(_display.dimOnBatAC && !_display.usingAsusBackLight) {
                CheckAsusBacklight();
            }
            
           if(!_display.dimOnBatAC)
                fnKeyCode = 0;
    
           else if(_display.usingAsusBackLight) {
                fnKeyCode = NOTIFY_BRIGHTNESS_DOWN;
                
                ReadAsusPanelBrightnessValue(fnKeyCode);

                _display.asusBrightnessLvl -= _display.blvlsToMove;

                DEBUG_LOG("%s: Change Asus backlight to level %d on AC disconnect\n", this->getName(), _display.asusBrightnessLvl);
            }
            else
            {
                if(_useFinerBrightnessControl)
                    fnKeyCode = NOTIFY_BRIGHTNESS_DOWN_MIN;
                else
                    fnKeyCode = NOTIFY_BRIGHTNESS_DOWN;

                //
                // Calculate brightness levels to move
                //
                
                ReadApplePanelBrightnessValue();
                
                if (_display.brighntessLevel > 0)
                {
                    int brightnessLvlDiff = (_display.brighntessLevel - _display.dimBLevels);
                    
                    if(brightnessLvlDiff > 0)
                        _display.blvlsToMove = _display.dimBLevels;
                    else if(brightnessLvlDiff < 0)
                        _display.blvlsToMove = 0 - brightnessLvlDiff;
                    else
                        _display.blvlsToMove = _display.dimBLevels - 1;
                }
                else {
                    _display.blvlsToMove = 0;
                }
                
                if (_display.brighntessLevel < _display.blvlsToMove)
                    _display.blvlsToMove = 0;
                    
                DEBUG_LOG("%s: Decrease Apple backlight by %d level(s) on AC disconnect\n", this->getName(), _display.brighntessLevel);
            }
            break;
         
        /* _QA0 Method in DSDT */
		case 0x58: // AC connected
            
            // Disable ALS on AC
            if(_als.exists && _als.disableOnAC && _als.enabled)
            {
                enableALS(false);
                _als.disabled = true;
            }
            
            // Using Asus backlight driver?
            if(_display.dimOnBatAC && !_display.usingAsusBackLight) {
                CheckAsusBacklight();
            }
            
            if(!_display.dimOnBatAC)
                fnKeyCode = 0;
            
            else if(_display.usingAsusBackLight)
            {
                fnKeyCode = NOTIFY_BRIGHTNESS_UP;
                ReadAsusPanelBrightnessValue(fnKeyCode);
                
                _display.asusBrightnessLvl += _display.blvlsToMove;
                
                DEBUG_LOG("%s: Change Asus backlight to level %d on AC connect\n", this->getName(), _display.asusBrightnessLvl);
            }
			else
            {
                if(_useFinerBrightnessControl)
                    fnKeyCode = NOTIFY_BRIGHTNESS_UP_MIN;
                else
                    fnKeyCode = NOTIFY_BRIGHTNESS_UP;
                
                //
                // Calculate brightness levels to move
                //
                
                ReadApplePanelBrightnessValue();
                
                if (_display.brighntessLevel > 0) {
                    _display.blvlsToMove = _display.dimBLevels;
                }
                else {
                    _display.blvlsToMove = 0;
                }
                
                DEBUG_LOG("%s: Increase Apple backlight by %d level(s) on AC disconnect\n", this->getName(), _display.brighntessLevel);
            }
			break;
          
        /* _QDD and _QCD Method in DSDT */
        case 0xC6: // ALS Notifcations
        case 0xC7: // ALS Notifcations Alternate notify fnKeyCode (Optional)

            if(_als.exists && _als.enabled && !_als.processing)
            {
                CancelTimerTimeout(_als.timer);
                ProcessALS();
            }
            return;
         
		default:
            if(_display.backlightOn)
            {
                //
                // Using Asus backlight driver?
                //
                
                if(!_display.usingAsusBackLight) {
                    CheckAsusBacklight();
                }
                
                /* _Q0E Method in DSDT for Fn + F5, Panel Brightness Down */
                if(fnKeyCode >= NOTIFY_BRIGHTNESS_DOWN_MIN && fnKeyCode <= NOTIFY_BRIGHTNESS_DOWN_MAX)
                {
                    // Make this Function key
                    if(_workAsFunctionKeys) {
                        fnKeyCode = 0x60;
                    }
                    // Control display brightness
                    else {
                        
                        if(_useFinerBrightnessControl && !_display.usingAsusBackLight)
                            fnKeyCode = NOTIFY_BRIGHTNESS_DOWN_MIN;
                        else
                            fnKeyCode = NOTIFY_BRIGHTNESS_DOWN;
                        
                        _display.blvlsToMove = 1;
                    }
                    
                    DEBUG_LOG("%s: Dispatched %s\n",this->getName(), _workAsFunctionKeys?"F5":"Brightness Down");
                }
                /* _Q0F Method in DSDT for Fn + F6, Panel Brightness Up */
                else if(fnKeyCode >= NOTIFY_BRIGHTNESS_UP_MIN && fnKeyCode <= NOTIFY_BRIGHTNESS_UP_MAX)
                {
                    // Make this Function key
                    if(_workAsFunctionKeys) {
                        fnKeyCode = 0x61;
                    }
                    // Control display brightness
                    else {
                        
                        if(_useFinerBrightnessControl && !_display.usingAsusBackLight)
                            fnKeyCode = NOTIFY_BRIGHTNESS_UP_MIN;
                        else
                            fnKeyCode = NOTIFY_BRIGHTNESS_UP;
                        
                        _display.blvlsToMove = 1;
                    }
                    
                    DEBUG_LOG("%s: Dispatched %s\n",this->getName(), _workAsFunctionKeys?"F6":"Brightness Up");
                }
                
                //
                // Read Asus ACPI Panel brightness level to control backlight
                //
                
                if(_display.usingAsusBackLight && !_workAsFunctionKeys) {
                    ReadAsusPanelBrightnessValue(fnKeyCode);
                }
                
                //
                // Dispatch screen backlight
                //
                
                if(_display.blvlsToMove > 0)
                {
                    _fnKeyAction = fnKeyCode;
                    CancelTimerTimeout(_brightnessTimer);
                    SetTimerTimeout(_brightnessTimer, 0);
                    
                    if(_als.exists && _als.enabled && _als.disableOnFnCtrl)
                        enableALS(false);
                    
                    fnKeyCode = 0; // Skip
                }
            }
            else
                fnKeyCode = 0; // Skip
            break;
            
    }

    //
    // Dispatch the fnKeyCode for the keyboard handler
    //
    
    DEBUG_LOG("%s: Dispatch...\n",this->getName());

    if(fnKeyCode != 0) {
        _keyboardDevice->keyPressed(fnKeyCode);
        DEBUG_LOG("%s: Dispatched Key %d(0x%x)\n",this->getName(), fnKeyCode, fnKeyCode);
    }
    
    //
    // Control idle auto keyboard backlight off
    //
    
    TriggerAutoAsusBacklightOff();

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Process Fn key events dispatched to OSX keyboard for the special functions
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::ControlScreenBrightness()
{    
    DEBUG_LOG("%s: Dispatched Key %d(0x%x) for brightness to move %d levels, Panel value %d\n", this->getName(), _fnKeyAction, _fnKeyAction, _display.blvlsToMove, _display.brighntessLevel);

    // Disable ALS timer
    switch (_fnKeyAction) {
        case NOTIFY_BRIGHTNESS_DOWN:
        case NOTIFY_BRIGHTNESS_DOWN_MIN:
        case NOTIFY_BRIGHTNESS_UP:
        case NOTIFY_BRIGHTNESS_UP_MIN:
            if(_als.exists && _als.enabled)  {
                CancelTimerTimeout(_als.timer);               
            }
            break;
            
        default:
            break;
    }
    
    // Dispatch 
    for (int b = 0; b < _display.blvlsToMove; b++)
    {
        DEBUG_LOG("%s: Dispatch Key %x Level down %d\n", this->getName(), _fnKeyAction, b);

        _keyboardDevice->keyPressed(_fnKeyAction);
        
        if(_useFinerBrightnessControl)
            IOSleep(10);
        else
            IOSleep(50);
    }
    
    ReadApplePanelBrightnessValue();

    if (_display.brighntessLevel >= 0) {
        if (_useFinerBrightnessControl)
            _display.brightness = _display.brighntessLevel * 16;
        else
            _display.brightness = _display.brighntessLevel * 64;
    }
    
    // Enable ALS timer back
    if(_als.exists && _als.enabled && !_als.disableOnFnCtrl)  {
        SetTimerTimeout(_als.timer, _als.timerInterval);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Process Ambient Light Sensor
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::enableALS(bool enable)
{
    UInt32 res;
    OSObject * params[1];
    params[0] = OSNumber::withNumber(enable, 8);
    
    //
    // Call Asus WMI method 'ALSC' to enable/disable ALS
    //
    
    _wmiDevice->evaluateInteger("ALSC", &res, params, 1);
    if(enable) {
        _als.ali_notify_counts = 0;
        _als.ali_total = 0;
        SetTimerTimeout(_als.timer, _als.timerInterval);
        IOLog("%s: ALS sensor enabled.\n", this->getName());
    }
    else {
        CancelTimerTimeout(_als.timer);
        IOLog("%s: ALS sensor disabled.\n", this->getName());
    }
    
    _als.enabled = enable;
}

void AsusNBFnKeys::ProcessALS ()
{
    UInt32 als_ali = 0;
    
    if(kIOReturnSuccess == _wmiDevice->validateObject("ALSS"))
    {
        _wmiDevice->evaluateInteger("ALSS", &als_ali, NULL, NULL);
                
        // Convert received value which will be in 2's complement sometimes
        // als_ali = als_ali & 0xFF;
        
        _als.ali_notify_counts++;
        _als.ali_total += als_ali;
        _als.ali_avg = _als.ali_total/_als.ali_notify_counts;
        
        if (_als.logsEnabled)
        {
            IOLog("%s: ALS value %d, Samples %d, Average %d\n",this->getName(), als_ali, _als.ali_notify_counts, _als.ali_avg);
        }

        
        DEBUG_LOG("%s: ALS value from DSDT %d, 2's %d, Nofifications %d, Avg %d, Total %d\n",this->getName(), als_ali, als_ali & 0xFF, _als.ali_notify_counts, _als.ali_avg, _als.ali_total);

        if(_als.ali_notify_counts < _als.samples) {
            goto TIMER;
        }
        else
        {
            _als.ali_notify_counts = 0;
            _als.ali_total = 0;
            _als.processing = true;
        }

       DEBUG_LOG("%s: Current KLevel %d, Has16Levels %d\n",this->getName(), _kBLight.level, _kBLight.has16Lvls);

        UInt32  brightnessToSet = 0;
        UInt8   alsKBlvl = _kBLight.level;
        bool    changeKBlight = false;

        if(_als.ali_avg >= _als.levelRStart[0] && _als.ali_avg <= _als.levelREnd[0])
        {
            if (_kBLight.level != 3 && !_kBLight.has16Lvls) {
                alsKBlvl = 3;
                changeKBlight = true;
            }
            else if (_kBLight.level != 15 && _kBLight.has16Lvls) {
                alsKBlvl = 15;
                changeKBlight = true;
            }
            brightnessToSet = (_als.brightnessForLevel[0] * 0x400)/100;
        }
        else if(_als.ali_avg >= _als.levelRStart[1] && _als.ali_avg <= _als.levelREnd[1])
        {
            if (_kBLight.level != 2 && !_kBLight.has16Lvls) {
                alsKBlvl = 2;
                changeKBlight = true;
            }
            else if (_kBLight.level != 12 && _kBLight.has16Lvls) {
                alsKBlvl = 12;
                changeKBlight = true;
            }
            brightnessToSet = (_als.brightnessForLevel[1] * 0x400)/100;
        }
        else if(_als.ali_avg >= _als.levelRStart[2] && _als.ali_avg <= _als.levelREnd[2])
        {
            if (_kBLight.level != 1 && !_kBLight.has16Lvls) {
                alsKBlvl = 1;
                changeKBlight = true;
            }
            else if (_kBLight.level != 9 && _kBLight.has16Lvls) {
                alsKBlvl = 9;
                changeKBlight = true;
            }
            brightnessToSet = (_als.brightnessForLevel[2] * 0x400)/100;
        }
        else if(_als.ali_avg >= _als.levelRStart[3] && _als.ali_avg <= _als.levelREnd[3])
        {
            if (_kBLight.level != 0 && !_kBLight.has16Lvls) {
                alsKBlvl = 0;
                changeKBlight = true;
            }
            else if (_kBLight.level != 6 && _kBLight.has16Lvls) {
                alsKBlvl = 6;
                changeKBlight = true;
            }
            brightnessToSet = (_als.brightnessForLevel[3] * 0x400)/100;
        }
        else if(_als.ali_avg >= _als.levelRStart[4] && _als.ali_avg <= _als.levelREnd[4])
        {
            if (_kBLight.level != 0) {
                alsKBlvl = 0;
                changeKBlight = true;
            }
            brightnessToSet = (_als.brightnessForLevel[4] * 0x400)/100;
        }
        else {
            DEBUG_LOG("%s: Dead range\n",this->getName());
            goto TIMER;
        }
        
        //
        // Change keyboard backlight if auto idle off not active
        //
        
        
        bool ps2EnabledAuto = false;
        
        if (ReadkeyboardBacklight(0xFF) == 0xEA)
            ps2EnabledAuto = true;
        
        DEBUG_LOG("%s: KB Auto not active %d, changeKBlight %d\n", this->getName(), ps2EnabledAuto, changeKBlight);

        if (changeKBlight && (!_kBLight.idleOff || ps2EnabledAuto))
        {
            SetkeyboardBackLight(alsKBlvl);

            if (ps2EnabledAuto) {
                DEBUG_LOG("%s: Keyboard backlight turned on to Level %d from keyboard/Touchpad interrupt\n",this->getName(), _kBLight.level);
                
                _kBLight.idleOff = false;
                
                // Trigger timeout
                TriggerAutoAsusBacklightOff();
            }
        }

        //
        // Read current brighntness level
        //
        
        ReadApplePanelBrightnessValue();
        
        if(_display.brighntessLevel < 0)
            goto TIMER;
        
        int bDiff = brightnessToSet - _display.brightness;
        UInt8 bEvent, bEventNum = 0;

        if(bDiff < 0) {
            bDiff = 0 - bDiff;
            if (_useFinerBrightnessControl)
            {
                bEvent = NOTIFY_BRIGHTNESS_DOWN_MIN;
                bEventNum = bDiff/16;
            }
            else {
                bEvent = NOTIFY_BRIGHTNESS_DOWN;
                bEventNum = bDiff/64;
            }
        }
        else {
            if (_useFinerBrightnessControl)
            {
                bEvent = NOTIFY_BRIGHTNESS_UP_MIN;
                bEventNum = bDiff/16;
            }
            else {
                bEvent = NOTIFY_BRIGHTNESS_UP;
                bEventNum = bDiff/64;
            }
        }
                
        DEBUG_LOG("%s: ALI Avg %d, Brightness Level to set %d(0x%x) Current Level %d(0x%x), Diff %d(0x%x), Events %d, Timer %d, KBlvl %d, Auto off %s, ALS Brightness control %s\n",this->getName(), _als.ali_avg, brightnessToSet, brightnessToSet, _display.brightness, _display.brightness, bDiff, bDiff, bEventNum, _als.timerInterval, alsKBlvl, _kBLight.idleOff?"Yes":"No", _als.disableDispBritCtrl?"Yes":"No");
        
        if(brightnessToSet == _display.brightness || _display.brighntessLevel == 0 || bEventNum == 0)
            goto TIMER;
        
        //
        // Increase/Decrase display brightness until it matches the close value
        //       
                
        while (bEventNum > 0 && !_als.disableDispBritCtrl)
        {
            DEBUG_LOG("%s: Brightness noitfy event %d\n",this->getName(), bEventNum);

            _keyboardDevice->keyPressed(bEvent);
            
            if(_useFinerBrightnessControl)
                IOSleep(25);
            else
                IOSleep(50);
            
            bEventNum--;
        }
        
        IOSleep(100);
        
        ReadApplePanelBrightnessValue();
        if(_display.brighntessLevel < 0)
            goto TIMER;
    }
    else {
        IOLog("%s: Cannot find the method ALSS.\n",this->getName());
    }
    
TIMER:
    CancelTimerTimeout(_als.timer);
    SetTimerTimeout(_als.timer, _als.timerInterval);
    _als.processing = false;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Process Keyboard backlight
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::TriggerAutoAsusBacklightOff()
{
    DEBUG_LOG("%s: Checking Keyboard backlight idle timeout off\n",this->getName());

    //
    // Control idle auto keyboard backlight off by setting timeout
    //
    
    if(_kBLight.exists && _kBLight.hasAutoShutOff && (ReadkeyboardBacklight(0) == kIOReturnSuccess))
    {
        DEBUG_LOG("%s: Keyboard backlight idle timeout off %d\n",this->getName(), _kBLight.level);
        
        if(_kBLight.level > 0 && _kBLight.idleOff)
        {
            DEBUG_LOG("%s: Keyboard backlight turned on to Level %d after idle timeout off\n",this->getName(), _kBLight.level);
            _kBLight.idleOff = false;
            
            // Turn on backlight
            SetkeyboardBackLight(0xFA);
            
            // Enable ALS timer brightness control back
            if(_als.enabled) {
                _als.disableDispBritCtrl = false;
            }
        }
        // Start idle backlight off timeout
        CancelTimerTimeout(_kBLight.timer);
        SetTimerTimeout(_kBLight.timer, _kBLight.offTimeout);
    }
}


IOReturn AsusNBFnKeys::SetkeyboardBackLight(UInt8 level)
{
    
    OSObject * params[1];
    UInt32 ret = -1;
    
   	params[0] =OSNumber::withNumber(level, 8);
    
    //
    // Call Asus WMI Method 'SKBL' to set the keyboard backlight
    //
    
    if(kIOReturnSuccess == _wmiDevice->validateObject("SKBL"))
    {
        if(kIOReturnSuccess == _wmiDevice->evaluateInteger("SKBL", &ret, params, 1))
        {
            DEBUG_LOG("%s: Keyboard backlight level set to 0x%x Status 0x%x.\n",this->getName(), level, ret);
            
            //
            // Save level in NVRAM (Ignore Auto off control values)
            //
            
            if(level != 0xFD && level != 0xFA) {
                _kBLight.level = level;
                saveValueToNVRAM("AsusKeyboardBackLightLvl", (UInt8)_kBLight.level);
            }
        }
        else {
            IOLog("%s: Failed to set Keyboard backlight to level 0x%x.\n",this->getName(), _kBLight.level);
            return kIOReturnError;
        }
    }
    else {
        IOLog("%s: Keyboard backlight method not found.\n",this->getName());
        return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

IOReturn AsusNBFnKeys::ReadkeyboardBacklight(UInt8 reqParm)
{
    DEBUG_LOG("%s: Reading keyboard backlight...\n",this->getName());
    
    OSObject * params[1];
    UInt32 kbLightLvl = 0;

   	params[0] =OSNumber::withNumber(reqParm, 8);
    
    //
    // Call Asus WMI Method 'GKBL' to read the keyboard backlight
    //
    
    if(kIOReturnSuccess == _wmiDevice->validateObject("GKBL"))
    {
        if(kIOReturnSuccess == _wmiDevice->evaluateInteger("GKBL", &kbLightLvl, params, 1))
        {
            //
            // Some systems have the backlight value in 2's complement,
            // so need to convert.
            //
            
            kbLightLvl = kbLightLvl & 0xFF;
            
            DEBUG_LOG("%s: Keyboard backight Value from DSDT 0x%x\n", this->getName(), kbLightLvl & 0xFF);
            
            if (reqParm == 0xFF)
                return kbLightLvl;
            else {
                _kBLight.level = kbLightLvl;
                return kIOReturnSuccess;
            }
        }
        else {
            IOLog("%s: Failed to read the Keyboard backlight.\n",this->getName());
            return kIOReturnError;
        }
    }
    else {
        IOLog("%s: Keyboard backlight method not found.\n",this->getName());
        return kIOReturnError;
    }
}

void AsusNBFnKeys::ControlfKeybrdBackLight()
{
    DEBUG_LOG("%s: Idle keyboard backlight off triggered after timeout %d.\n",this->getName(), _kBLight.offTimeout);
  
    //
    // Turn backlight off after timeout
    //
    
    SetkeyboardBackLight(0xFD);
    _kBLight.idleOff = true;
    
    //
    // Disable ALS timer brightness control
    //
    
    if(_als.enabled) {
        _als.disableDispBritCtrl = true;
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Load and Save NVRAM value for Keyboard Backlight
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::LoadValuesFromNVRAM()
{

    if(OSDictionary *nvramServiceMatch = serviceMatching("IODTNVRAM"))
    {
        if (IODTNVRAM *nvram = OSDynamicCast(IODTNVRAM, waitForMatchingService(nvramServiceMatch, 1000000000ULL * 15))) {
            
            DEBUG_LOG("%s: NVRAM name->%s\n", this->getName(),nvram->getName());
            
            OSData *bval = 0;
            UInt8 rValue = 0;

            //
            // Get the object for NVRAM options
            //
            
            IORegistryEntry *nvramEntry = IORegistryEntry::fromPath("IODeviceTree:/options");
            
            if(!nvramEntry)
                IOLog("%s: Failed to find NVRAM options\n", this->getName());
            else
            {
                DEBUG_LOG("%s: Found NVRAM options\n", this->getName());
                
                if(_kBLight.exists)
                {
                    //
                    // Read keyboard backlight value from NVRAM
                    //
                    
                    bval = OSDynamicCast(OSData, nvramEntry->getProperty("AsusKeyboardBackLightLvl"));
                    if(bval) {
                        memcpy(&rValue, bval->getBytesNoCopy(), bval->getLength());
                        DEBUG_LOG("%s: Keyboard Backlight value in NVRAM %d\n", this->getName(), rValue);
                        _kBLight.level = rValue;
                    } else
                        IOLog("%s: Keyboard Backlight value not found.\n", this->getName());
                }
                
                nvramEntry->release();
            }
            nvram->release();
        }
        else
            IOLog("%s: NVRAM not available.\n", this->getName());

        nvramServiceMatch->release();
    }
}

void AsusNBFnKeys::saveValueToNVRAM(const char * symbol, UInt8 value)
{
    IORegistryEntry *nvramEntry = IORegistryEntry::fromPath("IODeviceTree:/options");
    
    if(!nvramEntry)
        IOLog("%s: Failed to find NVRAM options\n", this->getName());
    else
    {
        DEBUG_LOG("%s: Found NVRAM options\n", this->getName());
        
        //
        // Save backlight value to NVRAM
        //
        
        if(nvramEntry->IORegistryEntry::setProperty(symbol, &value, sizeof(value)) == false)
        {
            IOLog("%s: Failed to save %s value %d in NVRAM\n", this->getName(), symbol, value);
        }
        else
            DEBUG_LOG("%s: %s value %d saved in NVRAM\n", this->getName(), symbol, value);

        nvramEntry->release();
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Find the GPU device display entry from ioreg to control screen backlight 
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::FindGPUDeviceEntry()
{
    //
    // Intel IGPU
    //
    
    DEBUG_LOG("%s: Searching for Intel GPU...\n",this->getName());

    // GFX0
    snprintf(_gpuPath.iGPU, sizeof(_gpuPath.iGPU), "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/GFX0@2");
    snprintf(_gpuDisplayPath.iGPU, sizeof(_gpuDisplayPath.iGPU), "%s/AppleIntelFramebuffer@0/display0/AppleBacklightDisplay", _gpuPath.iGPU);
    
    if(FindDisplayEntry(false))
        return;
    
    // IGPU
    snprintf(_gpuPath.iGPU, sizeof(_gpuPath.iGPU), "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/IGPU@2");
    snprintf(_gpuDisplayPath.iGPU, sizeof(_gpuDisplayPath.iGPU), "%s/AppleIntelFramebuffer@0/display0/AppleBacklightDisplay", _gpuPath.iGPU);
    
    if(FindDisplayEntry(false))
        return;
    
    
    //
    // Discrete Nvidia GPU
    //
    
    DEBUG_LOG("%s: Searching for NVIDIA GPU...\n",this->getName());

    for (int i = 0; i < 5; i++)
    {
        switch (i) {
                
            case 0:
                snprintf(_pciDeviceName, sizeof(_pciDeviceName), "PEG0");
                break;
                
            case 1:
                snprintf(_pciDeviceName, sizeof(_pciDeviceName), "PEGP");
                break;
                
            case 2:
                snprintf(_pciDeviceName, sizeof(_pciDeviceName), "PEGR");
                break;
                
            case 3:
                snprintf(_pciDeviceName, sizeof(_pciDeviceName), "P0P2");
                break;
                
            case 4: // Nvidia chipset
                snprintf(_pciDeviceName, sizeof(_pciDeviceName), "IXVE");
                break;
            
        }
        
        //  New Nvidia GPU uses this
        
        snprintf(_gpuPath.dGPU, sizeof(_gpuPath.dGPU), "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/%s@1/%s/GFX0@0", _pciDeviceName, _pciBridgeName);
        
        snprintf(_gpuDisplayPath.dGPU1, sizeof(_gpuDisplayPath.dGPU1), "%s/NVDA,Display-A@0/NVDA/display0/AppleDisplay", _gpuPath.dGPU);
        snprintf(_gpuDisplayPath.dGPU2, sizeof(_gpuDisplayPath.dGPU2), "%s/NVDA,Display-A@0/NVDA/display0/AppleBacklightDisplay", _gpuPath.dGPU);
        
        if(FindDisplayEntry(true))
            return;
        
        //  Old Nvidia GPU uses this
        
        snprintf(_gpuPath.dGPU, sizeof(_gpuPath.dGPU), "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/%s@3/%s/GFX0@0", _pciDeviceName, _pciBridgeName);
        
        snprintf(_gpuDisplayPath.dGPU1, sizeof(_gpuDisplayPath.dGPU1), "%s/NVDA,Display-A@0/NVDATesla/display0/AppleDisplay", _gpuPath.dGPU);
        snprintf(_gpuDisplayPath.dGPU2, sizeof(_gpuDisplayPath.dGPU2), "%s/NVDA,Display-A@0/NVDATesla/display0/AppleBacklightDisplay", _gpuPath.dGPU);
        
        if(FindDisplayEntry(true))
            return;
        
        //  Old Nvidia GPU and Non-Intel controller uses this
        
        snprintf(_gpuPath.dGPU, sizeof(_gpuPath.dGPU), "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/%s@10/%s/GFX0@0", _pciDeviceName, _pciBridgeName);
        
        snprintf(_gpuDisplayPath.dGPU1, sizeof(_gpuDisplayPath.dGPU1), "%s/NVDA,Display-A@0/NVDATesla/display0/AppleDisplay", _gpuPath.dGPU);
        snprintf(_gpuDisplayPath.dGPU2, sizeof(_gpuDisplayPath.dGPU2), "%s/NVDA,Display-A@0/NVDATesla/display0/AppleBacklightDisplay", _gpuPath.dGPU);
        
        if(FindDisplayEntry(true))
            return;
        
        //
        // If the primary DSDT table did not have the GFX device
        // then we may get device at display@0
        //
        
        //  New Nvidia GPU uses this

        snprintf(_gpuPath.dGPU, sizeof(_gpuPath.dGPU), "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/%s@1/%s/display@0", _pciDeviceName, _pciBridgeName);
        
        snprintf(_gpuDisplayPath.dGPU1, sizeof(_gpuDisplayPath.dGPU1), "%s/NVDA,Display-A@0/NVDA/display0/AppleDisplay", _gpuPath.dGPU);
        snprintf(_gpuDisplayPath.dGPU2, sizeof(_gpuDisplayPath.dGPU2), "%s/NVDA,Display-A@0/NVDA/display0/AppleBacklightDisplay", _gpuPath.dGPU);
        
        if(FindDisplayEntry(true))
            return;
        
        //  Old Nvidia GPU uses this
        
        snprintf(_gpuPath.dGPU, sizeof(_gpuPath.dGPU), "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/%s@3/%s/display@0", _pciDeviceName, _pciBridgeName);
        
        snprintf(_gpuDisplayPath.dGPU1, sizeof(_gpuDisplayPath.dGPU1), "%s/NVDA,Display-A@0/NVDATesla/display0/AppleDisplay", _gpuPath.dGPU);
        snprintf(_gpuDisplayPath.dGPU2, sizeof(_gpuDisplayPath.dGPU2), "%s/NVDA,Display-A@0/NVDATesla/display0/AppleBacklightDisplay", _gpuPath.dGPU);
        
        if(FindDisplayEntry(true))
            return;
        
        //  Old Nvidia GPU and Non-Intel controller uses this
        
        snprintf(_gpuPath.dGPU, sizeof(_gpuPath.dGPU), "IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/%s@10/%s/display@0", _pciDeviceName, _pciBridgeName);
        
        snprintf(_gpuDisplayPath.dGPU1, sizeof(_gpuDisplayPath.dGPU1), "%s/NVDA,Display-A@0/NVDATesla/display0/AppleDisplay", _gpuPath.dGPU);
        snprintf(_gpuDisplayPath.dGPU2, sizeof(_gpuDisplayPath.dGPU2), "%s/NVDA,Display-A@0/NVDATesla/display0/AppleBacklightDisplay", _gpuPath.dGPU);
        
        if(FindDisplayEntry(true))
            return;
    }
}

bool AsusNBFnKeys::FindDisplayEntry(bool dedicatdGPU)
{
    if(dedicatdGPU) {
        _display.deviceEntry = IORegistryEntry::fromPath(_gpuPath.dGPU);
        
        if(_display.deviceEntry)
        {
            _display.deviceEntry = 0;
            
            // look for AppleDisplay
            _display.deviceEntry = IORegistryEntry::fromPath(_gpuDisplayPath.dGPU1);
            
            if(_display.deviceEntry == NULL) {
                // look for AppleBacklightDisplay
                _display.deviceEntry = IORegistryEntry::fromPath(_gpuDisplayPath.dGPU2);
                
                if(_display.deviceEntry == NULL) {
                    DEBUG_LOG("%s: Failed to find nvidia display backlight entry for %s\n",this->getName(), _gpuPath.dGPU);
                }
                else {
                    DEBUG_LOG("%s: Found nvidia display device at %s\n",this->getName(), _gpuDisplayPath.dGPU2);
                    return true;
                }
            }
            else {
                DEBUG_LOG("%s: Found nvidia display device at %s\n",this->getName(), _gpuDisplayPath.dGPU1);
                return true;
            }
        }
        else
           DEBUG_LOG("%s: %s device not found\n",this->getName(), _gpuPath.dGPU);
    }
    else {
        _display.deviceEntry = IORegistryEntry::fromPath(_gpuPath.iGPU);
        
        if(_display.deviceEntry)
        {
            _display.deviceEntry = 0;
            
            // look for AppleDisplay
            _display.deviceEntry = IORegistryEntry::fromPath(_gpuDisplayPath.iGPU);
            
            if(_display.deviceEntry == NULL) {
                IOLog("%s: Failed to find Intel display backlight entry for %s\n",this->getName(), _gpuPath.iGPU);
            }
            else {
                DEBUG_LOG("%s: Found Intel display device %s\n",this->getName(), _gpuDisplayPath.iGPU);
                return true;
            }
        }
        else
            DEBUG_LOG("%s: %s device not found\n",this->getName(), _gpuPath.iGPU);
    }
    
    return false;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Find and read Asus Backlight driver brightness value from ioreg
// to control screen backlight bezel levels because Asus backlight 
// has only 10 levels, so we need to map those 10 levels to 16 bezels.
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::CheckAsusBacklight()
{
    IORegistryEntry *asusBLight = IORegistryEntry::fromPath("IOService:/AppleACPIPlatformExpert/PNLF/AsusACPIBacklightPanel");
    
    if(asusBLight) {
        _display.usingAsusBackLight = true;
        DEBUG_LOG("%s: Found Asus Backlight device.\n", this->getName());
        asusBLight->release();
    }
    else {
        _display.usingAsusBackLight = false;
        DEBUG_LOG("%s: Asus Backlight device not Found , disabled asus backlight support.\n", this->getName());
    }
}

void AsusNBFnKeys::ReadAsusPanelBrightnessValue(int fnKeyCode)
{
    UInt8 asusBritLvl = 0;
    
    IORegistryEntry *asusACPIDeviceEntry = IORegistryEntry::fromPath("IOService:/AppleACPIPlatformExpert/PNLF/AsusACPIBacklightPanel");
    
    if (asusACPIDeviceEntry != NULL) {
        
        OSNumber *ioregAsusTLvls = OSDynamicCast(OSNumber, asusACPIDeviceEntry->getProperty("AsusBrightnessLevelsTotal"));
        
        if(!ioregAsusTLvls) {
            IOLog("%s: Asus Backlight total brightness levels not found.\n",this->getName());
            return;
        }
    
        OSNumber *ioregAsusLvl = OSDynamicCast(OSNumber, asusACPIDeviceEntry->getProperty("AsusBrightnessLevel"));
        
        if(!ioregAsusLvl) {
            IOLog("%s: Asus Backlight brightness level not Found.\n",this->getName());
            return;
        }
        
        asusBritLvl = ioregAsusLvl->unsigned8BitValue();
        
        if(fnKeyCode == NOTIFY_BRIGHTNESS_UP)
            asusBritLvl++;
        
        if (ioregAsusTLvls->unsigned8BitValue() == 0x0B)
        {
            switch (asusBritLvl) {
                case 11:
                case 10:
                    _display.blvlsToMove = 3;
                    break;
                    
                case 9:
                case 8:
                    _display.blvlsToMove = 2;
                    break;
                    
                default:
                    _display.blvlsToMove = 1;
                    break;
            }
        }
        else
            _display.blvlsToMove = 1;

        DEBUG_LOG("%s: Asus Backlight device Entry Found with brightness value %d (Total levels %d), Blight Levels to move %d\n",this->getName(), asusBritLvl, ioregAsusTLvls->unsigned8BitValue(), _display.blvlsToMove);
        
        asusACPIDeviceEntry->release();
    }
    else
        IOLog("%s: Asus Backlight device not Found.\n",this->getName());
    
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// Find and read Intel/Nvidia backlightPanel brightness values from ioreg
// to control soft backlight on/off and AC/Battery brightness dim
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::ReadApplePanelBrightnessValue()
{
    //
    // Only for Intel Nvidia and GPU:
    // Reading Apple Backlight Panel values from IOReg for controlling the bezel levels
    // Apple Panel values for each bezel level (0 - 16) are:
    // 0x400, 0x3c0, 0x380, 0x340, 0x300, 0x2c0, 0x280, 0x240,
    // 0x200, 0x1c0, 0x180, 0x140, 0x100, 0x0c0, 0x080, 0x040, 0x00
    // Deicmal Value of Panel Brightness value /64 = Normal Bezel Level in the range 0 - 15
    // Deicmal Value of Panel Brightness value /16 = Finer Bezel Level in the range 0 - 63
    //
    
    _display.brighntessLevel = -1;

    FindGPUDeviceEntry();
    
    if (_display.deviceEntry != NULL)
    {
        OSNumber *brightnessValue = 0;
        
        if(OSDictionary *ioDisplayParaDict = OSDynamicCast(OSDictionary, _display.deviceEntry->getProperty("IODisplayParameters")))
        {
            if(OSDictionary *brightnessDict = OSDynamicCast(OSDictionary, ioDisplayParaDict->getObject("brightness")))
            {
                brightnessValue = OSDynamicCast(OSNumber, brightnessDict->getObject("value"));
                if(brightnessValue)
                {
                    if(brightnessValue->unsigned32BitValue() > 0) {
                        
                        if(_useFinerBrightnessControl) {
                            _display.brighntessLevel = brightnessValue->unsigned32BitValue()/16;
                            _display.brightness = _display.brighntessLevel * 16;
                        }
                        else {
                            _display.brighntessLevel = brightnessValue->unsigned32BitValue()/64;
                            _display.brightness = _display.brighntessLevel * 64;
                        }
                    }
                    else {
                        _display.brighntessLevel = 0;
                        _display.brightness = 0;
                    }
                    
                    DEBUG_LOG("%s: Brightness from IOReg %d(0x%x), Screen Brightness %d(0x%x) , Panel Level %d\n", this->getName(), brightnessValue->unsigned32BitValue(), brightnessValue->unsigned32BitValue(), _display.brightness, _display.brightness, _display.brighntessLevel);
                }
            }
            else
            {
                IOLog("%s: Cannot find the dictionary brightness\n",this->getName());
            }
        }
        else
        {
            IOLog("%s: Cannot find the dictionary IODisplayParameters\n",this->getName());
        }
        
        OSSafeReleaseNULL(_display.deviceEntry);
    }
    else
        IOLog("%s: GPU device not found\n",this->getName());

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// ACPI set and get methods for Asus ATK Events
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void AsusNBFnKeys::getDeviceStatus(const char * guid, UInt32 methodId, UInt32 deviceId, UInt32 *status)
{
	DEBUG_LOG("%s: getDeviceStatus()\n", this->getName());
	
	char method[5];
	OSObject * params[3];
	OSString *str;
	OSDictionary *dict = getDictByUUID(guid);
	if (NULL == dict)
		return;
	
	str = OSDynamicCast(OSString, dict->getObject("object_id"));
	if (NULL == str)
		return;
	
	snprintf(method, 5, "WM%s", str->getCStringNoCopy());
    
	params[0] = OSNumber::withNumber(0x00D,32);
	params[1] = OSNumber::withNumber(methodId,32);
	params[2] = OSNumber::withNumber(deviceId,32);
	
	_wmiDevice->evaluateInteger(method, status, params, 3);
	
	params[0]->release();
	params[1]->release();
	params[2]->release();
	
	return;
}

void AsusNBFnKeys::setDeviceStatus(const char * guid, UInt32 methodId, UInt32 deviceId, UInt32 *status)
{
	DEBUG_LOG("%s: setDeviceStatus()\n", this->getName());
	
	char method[5];
	char buffer[8];
	OSObject * params[3];
	OSString *str;
	OSDictionary *dict = getDictByUUID(guid);
	if (NULL == dict)
		return;
	
	str = OSDynamicCast(OSString, dict->getObject("object_id"));
	if (NULL == str)
		return;
	
	snprintf(method, 5, "WM%s", str->getCStringNoCopy());
	
	memcpy(buffer, &deviceId, 4);
	memcpy(buffer+4, status, 4);
	
	params[0] = OSNumber::withNumber(0x00D,32);
	params[1] = OSNumber::withNumber(methodId,32);
	params[2] = OSData::withBytes(buffer, 8);
    
	*status = ~0;
	_wmiDevice->evaluateInteger(method, status, params, 3);
	
	DEBUG_LOG("%s: setDeviceStatus Res = %x\n", this->getName(), (unsigned int)*status);
	
	params[0]->release();
	params[1]->release();
	params[2]->release();
    
	return;
}

void AsusNBFnKeys::setDevice(const char * guid, UInt32 methodId, UInt32 *status)
{
	DEBUG_LOG("%s: setDevice(%d)\n", this->getName(), (int)*status);
	
	char method[5];
	char buffer[4];
	OSObject * params[3];
	OSString *str;
	OSDictionary *dict = getDictByUUID(guid);
	if (NULL == dict)
		return;
	
	str = OSDynamicCast(OSString, dict->getObject("object_id"));
	if (NULL == str)
		return;
	
	snprintf(method, 5, "WM%s", str->getCStringNoCopy());
	
	memcpy(buffer, status, 4);
	
	params[0] = OSNumber::withNumber(0x00D,32);
	params[1] = OSNumber::withNumber(methodId,32);
	params[2] = OSData::withBytes(buffer, 8);
	
	*status = ~0;
	_wmiDevice->evaluateInteger(method, status, params, 3);
	
	DEBUG_LOG("%s: setDevice Res = %x\n", this->getName(), (unsigned int)*status);
	
	params[0]->release();
	params[1]->release();
	params[2]->release();
	
	return;
}


OSDictionary* AsusNBFnKeys::getDictByUUID(const char * guid)
{
	UInt32 i;
	OSDictionary	*dict = NULL;
	OSString		*uuid;
	OSArray *array = OSDynamicCast(OSArray, getProperty("WDG"));
	if (NULL == array)
		return NULL;
    
	for (i=0; i<array->getCount(); i++) {
		dict = OSDynamicCast(OSDictionary, array->getObject(i));
		uuid = OSDynamicCast(OSString, dict->getObject("UUID"));
		if (uuid->isEqualTo(guid)){
            break;
        } else {
            dict = NULL;
        }
	}
	return dict;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
//
// GUID(WMI) parsing functions
//
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

/*
 * wmi_parse_hexbyte - Convert a ASCII hex number to a byte
 * @src:  Pointer to at least 2 characters to convert.
 *
 * Convert a two character ASCII hex string to a number.
 *
 * Return:  0-255  Success, the byte was parsed correctly
 *          -1     Error, an invalid character was supplied
 */

int AsusNBFnKeys::wmi_parse_hexbyte(const UInt8 *src)
{
	unsigned int x; /* For correct wrapping */
	int h;
	
	/* high part */
	x = src[0];
	if (x - '0' <= '9' - '0') {
		h = x - '0';
	} else if (x - 'a' <= 'f' - 'a') {
		h = x - 'a' + 10;
	} else if (x - 'A' <= 'F' - 'A') {
		h = x - 'A' + 10;
	} else {
		return -1;
	}
	h <<= 4;
	
	/* low part */
	x = src[1];
	if (x - '0' <= '9' - '0')
		return h | (x - '0');
	if (x - 'a' <= 'f' - 'a')
		return h | (x - 'a' + 10);
	if (x - 'A' <= 'F' - 'A')
		return h | (x - 'A' + 10);
	
    return -1;
}


/*
 * wmi_swap_bytes - Rearrange GUID bytes to match GUID binary
 * @src:   Memory block holding binary GUID (16 bytes)
 * @dest:  Memory block to hold byte swapped binary GUID (16 bytes)
 *
 * Byte swap a binary GUID to match it's real GUID value
 */

void AsusNBFnKeys::wmi_swap_bytes(UInt8 *src, UInt8 *dest)
{
	int i;
	
	for (i = 0; i <= 3; i++)
		memcpy(dest + i, src + (3 - i), 1);
	
	for (i = 0; i <= 1; i++)
		memcpy(dest + 4 + i, src + (5 - i), 1);
	
	for (i = 0; i <= 1; i++)
		memcpy(dest + 6 + i, src + (7 - i), 1);
	
	memcpy(dest + 8, src + 8, 8);
}


/*
 * wmi_parse_guid - Convert GUID from ASCII to binary
 * @src:   36 char string of the form fa50ff2b-f2e8-45de-83fa-65417f2f49ba
 * @dest:  Memory block to hold binary GUID (16 bytes)
 *
 * N.B. The GUID need not be NULL terminated.
 *
 * Return:  'true'   @dest contains binary GUID
 *          'false'  @dest contents are undefined
 */

bool AsusNBFnKeys::wmi_parse_guid(const UInt8 *src, UInt8 *dest)
{
	static const int size[] = { 4, 2, 2, 2, 6 };
	int i, j, v;
	
	if (src[8]  != '-' || src[13] != '-' ||
		src[18] != '-' || src[23] != '-')
		return false;
	
	for (j = 0; j < 5; j++, src++) {
		for (i = 0; i < size[j]; i++, src += 2, *dest++ = v) {
			v = wmi_parse_hexbyte(src);
			if (v < 0)
				return false;
		}
	}
	
	return true;
}


/*
 * wmi_dump_wdg - dumps tables to dmesg
 * @src: guid_block *
 */

void AsusNBFnKeys::wmi_dump_wdg(struct guid_block *g)
{
	char guid_string[37];
	
	wmi_data2Str(g->guid, guid_string);
	DEBUG_LOG("%s:\n", guid_string);
	if (g->flags & ACPI_WMI_EVENT) {
		DEBUG_LOG("\tnotify_value: %02X\n", g->notify_id);
    }
	else {
		DEBUG_LOG("\tobject_id: %c%c\n",g->object_id[0], g->object_id[1]);
    }
	DEBUG_LOG("\tinstance_count: %d\n", g->instance_count);
	DEBUG_LOG("\tflags: %#x", g->flags);
	if (g->flags) {
		DEBUG_LOG(" ");
		if (g->flags & ACPI_WMI_EXPENSIVE)
			DEBUG_LOG("ACPI_WMI_EXPENSIVE ");
		if (g->flags & ACPI_WMI_METHOD)
			DEBUG_LOG("ACPI_WMI_METHOD ");
		if (g->flags & ACPI_WMI_STRING)
			DEBUG_LOG("ACPI_WMI_STRING ");
		if (g->flags & ACPI_WMI_EVENT)
			DEBUG_LOG("ACPI_WMI_EVENT ");
	}
}



/*
 * wmi_data2Str - converts binary guid to ascii guid
 *
 */

int AsusNBFnKeys::wmi_data2Str(const char *in, char *out)
{
	int i;
	
	for (i = 3; i >= 0; i--)
		out += snprintf(out, 3, "%02X", in[i] & 0xFF);
	
	out += snprintf(out, 2, "-");
	out += snprintf(out, 3, "%02X", in[5] & 0xFF);
	out += snprintf(out, 3, "%02X", in[4] & 0xFF);
	out += snprintf(out, 2, "-");
	out += snprintf(out, 3, "%02X", in[7] & 0xFF);
	out += snprintf(out, 3, "%02X", in[6] & 0xFF);
	out += snprintf(out, 2, "-");
	out += snprintf(out, 3, "%02X", in[8] & 0xFF);
	out += snprintf(out, 3, "%02X", in[9] & 0xFF);
	out += snprintf(out, 2, "-");
	
	for (i = 10; i <= 15; i++)
		out += snprintf(out, 3, "%02X", in[i] & 0xFF);
	
	*out = '\0';
	return 0;
}

/*
 * flagsToStr - converts binary flag to ascii flag
 *
 */

OSString * AsusNBFnKeys::flagsToStr(UInt8 flags)
{
	char str[80];
	char *pos = str;
	if (flags != 0)
	{
		if (flags & ACPI_WMI_EXPENSIVE)
		{
			strncpy(pos, "ACPI_WMI_EXPENSIVE ", 20);
			pos += strlen(pos);
		}
		if (flags & ACPI_WMI_METHOD)
		{
			strncpy(pos, "ACPI_WMI_METHOD ", 20);
			pos += strlen(pos);
            DEBUG_LOG("%s: Found WMI METHOD\n", this->getName());
		}
		if (flags & ACPI_WMI_STRING)
		{
			strncpy(pos, "ACPI_WMI_STRING ", 20);
			pos += strlen(pos);
		}
		if (flags & ACPI_WMI_EVENT)
		{
			strncpy(pos, "ACPI_WMI_EVENT ", 20);
			// pos += strlen(pos);
            DEBUG_LOG("%s: Found WMI EVENT\n", this->getName());
		}
		//suppress the last trailing space
		str[strlen(str)] = 0;
	}
	else
	{
		str[0] = 0;
	}
	return (OSString::withCString(str));
}


/*
 * wmi_wdg2reg - adds the WDG structure to a dictionary
 *
 */

void AsusNBFnKeys::wmi_wdg2reg(struct guid_block *g, OSArray *array, OSArray *dataArray)
{
	char guid_string[37];
	char object_id_string[3];
	OSDictionary *dict = OSDictionary::withCapacity(6);
	
	wmi_data2Str(g->guid, guid_string);
    
	dict->setObject("UUID", OSString::withCString(guid_string));
	if (g->flags & ACPI_WMI_EVENT)
		dict->setObject("notify_value", OSNumber::withNumber(g->notify_id, 8));
	else
	{
		snprintf(object_id_string, 3, "%c%c", g->object_id[0], g->object_id[1]);
		dict->setObject("object_id", OSString::withCString(object_id_string));
	}
	dict->setObject("instance_count", OSNumber::withNumber(g->instance_count, 8));
	dict->setObject("flags", OSNumber::withNumber(g->flags, 8));
#if DEBUG
	dict->setObject("flags Str", flagsToStr(g->flags));
#endif
	if (g->flags == 0)
		dataArray->setObject(readDataBlock(object_id_string));
    
	
	array->setObject(dict);
}

OSDictionary * AsusNBFnKeys::readDataBlock(char *str)
{
	OSObject	*wqxx;
	OSData		*data = NULL;
	OSDictionary *dict;
	char name[5];
	
	snprintf(name, 5, "WQ%s", str);
	dict = OSDictionary::withCapacity(1);
	
	do
	{
		if (_wmiDevice->evaluateObject(name, &wqxx) != kIOReturnSuccess)
		{
			IOLog("%s: No object of method %s\n", this->getName(), name);
			continue;
		}
		
		data = OSDynamicCast(OSData , wqxx);
		if (data == NULL){
			IOLog("%s: Cast error %s\n", this->getName(), name);
			continue;
		}
		dict->setObject(name, data);
	}
	while (false);
	return dict;
}

/*
 * Parse the _WDG method for the GUID data blocks
 */

int AsusNBFnKeys::parse_wdg()
{
	UInt32 i, total;
	OSObject	*wdg;
	OSData		*data;
	OSArray		*array, *dataArray;
    
	do
	{
		if (_wmiDevice->evaluateObject("_WDG", &wdg) != kIOReturnSuccess)
		{
			DEBUG_LOG("%s: Method _WDG not found\n", this->getName());
			continue;
		}
		
		data = OSDynamicCast(OSData , wdg);
		if (data == NULL){
			IOLog("%s: Cast error _WDG\n", this->getName());
			continue;
		}
		total = data->getLength() / sizeof(struct guid_block);
		array = OSArray::withCapacity(total);
		dataArray = OSArray::withCapacity(1);
		
		for (i = 0; i < total; i++) {
			wmi_wdg2reg((struct guid_block *) data->getBytesNoCopy(i * sizeof(struct guid_block), sizeof(struct guid_block)), array, dataArray);
		}
		setProperty("WDG", array);
		setProperty("DataBlocks", dataArray);
		data->release();
	}
	while (false);
	
	return 0;
}

