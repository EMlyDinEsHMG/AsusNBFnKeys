/*
 *  Copyright (c) 2012 Hotkoffy and EMlyDinEsHMG. All rights reserved.
 *
 *  AsusNBWMI Driver ported from Linux by Hotkoffy and modified to Asus by EMlyDinEsHMG
 *
 *  AsusWMIController.cpp
 *  IOWMIFamily
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

#define DEBUG_START 0

#if DEBUG_START
#define DEBUG_LOG(fmt, args...) IOLog(fmt, ## args)
#else
#define DEBUG_LOG(fmt, args...)
#endif


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

#define super AsusNBWMI

OSDefineMetaClassAndStructors(AsusWMIController, AsusNBWMI)


bool       AsusWMIController::init(OSDictionary *dictionary)
{

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
	
    return (ret);
}


const wmiKeyMap AsusWMIController::keyMap[] = {
	{0x30, NX_KEYTYPE_SOUND_UP, "NX_KEYTYPE_SOUND_UP"},
	{0x31, NX_KEYTYPE_SOUND_DOWN, "NX_KEYTYPE_SOUND_DOWN"},
	{0x32, NX_KEYTYPE_MUTE, "NX_KEYTYPE_MUTE"},
    {0xCC, NX_KEYTYPE_VIDMIRROR, "NX_KEYTYPE_VIDMIRROR"},
    {0x40, NX_KEYTYPE_PREVIOUS, "NX_KEYTYPE_PREVIOUS"},
    {0x41, NX_KEYTYPE_NEXT, "NX_KEYTYPE_NEXT"},
    {0x45, NX_KEYTYPE_PLAY, "NX_KEYTYPE_PLAY"},
	{0,0xFF,NULL}
};


void AsusWMIController::enableEvent()
{
	DEBUG_LOG("%s: AsusWMIController::enableEvent()\n", this->getName());
	
	if (super::setEvent(ASUS_WMI_MGMT_GUID, ASUS_WMI_METHODID_INIT) != kIOReturnSuccess)
		IOLog("Unable to enable events!!!\n");
	else
	{
		super::_keyboardDevice = new WMIHIKeyboardDevice;
		
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
            IOLog("%s: Hotkey Events Enabled\n", this->getName());
            IOLog("WMIController Driver ported by Hotkoffy(insanelymac) and modified to AsusNB by EMlyDinEsHMG Copyright (c) 2012\n");

		}
	}
	
}


void AsusWMIController::disableEvent()
{
	if (_keyboardDevice)
	{
        super::setEvent(ASUS_NB_WMI_EVENT_GUID, false);
		_keyboardDevice->release();
	}
}


void AsusWMIController::handleMessage(int code)
{
    super::handleMessage(code);
    
}


//trackpad led ON/OFF only !
/*void AsusWMIController::trackPadEvent()
 {
 UInt32 status = -1;
 getDeviceStatus(EEEPC_WMI_MGMT_GUID, EEEPC_WMI_METHODID_DSTS, EEEPC_WMI_DEVID_TRACKPAD, &status);
 status = (status & 0x0001) xor 1;
 setDeviceStatus(EEEPC_WMI_MGMT_GUID, EEEPC_WMI_METHODID_DEVS, EEEPC_WMI_DEVID_TRACKPAD, &status);
 }*/
