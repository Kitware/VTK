# Microsoft Developer Studio Generated NMAKE File, Format Version 40001
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101
# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=cpp_parse - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to cpp_parse - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "pcmaker - Win32 Release" && "$(CFG)" !=\
 "pcmaker - Win32 Debug" && "$(CFG)" != "cpp_parse - Win32 Release" && "$(CFG)"\
 != "cpp_parse - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "pcmaker.mak" CFG="cpp_parse - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pcmaker - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "pcmaker - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "cpp_parse - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "cpp_parse - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "cpp_parse - Win32 Release" "$(OUTDIR)\pcmaker.exe"\
 "$(OUTDIR)\pcmaker.pch"

CLEAN : 
	-@erase ".\Release\pcmaker.pch"
	-@erase ".\Release\pcmaker.exe"
	-@erase ".\Release\StdAfx.obj"
	-@erase ".\Release\pcmaker.obj"
	-@erase ".\Release\getclasses.obj"
	-@erase ".\Release\pcmakerDlg.obj"
	-@erase ".\Release\pcmaker.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=

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
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/pcmaker.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/pcmaker.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/pcmaker.pdb" /machine:I386 /out:"$(OUTDIR)/pcmaker.exe" 
LINK32_OBJS= \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/pcmaker.obj" \
	"$(INTDIR)/getclasses.obj" \
	"$(INTDIR)/pcmakerDlg.obj" \
	"$(INTDIR)/pcmaker.res"

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
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "cpp_parse - Win32 Debug" "$(OUTDIR)\pcmaker.exe" "$(OUTDIR)\pcmaker.pch"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\pcmaker.pch"
	-@erase ".\Debug\pcmaker.exe"
	-@erase ".\Debug\pcmakerDlg.obj"
	-@erase ".\Debug\getclasses.obj"
	-@erase ".\Debug\pcmaker.obj"
	-@erase ".\Debug\StdAfx.obj"
	-@erase ".\Debug\pcmaker.res"
	-@erase ".\Debug\pcmaker.ilk"
	-@erase ".\Debug\pcmaker.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /c
# SUBTRACT CPP /YX /Yc /Yu
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=

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
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/pcmaker.res" /d "_DEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/pcmaker.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386
LINK32_FLAGS=/nologo /subsystem:windows /incremental:yes\
 /pdb:"$(OUTDIR)/pcmaker.pdb" /debug /machine:I386 /out:"$(OUTDIR)/pcmaker.exe" 
LINK32_OBJS= \
	"$(INTDIR)/pcmakerDlg.obj" \
	"$(INTDIR)/getclasses.obj" \
	"$(INTDIR)/pcmaker.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/pcmaker.res"

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
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "cpp_parse\Release"
# PROP Intermediate_Dir "cpp_parse\Release"
# PROP Target_Dir "cpp_parse"
OUTDIR=.\cpp_parse\Release
INTDIR=.\cpp_parse\Release

ALL : "$(OUTDIR)\cpp_parse.exe"

CLEAN : 
	-@erase ".\cpp_parse\Release\cpp_parse.exe"
	-@erase ".\cpp_parse\Release\y.tab.obj"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/cpp_parse.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\cpp_parse\Release/
CPP_SBRS=

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
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)/cpp_parse.pdb" /machine:I386 /out:"$(OUTDIR)/cpp_parse.exe" 
LINK32_OBJS= \
	"$(INTDIR)/y.tab.obj"

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
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "cpp_parse\Debug"
# PROP Intermediate_Dir "cpp_parse\Debug"
# PROP Target_Dir "cpp_parse"
OUTDIR=.\cpp_parse\Debug
INTDIR=.\cpp_parse\Debug

ALL : "$(OUTDIR)\cpp_parse.exe"

CLEAN : 
	-@erase ".\cpp_parse\Debug\vc40.pdb"
	-@erase ".\cpp_parse\Debug\vc40.idb"
	-@erase ".\cpp_parse\Debug\cpp_parse.exe"
	-@erase ".\cpp_parse\Debug\y.tab.obj"
	-@erase ".\cpp_parse\Debug\cpp_parse.ilk"
	-@erase ".\cpp_parse\Debug\cpp_parse.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE"\
 /Fp"$(INTDIR)/cpp_parse.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\cpp_parse\Debug/
CPP_SBRS=

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
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)/cpp_parse.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/cpp_parse.exe" 
LINK32_OBJS= \
	"$(INTDIR)/y.tab.obj"

"$(OUTDIR)\cpp_parse.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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
	$(CPP) /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D\
 "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/pcmaker.pch" /Yc"stdafx.h" /Fo"$(INTDIR)/"\
 /c $(SOURCE) \
	

"$(INTDIR)\StdAfx.obj" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

"$(INTDIR)\pcmaker.pch" : $(SOURCE) $(DEP_CPP_STDAF) "$(INTDIR)"
   $(BuildCmds)

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

# ADD CPP /Yc"stdafx.h"

BuildCmds= \
	$(CPP) /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /D "_AFXDLL" /D "_MBCS" /Fp"$(INTDIR)/pcmaker.pch" /Yc"stdafx.h"\
 /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c $(SOURCE) \
	

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
	".\StdAfx.h"\
	".\pcmaker.h"\
	".\pcmakerDlg.h"\
	

"$(INTDIR)\getclasses.obj" : $(SOURCE) $(DEP_CPP_GETCL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Project Dependency

# Project_Dep_Name "cpp_parse"

!IF  "$(CFG)" == "pcmaker - Win32 Release"

"cpp_parse - Win32 Release" : 
   $(MAKE) /$(MAKEFLAGS) /F .\pcmaker.mak CFG="cpp_parse - Win32 Release" 

!ELSEIF  "$(CFG)" == "pcmaker - Win32 Debug"

"cpp_parse - Win32 Debug" : 
   $(MAKE) /$(MAKEFLAGS) /F .\pcmaker.mak CFG="cpp_parse - Win32 Debug" 

!ENDIF 

# End Project Dependency
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
	".\..\tcl\lex.yy.c"\
	

"$(INTDIR)\y.tab.obj" : $(SOURCE) $(DEP_CPP_Y_TAB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "cpp_parse - Win32 Debug"

DEP_CPP_Y_TAB=\
	".\..\tcl\lex.yy.c"\
	

"$(INTDIR)\y.tab.obj" : $(SOURCE) $(DEP_CPP_Y_TAB) "$(INTDIR)"
   $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
