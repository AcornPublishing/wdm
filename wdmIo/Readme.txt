Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
WDM Book for R&D Books, Miller Freeman Inc

The WdmIo example virtual WDM device driver illustrates basic device programmed I/O.
It has full Plug and Play support.

The SYS subdirectory contains the driver source.
Use the Control Panel Add New Hardware Wizard to install a WdmIo device
from one of the installation INF files in SYS.
The checked build includes DebugPrint trace output messages.

Use WdmIoLpt1Free.inf or WdmIoLpt1Checked.inf to assign the LPT1 parallel
port resources to the WdmIo devie.

The EXE subdirectory contains some test Win32 user mode code.
Run EXE\Release\WdmIoTest to output a couple of lines of text to the printer.
Read the book for for full installation instructions.

The Cancel subdirectory contains code to test the cancelling and Cancel IRP handling
of WdmIo.
