rem  VTK batch build script
@echo off

rem  Define Pathnames

set vtk_source_tree=c:\darin\vtk\vtk
set vtk_build_tree=c:\darin\vtkbin_opt
set vtk_builddbg_tree=c:\darin\vtkbin
set vtk_build_log=c:\darin\buildvtk.log
rem  set CVSROOT=lymbdemo@3.1.7.232:/projects/vtk/cvs


echo beginning vtk build on %COMPUTERNAME% > %vtk_build_log%
date /t >> %vtk_build_log%
time /t >> %vtk_build_log%
echo using vtk source path '%vtk_source_tree%' >> %vtk_build_log%
echo using vtk build path '%vtk_build_tree%' >> %vtk_build_log%
echo using vtk debug build path '%vtk_builddbg_tree%' >> %vtk_build_log%

rem  Record environment
echo current environment >> %vtk_build_log%
set >> %vtk_build_log%


if "%1" == "-u" goto do_update
if "%2" == "-u" goto do_update
if "%3" == "-u" goto do_update
goto config


rem  Get latest source updates from cvs
rem  We still need to use additional arguments to control the cvs update
rem  such as the "-d" option
:do_update

echo beginning cvs update at >> %vtk_build_log%
time /t >> %vtk_build_log%
cd %vtk_source_tree%
cd >> %vtk_build_log%
cvs update -d -A -R >> %vtk_build_log%


:config
if "%1" == "-c" goto do_config
if "%2" == "-c" goto do_config
if "%3" == "-c" goto do_config
goto build

rem  Generate Makefiles
rem  usage:  pcmaker vtkHome BuildLocation MSVCLoc JDKLoc useMS useBorland
rem          doDebug doPatented useNullDebugMacro doGraphics doImagig doContrib
rem          doWorking doGEMSIO/IP doGEMSVOLUME
:do_config

echo beginning pcmaker config at >> %vtk_build_log%
time /t >> %vtk_build_log%
%vtk_source_tree%\pcmaker\Debug\pcmaker.exe %vtk_source_tree% %vtk_build_tree% c:\msdev 0 1 0 0 1 1 1 0 0 0 1 1 >> %vtk_build_log%


:build

rem  Build vtkdll.dll 

echo beginning vtkdll build at >> %vtk_build_log%
time /t >> %vtk_build_log%
cd %vtk_build_tree%\vtkdll
cd >> %vtk_build_log%
nmake >> %vtk_build_log%

rem  Build vtktcl.dll 

echo beginning vtktcl build at >> %vtk_build_log%
time /t >> %vtk_build_log%
cd %vtk_build_tree%\vtktcl
cd >> %vtk_build_log%
nmake >> %vtk_build_log%


rem  If we're not building a debug version, then we're done

if "%1" == "-d" goto debug_build
if "%2" == "-d" goto debug_build
if "%3" == "-d" goto debug_build
goto done


:debug_build
if "%1" == "-c" goto do_configdbg
if "%2" == "-c" goto do_configdbg
if "%3" == "-c" goto do_configdbg
goto builddbg

rem  Generate debug version of Makefiles
rem  usage:  pcmaker vtkHome BuildLocation MSVCLoc JDKLoc useMS useBorland
rem          doDebug doPatented useNullDebugMacro doGraphics doImagig doContrib
rem          doWorking doGEMSIO/IP doGEMSVOLUME
:do_configdbg

echo beginning pcmaker config for debug version at >> %vtk_build_log%
time /t >> %vtk_build_log%
%vtk_source_tree%\pcmaker\Debug\pcmaker.exe %vtk_source_tree% %vtk_builddbg_tree% c:\msdev 0 1 0 1 1 0 1 0 0 0 1 1 >> %vtk_build_log%


:builddbg

rem  Build debug version of vtkdll.dll 

echo beginning vtkdll debug build at >> %vtk_build_log%
time /t >> %vtk_build_log%
cd %vtk_builddbg_tree%\vtkdll
cd >> %vtk_build_log%
nmake >> %vtk_build_log%


rem  Build debug version of vtktcl.dll 

echo beginning vtktcl debug build at >> %vtk_build_log%
time /t >> %vtk_build_log%
cd %vtk_builddbg_tree%\vtktcl
cd >> %vtk_build_log%
nmake >> %vtk_build_log%


rem  All Done!

:done
echo done with vtk build at >> %vtk_build_log%
time /t >> %vtk_build_log%

