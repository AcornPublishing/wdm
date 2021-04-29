# Microsoft Developer Studio Project File - Name="Wdm1" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=Wdm1 - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Wdm1.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Wdm1.mak" CFG="Wdm1 - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Wdm1 - Win32 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "Wdm1 - Win32 Free" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "Wdm1 - Win32 Checked"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Checked"
# PROP BASE Intermediate_Dir "Checked"
# PROP BASE Cmd_Line "NMAKE /f Wdm1.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Wdm1.exe"
# PROP BASE Bsc_Name "Wdm1.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Checked"
# PROP Intermediate_Dir "Checked"
# PROP Cmd_Line "MakeDrvr %DDKROOT% c: %WDMBook%\wdm1\sys checked"
# PROP Rebuild_Opt "-nmake /a"
# PROP Target_File "Wdm1.sys"
# PROP Bsc_Name "obj\i386\checked\Wdm1.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "Wdm1 - Win32 Free"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Free"
# PROP BASE Intermediate_Dir "Free"
# PROP BASE Cmd_Line "NMAKE /f Wdm1.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "Wdm1.exe"
# PROP BASE Bsc_Name "Wdm1.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Free"
# PROP Intermediate_Dir "Free"
# PROP Cmd_Line "MakeDrvr %DDKROOT% c: %WDMBook%\wdm1\sys free"
# PROP Rebuild_Opt "-nmake /a"
# PROP Target_File "Wdm1.sys"
# PROP Bsc_Name "obj\i386\free\Wdm1.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "Wdm1 - Win32 Checked"
# Name "Wdm1 - Win32 Free"

!IF  "$(CFG)" == "Wdm1 - Win32 Checked"

!ELSEIF  "$(CFG)" == "Wdm1 - Win32 Free"

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

SOURCE=.\buildfre.log
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

SOURCE=.\DebugPrint.c
# End Source File
# Begin Source File

SOURCE=.\DebugPrint.h
# End Source File
# Begin Source File

SOURCE=.\Dispatch.cpp
# End Source File
# Begin Source File

SOURCE=.\GUIDs.h
# End Source File
# Begin Source File

SOURCE=.\Init.cpp
# End Source File
# Begin Source File

SOURCE=.\Ioctl.h
# End Source File
# Begin Source File

SOURCE=.\Pnp.cpp
# End Source File
# Begin Source File

SOURCE=.\ReadReg.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\wdm1.h
# End Source File
# Begin Source File

SOURCE=.\Wdm1.rc
# End Source File
# Begin Source File

SOURCE=.\Wdm1checked.inf
# End Source File
# Begin Source File

SOURCE=.\Wdm1free.inf
# End Source File
# End Group
# Begin Source File

SOURCE=..\README.TXT
# End Source File
# End Target
# End Project
