@echo off
if "%1"=="" goto usage
if "%3"=="" goto usage
if not exist %1\bin\setenv.bat goto usage
call %1\bin\setenv %1 %4
%2
cd %3
build -b -w %5 %6 %7 %8 %9
goto exit

:usage
echo usage   MakeDrvr DDK_dir Driver_Drive Driver_Dir free/checked [build_options]
echo eg      MakeDrvr %%DDKROOT%% C: %%WDMBOOK%% free -cef
:exit