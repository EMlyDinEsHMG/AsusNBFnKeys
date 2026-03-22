/*
 *  Copyright (c) EMlyDinEsH (mg-dinesh@live.com) 2012-2018. All rights reserved.
 *
 *
 *  FnKeysHIKeyboardDevice.cpp
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

#include "FnKeysHIKeyboardDevice.h"
#include "AsusNBFnKeys.h"

#define super IOService

OSDefineMetaClassAndStructors(FnKeysHIKeyboardDevice, IOService);

bool FnKeysHIKeyboardDevice::attach( IOService * provider )
{
	if( !super::attach(provider) )  return false;
    
    IOLog("%s: Attaching..\n", this->getName());

    //
    // Obtain a referece to Asus Fn keys who is our provider
    // to see whether it exists or not, so we can start.
    //
    
	_FnKeys = OSDynamicCast(AsusNBFnKeys ,provider);
	
    if (NULL == _FnKeys) {
        IOLog("%s: Failed to find Asus Fn keys provider.\n", this->getName());
		return false;
    }
	
	_FnKeys->retain();
	
	return true;
}

void FnKeysHIKeyboardDevice::detach( IOService * provider )
{
    IOLog("%s: Detaching..\n", this->getName());

    //
    // Release our reference to Asus Fn keys
    //
    
	_FnKeys->release();
	_FnKeys = 0;
	
	super::detach(provider);
}

void FnKeysHIKeyboardDevice::keyPressed(int code)
{
    messageClients(kIOACPIMessageDeviceNotification, &code);
}