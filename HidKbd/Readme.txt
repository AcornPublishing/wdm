Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
WDM Book for R&D Books, Miller Freeman Inc

The USER directory contains the HidKbdUser Win32 program which finds a HID
keyboard, reads input reports and flashes the LEDs.
Run USER\Release\HidKbdUser

The HidKbd example HID client illustrates how to interrogate a HID device,
specifically a HID keyboard.

The SYS subdirectory contains the driver source.
You need to install the driver by hand by adding registry entries and rebooting.
The checked build includes DebugPrint trace output messages.

The EXE subdirectory contains some test Win32 user mode code.
Run EXE\Release\HidKbdTest to put a HidKbd device through its paces.

