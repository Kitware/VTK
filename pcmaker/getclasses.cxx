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
int num_abstract_h = 0;
int num_concrete_h = 0;
int num_abstract = 0;
int num_concrete = 0;

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
  fprintf(fp,"    \"$(INTDIR)\\CommonInit.obj\" \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtkTclUtil.obj\" \\\n");
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
  fprintf(fp,"# ADD CPP /nologo /MD /Gm /GX /Zi /Od /I \"%s\\common\" /I \"%s\\graphics\" \"%s\\imaging\" \"%s\\contrib\" \"%s\\pcmaker\\xlib\" /D \"_DEBUG\" /D \"WIN32\" /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_AFXDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\" /YX /c\n",
    vtkHome, vtkHome, vtkHome, vtkHome, vtkHome);
  fprintf(fp,"CPP_PROJ=/nologo /MD /Gm /GX /Zi /Od /I \"%s\\common\" /I \"%s\\graphics\" \"%s\\imaging\" \"%s\\contrib\" \"%s\\pcmaker\\xlib\" /D \"_DEBUG\" /D \"WIN32\" /D\\\n",
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
  fprintf(fp,"    \"$(INTDIR)\\CommonInit.obj\" \\\n");
  fprintf(fp,"    \"$(INTDIR)\\vtkTclUtil.obj\" \\\n");
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

  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"###############################################################################\n");
    fprintf(fp,"# Begin Source File\n");
    fprintf(fp,"\n");
    fprintf(fp,"SOURCE=%s\\%s\\%s.cxx\n",vtkHome,abstract_lib[i],abstract[i]);
    fprintf(fp,"\n");
    fprintf(fp,"# Begin Custom Build\n");
    fprintf(fp,"ProjDir=.\n");
    fprintf(fp,"InputPath=%s\\%s\\%s.h \n",vtkHome, 

"$(ProjDir)\src\vtkObjectTcl.cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   f:\vtk\pcmaker\cpp_parse\Debug\cpp_parse f:\vtk\common\vtkObject.h\
  f:\vtk\tcl\hints 1 > $(ProjDir)\src\vtkObjectTcl.cxx

# End Custom Build
    fprintf(fp,"\"$(INTDIR)\\%s.obj\" : $(SOURCE) \"$(INTDIR)\"\n",abstract[i]);
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

void makeMakefile(const char *vtkHome, const char *vtkBuild,
                  int useMS)
{
  char fname[256];
  FILE *ofp;
    
  sprintf(fname,"%s\\common\\Makefile.in",vtkHome);
  readInMakefile(fname,strdup("common"));
  sprintf(fname,"%s/graphics/Makefile.in",vtkHome);
  readInMakefile(fname,strdup("graphics"));
  sprintf(fname,"%s/imaging/Makefile.in",vtkHome);
  readInMakefile(fname,strdup("imaging"));
  sprintf(fname,"%s/contrib/Makefile.in",vtkHome);
  readInMakefile(fname,strdup("contrib"));

  //remove any UNIX only stuff
  removeUNIXOnlyFiles();

  // add in the OpenGL stuff etc that we need
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
