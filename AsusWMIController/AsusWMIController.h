/*
 *  Copyright (c) 2012 - 2013 EMlyDinEsH(OSXLatitude). All rights reserved.
 *
 *
 *  AsusWMIController.h
 *  Asus ATK Device Controller
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

#ifndef _AsusWMIController_h
#define _AsusWMIController_h

#include <IOKit/IOService.h>

#include "AsusNBFnKeys.h"
#include  "FnKeysHIKeyboardDevice.h"

const UInt8 NOTIFY_BRIGHTNESS_UP_MIN = 0x10;
const UInt8 NOTIFY_BRIGHTNESS_UP_MAX = 0x1F;

const UInt8 NOTIFY_BRIGHTNESS_DOWN_MIN = 0x20;
const UInt8 NOTIFY_BRIGHTNESS_DOWN_MAX = 0x2F;


class AsusWMIController : public AsusNBFnKeys
{
	
	OSDeclareDefaultStructors(AsusWMIController)
	
public:
	virtual bool       init(OSDictionary *dictionary = 0);
	virtual bool       start(IOService *provider);
	virtual void       stop(IOService *provider);
	virtual void       free(void);
	virtual IOService * probe(IOService *provider, SInt32 *score );
	
protected:
    
    UInt32 keybrdBLightLvl, curKeybrdBlvl, panelBrighntessLevel, appleBezelValue;
    bool   tochpadEnabled;
    bool   alsMode, hasALSensor, isALSenabled, alsAtBoot;
    bool   asusBackLightMode, hasAsusBackLightDriver;
    bool   isPanelBackLightOn;
    bool   hasMediaButtons, hasKeybrdBLight;
    int loopCount, kLoopCount;
    OSObject * params[1];
    UInt32 res;
    
	virtual void enableEvent();
	virtual void disableEvent();
	virtual void handleMessage(int code);
    virtual UInt32 processALS();
	virtual void keyboardBackLightEvent(UInt32 level);
    virtual void ReadPanelBrightnessValue();
    
	static const FnKeysKeyMap keyMap[];
	
};

#endif