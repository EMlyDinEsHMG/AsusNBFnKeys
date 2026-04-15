# AsusNBFnKeys

macOS kernel extension (kext) enabling **Fn key functionality** and **ambient light sensor (ALS)** support on ASUS laptops in Hackintosh environments.

---

## Overview

AsusNBFnKeys extends macOS hardware compatibility by implementing ASUS-specific input handling using low-level system interfaces.  
It enables Fn key events and sensor data integration that are not natively supported on non-Apple hardware.

---

## Features

- Fn key event handling  
- Ambient Light Sensor (ALS) integration  
- Partial keyboard backlight support  

---

## Technical Details

- Language: C++  
- Framework: macOS IOKit  
- Interfaces: ACPI / DSDT (WMI-based event handling)  
- Type: Kernel Extension (kext)  
- Environment: Hackintosh (OpenCore / Clover)  

---

## Installation

1. Copy `AsusNBFnKeys.kext` to:
   - `/Library/Extensions/`  
   or  
   - EFI (`Clover` / `OpenCore`)

2. Rebuild cache: `sudo kextcache -i /`  
3. Reboot  

---

## Compatibility

- macOS (Hackintosh only)  
- ASUS laptops with compatible ACPI configuration  

---

## Status

⚠️ Legacy project – not actively maintained.

---

## License

MIT License
