# Microsoft Developer Studio Project File - Name="Sample" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Sample - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Sample.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Sample.mak" CFG="Sample - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Sample - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Sample - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Sample - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 vtkCommon.lib vtkRendering.lib vtkIO.lib vtkGraphics.lib vtkFiltering.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\vtkbin\lib"

!ELSEIF  "$(CFG)" == "Sample - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\lib\vtkdll.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\vtkbin\Debug\lib"

!ENDIF 

# Begin Target

# Name "Sample - Win32 Release"
# Name "Sample - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ChildFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\Sample.cpp
# End Source File
# Begin Source File

SOURCE=.\Sample.rc
# End Source File
# Begin Source File

SOURCE=.\SampleDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\SampleView.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\vtkMFCDocument.cpp
# End Source File
# Begin Source File

SOURCE=.\vtkMFCRenderView.cpp
# End Source File
# Begin Source File

SOURCE=.\vtkMFCView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ChildFrm.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\Sample.h
# End Source File
# Begin Source File

SOURCE=.\SampleDoc.h
# End Source File
# Begin Source File

SOURCE=.\SampleView.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\vtkMFCDocument.h
# End Source File
# Begin Source File

SOURCE=.\vtkMFCRenderView.h
# End Source File
# Begin Source File

SOURCE=.\vtkMFCView.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Sample.ico
# End Source File
# Begin Source File

SOURCE=.\res\Sample.rc2
# End Source File
# Begin Source File

SOURCE=.\res\SampleDoc.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\Sample.reg
# End Source File
# End Target
# End Project
# Section Sample : {B7F7B86C-EEC9-11D2-87FE-0060082B79FD}
# 	2:20:vtkMFCRenderView.cpp:vtkMFCRenderView.cpp
# 	2:18:vtkMFCRenderView.h:vtkMFCRenderView.h
# 	2:23:CLASS: vtkMFCRenderView:vtkMFCRenderView
# 	2:19:Application Include:Sample.h
# End Section
# Section Sample : {B7F7B86B-EEC9-11D2-87FE-0060082B79FD}
# 	2:16:vtkMFCDocument.h:vtkMFCDocument.h
# 	2:21:CLASS: vtkMFCDocument:vtkMFCDocument
# 	2:18:vtkMFCDocument.cpp:vtkMFCDocument.cpp
# 	2:19:Application Include:Sample.h
# End Section
# Section Sample : {B7F7B86D-EEC9-11D2-87FE-0060082B79FD}
# 	2:14:vtkMFCView.cpp:vtkMFCView.cpp
# 	2:17:CLASS: vtkMFCView:vtkMFCView
# 	2:12:vtkMFCView.h:vtkMFCView.h
# 	2:19:Application Include:Sample.h
# End Section
