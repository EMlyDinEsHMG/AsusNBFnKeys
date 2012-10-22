/*
 *  Copyright (c) 2012 Hotkoffy and EMlyDinEsHMG. All rights reserved.
 *
 *  IOWMIController Driver ported from Linux by Hotkoffy and modified to Asus by EMlyDinEsHMG
 *
 *  WMIHIKeyboardDevice.h
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

#ifndef _WMIHIKeyboardDevice_h
#define _WMIHIKeyboardDevice_h

#include <IOKit/IOService.h>


typedef struct  {
	UInt16 in;
	UInt8 out;
	const char *description;
} wmiKeyMap;



class AsusNBWMI;

class WMIHIKeyboardDevice : public IOService
{
	OSDeclareDefaultStructors(WMIHIKeyboardDevice);
	
private:
	AsusNBWMI *wmi;
	
public:
	virtual bool attach(IOService * provider);
	virtual void detach(IOService * provider);
	
	void keyPressed(int code); 
	
	//IOReturn message( UInt32 type, IOService * provider, void * argument);

	const wmiKeyMap * keyMap;
	void setKeyMap(const wmiKeyMap * _keyMap);
	
};

#endif