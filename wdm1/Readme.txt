Copyright © 1998,1999 Chris Cant, PHD Computer Consultants Ltd
WDM Book for R&D Books, Miller Freeman Inc

The Wdm1 example shows a basic virtual WDM device driver.

The SYS subdirectory contains the driver source.
Use the Control Panel Add New Hardware Wizard to install a Wdm1 device
from one of the installation INF files in SYS.
The checked build includes DebugPrint trace output messages.

The EXE subdirectory contains some test Win32 user mode code.
Run EXE\Release\Wdm1Test to check that a Wdm1 device was installed properly.
