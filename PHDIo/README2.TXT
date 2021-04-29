Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
WDM Book for R&D Books, Miller Freeman Inc

PHDIo is an example NT style device driver which illustrates basic device programmed I/O.

The SYS subdirectory contains the driver source.
Follow the instructions in the book to install PHDIo.
The checked build includes DebugPrint trace output messages.

The EXE subdirectory contains some test Win32 user mode code.
Run EXE\Release\PHDIoTest to output a couple of lines of text to the printer on LPT1.

The Cancel subdirectory contains code to test the cancelling and Cancel IRP handling
of PHDIo.