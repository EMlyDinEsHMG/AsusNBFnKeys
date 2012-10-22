/*
 *  Copyright (c) 2012 Hotkoffy and EMlyDinEsHMG. All rights reserved.
 *
 *  IOWMIController Driver ported from Linux by Hotkoffy and modified to Asus by EMlyDinEsHMG
 *
 *  WMIHIKeyboardDevice.cpp
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


#include "WMIHIKeyboardDevice.h"
#include "AsusNBWMI.h"

#define DEBUG_START 0

#if DEBUG_START
#define DEBUG_LOG(fmt, args...) IOLog(fmt, ## args)
#else
#define DEBUG_LOG(fmt, args...)
#endif

#define super IOService
OSDefineMetaClassAndStructors(WMIHIKeyboardDevice, IOService);


bool WMIHIKeyboardDevice::attach( IOService * provider )
{
	if( !super::attach(provider) )  return false;
	
	wmi = OSDynamicCast(AsusNBWMI ,provider);
	if (NULL == wmi)
		return false;
	
	wmi->retain();
	
	return true;
}


void WMIHIKeyboardDevice::detach( IOService * provider )
{
	wmi->release();
	wmi = 0;
	
	super::detach(provider);
}



void WMIHIKeyboardDevice::keyPressed(int code)
{
	int i = 0, out;
	do
        
	{

		if (keyMap[i].description == NULL && keyMap[i].in == 0 && keyMap[i].out == 0xFF)
		{
			DEBUG_LOG("%s: Unknown key %02X i=%d\n",this->getName(), code, i);
			break;
		}
		if (keyMap[i].in == code)
		{
            DEBUG_LOG("%s: Key Pressed %02X i=%d\n",this->getName(), code, i);
			out = keyMap[i].out;
			messageClients(kIOACPIMessageDeviceNotification, &out);
			break;
		}
		i++;
	}
	while (true);	
}


void WMIHIKeyboardDevice::setKeyMap(const wmiKeyMap *_keyMap)
{
	int i = 0;
	keyMap = _keyMap;
	OSDictionary *dict = OSDictionary::withCapacity(10);
	DEBUG_LOG("%s: Setting key %02X i=%d\n",this->getName(), keyMap[i].in, i);
	do
	{
		if (keyMap[i].description == NULL && keyMap[i].in == 0 && keyMap[i].out == 0xFF)
			break;
        DEBUG_LOG("%s: Setting key %02X i=%d\n",this->getName(), keyMap[i].in, i);
		dict->setObject(keyMap[i].description, OSNumber::withNumber(keyMap[i].in,8));
		i++;
	}
	while (true);
	
	setProperty("KeyMap", dict);
}
