# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=java_wrap - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to java_wrap - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "pcmaker - Win32 Release" && "$(CFG)" !=\
 "pcmaker - Win32 Debug" && "$(CFG)" != "cpp_parse - Win32 Release" && "$(CFG)"\
 != "cpp_parse - Win32 Debug" && "$(CFG)" != "java_parse - Win32 Release" &&\
 "$(CFG)" != "java_parse - Win32 Debug" && "$(CFG)" !=\
 "java_wrap - Win32 Release" && "$(CFG)" != "java_wrap - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "pcmaker.mak" CFG="java_wrap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pcmaker - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "pcmaker - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "cpp_parse - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "cpp_parse - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "java_parse - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "java_parse - Win32 Debug" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "java_wrap - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "java_wrap - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "pcmaker - Win32 Debug"

!IF  "$(CFG)" == "pcmaker - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "java_wrap - Win32 Release" "java_parse - Win32 Release"\
 "cpp_parse - Win32 Release" "$(OUTDIR)\pcmaker.exe" "$(OUTDIR)\pcmaker.pch"

CLEAN : 
	-@erase "$(INTDIR)\getclasses.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\makedepend.obj"
	-@erase "$(INTDIR)\pcmaker.obj"
	-@erase "$(INTDIR)\pcmaker.pch"
	-@erase "$(INTDIR)\pcmaker.res"
	-@erase "$(INTDIR)\pcmakerDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(OUTDIR)\pcmaker.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MT /W2 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
CPP_PROJ=/nologo /MT /W2 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_MBCS" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/pcmaker.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/pcmaker.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/pcmaker.pdb" /machine:I386 /out:"$(OUTDIR)/pcmaker.exe" 
LINK32_OBJS= \
	"$(INTDIR)\getclasses.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\makedepend.obj" \
	"$(INTDIR)\pcmaker.obj" \
	"$(INTDIR)\pcmaker.res" \
	"$(INTDIR)\pcmakerDlg.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\pcmaker.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "java_wrap - Win32 Debug" "java_parse - Win32 Debug"\
 "cpp_parse - Win32 Debug" "$(OUTDIR)\pcmaker.exe" "$(OUTDIR)\pcmaker.pch"

CLEAN : 
	-@erase "$(INTDIR)\getclasses.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\makedepend.obj"
	-@erase "$(INTDIR)\pcmaker.obj"
	-@erase "$(INTDIR)\pcmaker.pch"
	-@erase "$(INTDIR)\pcmaker.res"
	-@erase "$(INTDIR)\pcmakerDlg.obj"
	-@erase "$(INTDIR)\StdAfx.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\pcmaker.exe"
	-@erase "$(OUTDIR)\pcmaker.ilk"
	-@erase "$(OUTDIR)\pcmaker.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_MBCS" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

MTL=mktyplib.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/pcmaker.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/pcmaker.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/pcmaker.pdb" /debug /machine:I386 /out:"$(OUTDIR)/pcmaker.exe" 
LINK32_OBJS= \
	"$(INTDIR)\getclasses.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\makedepend.obj" \
	"$(INTDIR)\pcmaker.obj" \
	"$(INTDIR)\pcmaker.res" \
	"$(INTDIR)\pcmakerDlg.obj" \
	"$(INTDIR)\StdAfx.obj"

"$(OUTDIR)\pcmaker.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "cpp_parse - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "cpp_parse\Release"
# PROP BASE Intermediate_Dir "cpp_parse\Release"
# PROP BASE Target_Dir "cpp_parse"
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "cpp_parse\Release"
# PROP Intermediate_Dir "cpp_parse\Release"
# PROP Target_Dir "cpp_parse"
OUTDIR=.\cpp_parse\Release
INTDIR=.\cpp_parse\Release

ALL : "$(OUTDIR)\cpp_parse.exe"

CLEAN : 
	-@erase "$(INTDIR)\y.tab.obj"
	-@erase "$(OUTDIR)\cpp_parse.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MT /W2 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MT /W2 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D\
 "_MBCS" /Fp"$(INTDIR)/cpp_parse.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\cpp_parse\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/cpp_parse.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 /nologo /subsystem:console /machine:I386
LINK32_FLAGS=/nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/cpp_parse.pdb" /machine:I386 /out:"$(OUTDIR)/cpp_parse.exe" 
LINK32_OBJS= \
	"$(INTDIR)\y.tab.obj"

"$(OUTDIR)\cpp_parse.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "cpp_parse - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "cpp_parse\Debug"
# PROP BASE Intermediate_Dir "cpp_parse\Debug"
# PROP BASE Target_Dir "cpp_parse"
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "cpp_parse\Debug"
# PROP Intermediate_Dir "cpp_parse\Debug"
# PROP Target_Dir "cpp_parse"
OUTDIR=.\cpp_parse\Debug
INTDIR=.\cpp_parse\Debug

ALL : "$(OUTDIR)\cpp_parse.exe"

CLEAN : 
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(INTDIR)\y.tab.obj"
	-@erase "$(OUTDIR)\cpp_parse.exe"
	-@erase "$(OUTDIR)\cpp_parse.ilk"
	-@erase "$(OUTDIR)\cpp_parse.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MTd /W2 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /c
CPP_PROJ=/nologo /MTd /W2 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /D "_MBCS" /Fp"$(INTDIR)/cpp_parse.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\cpp_parse\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/cpp_parse.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/cpp_parse.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/cpp_parse.exe" 
LINK32_OBJS= \
	"$(INTDIR)\y.tab.obj"

"$(OUTDIR)\cpp_parse.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "java_parse - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "java_parse\Release"
# PROP BASE Intermediate_Dir "java_parse\Release"
# PROP BASE Target_Dir "java_parse"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "java_parse\Release"
# PROP Intermediate_Dir "java_parse\Release"
# PROP Target_Dir "java_parse"
OUTDIR=.\java_parse\Release
INTDIR=.\java_parse\Release

ALL : "$(OUTDIR)\java_parse.exe"

CLEAN : 
	-@erase "$(INTDIR)\java_parse.tab.obj"
	-@erase "$(OUTDIR)\java_parse.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/java_parse.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\java_parse\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/java_parse.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/java_parse.pdb" /machine:I386 /out:"$(OUTDIR)/java_parse.exe" 
LINK32_OBJS= \
	"$(INTDIR)\java_parse.tab.obj"

"$(OUTDIR)\java_parse.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "java_parse - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "java_parse\Debug"
# PROP BASE Intermediate_Dir "java_parse\Debug"
# PROP BASE Target_Dir "java_parse"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "java_parse\Debug"
# PROP Intermediate_Dir "java_parse\Debug"
# PROP Target_Dir "java_parse"
OUTDIR=.\java_parse\Debug
INTDIR=.\java_parse\Debug

ALL : "$(OUTDIR)\java_parse.exe"

CLEAN : 
	-@erase "$(INTDIR)\java_parse.tab.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\java_parse.exe"
	-@erase "$(OUTDIR)\java_parse.ilk"
	-@erase "$(OUTDIR)\java_parse.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/java_parse.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\java_parse\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/java_parse.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/java_parse.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/java_parse.exe" 
LINK32_OBJS= \
	"$(INTDIR)\java_parse.tab.obj"

"$(OUTDIR)\java_parse.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "java_wrap - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "java_wrap\Release"
# PROP BASE Intermediate_Dir "java_wrap\Release"
# PROP BASE Target_Dir "java_wrap"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "java_wrap\Release"
# PROP Intermediate_Dir "java_wrap\Release"
# PROP Target_Dir "java_wrap"
OUTDIR=.\java_wrap\Release
INTDIR=.\java_wrap\Release

ALL : "$(OUTDIR)\java_wrap.exe"

CLEAN : 
	-@erase "$(INTDIR)\java_wrap.tab.obj"
	-@erase "$(OUTDIR)\java_wrap.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/java_wrap.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\java_wrap\Release/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/java_wrap.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/java_wrap.pdb" /machine:I386 /out:"$(OUTDIR)/java_wrap.exe" 
LINK32_OBJS= \
	"$(INTDIR)\java_wrap.tab.obj"

"$(OUTDIR)\java_wrap.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "java_wrap - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "java_wrap\Debug"
# PROP BASE Intermediate_Dir "java_wrap\Debug"
# PROP BASE Target_Dir "java_wrap"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "java_wrap\Debug"
# PROP Intermediate_Dir "java_wrap\Debug"
# PROP Target_Dir "java_wrap"
OUTDIR=.\java_wrap\Debug
INTDIR=.\java_wrap\Debug

ALL : "$(OUTDIR)\java_wrap.exe"

CLEAN : 
	-@erase "$(INTDIR)\java_wrap.tab.obj"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\java_wrap.exe"
	-@erase "$(OUTDIR)\java_wrap.ilk"
	-@erase "$(OUTDIR)\java_wrap.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/java_wrap.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\java_wrap\Debug/
CPP_SBRS=.\.

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/java_wrap.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/java_wrap.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/java_wrap.exe" 
LINK32_OBJS= \
	"$(INTDIR)\java_wrap.tab.obj"

"$(OUTDIR)\java_wrap.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

################################################################################
# Begin Target

# Name "pcmaker - Win32 Release"
# Name "pcmaker - Win32 Debug"

!IF  "$(CFG)" == "pcmaker - Win32 Release"

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\ReadMe.txt

!IF  "$(CFG)" == "pcmaker - Win32 Release"

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pcmaker.cpp
DEP_CPP_PCMAK=\
	".\pcmaker.h"\
	".\pcmakerDlg.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\pcmaker.obj" : $(SOURCE) $(DEP_CPP_PCMAK) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\pcmakerDlg.cpp
DEP_CPP_PCMAKE=\
	".\help.h"\
	".\pcmaker.h"\
	".\pcmakerDlg.h"\
	".\StdAfx.h"\
	{$(INCLUDE)}"\sys\Stat.h"\
	{$(INCLUDE)}"\sys\Types.h"\
	

"$(INTDIR)\pcmakerDlg.obj" : $(SOURCE) $(DEP_CPP_PCMAKE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\StdAfx.cpp
DEP_CPP_STDAF=\
	".\StdAfx.h"\
	

!IF  "$(CFG)" == "pcmaker - Win32 Release"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MT /W2 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS"\
 /Fp"$(INTDIR)/pcmaker.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\pcmaker.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_MBCS" /Fp"$(INTDIR)/pcmaker.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /Fd"$(INTDIR)/" /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\pcmaker.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pcmaker.rc
DEP_RSC_PCMAKER=\
	".\res\pcmaker.ico"\
	".\res\pcmaker.rc2"\
	

"$(INTDIR)\pcmaker.res" : $(SOURCE) $(DEP_RSC_PCMAKER) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=.\getclasses.cxx
DEP_CPP_GETCL=\
	".\pcmaker.h"\
	".\pcmakerDlg.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\getclasses.obj" : $(SOURCE) $(DEP_CPP_GETCL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Project Dependency

# Project_Dep_Name "cpp_parse"

!IF  "$(CFG)" == "pcmaker - Win32 Release"

"cpp_parse - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\pcmaker.mak" CFG="cpp_parse - Win32 Release" 

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

"cpp_parse - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\pcmaker.mak" CFG="cpp_parse - Win32 Debug" 

!ENDIF 

# End Project Dependency
################################################################################
# Begin Project Dependency

# Project_Dep_Name "java_parse"

!IF  "$(CFG)" == "pcmaker - Win32 Release"

"java_parse - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\pcmaker.mak" CFG="java_parse - Win32 Release" 

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

"java_parse - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\pcmaker.mak" CFG="java_parse - Win32 Debug" 

!ENDIF 

# End Project Dependency
################################################################################
# Begin Project Dependency

# Project_Dep_Name "java_wrap"

!IF  "$(CFG)" == "pcmaker - Win32 Release"

"java_wrap - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\pcmaker.mak" CFG="java_wrap - Win32 Release" 

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

"java_wrap - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F ".\pcmaker.mak" CFG="java_wrap - Win32 Debug" 

!ENDIF 

# End Project Dependency
################################################################################
# Begin Source File

SOURCE=.\makedepend.cxx

"$(INTDIR)\makedepend.obj" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\help.cpp
DEP_CPP_HELP_=\
	".\help.h"\
	".\pcmaker.h"\
	".\StdAfx.h"\
	

"$(INTDIR)\help.obj" : $(SOURCE) $(DEP_CPP_HELP_) "$(INTDIR)"


# End Source File
# End Target
################################################################################
# Begin Target

# Name "cpp_parse - Win32 Release"
# Name "cpp_parse - Win32 Debug"

!IF  "$(CFG)" == "cpp_parse - Win32 Release"

!ELSEIF  "$(CFG)" == "cpp_parse - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=..\tcl\y.tab.c

!IF  "$(CFG)" == "cpp_parse - Win32 Release"

DEP_CPP_Y_TAB=\
	"..\tcl\lex.yy.c"\
	

"$(INTDIR)\y.tab.obj" : $(SOURCE) $(DEP_CPP_Y_TAB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "cpp_parse - Win32 Debug"

DEP_CPP_Y_TAB=\
	"..\tcl\lex.yy.c"\
	

"$(INTDIR)\y.tab.obj" : $(SOURCE) $(DEP_CPP_Y_TAB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
# End Target
################################################################################
# Begin Target

# Name "java_parse - Win32 Release"
# Name "java_parse - Win32 Debug"

!IF  "$(CFG)" == "java_parse - Win32 Release"

!ELSEIF  "$(CFG)" == "java_parse - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=..\java\java_parse.tab.c
DEP_CPP_JAVA_=\
	"..\java\lex.yy.c"\
	

"$(INTDIR)\java_parse.tab.obj" : $(SOURCE) $(DEP_CPP_JAVA_) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
################################################################################
# Begin Target

# Name "java_wrap - Win32 Release"
# Name "java_wrap - Win32 Debug"

!IF  "$(CFG)" == "java_wrap - Win32 Release"

!ELSEIF  "$(CFG)" == "java_wrap - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=..\java\java_wrap.tab.c
DEP_CPP_JAVA_W=\
	"..\java\lex.yy.c"\
	

"$(INTDIR)\java_wrap.tab.obj" : $(SOURCE) $(DEP_CPP_JAVA_W) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


# End Source File
# End Target
# End Project
################################################################################
