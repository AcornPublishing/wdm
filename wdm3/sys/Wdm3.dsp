# Microsoft Developer Studio Project File - Name="Wdm3" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Wdm3 - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Wdm3.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Wdm3.mak" CFG="Wdm3 - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Wdm3 - Win32 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "Wdm3 - Win32 Free" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Wdm3 - Win32 Checked"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Checked"
# PROP BASE Intermediate_Dir "Checked"
# PROP BASE Cmd_Line "NMAKE /f Wdm3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Wdm3.exe"
# PROP BASE Bsc_Name "Wdm3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Checked"
# PROP Intermediate_Dir "Checked"
# PROP Cmd_Line "MakeDrvr %DDKROOT% c: %WDMBook%\Wdm3\sys checked"
# PROP Rebuild_Opt "-nmake /a"
# PROP Target_File "Wdm3.sys"
# PROP Bsc_Name "obj\i386\checked\Wdm3.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Wdm3 - Win32 Free"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Free"
# PROP BASE Intermediate_Dir "Free"
# PROP BASE Cmd_Line "NMAKE /f Wdm3.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Wdm3.exe"
# PROP BASE Bsc_Name "Wdm3.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Free"
# PROP Intermediate_Dir "Free"
# PROP Cmd_Line "MakeDrvr %DDKROOT% c: %WDMBook%\Wdm3\sys free"
# PROP Rebuild_Opt "-nmake /a"
# PROP Target_File "Wdm3.sys"
# PROP Bsc_Name "obj\i386\free\Wdm3.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Wdm3 - Win32 Checked"
# Name "Wdm3 - Win32 Free"

!IF  "$(CFG)" == "Wdm3 - Win32 Checked"

!ELSEIF  "$(CFG)" == "Wdm3 - Win32 Free"

!ENDIF 

# Begin Group "Build"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\build.err
# End Source File
# Begin Source File

SOURCE=.\build.log
# End Source File
# Begin Source File

SOURCE=.\build.wrn
# End Source File
# Begin Source File

SOURCE=..\..\MakeDrvr.bat
# End Source File
# Begin Source File

SOURCE=.\MAKEFILE
# End Source File
# Begin Source File

SOURCE=.\Makefile.inc
# End Source File
# Begin Source File

SOURCE=.\SOURCES
# End Source File
# Begin Source File

SOURCE=.\Wdm3.map
# End Source File
# End Group
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Group "Headers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\DebugPrint.h
# End Source File
# Begin Source File

SOURCE=.\Eventlog.h
# End Source File
# Begin Source File

SOURCE=..\..\GUIDs.h
# End Source File
# Begin Source File

SOURCE=.\Ioctl.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Wdm3.h
# End Source File
# End Group
# Begin Group "Install"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Wdm3checked.inf
# End Source File
# Begin Source File

SOURCE=.\Wdm3free.inf
# End Source File
# End Group
# Begin Group "Event messages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Wdm3Msg.h
# End Source File
# Begin Source File

SOURCE=.\Wdm3Msg.mc
# End Source File
# Begin Source File

SOURCE=.\Wdm3Msg.rc
# End Source File
# End Group
# Begin Group "WMI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Wdm3.bmf
# End Source File
# Begin Source File

SOURCE=.\Wdm3.mof
# End Source File
# End Group
# Begin Source File

SOURCE=.\DebugPrint.c
# End Source File
# Begin Source File

SOURCE=.\DeviceIo.cpp
# End Source File
# Begin Source File

SOURCE=.\Dispatch.cpp
# End Source File
# Begin Source File

SOURCE=.\Eventlog.cpp
# End Source File
# Begin Source File

SOURCE=.\Init.cpp
# End Source File
# Begin Source File

SOURCE=.\Pnp.cpp
# End Source File
# Begin Source File

SOURCE=.\Power.cpp
# End Source File
# Begin Source File

SOURCE=.\Wdm3.rc
# End Source File
# Begin Source File

SOURCE=.\Wmi.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\README.TXT
# End Source File
# End Target
# End Project
