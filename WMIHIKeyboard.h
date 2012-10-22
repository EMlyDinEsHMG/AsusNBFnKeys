/*
 *  Copyright (c) 2012 Hotkoffy and EMlyDinEsHMG. All rights reserved.
 *
 *  IOWMIController Driver ported from Linux by Hotkoffy and modified to Asus by EMlyDinEsHMG
 *
 *  WMIHIDKeyboard.h
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

#ifndef _WMIHIDKeyboard_h
#define _WMIHIDKeyboard_h

#include <IOKit/hidsystem/IOHIKeyboard.h>

class WMIHIKeyboardDevice;

class WMIHIKeyboard : public IOHIKeyboard
{
	OSDeclareDefaultStructors(WMIHIKeyboard)
	
private:
	WMIHIKeyboardDevice *Device;
	clock_sec_t  lastEventSecs;
	clock_usec_t lastEventMicrosecs;
	
public:
	// standard IOKit methods
	virtual bool       init(OSDictionary *dictionary = 0);
	virtual bool       start(IOService *provider);
	virtual void       stop(IOService *provider);
	virtual void       free(void);
	
	IOReturn message( UInt32 type, IOService * provider, void * argument);
	
	// IOHIKeyboard specific methods
	virtual const unsigned char * defaultKeymapOfLength(UInt32 * length);
};

#endif