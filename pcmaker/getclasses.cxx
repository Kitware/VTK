#include <direct.h>
#include <fstream.h>
#include <string.h>
#include <stdio.h>

char *abstract_h[2048];
char *concrete_h[2048];
char *abstract[2048];
char *concrete[2048];
const char *abstract_lib[2048];
const char *concrete_lib[2048];
const char *abstract_h_lib[2048];
const char *concrete_h_lib[2048];
int num_abstract_h = 0;
int num_concrete_h = 0;
int num_abstract = 0;
int num_concrete = 0;
char *kitName;
char *names[2048];
int anindex = 0;

void readInMakefile(char *fname,const char *libname)
{
  ifstream *IS;
  char line[256];

  if (!(IS = new ifstream(fname)))
    {
    }

  // search for keywords
  do
    {
    *IS >> line;
    }
  while (strcmp(line,"ABSTRACT_H"));
  
  // now store abstract classes
  while (strcmp(line,"CONCRETE_H"))
    {
    if ((line[0] == 'v')&&(line[1] == 't'))
      {
      abstract_h[num_abstract_h] = strdup(line);
	  abstract_h_lib[num_abstract_h] = libname;
      num_abstract_h++;
      }
    *IS >> line;
    }
  
  // now store concrete classes
  while (strcmp(line,"ABSTRACT"))
    {
    if ((line[0] == 'v')&&(line[1] == 't'))
      {
      concrete_h[num_concrete_h] = strdup(line);
	  concrete_h_lib[num_concrete_h] = libname;
      num_concrete_h++;
      }
    *IS >> line;
    }

  // now store abstract classes
  while (strcmp(line,"CONCRETE"))
    {
    if ((line[0] == 'v')&&(line[1] == 't'))
      {
      abstract[num_abstract] = strdup(line);
      abstract_lib[num_abstract] = libname;
      num_abstract++;
      }
    *IS >> line;
    }
  
  // now store concrete classes
  while (strcmp(line,"@MAKEINCLUDE@"))
    {
    if ((line[0] == 'v')&&(line[1] == 't'))
      {
      concrete[num_concrete] = strdup(line);
      concrete_lib[num_concrete] = libname;
      num_concrete++;
      }
    *IS >> line;
    }
}

void doMSHeader(FILE *fp, const char *vtkHome,
                const char *vtkBuild)
{
  int i;

  fprintf(fp,"# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20\n");
  fprintf(fp,"# ** DO NOT EDIT **\n\n");
  fprintf(fp,"# TARGTYPE \"Win32 (x86) Dynamic-Link Library\" 0x0102\n\n");
  fprintf(fp,"!IF \"$(CFG)\" == \"\"\n");
  fprintf(fp,"CFG=vtkdll - Win32 Debug\n");
  fprintf(fp,"!MESSAGE No configuration specified.  Defaulting to vtkdll - Win32 Debug.\n");
  fprintf(fp,"!ENDIF \n\n");
  fprintf(fp,"!IF \"$(CFG)\" != \"vtkdll - Win32 Release\" && \"$(CFG)\" != \"vtkdll - Win32 Debug\"\n");
  fprintf(fp,"!MESSAGE Invalid configuration \"$(CFG)\" specified.\n");
  fprintf(fp,"!MESSAGE You can specify a configuration when running NMAKE on this makefile\n");
  fprintf(fp,"!MESSAGE by defining the macro CFG on the command line.  For example:\n");
  fprintf(fp,"!MESSAGE\n"); 
  fprintf(fp,"!MESSAGE NMAKE /f \"vtkdll.mak\" CFG=\"vtkdll - Win32 Debug\"\n");
  fprintf(fp,"!MESSAGE \n");
  fprintf(fp,"!MESSAGE Possible choices for configuration are:\n");
  fprintf(fp,"!MESSAGE \n");
  fprintf(fp,"!MESSAGE \"vtkdll - Win32 Release\" (based on \"Win32 (x86) Dynamic-Link Library\")\n");
  fprintf(fp,"!MESSAGE \"vtkdll - Win32 Debug\" (based on \"Win32 (x86) Dynamic-Link Library\")\n");
  fprintf(fp,"!MESSAGE \n");
  fprintf(fp,"!ERROR An invalid configuration is specified.\n");
  fprintf(fp,"!ENDIF \n\n");
  fprintf(fp,"!IF \"$(OS)\" == \"Windows_NT\"\n");
  fprintf(fp,"NULL=\n");
  fprintf(fp,"!ELSE \n");
  fprintf(fp,"NULL=nul\n");
  fprintf(fp,"!ENDIF \n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Project\n");
  fprintf(fp,"# PROP Target_Last_Scanned \"vtkdll - Win32 Debug\"\n");
  fprintf(fp,"RSC=rc.exe\n");
  fprintf(fp,"MTL=mktyplib.exe\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"!IF  \"$(CFG)\" == \"vtkdll - Win32 Release\"\n\n");
  fprintf(fp,"# PROP BASE Use_MFC 6\n");
  fprintf(fp,"# PROP BASE Use_Debug_Libraries 0\n");
  fprintf(fp,"# PROP BASE Output_Dir \"Release\"\n");
  fprintf(fp,"# PROP BASE Intermediate_Dir \"Release\"\n");
  fprintf(fp,"# PROP BASE Target_Dir \"\"\n");
  fprintf(fp,"# PROP Use_MFC 6\n");
  fprintf(fp,"# PROP Use_Debug_Libraries 0\n");
  fprintf(fp,"# PROP Output_Dir \"Release\"\n");
  fprintf(fp,"# PROP Intermediate_Dir \"Release\"\n");
  fprintf(fp,"# PROP Target_Dir \"\"\n");
  fprintf(fp,"OUTDIR=.\\Release\n");
  fprintf(fp,"INTDIR=.\\Release\n\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtkdll.dll\" \"$(OUTDIR)\\vtkdll.tlb\" \"$(OUTDIR)\\vtkdll.pch\"\n\n");

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D \"WIN32\" /D \"NDEBUG\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /Yu\"stdafx.h\" /c\n");
  fprintf(fp,"# ADD CPP /nologo /MD /GX /O2 /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\" /YX /c\n",
    vtkHome,vtkHome);
  fprintf(fp,"CPP_PROJ=/nologo /MD /GX /O2 /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
    vtkHome,vtkHome);
  fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\"\\\n");
  fprintf(fp," /Fp\"$(INTDIR)/vtkdll.pch\" /YX /Fo\"$(INTDIR)/\" /c \n");
  fprintf(fp,"CPP_OBJS=.\\Release/\n");
  fprintf(fp,"CPP_SBRS=.\\.\n");
  fprintf(fp,"# ADD BASE MTL /nologo /D \"NDEBUG\" /win32\n");
  fprintf(fp,"# ADD MTL /nologo /D \"NDEBUG\" /win32\n");
  fprintf(fp,"MTL_PROJ=/nologo /D \"NDEBUG\" /win32 \n");
  fprintf(fp,"# ADD BASE RSC /l 0x409 /d \"NDEBUG\" /d \"_AFXDLL\"\n");
  fprintf(fp,"# ADD RSC /l 0x409 /d \"NDEBUG\" /d \"_AFXDLL\"\n");
  fprintf(fp,"RSC_PROJ=/l 0x409 /fo\"$(INTDIR)/vtkdll.res\" /d \"NDEBUG\" /d \"_AFXDLL\" \n");
  fprintf(fp,"BSC32=bscmake.exe\n");
  fprintf(fp,"# ADD BASE BSC32 /nologo\n");
  fprintf(fp,"# ADD BSC32 /nologo\n");
  fprintf(fp,"BSC32_FLAGS=/nologo /o\"$(OUTDIR)/vtkdll.bsc\" \n");
  fprintf(fp,"BSC32_SBRS= \\\n");
  fprintf(fp,"	\n");
  fprintf(fp,"LINK32=link.exe\n");
  fprintf(fp,"# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386\n");
  fprintf(fp,"# ADD LINK32 opengl32.lib glaux.lib /nologo /version:1.3 /subsystem:windows /dll /machine:I386\n");
  fprintf(fp,"# SUBTRACT LINK32 /incremental:yes /nodefaultlib\n");
  fprintf(fp,"LINK32_FLAGS=opengl32.lib glaux.lib /nologo /version:1.3 /subsystem:windows\\\n");
  fprintf(fp," /dll /incremental:no /pdb:\"$(OUTDIR)/vtkdll.pdb\" /machine:I386\\\n");
  fprintf(fp," /def:\".\\vtkdll.def\" /out:\"$(OUTDIR)/vtkdll.dll\" /implib:\"$(OUTDIR)/vtkdll.lib\" \n");
  fprintf(fp,"DEF_FILE= \\\n");
  fprintf(fp,"	\".\\vtkdll.def\"\n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(INTDIR)\\StdAfx.obj\" \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtkdll.obj\" \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtkdll.res\" \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%s.obj\" \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%s.obj\" \\\n",concrete[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkdll.dll\" : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"<<\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ELSEIF  \"$(CFG)\" == \"vtkdll - Win32 Debug\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"# PROP BASE Use_MFC 6\n");
  fprintf(fp,"# PROP BASE Use_Debug_Libraries 1\n");
  fprintf(fp,"# PROP BASE Output_Dir \"Debug\"\n");
  fprintf(fp,"# PROP BASE Intermediate_Dir \"Debug\"\n");
  fprintf(fp,"# PROP BASE Target_Dir \"\"\n");
  fprintf(fp,"# PROP Use_MFC 6\n");
  fprintf(fp,"# PROP Use_Debug_Libraries 1\n");
  fprintf(fp,"# PROP Output_Dir \"Debug\"\n");
  fprintf(fp,"# PROP Intermediate_Dir \"Debug\"\n");
  fprintf(fp,"# PROP Target_Dir \"\"\n");
  fprintf(fp,"OUTDIR=.\\Debug\n");
  fprintf(fp,"INTDIR=.\\Debug\n");
  fprintf(fp,"\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtkdll.dll\" \"$(OUTDIR)\\vtkdll.tlb\" \"$(OUTDIR)\\vtkdll.pch\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D \"WIN32\" /D \"_DEBUG\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /Yu\"stdafx.h\" /c\n");
  fprintf(fp,"# ADD CPP /nologo /MD /Gm /GX /Zi /Od /I \"%s\\common\" /I \"%s\\graphics\" /D \"_DEBUG\" /D \"WIN32\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\" /YX /c\n",
    vtkHome,vtkHome);
  fprintf(fp,"CPP_PROJ=/nologo /MD /Gm /GX /Zi /Od /I \"%s\\common\" /I \"%s\\graphics\" /D \"_DEBUG\" /D \"WIN32\" /D\\\n",
    vtkHome,vtkHome);
  fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\"\\\n");
  fprintf(fp," /Fp\"$(INTDIR)/vtkdll.pch\" /YX /Fo\"$(INTDIR)/\" /Fd\"$(INTDIR)/\" /c \n");
  fprintf(fp,"CPP_OBJS=.\\Debug/\n");
  fprintf(fp,"CPP_SBRS=.\\.\n");
  fprintf(fp,"# ADD BASE MTL /nologo /D \"_DEBUG\" /win32\n");
  fprintf(fp,"# ADD MTL /nologo /D \"_DEBUG\" /win32\n");
  fprintf(fp,"MTL_PROJ=/nologo /D \"_DEBUG\" /win32 \n");
  fprintf(fp,"# ADD BASE RSC /l 0x409 /d \"_DEBUG\" /d \"_AFXDLL\"\n");
  fprintf(fp,"# ADD RSC /l 0x409 /d \"_DEBUG\" /d \"_AFXDLL\"\n");
  fprintf(fp,"RSC_PROJ=/l 0x409 /fo\"$(INTDIR)/vtkdll.res\" /d \"_DEBUG\" /d \"_AFXDLL\" \n");
  fprintf(fp,"BSC32=bscmake.exe\n");
  fprintf(fp,"# ADD BASE BSC32 /nologo\n");
  fprintf(fp,"# ADD BSC32 /nologo\n");
  fprintf(fp,"BSC32_FLAGS=/nologo /o\"$(OUTDIR)/vtkdll.bsc\" \n");
  fprintf(fp,"BSC32_SBRS= \\\n");
  fprintf(fp,"	\n");
  fprintf(fp,"LINK32=link.exe\n");
  fprintf(fp,"# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386\n");
  fprintf(fp,"# ADD LINK32 opengl32.lib glaux.lib /nologo /version:1.3 /subsystem:windows /dll /incremental:no /debug /machine:I386\n");
  fprintf(fp,"# SUBTRACT LINK32 /nodefaultlib\n");
  fprintf(fp,"LINK32_FLAGS=opengl32.lib glaux.lib /nologo /version:1.3 /subsystem:windows\\\n");
  fprintf(fp," /dll /incremental:no /pdb:\"$(OUTDIR)/vtkdll.pdb\" /debug /machine:I386\\\n");
  fprintf(fp," /def:\".\\vtkdll.def\" /out:\"$(OUTDIR)/vtkdll.dll\" /implib:\"$(OUTDIR)/vtkdll.lib\" \n");
  fprintf(fp,"DEF_FILE= \\\n");
  fprintf(fp,"	\".\\vtkdll.def\"\n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(INTDIR)\\StdAfx.obj\" \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtkdll.obj\" \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtkdll.res\" \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%s.obj\" \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%s.obj\" \\\n",concrete[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkdll.dll\" : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"<<\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ENDIF \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Target\n");
  fprintf(fp,"\n");
  fprintf(fp,"# Name \"vtkdll - Win32 Release\"\n");
  fprintf(fp,"# Name \"vtkdll - Win32 Debug\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"!IF  \"$(CFG)\" == \"vtkdll - Win32 Release\"\n\n");
  fprintf(fp,"!ELSEIF  \"$(CFG)\" == \"vtkdll - Win32 Debug\"\n\n");
  fprintf(fp,"!ENDIF\n"); 
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Source File\n");
  fprintf(fp,"\n");
  fprintf(fp,"SOURCE=%s\\vtkdll\\vtkdll.odl\n",vtkHome);
  fprintf(fp,"\n");
  fprintf(fp,"!IF  \"$(CFG)\" == \"vtkdll - Win32 Release\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkdll.tlb\" : $(SOURCE) \"$(OUTDIR)\"\n");
  fprintf(fp,"   $(MTL) /nologo /D \"NDEBUG\" /tlb \"$(OUTDIR)/vtkdll.tlb\" /win32 $(SOURCE)\n");
  fprintf(fp,"\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ELSEIF  \"$(CFG)\" == \"vtkdll - Win32 Debug\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkdll.tlb\" : $(SOURCE) \"$(OUTDIR)\"\n");
  fprintf(fp,"   $(MTL) /nologo /D \"_DEBUG\" /tlb \"$(OUTDIR)/vtkdll.tlb\" /win32 $(SOURCE)\n");
  fprintf(fp,"\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ENDIF \n");
  fprintf(fp,"\n");
  fprintf(fp,"# End Source File\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Source File\n");
  fprintf(fp,"\n");
  fprintf(fp,"SOURCE=%s\\vtkdll\\StdAfx.cpp\n",vtkHome);
  fprintf(fp,"DEP_CPP_STDAF=\\\n");
  fprintf(fp,"	\"%s\\vtkdll\\StdAfx.h\"\\\n",vtkHome);
  fprintf(fp,"	\n");
  fprintf(fp,"\n");
  fprintf(fp,"!IF  \"$(CFG)\" == \"vtkdll - Win32 Release\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"# ADD CPP /Yc\"stdafx.h\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"BuildCmds= \\\n");
  fprintf(fp,"	$(CPP) /nologo /MD /GX /O2 /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\"\\\n",
    vtkHome,vtkHome);
  fprintf(fp," /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\"\\\n");
  fprintf(fp," /Fp\"$(INTDIR)/vtkdll.pch\" /Yc\"stdafx.h\" /Fo\"$(INTDIR)/\" /c $(SOURCE) \\\n");
  fprintf(fp,"	\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(INTDIR)\\StdAfx.obj\" : $(SOURCE) $(DEP_CPP_STDAF) \"$(INTDIR)\"\n");
  fprintf(fp,"   $(BuildCmds)\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(INTDIR)\\vtkdll.pch\" : $(SOURCE) $(DEP_CPP_STDAF) \"$(INTDIR)\"\n");
  fprintf(fp,"   $(BuildCmds)\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ELSEIF  \"$(CFG)\" == \"vtkdll - Win32 Debug\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"# ADD CPP /Yc\"stdafx.h\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"BuildCmds= \\\n");
  fprintf(fp,"	$(CPP) /nologo /MD /Gm /GX /Zi /Od /I \"%s\\common\" /I \"%s\\graphics\" /D \"_DEBUG\" /D \"WIN32\" /D\\\n",
    vtkHome,vtkHome);
  fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\"\\\n");
  fprintf(fp," /Fp\"$(INTDIR)/vtkdll.pch\" /Yc\"stdafx.h\" /Fo\"$(INTDIR)/\" /Fd\"$(INTDIR)/\" /c\\\n");
  fprintf(fp," $(SOURCE) \\\n");
  fprintf(fp,"	\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(INTDIR)\\StdAfx.obj\" : $(SOURCE) $(DEP_CPP_STDAF) \"$(INTDIR)\"\n");
  fprintf(fp,"   $(BuildCmds)\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(INTDIR)\\vtkdll.pch\" : $(SOURCE) $(DEP_CPP_STDAF) \"$(INTDIR)\"\n");
  fprintf(fp,"   $(BuildCmds)\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ENDIF \n");
  fprintf(fp,"\n");
  fprintf(fp,"# End Source File\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Source File\n");
  fprintf(fp,"\n");
  fprintf(fp,"SOURCE=%s\\vtkdll\\vtkdll.rc\n",vtkHome);
  fprintf(fp,"\n");
  fprintf(fp,"!IF  \"$(CFG)\" == \"vtkdll - Win32 Release\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(INTDIR)\\vtkdll.res\" : $(SOURCE) \"$(INTDIR)\"\n");
  fprintf(fp,"   $(RSC) /l 0x409 /fo\"$(INTDIR)/vtkdll.res\" /i \"Release\" /d \"NDEBUG\" /d\\\n");
  fprintf(fp," \"_AFXDLL\" $(SOURCE)\n");
  fprintf(fp,"\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ELSEIF  \"$(CFG)\" == \"vtkdll - Win32 Debug\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(INTDIR)\\vtkdll.res\" : $(SOURCE) \"$(INTDIR)\"\n");
  fprintf(fp,"   $(RSC) /l 0x409 /fo\"$(INTDIR)/vtkdll.res\" /i \"Debug\" /d \"_DEBUG\" /d\\\n");
  fprintf(fp," \"_AFXDLL\" $(SOURCE)\n");
  fprintf(fp,"\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ENDIF \n");
  fprintf(fp,"\n");
  fprintf(fp,"# End Source File\n");

  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=%s\\%s\\%s.cxx\n",vtkHome,abstract_lib[i],abstract[i]);
    fprintf(fp,"\n");
    fprintf(fp,"\"$(INTDIR)\\%s.obj\" : $(SOURCE) \"$(INTDIR)\"\n",abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) $(SOURCE)\n\n");
    fprintf(fp,"# End Source File\n");
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"################################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=%s\\%s\\%s.cxx\n",vtkHome,concrete_lib[i],concrete[i]);
    fprintf(fp,"\n");
    fprintf(fp,"\"$(INTDIR)\\%s.obj\" : $(SOURCE) \"$(INTDIR)\"\n",abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) $(SOURCE)\n\n");
    fprintf(fp,"# End Source File\n");
  }
  fprintf(fp,"# End Target\n");
  fprintf(fp,"# EndProject\n");
  fprintf(fp,"################################################################################\n");
}

void doMSTclHeader(FILE *fp, const char *vtkHome,
		   const char *vtkBuild)
{
  int i;

  fprintf(fp,"# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20\n");
  fprintf(fp,"# ** DO NOT EDIT **\n\n");
  fprintf(fp,"# TARGTYPE \"Win32 (x86) Dynamic-Link Library\" 0x0102\n\n");
  fprintf(fp,"!IF \"$(CFG)\" == \"\"\n");
  fprintf(fp,"CFG=vtktcl - Win32 Debug\n");
  fprintf(fp,"!MESSAGE No configuration specified.  Defaulting to vtktcl - Win32 Debug.\n");
  fprintf(fp,"!ENDIF \n\n");
  fprintf(fp,"!IF \"$(CFG)\" != \"vtktcl - Win32 Release\" && \"$(CFG)\" != \"vtktcl - Win32 Debug\"\n");
  fprintf(fp,"!MESSAGE Invalid configuration \"$(CFG)\" specified.\n");
  fprintf(fp,"!MESSAGE You can specify a configuration when running NMAKE on this makefile\n");
  fprintf(fp,"!MESSAGE by defining the macro CFG on the command line.  For example:\n");
  fprintf(fp,"!MESSAGE\n"); 
  fprintf(fp,"!MESSAGE NMAKE /f \"vtktcl.mak\" CFG=\"vtktcl - Win32 Debug\"\n");
  fprintf(fp,"!MESSAGE \n");
  fprintf(fp,"!MESSAGE Possible choices for configuration are:\n");
  fprintf(fp,"!MESSAGE \n");
  fprintf(fp,"!MESSAGE \"vtktcl - Win32 Release\" (based on \"Win32 (x86) Dynamic-Link Library\")\n");
  fprintf(fp,"!MESSAGE \"vtktcl - Win32 Debug\" (based on \"Win32 (x86) Dynamic-Link Library\")\n");
  fprintf(fp,"!MESSAGE \n");
  fprintf(fp,"!ERROR An invalid configuration is specified.\n");
  fprintf(fp,"!ENDIF \n\n");
  fprintf(fp,"!IF \"$(OS)\" == \"Windows_NT\"\n");
  fprintf(fp,"NULL=\n");
  fprintf(fp,"!ELSE \n");
  fprintf(fp,"NULL=nul\n");
  fprintf(fp,"!ENDIF \n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Project\n");
  fprintf(fp,"# PROP Target_Last_Scanned \"vtktcl - Win32 Debug\"\n");
  fprintf(fp,"RSC=rc.exe\n");
  fprintf(fp,"MTL=mktyplib.exe\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"!IF  \"$(CFG)\" == \"vtktcl - Win32 Release\"\n\n");
  fprintf(fp,"# PROP BASE Use_MFC 6\n");
  fprintf(fp,"# PROP BASE Use_Debug_Libraries 0\n");
  fprintf(fp,"# PROP BASE Output_Dir \"Release\"\n");
  fprintf(fp,"# PROP BASE Intermediate_Dir \"Release\"\n");
  fprintf(fp,"# PROP BASE Target_Dir \"\"\n");
  fprintf(fp,"# PROP Use_MFC 6\n");
  fprintf(fp,"# PROP Use_Debug_Libraries 0\n");
  fprintf(fp,"# PROP Output_Dir \"Release\"\n");
  fprintf(fp,"# PROP Intermediate_Dir \"Release\"\n");
  fprintf(fp,"# PROP Target_Dir \"\"\n");
  fprintf(fp,"OUTDIR=.\\Release\n");
  fprintf(fp,"INTDIR=.\\Release\n\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtktcl.dll\"\n\n");
  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D \"WIN32\" /D \"NDEBUG\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /Yu\"stdafx.h\" /c\n");
  fprintf(fp,"# ADD CPP /nologo /MD /GX /O2 /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\" /YX /c\n",
    vtkHome, vtkHome, vtkHome, vtkHome, vtkHome);
  fprintf(fp,"CPP_PROJ=/nologo /MD /GX /O2 /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
    vtkHome, vtkHome, vtkHome, vtkHome, vtkHome);
  fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\"\\\n");
  fprintf(fp," /Fp\"$(INTDIR)/vtkdll.pch\" /YX /Fo\"$(INTDIR)/\" /c \n");
  fprintf(fp,"CPP_OBJS=.\\Release/\n");
  fprintf(fp,"CPP_SBRS=.\\.\n");
  fprintf(fp,"# ADD BASE MTL /nologo /D \"NDEBUG\" /win32\n");
  fprintf(fp,"# ADD MTL /nologo /D \"NDEBUG\" /win32\n");
  fprintf(fp,"MTL_PROJ=/nologo /D \"NDEBUG\" /win32 \n");
  fprintf(fp,"# ADD BASE RSC /l 0x409 /d \"NDEBUG\" /d \"_AFXDLL\"\n");
  fprintf(fp,"# ADD RSC /l 0x409 /d \"NDEBUG\" /d \"_AFXDLL\"\n");
  fprintf(fp,"RSC_PROJ=/l 0x409 /fo\"$(INTDIR)/vtktcl.res\" /d \"NDEBUG\" /d \"_AFXDLL\" \n");
  fprintf(fp,"BSC32=bscmake.exe\n");
  fprintf(fp,"# ADD BASE BSC32 /nologo\n");
  fprintf(fp,"# ADD BSC32 /nologo\n");
  fprintf(fp,"BSC32_FLAGS=/nologo /o\"$(OUTDIR)/vtktcl.bsc\" \n");
  fprintf(fp,"BSC32_SBRS= \\\n");
  fprintf(fp,"	\n");
  fprintf(fp,"LINK32=link.exe\n");
  fprintf(fp,"# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386\n");
  fprintf(fp,"# ADD LINK32 ..\\vtkdll\\Release\\vtkdll.lib %s\\pcmaker\\tk42.lib %s\\pcmaker\\tcl76.lib /nologo /version:1.3 /subsystem:windows /dll /machine:I386\n",
	  vtkHome, vtkHome);
  fprintf(fp,"# SUBTRACT LINK32 /incremental:yes /nodefaultlib\n");
  fprintf(fp,"LINK32_FLAGS= ..\\vtkdll\\Release\\vtkdll.lib %s\\pcmaker\\tk42.lib %s\\pcmaker\\tcl76.lib /nologo /version:1.3 /subsystem:windows\\\n",
	  vtkHome, vtkHome);
  fprintf(fp," /dll /incremental:no /pdb:\"$(OUTDIR)/vtktcl.pdb\" /machine:I386\\\n");
  fprintf(fp," /out:\"$(OUTDIR)/vtktcl.dll\" /implib:\"$(OUTDIR)/vtktcl.lib\" \n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtkTclUtil.obj\" \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtktcl.obj\" \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%sTcl.obj\" \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%sTcl.obj\" \\\n",concrete[i]);
  }
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%sTcl.obj\" \\\n",abstract_h[i]);
  }
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%sTcl.obj\" \\\n",concrete_h[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtktcl.dll\" : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"<<\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ELSEIF  \"$(CFG)\" == \"vtktcl - Win32 Debug\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"# PROP BASE Use_MFC 6\n");
  fprintf(fp,"# PROP BASE Use_Debug_Libraries 1\n");
  fprintf(fp,"# PROP BASE Output_Dir \"Debug\"\n");
  fprintf(fp,"# PROP BASE Intermediate_Dir \"Debug\"\n");
  fprintf(fp,"# PROP BASE Target_Dir \"\"\n");
  fprintf(fp,"# PROP Use_MFC 6\n");
  fprintf(fp,"# PROP Use_Debug_Libraries 1\n");
  fprintf(fp,"# PROP Output_Dir \"Debug\"\n");
  fprintf(fp,"# PROP Intermediate_Dir \"Debug\"\n");
  fprintf(fp,"# PROP Target_Dir \"\"\n");
  fprintf(fp,"OUTDIR=.\\Debug\n");
  fprintf(fp,"INTDIR=.\\Debug\n");
  fprintf(fp,"\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtktcl.dll\" \n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D \"WIN32\" /D \"_DEBUG\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /Yu\"stdafx.h\" /c\n");
  fprintf(fp,"# ADD CPP /nologo /MD /Gm /GX /Zi /Od /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"_DEBUG\" /D \"WIN32\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\" /YX /c\n",
    vtkHome, vtkHome, vtkHome, vtkHome, vtkHome);
  fprintf(fp,"CPP_PROJ=/nologo /MD /Gm /GX /Zi /Od /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"_DEBUG\" /D \"WIN32\" /D\\\n",
    vtkHome, vtkHome, vtkHome, vtkHome, vtkHome);
  fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" \\\n");
  fprintf(fp," /Fp\"$(INTDIR)/vtkdll.pch\" /YX /Fo\"$(INTDIR)/\" /Fd\"$(INTDIR)/\" /c \n");
  fprintf(fp,"CPP_OBJS=.\\Debug/\n");
  fprintf(fp,"CPP_SBRS=.\\.\n");
  fprintf(fp,"# ADD BASE MTL /nologo /D \"_DEBUG\" /win32\n");
  fprintf(fp,"# ADD MTL /nologo /D \"_DEBUG\" /win32\n");
  fprintf(fp,"MTL_PROJ=/nologo /D \"_DEBUG\" /win32 \n");
  fprintf(fp,"# ADD BASE RSC /l 0x409 /d \"_DEBUG\" /d \"_AFXDLL\"\n");
  fprintf(fp,"# ADD RSC /l 0x409 /d \"_DEBUG\" /d \"_AFXDLL\"\n");
  fprintf(fp,"RSC_PROJ=/l 0x409 /fo\"$(INTDIR)/vtktcl.res\" /d \"_DEBUG\" /d \"_AFXDLL\" \n");
  fprintf(fp,"BSC32=bscmake.exe\n");
  fprintf(fp,"# ADD BASE BSC32 /nologo\n");
  fprintf(fp,"# ADD BSC32 /nologo\n");
  fprintf(fp,"BSC32_FLAGS=/nologo /o\"$(OUTDIR)/vtktcl.bsc\" \n");
  fprintf(fp,"BSC32_SBRS= \\\n");
  fprintf(fp,"	\n");
  fprintf(fp,"LINK32=link.exe\n");
  fprintf(fp,"# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386\n");
  fprintf(fp,"# ADD LINK32 ..\\vtkdll\\Debug\\vtkdll.lib %s\\pcmaker\\tk42.lib %s\\pcmaker\\tcl76.lib /nologo /version:1.3 /subsystem:windows /dll /incremental:no /debug /machine:I386\n",
	  vtkHome, vtkHome);
  fprintf(fp,"# SUBTRACT LINK32 /nodefaultlib\n");
  fprintf(fp,"LINK32_FLAGS= ..\\vtkdll\\Debug\\vtkdll.lib %s\\pcmaker\\tk42.lib %s\\pcmaker\\tcl76.lib /nologo /version:1.3 /subsystem:windows\\\n",
	  vtkHome, vtkHome);
  fprintf(fp," /dll /incremental:no /pdb:\"$(OUTDIR)/vtkdll.pdb\" /debug /machine:I386\\\n");
  fprintf(fp," /out:\"$(OUTDIR)/vtktcl.dll\" /implib:\"$(OUTDIR)/vtktcl.lib\" \n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtkTclUtil.obj\" \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtktcl.obj\" \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%sTcl.obj\" \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%sTcl.obj\" \\\n",concrete[i]);
  }
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%sTcl.obj\" \\\n",abstract_h[i]);
  }
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"    \"$(INTDIR)\\%sTcl.obj\" \\\n",concrete_h[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtktcl.dll\" : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"<<\n");
  fprintf(fp,"\n");
  fprintf(fp,"!ENDIF \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) $(CPP_PROJ) $<  \n");
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Target\n");
  fprintf(fp,"\n");
  fprintf(fp,"# Name \"vtktcl - Win32 Release\"\n");
  fprintf(fp,"# Name \"vtktcl - Win32 Debug\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"!IF  \"$(CFG)\" == \"vtktcl - Win32 Release\"\n\n");
  fprintf(fp,"!ELSEIF  \"$(CFG)\" == \"vtktcl - Win32 Debug\"\n\n");
  fprintf(fp,"!ENDIF\n"); 
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Source File\n");
  fprintf(fp,"\n");
  fprintf(fp,"SOURCE=%s\\common\\vtkTclUtil.cxx\n",vtkHome);
  fprintf(fp,"\n");
  fprintf(fp,"\"$(INTDIR)\\vtkTclUtil.obj\" : $(SOURCE) \"$(INTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) $(SOURCE)\n\n");
  fprintf(fp,"# End Source File\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"# Begin Source File\n");
  fprintf(fp,"\n");
  fprintf(fp,"SOURCE=.\\src\\vtktcl.cxx\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(INTDIR)\\vtktcl.obj\" : $(SOURCE) \"$(INTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) $(SOURCE)\n\n");
  fprintf(fp,"# End Source File\n");

  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=%s\\%s\\%s.h\n",vtkHome,abstract_lib[i],abstract[i]);
    fprintf(fp,"\n!IF \"$(CFG)\" == \"vtktcl - Win32 Release\"\n\n");
    fprintf(fp,"# Begin Custom Build\n");
    //fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome,abstract_lib[i],abstract[i]); 
    fprintf(fp,"\".\\src\\%sTcl.cxx\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n",
		abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > .\\src\\%sTcl.cxx\n\n",
		vtkHome, abstract[i]);
    fprintf(fp,"# End Custom Build\n\n");
    fprintf(fp,"!ELSEIF \"$(CFG)\" == \"vtktcl - Win32 Debug\"\n\n");
    fprintf(fp,"# Begin Custom Build\n");
    //fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome,abstract_lib[i],abstract[i]); 
    fprintf(fp,"\".\\src\\%sTcl.cxx\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n",
		abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > .\\src\\%sTcl.cxx\n\n",
		vtkHome, abstract[i]);
    fprintf(fp,"# End Custom Build\n\n");
    fprintf(fp,"!ENDIF\n\n");
    fprintf(fp,"# End Source File\n");
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=.\\src\\%sTcl.cxx\n",abstract[i]);
    fprintf(fp,"\n");
    fprintf(fp,"\"$(INTDIR)\\%sTcl.obj\" : $(SOURCE) \"$(INTDIR)\"\n",
		abstract[i],vtkHome,abstract_lib[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) $(SOURCE)\n\n");
    fprintf(fp,"# End Source File\n");
  }
  
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=%s\\%s\\%s.h\n",vtkHome,concrete_lib[i],concrete[i]);
    fprintf(fp,"\n!IF \"$(CFG)\" == \"vtktcl - Win32 Release\"\n\n");
    fprintf(fp,"# Begin Custom Build\n");
    //fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome,concrete_lib[i],concrete[i]); 
    fprintf(fp,"\".\\src\\%sTcl.cxx\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n",
		concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > .\\src\\%sTcl.cxx\n\n",
		vtkHome, concrete[i]);
    fprintf(fp,"# End Custom Build\n\n");
    fprintf(fp,"!ELSEIF \"$(CFG)\" == \"vtktcl - Win32 Debug\"\n\n");
    fprintf(fp,"# Begin Custom Build\n");
    //fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome,concrete_lib[i],concrete[i]); 
    fprintf(fp,"\".\\src\\%sTcl.cxx\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n",
		concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > .\\src\\%sTcl.cxx\n\n",
		vtkHome, concrete[i]);
    fprintf(fp,"# End Custom Build\n\n");
    fprintf(fp,"!ENDIF\n\n");
    fprintf(fp,"# End Source File\n");
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=.\\src\\%sTcl.cxx\n",concrete[i]);
    fprintf(fp,"\n");
    fprintf(fp,"\"$(INTDIR)\\%sTcl.obj\" : $(SOURCE) \"$(INTDIR)\"\n",
		concrete[i],vtkHome,concrete_lib[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) $(SOURCE)\n\n");
    fprintf(fp,"# End Source File\n");
  }
  
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=%s\\%s\\%s.h\n",vtkHome,abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"\n!IF \"$(CFG)\" == \"vtktcl - Win32 Release\"\n\n");
    fprintf(fp,"# Begin Custom Build\n");
    //fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome,abstract_h_lib[i],abstract_h[i]); 
    fprintf(fp,"\".\\src\\%sTcl.cxx\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n",
		abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > .\\src\\%sTcl.cxx\n\n",
		vtkHome, abstract_h[i]);
    fprintf(fp,"# End Custom Build\n\n");
    fprintf(fp,"!ELSEIF \"$(CFG)\" == \"vtktcl - Win32 Debug\"\n\n");
    fprintf(fp,"# Begin Custom Build\n");
    //fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome,abstract_h_lib[i],abstract_h[i]); 
    fprintf(fp,"\".\\src\\%sTcl.cxx\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n",
		abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > .\\src\\%sTcl.cxx\n\n",
		vtkHome, abstract_h[i]);
    fprintf(fp,"# End Custom Build\n\n");
    fprintf(fp,"!ENDIF\n\n");
    fprintf(fp,"# End Source File\n");
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=.\\src\\%sTcl.cxx\n",abstract_h[i]);
    fprintf(fp,"\n");
    fprintf(fp,"\"$(INTDIR)\\%sTcl.obj\" : $(SOURCE) \"$(INTDIR)\"\n",
		abstract_h[i],vtkHome,abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) $(SOURCE)\n\n");
    fprintf(fp,"# End Source File\n");
  }
  
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=%s\\%s\\%s.h\n",vtkHome,concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"\n!IF \"$(CFG)\" == \"vtktcl - Win32 Release\"\n\n");
    fprintf(fp,"# Begin Custom Build\n");
    //fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome,concrete_h_lib[i],concrete_h[i]); 
    fprintf(fp,"\".\\src\\%sTcl.cxx\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n",
		concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > .\\src\\%sTcl.cxx\n\n",
		vtkHome, concrete_h[i]);
    fprintf(fp,"# End Custom Build\n\n");
    fprintf(fp,"!ELSEIF \"$(CFG)\" == \"vtktcl - Win32 Debug\"\n\n");
    fprintf(fp,"# Begin Custom Build\n");
    //fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome,concrete_h_lib[i],concrete_h[i]); 
    fprintf(fp,"\".\\src\\%sTcl.cxx\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"\n",
		concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > .\\src\\%sTcl.cxx\n\n",
		vtkHome, concrete_h[i]);
    fprintf(fp,"# End Custom Build\n\n");
    fprintf(fp,"!ENDIF\n\n");
    fprintf(fp,"# End Source File\n");
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=.\\src\\%sTcl.cxx\n",concrete_h[i]);
    fprintf(fp,"\n");
    fprintf(fp,"\"$(INTDIR)\\%sTcl.obj\" : $(SOURCE) \"$(INTDIR)\"\n",
		concrete_h[i],vtkHome, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) $(SOURCE)\n\n");
    fprintf(fp,"# End Source File\n");
  }
  
    
  fprintf(fp,"# End Target\n");
  fprintf(fp,"# EndProject\n");
  fprintf(fp,"################################################################################\n");
}

void removeUNIXOnlyFiles()
{
  int i;

  for (i = 0; i < num_concrete; i++)
  {
    if (!(strcmp(concrete[i],"vtkXRenderWindow") &&
          strcmp(concrete[i],"vtkXRenderWindowInteractor") &&
          strcmp(concrete[i],"vtkImageXViewer")))
    {
      concrete[i] = concrete[num_concrete - 1];
      concrete_lib[i] = concrete_lib[num_concrete-1];
      num_concrete--;
      i--;
    }
  }
  for (i = 0; i < num_abstract; i++)
  {
    if (!(strcmp(abstract[i],"vtkXRenderWindow") &&
          strcmp(abstract[i],"vtkXRenderWindowInteractor") &&
          strcmp(abstract[i],"vtkImageXViewer")))
    {
      abstract[i] = abstract[num_abstract - 1];
      abstract_lib[i] = abstract_lib[num_abstract - 1];
      num_abstract--;
      i--;
    }
  }
}

// warning this code is also in kit_init.cxx under tcl
void stuffit(FILE *fp)
{
  int i;
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(fp,"int %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",names[i]);
    fprintf(fp,"ClientData %sNewCommand();\n",names[i]);
    }

  if (!strcmp(kitName,"Vtktcl"))
    {
    fprintf(fp,"int vtkCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[]);\n");
    fprintf(fp,"\nTcl_HashTable vtkInstanceLookup;\n");
    fprintf(fp,"Tcl_HashTable vtkPointerLookup;\n");
    fprintf(fp,"Tcl_HashTable vtkCommandLookup;\n");
    }
  else
    {
    fprintf(fp,"\nextern Tcl_HashTable vtkInstanceLookup;\n");
    fprintf(fp,"extern Tcl_HashTable vtkPointerLookup;\n");
    fprintf(fp,"extern Tcl_HashTable vtkCommandLookup;\n");
    }
  fprintf(fp,"extern void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);\n");
  
  fprintf(fp,"\n\nextern \"C\" {__declspec(dllexport) int %s_SafeInit(Tcl_Interp *interp);}\n\n",
	  kitName);
  fprintf(fp,"\n\nextern \"C\" {__declspec(dllexport) int %s_Init(Tcl_Interp *interp);}\n\n",
	  kitName);

  /* create an extern ref to the generic delete function */
  fprintf(fp,"\n\nextern void vtkTclGenericDeleteObject(ClientData cd);\n\n");

  /* define the vtkNewInstanceCommand */
  fprintf(fp,"int vtk%sNewInstanceCommand(ClientData cd, Tcl_Interp *interp,\n                         int argc, char *argv[])\n{\n",kitName);
  fprintf(fp,"  Tcl_HashEntry *entry;\n  int is_new;\n  char temps[80];\n");
  fprintf(fp,"  cd = 0; /* just prevents compiler warnings */\n");

  fprintf(fp,"\n  if (argc != 2)\n    {\n    interp->result = \"vtk object creation requires one argument, a name.\";\n    return TCL_ERROR;\n    }\n\n");
  fprintf(fp,"  if ((argv[1][0] >= '0')&&(argv[1][0] <= '9'))\n    {\n    interp->result = \"vtk object names must start with a letter.\";\n    return TCL_ERROR;\n    }\n\n");
  fprintf(fp,"  if (Tcl_FindHashEntry(&vtkInstanceLookup,argv[1]))\n    {\n    interp->result = \"a vtk object with that name already exists.\";\n    return TCL_ERROR;\n    }\n\n");

  for (i = 0; i < anindex; i++)
    {
    fprintf(fp,"  if (!strcmp(\"%s\",argv[0]))\n    {\n",names[i]);
    fprintf(fp,"    ClientData temp;\n");
    fprintf(fp,"    if (!strcmp(\"ListInstances\",argv[1]))\n      {\n");
    fprintf(fp,"      vtkTclListInstances(interp,%sCommand);\n",names[i]);
    fprintf(fp,"      return TCL_OK;\n      }\n");

    fprintf(fp,"    temp = %sNewCommand();\n",names[i]);
    fprintf(fp,"\n    entry = Tcl_CreateHashEntry(&vtkInstanceLookup,argv[1],&is_new);\n    Tcl_SetHashValue(entry,temp);\n");
    fprintf(fp,"    sprintf(temps,\"%%p\",(void *)temp);\n");
    fprintf(fp,"    entry = Tcl_CreateHashEntry(&vtkPointerLookup,temps,&is_new);\n    Tcl_SetHashValue(entry,(ClientData)(strdup(argv[1])));\n");
    fprintf(fp,"    Tcl_CreateCommand(interp,argv[1],%sCommand,\n",
	    names[i]);
    fprintf(fp,"                      temp,(Tcl_CmdDeleteProc *)vtkTclGenericDeleteObject);\n");
    fprintf(fp,"    entry = Tcl_CreateHashEntry(&vtkCommandLookup,argv[1],&is_new);\n    Tcl_SetHashValue(entry,(ClientData)(%sCommand));\n",names[i]);
    fprintf(fp,"    }\n\n");
    }

  fprintf(fp,"  sprintf(interp->result,\"%%s\",argv[1]);\n  return TCL_OK;\n}");

  /* the main declaration */
  fprintf(fp,"\n\nint %s_SafeInit(Tcl_Interp *interp)\n{\n",kitName);
  fprintf(fp,"  return %s_Init(interp);\n}\n",kitName);
  
  fprintf(fp,"\n\nint %s_Init(Tcl_Interp *interp)\n{\n",kitName);
  if (!strcmp(kitName,"Vtktcl"))
    {
    fprintf(fp,
	    "  Tcl_InitHashTable(&vtkInstanceLookup, TCL_STRING_KEYS);\n");
    fprintf(fp,
	    "  Tcl_InitHashTable(&vtkPointerLookup, TCL_STRING_KEYS);\n");
    fprintf(fp,
	    "  Tcl_InitHashTable(&vtkCommandLookup, TCL_STRING_KEYS);\n");

    /* create special vtkCommand command */
    fprintf(fp,"  Tcl_CreateCommand(interp,\"vtkCommand\",vtkCommand,\n		    (ClientData *)NULL, NULL);\n\n");
    }
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(fp,"  Tcl_CreateCommand(interp,\"%s\",vtk%sNewInstanceCommand,\n		    (ClientData *)NULL,\n		    (Tcl_CmdDeleteProc *)NULL);\n\n",
	    names[i],kitName);
    }

  fprintf(fp,"  return TCL_OK;\n}\n");
}


void MakeInit(char *fname, char *argv1)
{
  int i;
  FILE *fp;

  /* we have to make sure that the name is the correct case */
  kitName = strdup(argv1);
  if (kitName[0] > 90) kitName[0] -= 32;
  for (i = 1; i < strlen(kitName); i++)
    {
    if ((kitName[i] > 64)&&(kitName[i] < 91))
      {
      kitName[i] += 32;
      }
    }
  
  /* fill in the correct arrays */
  for (i = 0; i < num_concrete; i++)
  {
    names[i] = concrete[i];
  }
  for (i = 0; i < num_concrete_h; i++)
  {
	names[i+num_concrete] = concrete_h[i];
  }
  anindex = num_concrete + num_concrete_h;
  
  fp = fopen(fname,"w");
  if (fp)
  {
  fprintf(fp,"#include <string.h>\n");
  fprintf(fp,"#include <tcl.h>\n\n");
  stuffit(fp);
  fclose(fp);
  }
}


void makeMakefile(const char *vtkHome, const char *vtkBuild,
                  int useMS, int useGraphics, int useImaging, int useContrib)
{
  char fname[256];
  FILE *ofp;
    
  sprintf(fname,"%s\\common\\Makefile.in",vtkHome);
  readInMakefile(fname,strdup("common"));
  if (useGraphics)
  {
    sprintf(fname,"%s\\graphics\\Makefile.in",vtkHome);
    readInMakefile(fname,strdup("graphics"));
  }
  if (useImaging)
  {
    sprintf(fname,"%s\\imaging\\Makefile.in",vtkHome);
    readInMakefile(fname,strdup("imaging"));
  }
  if (useContrib)
  {
    sprintf(fname,"%s\\contrib\\Makefile.in",vtkHome);
    readInMakefile(fname,strdup("contrib"));
  }

  //remove any UNIX only stuff
  removeUNIXOnlyFiles();

  // add in the OpenGL stuff etc that we need
  if (useGraphics)
  {
	concrete[num_concrete] = strdup("vtkOglrRenderer");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOglrTexture");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOglrProperty");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOglrActor");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOglrCamera");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOglrPolyMapper");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOglrLight");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkWin32OglrRenderWindow");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkWin32RenderWindowInteractor");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkMFCInteractor");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
  }

  // we must create CommonInit.cxx etc
  sprintf(fname,"%s\\vtktcl\\src\\vtktcl.cxx",vtkBuild);
  MakeInit(fname,"Vtktcl");

  // spit out a Makefile
  sprintf(fname,"%s\\vtkdll\\vtkdll.mak",vtkBuild);
  ofp = fopen(fname,"w");
  if (useMS) doMSHeader(ofp, vtkHome, vtkBuild);
  fclose(ofp);
  
  sprintf(fname,"%s\\vtktcl\\vtktcl.mak",vtkBuild);
  ofp = fopen(fname,"w");
  if (useMS) doMSTclHeader(ofp, vtkHome, vtkBuild);
  fclose(ofp);
  
}
