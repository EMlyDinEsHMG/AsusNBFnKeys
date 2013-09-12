/*
 *  Copyright (c) 2012 - 2013 EMlyDinEsH(OSXLatitude). All rights reserved.
 *
 *  Asus Notebooks Fn keys Driver v1.7.2 by EMlyDinEsH for Mac OSX
 *
 *  Credits: Hotkoffy(insanelymac) for initial source
 *
 *  Asus ATK Device Controller
 *  AsusWMIController.cpp
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


#include <IOKit/hidsystem/ev_keymap.h>

#include "AsusWMIController.h"


#define ASUS_WMI_MGMT_GUID      "97845ED0-4E6D-11DE-8A39-0800200C9A66"
#define ASUS_NB_WMI_EVENT_GUID  "0B3CBB35-E3C2-45ED-91C2-4C5A6D195D1C"

/* WMI Methods */
#define ASUS_WMI_METHODID_SPEC          0x43455053 /* BIOS SPECification */
#define ASUS_WMI_METHODID_SFBD          0x44424653 /* Set First Boot Device */
#define ASUS_WMI_METHODID_GLCD          0x44434C47 /* Get LCD status */
#define ASUS_WMI_METHODID_GPID          0x44495047 /* Get Panel ID?? (Resol) */
#define ASUS_WMI_METHODID_QMOD          0x444F4D51 /* Quiet MODe */
#define ASUS_WMI_METHODID_SPLV          0x4C425053 /* Set Panel Light Value */
#define ASUS_WMI_METHODID_SFUN          0x4E554653 /* FUNCtionalities */
#define ASUS_WMI_METHODID_SDSP          0x50534453 /* Set DiSPlay output */
#define ASUS_WMI_METHODID_GDSP          0x50534447 /* Get DiSPlay output */
#define ASUS_WMI_METHODID_DEVP          0x50564544 /* DEVice Policy */
#define ASUS_WMI_METHODID_OSVR          0x5256534F /* OS VeRsion */
#define ASUS_WMI_METHODID_DSTS          0x53544344 /* Device STatuS */
#define ASUS_WMI_METHODID_DSTS2         0x53545344 /* Device STatuS #2*/
#define ASUS_WMI_METHODID_BSTS          0x53545342 /* Bios STatuS ? */
#define ASUS_WMI_METHODID_DEVS          0x53564544 /* DEVice Set */
#define ASUS_WMI_METHODID_CFVS          0x53564643 /* CPU Frequency Volt Set */
#define ASUS_WMI_METHODID_KBFT          0x5446424B /* KeyBoard FilTer */
#define ASUS_WMI_METHODID_INIT          0x54494E49 /* INITialize */
#define ASUS_WMI_METHODID_HKEY          0x59454B48 /* Hot KEY ?? */

#define ASUS_WMI_UNSUPPORTED_METHOD     0xFFFFFFFE

/* Wireless */
#define ASUS_WMI_DEVID_HW_SWITCH        0x00010001
#define ASUS_WMI_DEVID_WIRELESS_LED     0x00010002
#define ASUS_WMI_DEVID_CWAP             0x00010003
#define ASUS_WMI_DEVID_WLAN             0x00010011
#define ASUS_WMI_DEVID_BLUETOOTH        0x00010013
#define ASUS_WMI_DEVID_GPS              0x00010015
#define ASUS_WMI_DEVID_WIMAX            0x00010017
#define ASUS_WMI_DEVID_WWAN3G           0x00010019
#define ASUS_WMI_DEVID_UWB              0x00010021

/* Leds */
/* 0x000200XX and 0x000400XX */
#define ASUS_WMI_DEVID_LED1             0x00020011
#define ASUS_WMI_DEVID_LED2             0x00020012
#define ASUS_WMI_DEVID_LED3             0x00020013
#define ASUS_WMI_DEVID_LED4             0x00020014
#define ASUS_WMI_DEVID_LED5             0x00020015
#define ASUS_WMI_DEVID_LED6             0x00020016

/* Backlight and Brightness */
#define ASUS_WMI_DEVID_BACKLIGHT        0x00050011
#define ASUS_WMI_DEVID_BRIGHTNESS       0x00050012
#define ASUS_WMI_DEVID_KBD_BACKLIGHT    0x00050021
#define ASUS_WMI_DEVID_LIGHT_SENSOR     0x00050022 /* ?? */

/* Misc */
#define ASUS_WMI_DEVID_CAMERA           0x00060013

/* Storage */
#define ASUS_WMI_DEVID_CARDREADER       0x00080013

/* Input */
#define ASUS_WMI_DEVID_TOUCHPAD         0x00100011
#define ASUS_WMI_DEVID_TOUCHPAD_LED     0x00100012

/* Fan, Thermal */
#define ASUS_WMI_DEVID_THERMAL_CTRL     0x00110011
#define ASUS_WMI_DEVID_FAN_CTRL         0x00110012

/* Power */
#define ASUS_WMI_DEVID_PROCESSOR_STATE  0x00120012

/* DSTS masks */
#define ASUS_WMI_DSTS_STATUS_BIT        0x00000001
#define ASUS_WMI_DSTS_UNKNOWN_BIT       0x00000002
#define ASUS_WMI_DSTS_PRESENCE_BIT      0x00010000
#define ASUS_WMI_DSTS_USER_BIT          0x00020000
#define ASUS_WMI_DSTS_BIOS_BIT          0x00040000
#define ASUS_WMI_DSTS_BRIGHTNESS_MASK   0x000000FF
#define ASUS_WMI_DSTS_MAX_BRIGTH_MASK   0x0000FF00


/*
 * <platform>/    - debugfs root directory
 *   dev_id      - current dev_id
 *   ctrl_param  - current ctrl_param
 *   method_id   - current method_id
 *   devs        - call DEVS(dev_id, ctrl_param) and print result
 *   dsts        - call DSTS(dev_id)  and print result
 *   call        - call method_id(dev_id, ctrl_param) and print result
 
 */

#define EEEPC_WMI_METHODID_SPEC 0x43455053
#define EEEPC_WMI_METHODID_DEVP 0x50564544
#define EEEPC_WMI_METHODID_DEVS	0x53564544
#define EEEPC_WMI_METHODID_DSTS	0x53544344
#define EEEPC_WMI_METHODID_CFVS	0x53564643

#define EEEPC_WMI_DEVID_BACKLIGHT	0x00050011
#define EEEPC_WMI_DEVID_BACKLIGHT2	0x00050012
#define EEEPC_WMI_DEVID_BLUETOOTH   0x00010013
#define EEEPC_WMI_DEVID_WIRELESS	0x00010011
#define EEEPC_WMI_DEVID_TRACKPAD	0x00100011

#define super AsusNBFnKeys

OSDefineMetaClassAndStructors(AsusWMIController, AsusNBFnKeys)


bool       AsusWMIController::init(OSDictionary *dictionary)
{

    keybrdBLightLvl = 0;//Stating with Zero Level
    panelBrighntessLevel = 16;//Mac starts with level 16
    res = 0;
    
    tochpadEnabled = true;//touch enabled by default on startup
    alsMode = false;
    isALSenabled  = true;
    isPanelBackLightOn = true;
    
    hasKeybrdBLight =false;
    hasMediaButtons = true;
	return super::init(dictionary);
}

bool       AsusWMIController::start(IOService *provider)
{
    	return super::start(provider);
}

void       AsusWMIController::stop(IOService *provider)
{
	super::stop(provider);
}

void       AsusWMIController::free(void)
{
	super::free();
}


IOService * AsusWMIController::probe(IOService *provider, SInt32 *score )
{
	IOService * ret = NULL;
	OSObject * obj;
	OSString * name;
	IOACPIPlatformDevice *dev;
	do
	{
		
		
		if (!super::probe(provider, score))
			continue;
		
		
		dev = OSDynamicCast(IOACPIPlatformDevice, provider);
		if (NULL == dev)
			continue;
		
		dev->evaluateObject("_UID", &obj);
		
		name = OSDynamicCast(OSString, obj);
		if (NULL == name)
			continue;
		
		
		if (name->isEqualTo("ATK"))
		{
			
			*score +=20;
			ret = this;
		}
		name->release();
		
    }
    while (false);
    
    //Reading the prefereces from the plist file
    OSDictionary *Configuration;
    Configuration = OSDynamicCast(OSDictionary, getProperty("Preferences"));
    if (Configuration){
        OSString *tmpString = 0;
        OSNumber *tmpNumber = 0;
        OSData   *tmpData = 0;
        OSBoolean *tmpBoolean = false;
        OSData   *tmpObj = 0;
        bool tmpBool = false;
        UInt64 tmpUI64 = 0;
        
        OSIterator *iter = 0;
        const OSSymbol *dictKey = 0;
        
        iter = OSCollectionIterator::withCollection(Configuration);
        if (iter) {
            while ((dictKey = (const OSSymbol *)iter->getNextObject())) {
                tmpObj = 0;
                
                tmpString = OSDynamicCast(OSString, Configuration->getObject(dictKey));
                if (tmpString) {
                    tmpObj = OSData::withBytes(tmpString->getCStringNoCopy(), tmpString->getLength()+1);
                }
                
                tmpNumber = OSDynamicCast(OSNumber, Configuration->getObject(dictKey));
                if (tmpNumber) {
                    tmpUI64 = tmpNumber->unsigned64BitValue();
                    tmpObj = OSData::withBytes(&tmpUI64, sizeof(UInt32));
                }
                
                tmpBoolean = OSDynamicCast(OSBoolean, Configuration->getObject(dictKey));
                if (tmpBoolean) {
                    tmpBool = (bool)tmpBoolean->getValue();
                    tmpObj = OSData::withBytes(&tmpBool, sizeof(bool));
                    
                }
                
                tmpData = OSDynamicCast(OSData, Configuration->getObject(dictKey));
                if (tmpData) {
                    tmpObj = tmpData;
                }
                if (tmpObj) {
                    //provider->setProperty(dictKey, tmpObj);
                    /*if(tmpUI64>0)
                        setProperty(dictKey->getCStringNoCopy(), tmpUI64 ,64);
                    else
                        setProperty(dictKey->getCStringNoCopy(), tmpBool?1:0 ,32);*/
                    
                    const char *tmpStr = dictKey->getCStringNoCopy();
                    
                    
                    if(!strncmp(dictKey->getCStringNoCopy(),"KeyboardBLightLevelAtBoot", strlen(tmpStr)))
                    {
                        keybrdBLightLvl = (UInt32)tmpUI64;
                        tmpUI64 = 0;
                    }
                    else if(!strncmp(dictKey->getCStringNoCopy(),"HasKeyboardBLight",strlen(tmpStr)))
                        hasKeybrdBLight = tmpBool;
                    
                    else if(!strncmp(dictKey->getCStringNoCopy(),"HasMediaButtons",strlen(tmpStr)))
                        hasMediaButtons = tmpBool;
                    
                    else if(!strncmp(dictKey->getCStringNoCopy(),"HasALSensor",strlen(tmpStr)))
                        hasALSensor = tmpBool;
                    
                    else if(!strncmp(dictKey->getCStringNoCopy(),"UsingAsusBackLightDriver",strlen(tmpStr)))
                        hasAsusBackLightDriver = tmpBool;
                    
                    else if(!strncmp(dictKey->getCStringNoCopy(),"ALS Turned on at boot",strlen(tmpStr)))
                        alsAtBoot = tmpBool;
                    
                }
            }
        }
    }
	
    return (ret);
}


const FnKeysKeyMap AsusWMIController::keyMap[] = {
	{0x30, NX_KEYTYPE_SOUND_UP, "NX_KEYTYPE_SOUND_UP"},
	{0x31, NX_KEYTYPE_SOUND_DOWN, "NX_KEYTYPE_SOUND_DOWN"},
	{0x32, NX_KEYTYPE_MUTE, "NX_KEYTYPE_MUTE"},
    {0x61, NX_KEYTYPE_VIDMIRROR, "NX_KEYTYPE_VIDMIRROR"},
    {0x10, NX_KEYTYPE_BRIGHTNESS_UP, "NX_KEYTYPE_BRIGHTNESS_UP"},
	{0x20, NX_KEYTYPE_BRIGHTNESS_DOWN, "NX_KEYTYPE_BRIGHTNESS_DOWN"},
    //Media buttons bound to Asus events keys Down, Left and Right Arrows in full keyboard
    {0x40, NX_KEYTYPE_PREVIOUS, "NX_KEYTYPE_PREVIOUS"},
    {0x41, NX_KEYTYPE_NEXT, "NX_KEYTYPE_NEXT"},
    {0x45, NX_KEYTYPE_PLAY, "NX_KEYTYPE_PLAY"},
    //Media button bound to Asus events keys C, V and Space keys in compact keyboard
    {0x8A, NX_KEYTYPE_PREVIOUS, "NX_KEYTYPE_PREVIOUS"},
    {0x82, NX_KEYTYPE_NEXT, "NX_KEYTYPE_NEXT"},
    {0x5C, NX_KEYTYPE_PLAY, "NX_KEYTYPE_PLAY"},
	{0,0xFF,NULL}
};


void AsusWMIController::enableEvent()
{
	
	if (super::enableFnKeyEvents(ASUS_WMI_MGMT_GUID, ASUS_WMI_METHODID_INIT) != kIOReturnSuccess)
		IOLog("Unable to enable events!!!\n");
	else
	{
		super::_keyboardDevice = new FnKeysHIKeyboardDevice;
		
		if ( !_keyboardDevice               ||
			!_keyboardDevice->init()       ||
			!_keyboardDevice->attach(this) )  // goto fail;
		{
			_keyboardDevice->release();
			IOLog("%s: Error creating keyboardDevice\n", this->getName());
		}
		else
		{
			_keyboardDevice->setKeyMap(keyMap);
			_keyboardDevice->registerService();
            
            IOLog("Asus notebooks Backlight Driver v1.7.2 by EMlyDinEsH(OSXLatitude) Copyright (c) 2012-2013, Credits:Hotkoffy(insanelymac)\n");
            
            
            //Setting Touchpad state on startup
            setProperty("TouchpadEnabled", true);
            
            /**** Keyboard brightness level at boot
            ***** Calling the keyboardBacklight Event for Setting the Backlight at boot ***/
            if(hasKeybrdBLight)
                keyboardBackLightEvent(keybrdBLightLvl);
            
            curKeybrdBlvl = keybrdBLightLvl;
            
            /**** ALS Sesnor at boot ***/
            if(alsAtBoot)
            {
                isALSenabled = !isALSenabled;
                params[0] = OSNumber::withNumber(isALSenabled, 8);
                WMIDevice->evaluateInteger("ALSC", &res,params,1);
            
                IOLog("AsusNBFnKeys: ALS turned on at boot %d\n",res);
            }
            
            IOLog("%s: Asus Fn Hotkey Events Enabled\n", this->getName());
           
        }
	}
    
}


void AsusWMIController::disableEvent()
{
	if (_keyboardDevice)
		_keyboardDevice->release();
}

void AsusWMIController::ReadPanelBrightnessValue()
{
        //
        //Reading AppleBezel Values from Apple Backlight Panel driver for controlling the bezel levels
        //                              
        
          IORegistryEntry *displayDeviceEntry = IORegistryEntry::fromPath("IOService:/AppleACPIPlatformExpert/PCI0@0/AppleACPIPCI/GFX0@2/AppleIntelFramebuffer@0/display0/AppleBacklightDisplay");
    
            if (displayDeviceEntry != NULL) {
            
                OSNumber *brightnessValue = 0;
                OSDictionary *ioDisplayParaDict = 0;
                
            ioDisplayParaDict = OSDynamicCast(OSDictionary, displayDeviceEntry->getProperty("IODisplayParameters"));
                
                if(ioDisplayParaDict)
                    {                        
                                OSDictionary  *brightnessDict = 0;
                                OSIterator *brightnessIter = 0;
                        
                                brightnessDict = OSDynamicCast(OSDictionary, ioDisplayParaDict->getObject("brightness"));
                        
                            if(brightnessDict){
                                const OSSymbol *dicKey = 0;
                                
                                
                                brightnessIter = OSCollectionIterator::withCollection(brightnessDict);
                                                                
                                if(brightnessIter)
                                {
                                    while((dicKey = (const OSSymbol *)brightnessIter->getNextObject()))
                                    {
                                
                                    //IOLog("AsusNB: Brightness %s\n",dicKey->getCStringNoCopy());
                                    brightnessValue = OSDynamicCast(OSNumber, brightnessDict->getObject(dicKey));
                                    
                                        if(brightnessValue)
                                        {
                                          if(brightnessValue->unsigned32BitValue() != 0)
                                            panelBrighntessLevel = brightnessValue->unsigned32BitValue()/64;
                                            //IOLog("AsusNB: PB %d BValue %d\n",panelBrighntessLevel,brightnessValue->unsigned32BitValue());
                                        }
                                    
                                    }

                                }
                            }
                        }
                }
}

void AsusWMIController::handleMessage(int code)
{
    loopCount = kLoopCount = res = 0;
	asusBackLightMode = alsMode = false;
    
    //Processing the code
	switch (code) {
		case 0x57: //AC disconnected
		case 0x58: //AC connected
			//ignore silently
			break;
            
         //Backlight
        case 0x33://hardwired On
        case 0x34://hardwired Off
        case 0x35://Soft Event, Fn + F7
            if(!hasAsusBackLightDriver)
            {
                if(isPanelBackLightOn)
                {
                    code = NOTIFY_BRIGHTNESS_DOWN_MIN;
                    loopCount = 16;
                    
                    //Read Panel brigthness value to restore later with backlight toggle
                    ReadPanelBrightnessValue();
                }
                else
                {
                    code = NOTIFY_BRIGHTNESS_UP_MIN;
                    loopCount = panelBrighntessLevel;
                }
                
                isPanelBackLightOn = !isPanelBackLightOn;
                
            }
			break;
			
		case 0x6B: //Fn + F9, Tochpad On/Off
            tochpadEnabled = !tochpadEnabled;
            if(tochpadEnabled)
            {
                setProperty("TouchpadEnabled", true);
                removeProperty("TouchpadDisabled");
            }
            else
            {
                removeProperty("TouchpadEnabled");
                setProperty("TouchpadDisabled", true);
            }
			break;
			
		case 0x5C: //Fn + space bar, Processor Speedstepping changes
            
            /*params[0] =OSNumber::withNumber(4, 8);
            
            if(WMIDevice->evaluateInteger("PSTT", &res, params, 1))
                IOLog("AsusNBFnKeys: Processor speedstep Changed\n");
            else
                IOLog("AsusNBFnKeys: Processor speedstep change failed %d\n",res);*/
                
			break;
            
        case 0x7A: // Fn + A, ALS Sensor
            isALSenabled = !isALSenabled;
            
            params[0] =OSNumber::withNumber(isALSenabled, 8);
                        
            if(WMIDevice->evaluateInteger("ALSC", &res, params, 1))
                IOLog("AsusNBFnKeys: ALS Enabled %d\n",isALSenabled);
            else
                IOLog("AsusNBFnKeys: ALS Disabled %d E %d\n",res,isALSenabled);
            break;
            
        case 0xC6: //ALS Notifcations
            if(hasALSensor)
            {
                code = processALS();
                alsMode = true;
            }
			break;
            
        case 0xC7: //ALS Notifcations (Optional)
            if(hasALSensor)
            {
                code = processALS();
                alsMode = true;
            }
			break;
            
        case 0xC5: //Fn + F3,Decrease Keyboard Backlight
            if(hasKeybrdBLight)
            {
                if(keybrdBLightLvl>0)
                    keybrdBLightLvl--;
                else
                    keybrdBLightLvl = 0;
                
                keyboardBackLightEvent(keybrdBLightLvl);
                
                curKeybrdBlvl  = keybrdBLightLvl;
                    
                //Updating value in ioregistry
                 setProperty("KeyboardBLightLevel", keybrdBLightLvl,32);
                            
            }        
       
            break;
            
        case 0xC4: //Fn + F4, Increase Keyboard Backlight
            if(hasKeybrdBLight)
            {
                if(keybrdBLightLvl == 3)
                    keybrdBLightLvl = 3;
                else
                    keybrdBLightLvl++;
                
                keyboardBackLightEvent(keybrdBLightLvl);
                
                curKeybrdBlvl  = keybrdBLightLvl;
                
                 //Updating value in ioregistry
                 setProperty("KeyboardBLightLevel", keybrdBLightLvl,32);
               
            }
            
			break;
            
		default:
            //Fn + F5, Panel Brightness Down
            if(code >= NOTIFY_BRIGHTNESS_DOWN_MIN && code<= NOTIFY_BRIGHTNESS_DOWN_MAX)
                {
                    code = NOTIFY_BRIGHTNESS_DOWN_MIN;
                    
                    if(panelBrighntessLevel > 0)
                    panelBrighntessLevel--;
                }
            //Fn + F6, Panel Brightness Up
            else if(code >= NOTIFY_BRIGHTNESS_UP_MIN && code<= NOTIFY_BRIGHTNESS_UP_MAX)
                {
                    code = NOTIFY_BRIGHTNESS_UP_MIN;
                    
                    panelBrighntessLevel++;
                    if(panelBrighntessLevel>16)
                        panelBrighntessLevel = 16;
                }
            
            if(hasAsusBackLightDriver)
            {
            //
            //Reading AppleBezel Values from Asus Backlight Panel driver for controlling the bezel levels
            //                              
            
              IORegistryEntry *fnDeviceEntry = IORegistryEntry::fromPath("IOService:/AppleACPIPlatformExpert/PNLF/AsusACPIBacklightPanel");
                if (fnDeviceEntry != NULL) {
                
                    OSNumber *tmpNum = 0;
                    tmpNum = OSDynamicCast(OSNumber, fnDeviceEntry->getProperty("AppleBezelLevel"));
                    
                    appleBezelValue = tmpNum->unsigned32BitValue();
                    
                    asusBackLightMode = true;
                                        
                    if(code == NOTIFY_BRIGHTNESS_UP_MIN)//going up
                        {
                            
                            if(appleBezelValue<8)//we're in between level 0-7
                                loopCount = 1;
                            else if(appleBezelValue == 8)//we're doing level 9
                                loopCount = 2;
                            else if(appleBezelValue == 9 || appleBezelValue == 10)//we're going level 10/11
                                loopCount = 3;
                            
                       }
                       else//going down
                       {
                                appleBezelValue--;
                           
                            //we're going level 10/9 we receive 11 for both 11 & 10 levels
                            if(appleBezelValue == 10 || appleBezelValue == 9)
                                loopCount = 3;
                            else if(appleBezelValue == 8)//we're doing level 8
                                loopCount = 2;
                            else //we're going level <8
                                loopCount = 1;
                       }
                    //IOLog("AsusNBFnKeys: Device Entry Found %d AM %d\n",appleBezelValue, asusBackLightMode);
                  
                    fnDeviceEntry->release();
                }
             
            }
            break;
    }
    
    //IOLog("AsusNBFnKeys: Received Key %d(0x%x) ALS mode %d\n",code, code, alsMode);
    
    
    
    //have media buttons then skip C, V and Space & ALS sensor keys events
    if(hasMediaButtons && (code == 0x8A || code == 0x82 || code == 0x5c || code == 0xc6 || code == 0x5c))
        return;
        
    //Sending the code for the keyboard handler
    super::processFnKeyEvents(code, alsMode, kLoopCount, asusBackLightMode,loopCount);
    
    //Clearing ALS mode after processing
    if(alsMode)
        alsMode = false;
    
}

UInt32 AsusWMIController::processALS()
{
    UInt32 brightnessLvlcode;
    keybrdBLightLvl = 0;
    
    WMIDevice->evaluateInteger("ALSS", &keybrdBLightLvl, NULL, NULL);
                //IOLog("AsusNBFnKeys: ALS %d\n",keybrdBLightLvl);
            
                if(keybrdBLightLvl == 1 && curKeybrdBlvl > keybrdBLightLvl)
                {
                    brightnessLvlcode = NOTIFY_BRIGHTNESS_DOWN_MIN;
                    kLoopCount = 6;
                }
                else if(keybrdBLightLvl == 1 && curKeybrdBlvl < keybrdBLightLvl)
                {
                    brightnessLvlcode = NOTIFY_BRIGHTNESS_UP_MIN;
                    kLoopCount = 6;
                }
                else if(keybrdBLightLvl == 2 && curKeybrdBlvl > keybrdBLightLvl)
                {
                    brightnessLvlcode = NOTIFY_BRIGHTNESS_DOWN_MIN;
                    kLoopCount = 3;
                }
                else if(keybrdBLightLvl == 2 && curKeybrdBlvl < keybrdBLightLvl)
                {
                    brightnessLvlcode = NOTIFY_BRIGHTNESS_UP_MIN;
                    kLoopCount = 3;
                }
                else if(keybrdBLightLvl == 3 && curKeybrdBlvl > keybrdBLightLvl)
                {
                    brightnessLvlcode = NOTIFY_BRIGHTNESS_DOWN_MIN;
                    kLoopCount = 3;
                }
                else if(keybrdBLightLvl == 3 && curKeybrdBlvl < keybrdBLightLvl)
                {
                    brightnessLvlcode = NOTIFY_BRIGHTNESS_UP_MIN;
                    kLoopCount = 3;
                }
                else if(keybrdBLightLvl == 4 && curKeybrdBlvl > keybrdBLightLvl)
                {
                    brightnessLvlcode = NOTIFY_BRIGHTNESS_DOWN_MIN;
                    kLoopCount = 6;
                }
                else if(keybrdBLightLvl == 4 && curKeybrdBlvl < keybrdBLightLvl)
                {
                    brightnessLvlcode = NOTIFY_BRIGHTNESS_UP_MIN;
                    kLoopCount = 6;
                }
                else
                {
                    brightnessLvlcode = 0xC6;//ALS event code which does nothing
                    kLoopCount = 0;
                }
    
    curKeybrdBlvl = keybrdBLightLvl;
    
    return brightnessLvlcode;
}

//Keyboard Backlight set
void AsusWMIController::keyboardBackLightEvent(UInt32 level)
{
    OSObject * params[1];
    OSObject * ret = NULL;

   	params[0] =OSNumber::withNumber(level,8);

    //Asus WMI Specific Method Inside the DSDT
    //Calling the Method SLKB from the DSDT For setting Keyboard Backlight control in DSDT
    WMIDevice->evaluateObject("SKBL", &ret, params,1);
    
}
//Keyboard Backlight Alternate
/*void AsusWMIController::keyboardBackLightEvent()
 {
 UInt32 status = -1;
 getDeviceStatus(ASUS_NB_WMI_EVENT_GUID, ASUS_WMI_METHODID_DSTS2, ASUS_WMI_DEVID_KBD_BACKLIGHT, &status);
 status = (status & 0x0001) xor 1;
 setDeviceStatus(ASUS_NB_WMI_EVENT_GUID, ASUS_WMI_METHODID_DEVS, ASUS_WMI_DEVID_KBD_BACKLIGHT, &status);
     IOLog("AsusNBFnKeys: Keyboard Backlight\n");

 }*/
