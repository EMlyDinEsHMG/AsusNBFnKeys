/*
 *  Copyright (c) 2012 Hotkoffy and EMlyDinEsHMG. All rights reserved.
 *
 *  AsusNBWMI Driver ported from Linux by Hotkoffy and modified to Asus by EMlyDinEsHMG
 *
 *  AsusNBWMI.h
 *  AsusNBWMI
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

#ifndef _AsusNBWMI_h
#define _AsusNBWMI_h

#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOService.h>
#include <sys/errno.h>
#include <mach/kern_return.h>
#include <sys/kern_control.h>
#include <IOKit/IOlib.h>
#include <libkern/OSTypes.h>

#include  "WMIHIKeyboardDevice.h"



struct guid_block {
	char guid[16];
	union {
		char object_id[2];
		struct {
			unsigned char notify_id;
			unsigned char reserved;
		};
	};
	UInt8 instance_count;
	UInt8 flags;
};



/*
 * If the GUID data block is marked as expensive, we must enable and
 * explicitily disable data collection.
 */
#define ACPI_WMI_EXPENSIVE   0x1
#define ACPI_WMI_METHOD      0x2	/* GUID is a method */
#define ACPI_WMI_STRING      0x4	/* GUID takes & returns a string */
#define ACPI_WMI_EVENT       0x8	/* GUID is an event */

enum
{
	kPowerStateOff = 0,
	kPowerStateOn,
	kPowerStateCount
};


class AsusNBWMI : public IOService
{
    OSDeclareDefaultStructors(AsusNBWMI)
    
protected:
    IOACPIPlatformDevice * WMIDevice;
    WMIHIKeyboardDevice * _keyboardDevice;
    
    OSDictionary * properties;
    
public:
    IOReturn message( UInt32 type, IOService * provider, void * argument);
    IOReturn poller( void );
    
    // standard IOKit methods
    virtual bool       init(OSDictionary *dictionary = 0);
    virtual bool       start(IOService *provider);
    virtual void       stop(IOService *provider);
    virtual void       free(void);
    virtual IOService *probe(IOService *provider, SInt32 *score);
    
    //power management events
    virtual IOReturn	setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker);
    
protected:
    OSDictionary* getDictByUUID(const char * guid);
    IOReturn setEvent(const char * guid, UInt32 methodID);
    
    virtual void enableEvent();
    virtual void disableEvent();
    virtual void handleMessage(int code);
    virtual void rfkillEvent();
    
    void getDeviceStatus(const char * guid, UInt32 methodId, UInt32 deviceId, UInt32 *status);
    void setDeviceStatus(const char * guid, UInt32 methodId, UInt32 deviceId, UInt32 *status);
    void setDevice(const char * guid, UInt32 methodId, UInt32 *status);
    
    static const wmiKeyMap keyMap[];
    
private:
    int parse_wdg(OSDictionary *dict);
    OSString *flagsToStr(UInt8 flags);
    void wmi_wdg2reg(struct guid_block *g, OSArray *array, OSArray *dataArray);
    OSDictionary * readDataBlock(char *str);
    
    //utilities
    int wmi_data2Str(const char *in, char *out);
#ifdef DEBUG
    bool wmi_parse_guid(const UInt8 *src, UInt8 *dest);
    void wmi_dump_wdg(struct guid_block *g);
    int wmi_parse_hexbyte(const UInt8 *src);
    void wmi_swap_bytes(UInt8 *src, UInt8 *dest);
#endif
    
};

#endif //_AsusNBWMI_h
