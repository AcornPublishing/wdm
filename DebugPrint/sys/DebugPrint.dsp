# Microsoft Developer Studio Project File - Name="DebugPrint" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=DebugPrint - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "DebugPrint.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "DebugPrint.mak" CFG="DebugPrint - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "DebugPrint - Win32 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "DebugPrint - Win32 Free" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "DebugPrint - Win32 Checked"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Checked"
# PROP BASE Intermediate_Dir "Checked"
# PROP BASE Cmd_Line "NMAKE /f DebugPrint.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "DebugPrint.exe"
# PROP BASE Bsc_Name "DebugPrint.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Checked"
# PROP Intermediate_Dir "Checked"
# PROP Cmd_Line "MakeDrvr %DDKROOT% c: %WDMBook%\DebugPrint\sys checked"
# PROP Rebuild_Opt "-nmake /a"
# PROP Target_File "DebugPrt.sys"
# PROP Bsc_Name "obj\i386\checked\DebugPrt.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "DebugPrint - Win32 Free"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Free"
# PROP BASE Intermediate_Dir "Free"
# PROP BASE Cmd_Line "NMAKE /f DebugPrint.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "DebugPrint.exe"
# PROP BASE Bsc_Name "DebugPrint.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Free"
# PROP Intermediate_Dir "Free"
# PROP Cmd_Line "MakeDrvr %DDKROOT% c: %WDMBook%\DebugPrint\sys free"
# PROP Rebuild_Opt "-nmake /a"
# PROP Target_File "DebugPrt.sys"
# PROP Bsc_Name "obj\i386\free\DebugPrt.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "DebugPrint - Win32 Checked"
# Name "DebugPrint - Win32 Free"

!IF  "$(CFG)" == "DebugPrint - Win32 Checked"

!ELSEIF  "$(CFG)" == "DebugPrint - Win32 Free"

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
# End Group
# Begin Group "Source"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\DbgPrtGUID.h
# End Source File
# Begin Source File

SOURCE=.\DebugPrint.h
# End Source File
# Begin Source File

SOURCE=.\DebugPrint.inf
# End Source File
# Begin Source File

SOURCE=.\DebugPrint.rc
# End Source File
# Begin Source File

SOURCE=.\DebugPrintchecked.inf
# End Source File
# Begin Source File

SOURCE=.\Dispatch.cpp
# End Source File
# Begin Source File

SOURCE=.\Init.cpp
# End Source File
# Begin Source File

SOURCE=.\Pnp.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\README2.TXT
# End Source File
# End Target
# End Project
