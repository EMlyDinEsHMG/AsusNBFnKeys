This is an ACPI driver to control the Fn Keys and ALS sensor found in
Asus Notebooks, developed by me based on the ideas from linux asus wmi,
hokoffy (insanelymac user) WMI controller source and my own research on
ACPI and OS X API. Also I\'ve introduced many new features with
customizations along the way and improved several things. Some of the Fn
keys require DSDT patches in order to work.\
 

**The Fn keys which my driver controls are:**

**Fn +**

    F3 Does OS X function (Misson control/Dashboard)/Keyboard Backlight Down (If supports)
    F4 Does OS X function (Launchpad/Desktop)/ Keyboard Backlight Up (If supports)
    F5 Brightness Down
    F6 Brightness Up
    F7 Screen Backlight On/Off (Soft mode)
    F8 Video Mirror
    F9 Touchpad (only if you use my Smart Touchpad driver)
    F10 Mute
    F11 Volume Down
    F12 Volume Up
    C - Rewind
    V - Fast
    A - Toggles ALS Sensor on/off (If supports)
    Asus Instant Key - Does OS X function (Launchpad) (If supports)
    Space - Play/Pause
    Left Arrow - Previous
    Down Arrow - Play/Pause
    Right Arrow - Next
    Up Arrow - Toggles Media Functions Fast/Rewind and Prev/Next between C,V & Left, Right Arrows and 
               Toggles OSX Functions for F3, F4
               Toggles Finer and Normal Brightness, Volume control

**[Supported Functions by F3, F4 and Asus Instant keys
are:]{.underline}**\
Edit the plist  entries \"F3KeyAction\" and \"F4KeyAction\" with the
below values to assign different functions\
\
0 - Default Functions\
1 - Launchpad\
2 - Misson control\
3 - Dashboard\
4 - Desktop\
5 - Application window \
6 - Eject\
 

**For Notebooks with compact Keyboard (without Numpad):**

** Fn +**

    C  - Previous
    V  - Next
    Space - Play/Pause 

 \
**[CONFIG DEFAULT PREFERENCES:]{.underline}**\
Edit the file \"info.plist\" file located inside my
kext \"AsusNBFnKeys.kext/Contents/\"\
 \
Look for the secton \"IOKitPersonalities-\>AsusNBFnKeys-\>Preferences\"
to change Fn key controls

- HasMediaFnKeys - Notebook has multi media fn keys Play/Pause, Next,
  Previous? 

- SoftDisplayBacklightOff -  Enable backlight on/off in soft mode?
  Useful it native backlight control is not working.

- DimBacklightOnAC/DC -  Enable brightness pop up on AC/DC to notify?
  Useful for AC/DC status.

- MediaKeysDoFastRewind -  Enable Media keys should do fast/rewind
  instead next/previous?

- FingerBrighntessControl -  Enable finer brightness control which
  provides more steps?

- FinerVolumeControl -  Enable finer volume control which provides more
  steps?

- DimBrightnessByLevels - 0 - 15, Default value is 3.

- KBackLightlvlAtBoot - 0-3 / 0-15

- F3KeyFunction - 0 - 6

- F4KeyFunction - 0 - 6

- InstantKeyFunction - 0 - 6

- IdleKBacklightAutoOff -  Enable idle auto keyboard backlight off?

- IdleKBacklightAutoOffTimeout - Default 10000 ms, idle timeout value.

\
Look for the
secton \"IOKitPersonalities-\>AsusNBFnKeys-\>Preferences-\>ConfigForALS\" to
change ALS sensor controls

- EnableALSLogs - Enable logging to see values it reports?

- DisableOnAC - Disable ALS on AC?

- DisableOnFnControl -  Disable Auto backlight when Fn key used to
  manually control?

- EnableAtBoot - Enable ALS sensor at boot?

- SamplesToProcess - Number of samples used to control backlight in
  background.

- TimerInterval - Interval used for checking the ALS in background.

- Level**x**Brightness - Brightness level for the level \'x\'. There are
  5 levels for x = 1 - 5.

- Level**x**RangeStart - ALS range start value for the level \'x\'.

- Level**x**RangeEnd - ALS range end value for the level \'x\'.

**Note:**

    For some dsdt files, you need to replace "\_SB.ATKD.IANE (0xXX)" with "Notify (ATKD, 0xXX)".

**[Use
the]{.underline}[ ]{.underline}[attached]{.underline}[ ]{.underline}[patches
in DSDT Editor app to patch your DSDT only if there is something not
working.]{.underline}**\
 

**Must Needed patches:**

**[Brightness Fn keys patch:]{.underline}**\
Modify the method **\_Q0E** and **\_Q0F** in DSDT for the **ATKP
conditional clause** code:

          Method (_Q0E, 0, NotSerialized)  
            {
                If (ATKP)
                {
                    \_SB.ATKD.IANE (0x20)
                }
            }
            Method (_Q0F, 0, NotSerialized) 
            {
                If (ATKP)
                {
                    \_SB.ATKD.IANE (0x10)
                }
            }

**[Keyboard backlight patch:]{.underline}**\
Use the dsdt patch and config to plist from this link manually or
attached patch
file: [https://osxlatitude.com/forums/topic/5966-details-about-the-smart-touchpad-driver-features/page/2/#comment-32299]{.underline}\
 

**[ALS sensor patch:]{.underline}**\
Use the dsdt patch attached.\
 \
 

**[Supports:]{.underline} Mac OSX 10.7 or greater**\
 

**[How to install:]{.underline}**

1\) Remove AsusNBFnKeys kext from System (Optional).\
2) Install AsusNBFnKeys kext using your favorite kext installers.\

**[Change Log]{.underline}**\

**Update v2.6 Released**

- Major code optimizations

- Some improvements, bug fixes and optimizations

- 

**Update v2.5.5**

- Improved ALS

- Some improvements, bug fixes and optimizations

- 

**Update v2.5**

- Reworked ALS sensor driver integration

- Updated DSDT patches

- Several improvements, bug fixes and optimizations

**Update v2.4**

- Fixed \"GPU not found\" bug in some systems

- Fixed a bug in keyboard backlight read for some systems

- Added auto detection of Nvidia GPU (Removed DisplayUsesNvidiaGPU
  option in plist)

- Added Finer controls for brightness and Volume to get more steps
  of brightness/volume levels (Can be enabled/disables in plist) like
  Shift + Option does.

- Minor bug fixes and optimizations

**Update v2.3.5**

- Reworked on some sections for improvements

- Added new \_HID value \"PnP0C14\" found in new Notebooks DSDT to load
  my kext

- Removed debug logs which was on before

- Some bug fixes and optimizations

**Update v2.3**

- Added support for old Asus models.

- Added support to auto detect the Keyboard backlight and ALS sensor
  support from DSDT.

- Added support for Asus Instant key found in some models.

- Added Eject function for F3, F4 and Instant key customization.

- Some bug fixes

- Optimized code 

**Update v2.2**

- Fixed ALS sensor screen backlight changes not working issue

- Fixed Media Next and Previous key press to work everywhere

- Added NVRAM support for keyboard backlight levels (need to use new
  DSDT patch)

- Added 16 Levels support for keyboard backlight (need to use new DSDT
  patch)

- Added idle auto off support for keyboard backlight (need to use new
  DSDT patch)

- Added automatic detection of my asus backlight driver use(removed
  plist option)

- Updated code to work with new version of my AsusBacklight driver

- Minor bug fixes

- Optimized code 

**Update v2.0**

- Enabled F3 and F4 keys to OSX functions which was not used in Full
  sized keyboard 

- Integrated two kexts used in previous versions into a Single kext 

- Assigned Fast, Rewind and Play functions to C, V and Space keys which
  was not used before in Full sized keyboard

- Assigned Up Arrow to toggle the actions of media keys and F3,F4

- Optimizations to the code for some improvements

- Bug fixes 

**Update v1.7.2**

- Fixed some bugs introduced in v1.7 which made kext not working

- Added screen backlight on/off for Fn + F7 for those whose backlight is
  not working (it is done in soft mode, but you can use attached patch
  for hard wired backlight control in zenbooks, **credits: qwerty12**)

- Improved support for ALS sensor of Zenbooks which is reported to be
  working well

**Update v1.6**

- Added support for the Fn key Fn+ F9 to enable/disable touchpad with my
  Elan touchpad driver

- Added support for Asus Backlight driver to match Apple brightness
  bezel values to Asus brightness levels

- Added support for old Asus notebooks with different \_UID for ATKD
  device in dsdt (still in progress so need testers with \_UID ASUSxxxx
  in dsdt)

- Added experimental support for ALS sensor in new Zenbooks (still in
  testing phase so it won\'t work for now)

**Update v1.4.1(Remove AsusNBWMI kext before installing this)**

- Renamed the kext name from AsusNBWMI to more sensible name
  \"AsusNBFnKeys\" and did some code changes for future improvements

**Update: v1.4**

- Integrated both compact keyboard version and normal version in single
  kext

- Added plist options to choose keyboard backlight at boot and map media
  buttons to c,v and space keys

- Optimized code

**Hoping this helps for many.** \
 \

***CREDITS: Hotkoffy (InsanelyMac) for his WMI source and to** **Linux
OS*** \
 
