# Microsoft Developer Studio Project File - Name="PassThru" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=PassThru - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PassThru.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PassThru.mak" CFG="PassThru - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PassThru - Win32 Checked" (based on "Win32 (x86) External Target")
!MESSAGE "PassThru - Win32 Free" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "PassThru - Win32 Checked"

# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Checked"
# PROP BASE Intermediate_Dir "Checked"
# PROP BASE Cmd_Line "NMAKE /f PassThru.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "PassThru.exe"
# PROP BASE Bsc_Name "PassThru.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Checked"
# PROP Intermediate_Dir "Checked"
# PROP Cmd_Line "MakeDrvr %DDKROOT% c: %WDMBook%\PassThru\sys checked"
# PROP Rebuild_Opt "-nmake /a"
# PROP Target_File "PassThru.sys"
# PROP Bsc_Name "obj\i386\checked\PassThru.bsc"
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "PassThru - Win32 Free"

# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Free"
# PROP BASE Intermediate_Dir "Free"
# PROP BASE Cmd_Line "NMAKE /f PassThru.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "PassThru.exe"
# PROP BASE Bsc_Name "PassThru.bsc"
# PROP BASE Target_Dir ""
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Free"
# PROP Intermediate_Dir "Free"
# PROP Cmd_Line "MakeDrvr %DDKROOT% c: %WDMBook%\PassThru\sys free"
# PROP Rebuild_Opt "-nmake /a"
# PROP Target_File "PassThru.sys"
# PROP Bsc_Name "obj\i386\free\PassThru.bsc"
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "PassThru - Win32 Checked"
# Name "PassThru - Win32 Free"

!IF  "$(CFG)" == "PassThru - Win32 Checked"

!ELSEIF  "$(CFG)" == "PassThru - Win32 Free"

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

SOURCE=.\DebugPrint.c
# End Source File
# Begin Source File

SOURCE=.\DebugPrint.h
# End Source File
# Begin Source File

SOURCE=.\GUIDs.h
# End Source File
# Begin Source File

SOURCE=.\Init.cpp
# End Source File
# Begin Source File

SOURCE=.\PassThru.h
# End Source File
# Begin Source File

SOURCE=.\PassThru.rc
# End Source File
# Begin Source File

SOURCE=.\PassThruchecked.inf
# End Source File
# Begin Source File

SOURCE=.\PassThrufree.inf
# End Source File
# Begin Source File

SOURCE=.\Pnp.cpp
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# End Group
# End Target
# End Project
