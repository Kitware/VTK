
rem  VTK batch build script
@echo off

rem  Define Pathnames

set vtk_source_tree=c:\darin\vtk\vtk
set vtk_build_tree=c:\darin\vtkbin_opt
set vtk_build_log=c:\darin\buildvtk.log
rem  set CVSROOT=lymbdemo@3.1.7.232:/projects/vtk/cvs


echo beginning vtk build on %COMPUTERNAME% > %vtk_build_log%
date /t >> %vtk_build_log%
time /t >> %vtk_build_log%
echo using vtk source path '%vtk_source_tree%' >> %vtk_build_log%
echo using vtk build path '%vtk_build_tree%' >> %vtk_build_log%

rem  Record environment
echo current environment >> %vtk_build_log%
set >> %vtk_build_log%


if "%1" == "-u" goto do_update
if "%2" == "-u" goto do_update
if "%3" == "-u" goto do_update
if "%4" == "-u" goto do_update
goto config


rem  Get latest source updates from cvs
rem  We still need to use additional arguments to control the cvs update
rem  such as the "-d" option
:do_update

echo beginning cvs update at >> %vtk_build_log%
time /t >> %vtk_build_log%
cd %vtk_source_tree%
cd >> %vtk_build_log%
cvs update -d -A >> %vtk_build_log%


:config
if "%1" == "-c" goto do_config
if "%2" == "-c" goto do_config
if "%3" == "-c" goto do_config
if "%4" == "-c" goto do_config
goto buildopt

rem  Generate Makefiles
rem  usage:  pcmaker vtkHome BuildLocation MSVCLoc JDKLoc useMS useBorland
rem          doPatented doLean doGraphics doImagig doContrib doWorking
rem          doGEMSIP doGEMSVOLUME doGEAE doDFA
:do_config

echo beginning pcmaker config at >> %vtk_build_log%
time /t >> %vtk_build_log%
%vtk_source_tree%\pcmaker\Debug\pcmaker.exe %vtk_source_tree% %vtk_build_tree% c:\msdev 0 1 0 1 1 1 0 0 0 1 1 0 0 >> %vtk_build_log%
rem          | | | | | | | | | | | | DFA
rem          | | | | | | | | | | | geae
rem          | | | | | | | | | | gemsvolume
rem          | | | | | | | | | gemsio/gemsip
rem          | | | | | | | | working
rem          | | | | | | | contrib
rem          | | | | | | imaging
rem          | | | | | graphics
rem          | | | | usenulldebugmacro
rem          | | | patented
rem          | | borland
rem          | ms
rem          jdkloc


:buildopt
if "%1" == "-o" goto do_buildopt
if "%2" == "-o" goto do_buildopt
if "%3" == "-o" goto do_buildopt
if "%4" == "-o" goto do_buildopt
goto builddbg


rem  Build Optimized version (no debug and without incremental linking)
:do_buildopt

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


:builddbg
if "%1" == "-d" goto do_builddbg
if "%2" == "-d" goto do_builddbg
if "%3" == "-d" goto do_builddbg
if "%4" == "-d" goto do_builddbg
goto done

rem  Build Incremental-link Debug version
:do_builddbg

echo beginning incremental-link debug build at >> %vtk_build_log%
time /t >> %vtk_build_log%
cd %vtk_build_tree%\Debug
cd >> %vtk_build_log%
nmake >> %vtk_build_log%


rem  All Done!

:done
echo done with vtk build at >> %vtk_build_log%
time /t >> %vtk_build_log%



