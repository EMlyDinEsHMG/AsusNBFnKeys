/*
 *  Copyright (c) EMlyDinEsH (mg-dinesh@live.com) 2012-2018. All rights reserved.
 *
 *
 *  FnKeysHIDKeyboard.cpp
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

#include "FnKeysHIKeyboard.h"

#define super IOHIKeyboard

#if APPLESDK
#define TIME_NOW  &now
#else
#define TIME_NOW  (uint64_t *)(&now)
#endif

OSDefineMetaClassAndStructors(FnKeysHIKeyboard, IOHIKeyboard)


bool FnKeysHIKeyboard::init(OSDictionary *dictionary)
{
	return super::init(dictionary);
}


bool FnKeysHIKeyboard::start(IOService *provider)
{
	if(!provider || !super::start( provider )) 
	{
		IOLog("%s: Failed to load.\n", this->getName());
		return false;
	}
	
    //
    // Obtain a referece to Asus Fn keys who is our provider
    // to see whether it exists or not, so we can start.
    //
    
	_kDevice = (FnKeysHIKeyboardDevice *) provider;
    if (_kDevice == NULL) {
        IOLog("%s: Failed to find Keyboard device.\n", this->getName());
        return false;
    }
    
    // Get system time
	clock_get_system_microtime(&lastEventSecs, &lastEventMicrosecs);
    
    // Set custom Keyboard name to be displayed at Keyboard preferences (Optional)
    setProperty("Product", "Fn Keys Keyboard (Asus)");

	return true;
}


void FnKeysHIKeyboard::stop(IOService *provider)
{
	super::stop(provider);
}


void FnKeysHIKeyboard::free(void)
{
	super::free();
}

/*
 * Receive hotkey event (only down) and send keyboard down and up event
 * Limits the rate of event send to HID stack, otherwise the system slow down and the sound/sun bezel lags.
 */

IOReturn FnKeysHIKeyboard::message( UInt32 type, IOService * provider, void * argument)
{
	if (type == kIOACPIMessageDeviceNotification)
	{
        AbsoluteTime now;

        UInt32 code = *((UInt32 *) argument);

		clock_sec_t  secs, deltaSecs;
		clock_usec_t microsecs, deltaMicrosecs;
		clock_get_system_microtime(&secs,&microsecs);
		deltaSecs = secs - lastEventSecs;
			
        // set Keyboard string to Fn keys Keyboard if it has the generic name "keyboard" for identification
        if(code == 0) {
            if(OSDynamicCast(OSString, getProperty("Product")) != NULL &&
               !strncmp(OSDynamicCast(OSString, getProperty("Product"))->getCStringNoCopy(),"Keyboard", strlen(OSDynamicCast(OSString, getProperty("Product"))->getCStringNoCopy())))
            {
                // setting Custom Keyboard Name to be displayed at Keyboard pref(Optional)
                setProperty("Product", "Fn Keys Keyboard (Asus)");
            }
            return kIOReturnSuccess;
        }
        
        // Skip rate limit for brightness and Volume keys so key hold can work to increase values
        switch (code) {
            // brightness keys
            case 0x4D:
            case 0x4F:
            case 0x10:
            case 0x20:
            // Volume keys
            case 0x4b:
            case 0x4c:
            case 0x48:
            case 0x49:
                break;
                
            default:
                // events rate limit
                if (deltaSecs < 2)
                {
                    // IOLog("Returned Key: %d(0x%x)\n",code,code);
                    
                    deltaMicrosecs = microsecs + (clock_usec_t)(1000000 * deltaSecs) - lastEventMicrosecs;
                    if (deltaMicrosecs < 100000) // rate limiter to 100 ms
                    {
                        // IOLog("%s: Skipping\n", this->getName());
                        return kIOReturnSuccess;
                    }
                }
                break;
        }
        lastEventSecs =		 secs;
		lastEventMicrosecs = microsecs;
        
        
        switch (code) {
                
            case 0x10:
                // Brightness up finer increments Shift +  Option +
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x38,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x3a,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x4d,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x4d,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x3a,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x38,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                break;
                
            case 0x20:
                // Brightness down finer increments with Shift + Option +
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x38,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x3a,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x4f,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x4f,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x3a,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x38,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                break;
                
            case 0x4b:
                // Volume up finer increments Shift +  Option +
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x38,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x3a,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x48,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x48,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x3a,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x38,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                break;
                
            case 0x4c:
                // Volume down finer increments with Shift + Option +
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x38,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x3a,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x49,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x49,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x3a,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x38,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                break;
                
            case 0x51:
                // 0x83 Launchpad
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 131,
                                          /*direction*/ true,
                                          /*timeStamp*/ now );
                    
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 131,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                    
                break;
            
            case 0x52:
                // 0xa0 Misson control
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 160,
                                          /*direction*/ true,
                                          /*timeStamp*/ now );
                    
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 160,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                    
                break;
                
            case 0x53:
                // 0x82 Dashboard
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 130,
                                          /*direction*/ true,
                                          /*timeStamp*/ now );
                    
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 130,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                    
                break;
                
            case 0x54:
                // Desktop, cmd + F3 of apple (cmd + Mission control)
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 0x37,
                                          /*direction*/ true,
                                          /*timeStamp*/ now );
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 160,
                                          /*direction*/ true,
                                          /*timeStamp*/ now );
                    
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 160,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 0x37,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                    
                break;
                
            case 0x55:
                // Application Windows, Control + F3 of apple (control + mission control)
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 0,
                                          /*direction*/ true,
                                          /*timeStamp*/ now );
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 160,
                                          /*direction*/ true,
                                          /*timeStamp*/ now );
                    
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 160,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 0,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                break;
                
            case 0x56:
                // Fast Release
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 0x40,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                
                break;
                
            case 0x57:
                // Rewind Release
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 0x34,
                                          /*direction*/ false,
                                          /*timeStamp*/ now );
                break;
            
            case 0x58:
                // 0x70 Eject
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x70,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x70,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                break;
                
            case 0x40:
                // Fast Begin
                    clock_get_uptime(TIME_NOW);
                    dispatchKeyboardEvent( 0x40,
                                          /*direction*/ true,
                                          /*timeStamp*/ now );
                break;
                
            case 0x34:
                // Rewind Begin
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x34,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                
                break;
                
            case 0x44:
                // Next
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x40,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x40,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                break;
                
            case 0x46:
                // Previous
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x34,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( 0x34,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                break;
                
            default:
                // other keys
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( code,
                                      /*direction*/ true,
                                      /*timeStamp*/ now );
                
                clock_get_uptime(TIME_NOW);
                dispatchKeyboardEvent( code,
                                      /*direction*/ false,
                                      /*timeStamp*/ now );
                break;
        }
    
     IOLog("Dispatched Key:  %d(0x%x)\n",code,code);

	}
	return kIOReturnSuccess;
}

//====================================================================================================
// defaultKeymapOfLength - IOHIKeyboard override
// This allows us to associate the scancodes we choose with the special
// keys we are interested in posting later. This gives us auto-repeats for free. Kewl.
//====================================================================================================

const unsigned char * FnKeysHIKeyboard::defaultKeymapOfLength( UInt32 * length )
{
    static const unsigned char ConsumerKeyMap[] =
    {
		// The first 16 bits are always read first, to determine if the rest of
        // the keymap is in shorts (16 bits) or bytes (8 bits). If the first 16 bits
        // equals 0, data is in bytes; if first 16 bits equal 1, data is in shorts.
        0x00,0x00, // data is in bytes
        
        // The next value is the number of modifier keys.
        0x04,
        
        // modifier definitions
        0x01,0x01,0x38,// Left Shift
        0x02,0x01,0x3b,// Left Control
        0x03,0x01,0x3a,// Left Option/Alt
        0x04,0x01,0x37,// Left Command/Windows
        
        
        // The next value is number of key definitions
        0xa2,
        
        // key defintions
/*0x00*/0x0d,0x00,0x61,0x00,0x41,0x00,0x01,0x00,0x01,0x00,0xca,0x00,0xc7,0x00,0x01,0x00,0x01,// 0x00, A
        0x0d,0x00,0x73,0x00,0x53,0x00,0x13,0x00,0x13,0x00,0xfb,0x00,0xa7,0x00,0x13,0x00,0x13,// 0x01, S
        0x0d,0x00,0x64,0x00,0x44,0x00,0x04,0x00,0x04,0x01,0x44,0x01,0xb6,0x00,0x04,0x00,0x04,// 0x02, D
        0x0d,0x00,0x66,0x00,0x46,0x00,0x06,0x00,0x06,0x00,0xa6,0x01,0xac,0x00,0x06,0x00,0x06,// 0x03, F
        0x0d,0x00,0x68,0x00,0x48,0x00,0x08,0x00,0x08,0x00,0xe3,0x00,0xeb,0x00,0x00,0x18,0x00,// 0x04, H
        0x0d,0x00,0x67,0x00,0x47,0x00,0x07,0x00,0x07,0x00,0xf1,0x00,0xe1,0x00,0x07,0x00,0x07,// 0x05, G
        0x0d,0x00,0x7a,0x00,0x5a,0x00,0x1a,0x00,0x1a,0x00,0xcf,0x01,0x57,0x00,0x1a,0x00,0x1a,// 0x06, Z
        0x0d,0x00,0x78,0x00,0x58,0x00,0x18,0x00,0x18,0x01,0xb4,0x01,0xce,0x00,0x18,0x00,0x18,// 0x07, X
        0x0d,0x00,0x63,0x00,0x43,0x00,0x03,0x00,0x03,0x01,0xe3,0x01,0xd3,0x00,0x03,0x00,0x03,// 0x08, C
        0x0d,0x00,0x76,0x00,0x56,0x00,0x16,0x00,0x16,0x01,0xd6,0x01,0xe0,0x00,0x16,0x00,0x16,// 0x09, V
        0x02,0x00,0x3c,0x00,0x3e,// 0x0a, ISO Keyboard extra key between Z and Shift
        0x0d,0x00,0x62,0x00,0x42,0x00,0x02,0x00,0x02,0x01,0xe5,0x01,0xf2,0x00,0x02,0x00,0x02,// 0x0b, B
        0x0d,0x00,0x71,0x00,0x51,0x00,0x11,0x00,0x11,0x00,0xfa,0x00,0xea,0x00,0x11,0x00,0x11,// 0x0c, Q
        0x0d,0x00,0x77,0x00,0x57,0x00,0x17,0x00,0x17,0x01,0xc8,0x01,0xc7,0x00,0x17,0x00,0x17,// 0x0d, W
        0x0d,0x00,0x65,0x00,0x45,0x00,0x05,0x00,0x05,0x00,0xc2,0x00,0xc5,0x00,0x05,0x00,0x05,// 0x0e, E
        0x0d,0x00,0x72,0x00,0x52,0x00,0x12,0x00,0x12,0x01,0xe2,0x01,0xd2,0x00,0x12,0x00,0x12,// 0x0f, R
/*0x10*/0x0d,0x00,0x79,0x00,0x59,0x00,0x19,0x00,0x19,0x00,0xa5,0x01,0xdb,0x00,0x19,0x00,0x19,// 0x10, Y
        0x0d,0x00,0x74,0x00,0x54,0x00,0x14,0x00,0x14,0x01,0xe4,0x01,0xd4,0x00,0x14,0x00,0x14,// 0x11, T
        0x0a,0x00,0x31,0x00,0x21,0x01,0xad,0x00,0xa1,// 0x12, digit 1
        0x0e,0x00,0x32,0x00,0x40,0x00,0x32,0x00,0x00,0x00,0xb2,0x00,0xb3,0x00,0x00,0x00,0x00,// 0x13, 2
        0x0a,0x00,0x33,0x00,0x23,0x00,0xa3,0x01,0xba,// 0x14, 3
        0x0a,0x00,0x34,0x00,0x24,0x00,0xa2,0x00,0xa8,// 0x15, 4
        0x0e,0x00,0x36,0x00,0x5e,0x00,0x36,0x00,0x1e,0x00,0xb6,0x00,0xc3,0x00,0x1e,0x00,0x1e,// 0x16, 6
        0x0a,0x00,0x35,0x00,0x25,0x01,0xa5,0x00,0xbd,// 0x17, 5
        0x0a,0x00,0x3d,0x00,0x2b,0x01,0xb9,0x01,0xb1,// 0x18, =+
        0x0a,0x00,0x39,0x00,0x28,0x00,0xac,0x00,0xab,// 0x19, 9
        0x0a,0x00,0x37,0x00,0x26,0x01,0xb0,0x01,0xab,// 0x1a, 7
        0x0e,0x00,0x2d,0x00,0x5f,0x00,0x1f,0x00,0x1f,0x00,0xb1,0x00,0xd0,0x00,0x1f,0x00,0x1f,// 0x1b, -_
        0x0a,0x00,0x38,0x00,0x2a,0x00,0xb7,0x00,0xb4,// 0x1c, 8
        0x0a,0x00,0x30,0x00,0x29,0x00,0xad,0x00,0xbb,// 0x1d, 0
        0x0e,0x00,0x5d,0x00,0x7d,0x00,0x1d,0x00,0x1d,0x00,0x27,0x00,0xba,0x00,0x1d,0x00,0x1d,// 0x1e, ]}
        0x0d,0x00,0x6f,0x00,0x4f,0x00,0x0f,0x00,0x0f,0x00,0xf9,0x00,0xe9,0x00,0x0f,0x00,0x0f,// 0x1f, O
/*0x20*/0x0d,0x00,0x75,0x00,0x55,0x00,0x15,0x00,0x15,0x00,0xc8,0x00,0xcd,0x00,0x15,0x00,0x15,// 0x20, U
        0x0e,0x00,0x5b,0x00,0x7b,0x00,0x1b,0x00,0x1b,0x00,0x60,0x00,0xaa,0x00,0x1b,0x00,0x1b,// 0x21, [{
        0x0d,0x00,0x69,0x00,0x49,0x00,0x09,0x00,0x09,0x00,0xc1,0x00,0xf5,0x00,0x09,0x00,0x09,// 0x22, I
        0x0d,0x00,0x70,0x00,0x50,0x00,0x10,0x00,0x10,0x01,0x70,0x01,0x50,0x00,0x10,0x00,0x10,// 0x23, P
        0x10,0x00,0x0d,0x00,0x03,// 0x24, Return
        0x0d,0x00,0x6c,0x00,0x4c,0x00,0x0c,0x00,0x0c,0x00,0xf8,0x00,0xe8,0x00,0x0c,0x00,0x0c,// 0x25, L
        0x0d,0x00,0x6a,0x00,0x4a,0x00,0x0a,0x00,0x0a,0x00,0xc6,0x00,0xae,0x00,0x0a,0x00,0x0a,// 0x26, J
        0x0a,0x00,0x27,0x00,0x22,0x00,0xa9,0x01,0xae,// 0x27, '"
        0x0d,0x00,0x6b,0x00,0x4b,0x00,0x0b,0x00,0x0b,0x00,0xce,0x00,0xaf,0x00,0x0b,0x00,0x0b,// 0x28, K
        0x0a,0x00,0x3b,0x00,0x3a,0x01,0xb2,0x01,0xa2,// 0x29, :;
        0x0e,0x00,0x5c,0x00,0x7c,0x00,0x1c,0x00,0x1c,0x00,0xe3,0x00,0xeb,0x00,0x1c,0x00,0x1c,// 0x2a, \|
        0x0a,0x00,0x2c,0x00,0x3c,0x00,0xcb,0x01,0xa3,// 0x2b, ,<
        0x0a,0x00,0x2f,0x00,0x3f,0x01,0xb8,0x00,0xbf,// 0x2c, /?
        0x0d,0x00,0x6e,0x00,0x4e,0x00,0x0e,0x00,0x0e,0x00,0xc4,0x01,0xaf,0x00,0x0e,0x00,0x0e,// 0x2d, N
        0x0d,0x00,0x6d,0x00,0x4d,0x00,0x0d,0x00,0x0d,0x01,0x6d,0x01,0xd8,0x00,0x0d,0x00,0x0d,// 0x2e, M
        0x0a,0x00,0x2e,0x00,0x3e,0x00,0xbc,0x01,0xb3,// 0x2f, .>
/*0x30*/0x02,0x00,0x09,0x00,0x19,// 0x30, Tab
        0x0c,0x00,0x20,0x00,0x00,0x00,0x80,0x00,0x00,// 0x31, Space
        0x0a,0x00,0x60,0x00,0x7e,0x00,0x60,0x01,0xbb,// 0x32, `~
        0x02,0x00,0x7f,0x00,0x08,// 0x33, Backspace
        0xff,// 0x34,(DEAD) Media Rewind
        0x02,0x00,0x1b,0x00,0x7e,// 0x35, ESC
        0xff,//  0x36, Right Windows
        0xff,//  0X37, Left Windows
        0xff,//  0x38, Left Shift
        0xff,//  0x39, Caps Locks
        0xff,//  0x3a, Left ALT
        0xff,//  0x3b, Left Control
        0xff,//  0x3c, Right Shift
        0xff,//  0x3d, Right ALT
        0xff,//  0x3e, Right Control
        0xff,//  0x3f, (DEAD) Apple Fn key
/*0x40*/0xff,//  0x40, (DEAD) Media Fast
        0x00,0x00,0x2e, // 0x41, Keypad ., Delete
        0xff,//  0x42,(DEAD) Media Play/Pause
        0x00,0x00,0x2a,// 0x43, Keyboad *
        0xff,//  0x44,(DEAD) Media Next
        0x00,0x00,0x2b,// 0x45, Keypard +
        0xff,//  0x46,(DEAD) Media Previous
        0x00,0x00,0x1b,//  0x47, Clear
        0xff,//  0x48 Volume Up
        0xff,//  0x49 Volume Down
        0xff,//  0x4a Volume Mute
        0x0e,0x00,0x2f,0x00,0x5c,0x00,0x2f,0x00,0x1c,0x00,0x2f,0x00,0x5c,0x00,0x00,0x0a,0x00,// 0x4b, Keypad /
        0x00,0x00,0x0d,// 0x4c, Keypad enter/ Apple fn + Return = enter
        0xff,//  0x4d,(DEAD) Brightness Up
        0x00,0x00,0x2d,// 0x4e, Keypad -
        0xff,//  0x4f,(DEAD) Brightness Down
/*0x50*/0xff,//  0x50,(DEAD) Video Mirror
        0x0e,0x00,0x3d,0x00,0x7c,0x00,0x3d,0x00,0x1c,0x00,0x3d,0x00,0x7c,0x00,0x00,0x18,0x46,// 0x51, Keypad =
        0x00,0x00,0x30,// 0x52, Keypad 0, Insert
        0x00,0x00,0x31,// 0x53, Keypad 1, End
        0x00,0x00,0x32,// 0x54, Keypad 2, Down Arrow
        0x00,0x00,0x33,// 0x55, Keypad 3, Page Down
        0x00,0x00,0x34,// 0x56, Keypad 4, Let Arrow
        0x00,0x00,0x35,// 0x57, Keypad 5, Clear
        0x00,0x00,0x36,// 0x58, Keypad 6, Right Arrow
        0x00,0x00,0x37,// 0x59, Keypad 7, Home
        0x00,0xfe,0x3a,// 0x5a, (DEAD) F21
        0x00,0x00,0x38,// 0x5b, Keypad 8, Up Arrow
        0x00,0x00,0x39,// 0x5c, Keypad 9, Page Up
        0x00,0xfe,0x3b,//  0x5d (DEAD) F22
        0x00,0xfe,0x3c,//  0x5e (DEAD) F23
        0x00,0xfe,0x3d,//  0x5f (DEAD) F24
/*0x60*/0x00,0xfe,0x24,//  0x60, F5
        0x00,0xfe,0x25,//  0x61, F6
        0x00,0xfe,0x26,//  0x62, F7
        0x00,0xfe,0x22,//  0x63, F3
        0x00,0xfe,0x27,//  0x64, F8
        0x00,0xfe,0x28,//  0x65, F9
        0x00,0xfe,0x38,//  0x66, (DEAD) F19
        0x00,0xfe,0x2a,//  0x67, F11
        0x00,0xfe,0x39,//  0x68, (DEAD) F20
        0x00,0xfe,0x32,// 0x69 F13
        0x00,0xfe,0x35,// 0x6a F16
        0x00,0xfe,0x33,// 0x6b F14
        0x00,0xfe,0x36,// 0x6c,(DEAD) F17
        0x00,0xfe,0x29,// 0x6d, F10
        0x00,0xfe,0x37,// 0x6e,(DEAD) F18
        0x00,0xfe,0x2b,// 0x6f, F12
/*0x70*/0xff,//  0x70, (DEAD) Eject
        0x00,0xfe,0x34,//  0x71 F15
        0xff,//  0x72,(DEAD) Help
        0x00,0xfe,0x2e,//  0x73, Keypad Home
        0x00,0xfe,0x30,//  0x74, Keypad PgUp
        0x00,0xfe,0x2d,//  0x75, Keypad Del
        0x00,0xfe,0x23,//  0x76, F4
        0x00,0xfe,0x2f,//  0x77, Keypad End
        0x00,0xfe,0x21,//  0x78, F2
        0x00,0xfe,0x31,//  0x79, Keypad PgDn
        0x00,0xfe,0x20,//  0x7a, F1
        0x00,0x01,0xac, // 0x7b  Left arrow
        0x00,0x01,0xae, // 0x7c  RDight arrow
        0x00,0x01,0xaf, // 0x7d  Down arrow.
        0x00,0x01,0xad, // 0x7e  Up arrow
/* Extended for Mapping */
        0x00,0x00,0x00, // 0x7f Power button
        0x00,0x00,0x00,
        0x00,0x00,0x00, //
        0x00,0x00,0x00, // 0x82 Dashboard
        0x00,0x00,0x00, // 0x83 Launchpad
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00,
        0x00,0x00,0x00, // 0xa0 Misson control
        0x00,0x00,0x00, // 0xa1
        
        
        // the next value is number of sequence definitions
        0x00,
        
        // The next value is the number of special keys
        0x10,
        
        NX_KEYTYPE_SOUND_UP,		0x48,
        NX_KEYTYPE_SOUND_DOWN,		0x49,
        NX_KEYTYPE_BRIGHTNESS_UP,	0x4d,
        NX_KEYTYPE_BRIGHTNESS_DOWN,	0x4f,
        NX_KEYTYPE_CAPS_LOCK,		0x39,
        NX_KEYTYPE_HELP,		0x72,
        NX_POWER_KEY,			0x7F,
        NX_KEYTYPE_MUTE,		0x4a,
        NX_KEYTYPE_NUM_LOCK,		0x47,
        NX_KEYTYPE_EJECT,		0x70,
        NX_KEYTYPE_VIDMIRROR,		0x50,
        NX_KEYTYPE_PLAY,		0x42,
        NX_KEYTYPE_NEXT,		0x44,
        NX_KEYTYPE_PREVIOUS,		0x46,
        NX_KEYTYPE_FAST,			0x40,
        NX_KEYTYPE_REWIND,          0x34,
        
    };
	
    if( length ) *length = sizeof( ConsumerKeyMap );
    
    return( ConsumerKeyMap );
}
