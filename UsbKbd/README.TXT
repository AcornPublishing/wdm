Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
WDM Book for R&D Books, Miller Freeman Inc

The UsbKbd example WDM device driver illustrates how to interrogate a USB device,
specifically a USB keyboard.

The SYS subdirectory contains the driver source.
The only way to install this driver is to replace the existing hidusb.sys driver.
Make sure that make a backup copy of hidusb.sys.
The checked build includes DebugPrint trace output messages.

The EXE subdirectory contains some test Win32 user mode code.
Run EXE\Release\UsbKbdTest to put a UsbKbd device through its paces.
