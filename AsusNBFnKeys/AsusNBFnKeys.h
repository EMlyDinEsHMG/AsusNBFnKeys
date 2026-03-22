/*
 *  Copyright (c) EMlyDinEsH (mg-dinesh@live.com) 2012-2018. All rights reserved.
 *
 *  Asus Notebooks Fn keys driver v2.6 for Mac OSX
 *
 *  Credits: Hotkoffy(insanelymac) for the initial source and ideas
 *
 *  AsusNBFnKeys.h
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

#ifndef _AsusNBFnKeys_h
#define _AsusNBFnKeys_h

#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOService.h>
#include <IOKit/IONVRAM.h>
#include <IOKit/IOTimerEventSource.h>

#include  "FnKeysHIKeyboardDevice.h"

/* Plist config keys */
#define HasMediaFnKeys          "HasMediaFnKeys"
#define MediaKeysDoFastRewind   "MediaKeysDoFastRewind"
#define SoftDisplayBacklightOff "SoftDisplayBacklightOff"
#define F3KeyFunction           "F3KeyFunction"
#define F4KeyFunction           "F4KeyFunction"
#define InstantKeyFunction      "InstantKeyFunction"
#define KBackLightlvlAtBoot     "KBackLightlvlAtBoot"
#define DimBacklightOnACDC      "DimBacklightOnAC/DC"
#define DimBrightnessByLevels   "DimBrightnessByLevels"
#define FinerBrightnessControl  "FinerBrightnessControl"
#define FinerVolumeControl      "FinerVolumeControl"
#define DisplayUsesNvidiaGPU    "DisplayUsesNvidiaGPU"
#define IdleKBacklightAutoOff   "IdleKBacklightAutoOff"
#define IdleKBacklightAutoOffTimeout "IdleKBacklightAutoOffTimeout"
#define MakeFnKeysAsFunction        "MakeFnKeysAsFunction"

#define ConfigForALS            "ConfigForALS"
#define TimerInterval           "TimerInterval"
#define EnableAtBoot            "EnableAtBoot"
#define DisableOnFnControl      "DisableOnFnControl"
#define DisableOnAC             "DisableOnAC"
#define SamplesToProcess        "SamplesToProcess"
#define EnableALSLogs           "EnableALSLogs"

#define Level1RangeStart      "Level1RangeStart"
#define Level2RangeStart      "Level2RangeStart"
#define Level3RangeStart      "Level3RangeStart"
#define Level4RangeStart      "Level4RangeStart"
#define Level5RangeStart      "Level5RangeStart"

#define Level1RangeEnd        "Level1RangeEnd"
#define Level2RangeEnd        "Level2RangeEnd"
#define Level3RangeEnd        "Level3RangeEnd"
#define Level4RangeEnd        "Level4RangeEnd"
#define Level5RangeEnd        "Level5RangeEnd"

#define Level1Brightness        "Level1Brightness"
#define Level2Brightness        "Level2Brightness"
#define Level3Brightness        "Level3Brightness"
#define Level4Brightness        "Level4Brightness"
#define Level5Brightness        "Level5Brightness"


/* WMI Event ID */
#define ASUS_WMI_MGMT_GUID              "97845ED0-4E6D-11DE-8A39-0800200C9A66"

#define ASUS_NB_WMI_EVENT_GUID          "0B3CBB35-E3C2-45ED-91C2-4C5A6D195D1C"
#define ASUS_EEEPC_WMI_EVENT_GUID       "ABBC0F72-8EA1-11D1-00A0-C90629100000"

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

/* Deep S3 / Resume on LID open */
#define ASUS_WMI_DEVID_LID_RESUME       0x00120031

/* DSTS masks */
#define ASUS_WMI_DSTS_STATUS_BIT        0x00000001
#define ASUS_WMI_DSTS_UNKNOWN_BIT       0x00000002
#define ASUS_WMI_DSTS_PRESENCE_BIT      0x00010000
#define ASUS_WMI_DSTS_USER_BIT          0x00020000
#define ASUS_WMI_DSTS_BIOS_BIT          0x00040000
#define ASUS_WMI_DSTS_BRIGHTNESS_MASK   0x000000FF
#define ASUS_WMI_DSTS_MAX_BRIGTH_MASK   0x0000FF00


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

/* Fn key events */
#define NOTIFY_LAUNCHPAD     0x51
#define NOTIFY_MISSON_CNTL   0x52
#define NOTIFY_DASHBOARD     0x53
#define NOTIFY_DESKTOP       0x54
#define NOTIFY_APPWINDS      0x55
#define NOTIFY_FAST_REL      0x56
#define NOTIFY_REWIND_REL    0x57
#define NOTIFY_EJECT         0x58

#define NOTIFY_VOL_MUTE      0x4a
#define NOTIFY_VOL_UP_MIN    0x4b
#define NOTIFY_VOL_DOWN_MIN  0x4c
#define NOTIFY_VOL_UP        0x48
#define NOTIFY_VOL_DOWN      0x49

#define NOTIFY_MEDIA_PLAY    0x42
#define NOTIFY_MEDIA_NEXT    0x44
#define NOTIFY_MEDIA_PREV    0x46
#define NOTIFY_MEDIA_FAST    0x40
#define NOTIFY_MEDIA_REWIND  0x34
#define NOTIFY_VID_MIRROR    0x50


#define NOTIFY_BRIGHTNESS_UP_MIN  0x10
#define NOTIFY_BRIGHTNESS_UP_MAX  0x1F
#define NOTIFY_BRIGHTNESS_UP      0x4D


#define NOTIFY_BRIGHTNESS_DOWN_MIN  0x20
#define NOTIFY_BRIGHTNESS_DOWN_MAX  0x2F
#define NOTIFY_BRIGHTNESS_DOWN      0x4F

/*
 * If the GUID data block is marked as expensive, we must enable and
 * explicitily disable data collection.
 */

#define ACPI_WMI_EXPENSIVE   0x1
#define ACPI_WMI_METHOD      0x2	/* GUID is a method */
#define ACPI_WMI_STRING      0x4	/* GUID takes & returns a string */
#define ACPI_WMI_EVENT       0x8	/* GUID is an event */


class AsusNBFnKeys : public IOService
{
    OSDeclareDefaultStructors(AsusNBFnKeys)
    
protected:
    FnKeysHIKeyboardDevice  * _keyboardDevice;
    IOACPIPlatformDevice    * _wmiDevice;
    IOWorkLoop              * _bWorkLoop;
    AbsoluteTime now;

    IOTimerEventSource      * _brightnessTimer;

    struct diplayStruct {
        IORegistryEntry* deviceEntry;
        
        SInt8   brighntessLevel;
        UInt8   blvlsToMove;
        UInt8   asusBrightnessLvl;
        UInt8   dimBLevels;
        
        bool    softBackLightOff;
        bool    backlightOn, dimOnBatAC;
        bool    usingAsusBackLight;
        
        int     brightness;
    } _display;
    
    union gpuPath {
        char iGPU[62];
        char dGPU[85];
    } _gpuPath;
    
    union gpuDPath {
        char iGPU[117];
        char dGPU1[129], dGPU2[138];
    } _gpuDisplayPath;
    
    struct kBoardBLight {
        IOTimerEventSource* timer;
        UInt32  offTimeout;
        SInt16  level;
        bool    hasAutoShutOff;
        bool    exists, has16Lvls;
        bool    idleOff;
    } _kBLight;
    
    struct alsStruct {
        IOTimerEventSource*  timer;
        UInt32 timerInterval;

        UInt32 levelRStart[5], levelREnd[5];
        UInt8  brightnessForLevel[5];
        UInt8  samples;
        
        UInt32 ali_total, ali_avg, ali_notify_counts;
        
        bool   processing, logsEnabled;
        bool   exists, enabled, atBoot, disabled;
        bool   disableOnAC, disableOnFnCtrl, disableDispBritCtrl;
    } _als;
    
    UInt8  _f3KeyAction, _f4KeyAction, _instantKeyAction, _fnKeyAction;

    char   _pciBridgeName[15];  // PCI2PCI Bridge ioreg name
    char   _pciDeviceName[5];   // PCI Express Device name
    
    bool   _isEEEpcWMI;
    bool   _touchPadEnabled;

    bool   _workAsFunctionKeys;
    bool   _useFinerBrightnessControl, _useFinerVolumeControl;
    bool   _hasMediaButtons;
    bool   _mediaDoesFastRewind, _didMediaForward, _didMediaRewind;
    
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

public:
    IOReturn message( UInt32 type, IOService * provider, void * argument);
    
    // standard IOKit methods
    virtual bool       init(OSDictionary *dictionary = 0);
    virtual bool       start(IOService *provider);
    virtual void       stop(IOService *provider);
    virtual void       free(void);
    virtual IOService *probe(IOService *provider, SInt32 *score);
    
    // power management event
    virtual IOReturn	setPowerState(unsigned long powerStateOrdinal, IOService *policyMaker);
    
    // Asus specific methods
    void    ProcessALS();
    void    enableALS(bool enable);
    
    IOReturn    SetkeyboardBackLight(UInt8 level);
    IOReturn    ReadkeyboardBacklight(UInt8 parm);
    void        CheckAsusBacklight();
    void        TriggerAutoAsusBacklightOff();

    void        FindGPUDeviceEntry();
    bool        FindDisplayEntry(bool dedicatdGPU);
    void        ReadApplePanelBrightnessValue();
    void        ReadAsusPanelBrightnessValue(int fnKeyCode);
    void        ControlScreenBrightness();
    
    // NVRAM for keyboard backlight
    void ControlfKeybrdBackLight();
    void LoadValuesFromNVRAM();
    void saveValueToNVRAM(const char * symbol, UInt8 value);
    
protected:
    OSDictionary* getDictByUUID(const char * guid);
    
    // Asus specific methods
    bool enableATKEvents();
    bool enable_EEEPC_Events(bool enable);
    void handleMessage(int fnKeyCode);
    
    void getDeviceStatus(const char * guid, UInt32 methodId, UInt32 deviceId, UInt32 *status);
    void setDeviceStatus(const char * guid, UInt32 methodId, UInt32 deviceId, UInt32 *status);
    void setDevice(const char * guid, UInt32 methodId, UInt32 *status);
    
    // inline method to improve performance
    inline void SetTimerTimeout(IOTimerEventSource *timer, UInt32 interval)
    { if(timer) { timer->setTimeoutMS(interval); } }
    
    inline void CancelTimerTimeout(IOTimerEventSource *timer)
    { if(timer) { timer->cancelTimeout(); } }
    
private:
    int parse_wdg();
    OSString *flagsToStr(UInt8 flags);
    void wmi_wdg2reg(struct guid_block *g, OSArray *array, OSArray *dataArray);
    OSDictionary * readDataBlock(char *str);
    
    // utilities
    int wmi_data2Str(const char *in, char *out);
    bool wmi_parse_guid(const UInt8 *src, UInt8 *dest);
    void wmi_dump_wdg(struct guid_block *g);
    int wmi_parse_hexbyte(const UInt8 *src);
    void wmi_swap_bytes(UInt8 *src, UInt8 *dest);
    
};

#endif //_AsusNBFnKeys_h
