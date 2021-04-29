Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
WDM Book for R&D Books, Miller Freeman Inc

The Wdm2 example virtual WDM device driver has full Plug and Play and
Power Management support.

The SYS subdirectory contains the driver source.
Use the Control Panel Add New Hardware Wizard to install a Wdm2 device
from one of the installation INF files in SYS.
The checked build includes DebugPrint trace output messages.

The EXE subdirectory contains some test Win32 user mode code.
Run EXE\Release\Wdm2Test to put a Wdm2 device through its paces.

The NOTIFY subdirectory contains a Win32 program which listens for Wdm2 device change events.
Run NOTIFY\Release\Wdm2Notify and then reinstall the Wdm2 device.

The POWER subdirectory has a Win32 program which tests various Power Management aspects of
your computer: AC, DC and disk status, Suspend/Hibernate and displays power messages.
Run POWER\Release\Wdm2Power to test these features of your computer and driver.
