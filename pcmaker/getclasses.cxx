#include "stdafx.h"
#include "pcmaker.h"
#include "pcmakerDlg.h"
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
  
  // prototype helper function
  if (anindex > 200) fprintf(fp,"int vtk%sNewInstanceHelper(ClientData cd, Tcl_Interp *interp,\n                      	   int argc, char *argv[]);\n\n",kitName);
  /* define the vtkNewInstanceCommand */
  fprintf(fp,"int vtk%sNewInstanceCommand(ClientData cd, Tcl_Interp *interp,\n                         int argc, char *argv[])\n{\n",kitName);
  fprintf(fp,"  Tcl_HashEntry *entry;\n  int is_new;\n  char temps[80];\n");
  fprintf(fp,"  cd = 0; /* just prevents compiler warnings */\n");

  fprintf(fp,"\n  if (argc != 2)\n    {\n    interp->result = \"vtk object creation requires one argument, a name.\";\n    return TCL_ERROR;\n    }\n\n");
  fprintf(fp,"  if ((argv[1][0] >= '0')&&(argv[1][0] <= '9'))\n    {\n    interp->result = \"vtk object names must start with a letter.\";\n    return TCL_ERROR;\n    }\n\n");
  fprintf(fp,"  if (Tcl_FindHashEntry(&vtkInstanceLookup,argv[1]))\n    {\n    interp->result = \"a vtk object with that name already exists.\";\n    return TCL_ERROR;\n    }\n\n");

  // we have to break this function into two because it is too large for some compilers
  if (anindex > 200)
    {
    for (i = 0; i < 200; i++)
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
    // call the helper function
	fprintf(fp,"  if (vtk%sNewInstanceHelper(cd,interp,argc,argv) == TCL_OK) return TCL_OK;\n",kitName);
    fprintf(fp,"  sprintf(interp->result,\"%%s\",argv[1]);\n  return TCL_OK;\n}\n");
    fprintf(fp,"int vtk%sNewInstanceHelper(ClientData cd, Tcl_Interp *interp,\n                         int argc, char *argv[])\n{\n",kitName);
    fprintf(fp,"  Tcl_HashEntry *entry;\n  int is_new;\n  char temps[80];\n");
    fprintf(fp,"  cd = 0; /* just prevents compiler warnings */\n");

    for ( ; i < anindex; i++)
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
	fprintf(fp,"  return TCL_ERROR;\n}\n");
    } 
  else
    {
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
    fprintf(fp,"  sprintf(interp->result,\"%%s\",argv[1]);\n  return TCL_OK;\n}\n");
    }

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

void MakeForce(char *fname)
{
  int i;
  FILE *fp;

  fp = fopen(fname,"w");
  if (fp)
  {
 	  for (i = 0; i < num_abstract_h; i++)
	  {
		fprintf(fp,"#include \"%s.h\"\n",abstract_h[i]);
    }
 	  for (i = 0; i < num_concrete_h; i++)
	  {
		fprintf(fp,"#include \"%s.h\"\n",concrete_h[i]);
    }  
  fclose(fp);
  }
}

void doHeader(FILE *fp, const char *vtkHome,
	      const char *vtkBuild, const char *vtkCompiler, int Debug, int doPatented);
void doTclHeader(FILE *fp, const char *vtkHome,
		 const char *vtkBuild, const char *vtkCompiler, int Debug,
     int doAddedValue, int doPatented);


void makeMakefile(CPcmakerDlg *vals)
{
  char fname[256];
  FILE *ofp;
  int doAddedValue = 0;
  
  sprintf(fname,"%s\\common\\Makefile.in",vals->m_WhereVTK);
  readInMakefile(fname,strdup("common"));
  if (vals->m_Graphics)
  {
    sprintf(fname,"%s\\graphics\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("graphics"));
  }
  if (vals->m_Imaging)
  {
    sprintf(fname,"%s\\imaging\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("imaging"));
  }
  if (vals->m_Contrib)
  {
    sprintf(fname,"%s\\contrib\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("contrib"));
  }
  if (vals->m_Patented)
  {
    sprintf(fname,"%s\\patented\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("patented"));
  }
  if (vals->m_GEMSIO)
  {
    doAddedValue = 1;
    sprintf(fname,"%s\\gemsio\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("gemsio"));
  }
  if (vals->m_GEMSIP)
  {
    doAddedValue = 1;
    sprintf(fname,"%s\\gemsip\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("gemsip"));
  }
  if (vals->m_GEMSVOLUME)
  {
    doAddedValue = 1;
    sprintf(fname,"%s\\gemsvolume\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("gemsvolume"));
  }

  //remove any UNIX only stuff
  removeUNIXOnlyFiles();

  // add in the OpenGL stuff etc that we need
  if (vals->m_Graphics)
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
  sprintf(fname,"%s\\vtktcl\\src\\vtktcl.cxx",vals->m_WhereBuild);
  MakeInit(fname,"Vtktcl");

  // we must create vtkPCForce.cxx
  sprintf(fname,"%s\\vtkdll\\vtkPCForce.cxx",vals->m_WhereBuild);
  MakeForce(fname);

  // spit out a Makefile
  sprintf(fname,"%s\\vtkdll\\makefile",vals->m_WhereBuild);
  ofp = fopen(fname,"w");
  if (vals->m_MSComp) doHeader(ofp, vals->m_WhereVTK, vals->m_WhereBuild, vals->m_WhereCompiler,
    vals->m_Debug,vals->m_Patented);
  fclose(ofp);
  
  sprintf(fname,"%s\\vtktcl\\makefile",vals->m_WhereBuild);
  ofp = fopen(fname,"w");
  if (vals->m_MSComp) doTclHeader(ofp, vals->m_WhereVTK, vals->m_WhereBuild, vals->m_WhereCompiler,
    vals->m_Debug, doAddedValue,vals->m_Patented);
  fclose(ofp);
  
}



/******************************************************************************
  Here are the different makefile methods
*******************************************************************************/
void doHeader(FILE *fp, const char *vtkHome,
              const char *vtkBuild, const char *vtkCompiler, int Debug,
              int doPatented)
{
  int i;

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"OUTDIR=obj\n\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtkdll.dll\"\n\n");

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");
  if (Debug)
    {
    fprintf(fp,"CPP_PROJ=/nologo /D \"_DEBUG\" /MTd /GX /Od /Zi /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vtkCompiler, vtkCompiler, vtkHome, vtkHome);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/nologo /MT /GX /O2 /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vtkCompiler, vtkCompiler, vtkHome, vtkHome);
    }
  if (doPatented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n",
      vtkHome);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n");
    }
  fprintf(fp," /Fp\"$(OUTDIR)/vtkdll.pch\" /YX /Fo\"$(OUTDIR)/\" /c \n");
  fprintf(fp,"LINK32=link.exe\n");
  if (Debug)
    {
    fprintf(fp,"LINK32_FLAGS=/debug /libpath:%s\\mfc\\lib /libpath:%s\\lib %s\\lib\\opengl32.lib %s\\lib\\glaux.lib /nologo /version:1.3 /subsystem:windows\\\n",
    vtkCompiler, vtkCompiler, vtkCompiler, vtkCompiler);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=/libpath:%s\\mfc\\lib /libpath:%s\\lib %s\\lib\\opengl32.lib %s\\lib\\glaux.lib /nologo /version:1.3 /subsystem:windows\\\n",
    vtkCompiler, vtkCompiler, vtkCompiler, vtkCompiler);
    }
  fprintf(fp," /dll /incremental:no /machine:I386\\\n");
  fprintf(fp," /out:\"$(OUTDIR)/vtkdll.dll\" /implib:\"$(OUTDIR)/vtkdll.lib\" \n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\StdAfx.obj\" \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtkdll.obj\" \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtkPCForce.obj\" \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%s.obj\" \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%s.obj\" \\\n",concrete[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkdll.dll\" : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"<<\n");
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
  fprintf(fp,"\n");
  fprintf(fp,"BuildCmds= \\\n");
  if (Debug)
    {
    fprintf(fp,"	$(CPP) /D \"_DEBUG\" /nologo /MTd /GX /Od /Zi /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\"\\\n",
      vtkCompiler, vtkCompiler, vtkHome,vtkHome);
    }
  else
    {
    fprintf(fp,"	$(CPP) /nologo /MT /GX /O2 /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\"\\\n",
      vtkCompiler, vtkCompiler, vtkHome,vtkHome);
    }
  fprintf(fp," /D \"_WINDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\"\\\n");
  fprintf(fp," /Fp\"$(OUTDIR)/vtkdll.pch\" /Yc\"stdafx.h\" /Fo\"$(OUTDIR)/\" /c %s\\vtkdll\\StdAfx.cpp \\\n",
	  vtkHome);
  fprintf(fp,"	\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\StdAfx.obj\" : %s\\vtkdll\\StdAfx.cpp \"$(OUTDIR)\"\n",
	  vtkHome);
  fprintf(fp,"   $(BuildCmds)\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkPCForce.obj\" : vtkPCForce.cxx \"$(OUTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) vtkPCForce.cxx\n\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkdll.obj\" : %s\\vtkdll\\vtkdll.cpp \"$(OUTDIR)\"\n",
	    vtkHome);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) %s\\vtkdll\\vtkdll.cpp\n\n",vtkHome);

  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"\"$(OUTDIR)\\%s.obj\" : %s\\%s\\%s.cxx \"$(OUTDIR)\"\n",
	    abstract[i],vtkHome,abstract_lib[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) %s\\%s\\%s.cxx\n\n",
		vtkHome,abstract_lib[i],abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"\"$(OUTDIR)\\%s.obj\" : %s\\%s\\%s.cxx \"$(OUTDIR)\"\n",
	    concrete[i],vtkHome,concrete_lib[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) %s\\%s\\%s.cxx\n\n",
	    vtkHome,concrete_lib[i],concrete[i]);
  }
  fprintf(fp,"################################################################################\n");
}

void doTclHeader(FILE *fp, const char *vtkHome,
		   const char *vtkBuild, const char *vtkCompiler, int Debug,
       int doAddedValue, int doPatented)
{
  int i;

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"OUTDIR=obj\n\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtktcl.dll\"\n\n");

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");

  if (Debug)
    {
    fprintf(fp,"CPP_PROJ=/D \"_DEBUG\" /nologo /MTd /GX /Od /Zi /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vtkCompiler, vtkCompiler, vtkHome, vtkHome, vtkHome, vtkHome, vtkHome);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/nologo /MT /GX /O2 /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vtkCompiler, vtkCompiler, vtkHome, vtkHome, vtkHome, vtkHome, vtkHome);
    }
  if (doPatented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" \\\n",
      vtkHome);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" \\\n");
    }
  if (doAddedValue) fprintf(fp," /I \"%s\\gemsio\" /I \"%s\\gemsip\" /I \"%s\\gemsvolume\" \\\n",
    vtkHome, vtkHome, vtkHome);
  fprintf(fp," /Fp\"$(OUTDIR)/vtktcl.pch\" /YX /Fo\"$(OUTDIR)/\" /c \n");
  fprintf(fp,"LINK32=link.exe\n");
  if (Debug)
    {
    fprintf(fp,"LINK32_FLAGS=/debug /libpath:%s\\mfc\\lib /libpath:%s\\lib nafxcwd.lib ..\\vtkdll\\obj\\vtkdll.lib %s\\pcmaker\\tk42.lib %s\\pcmaker\\tcl76.lib /nologo /version:1.3 /subsystem:windows\\\n",
	    vtkCompiler, vtkCompiler, vtkHome, vtkHome);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=/libpath:%s\\mfc\\lib /libpath:%s\\lib nafxcw.lib ..\\vtkdll\\obj\\vtkdll.lib %s\\pcmaker\\tk42.lib %s\\pcmaker\\tcl76.lib /nologo /version:1.3 /subsystem:windows\\\n",
	    vtkCompiler, vtkCompiler, vtkHome, vtkHome);
    }
  fprintf(fp," /dll /incremental:no /pdb:\"$(OUTDIR)/vtktcl.pdb\" /machine:I386\\\n");
  fprintf(fp," /out:\"$(OUTDIR)/vtktcl.dll\" /implib:\"$(OUTDIR)/vtktcl.lib\" \n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtkTclUtil.obj\" \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtktcl.obj\" \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",concrete[i]);
  }
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",abstract_h[i]);
  }
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",concrete_h[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtktcl.dll\" : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"<<\n");
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
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkTclUtil.obj\" : %s\\common\\vtkTclUtil.cxx \"$(OUTDIR)\"\n",
	  vtkHome);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) %s\\common\\vtkTclUtil.cxx\n\n",vtkHome);
  fprintf(fp,"\"$(OUTDIR)\\vtktcl.obj\" : src\\vtktcl.cxx \"$(OUTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtktcl.cxx\n\n");

  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"\"src\\%sTcl.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		abstract[i],vtkHome,abstract_lib[i],abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > src\\%sTcl.cxx\n\n",
		vtkHome, abstract[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx \"$(OUTDIR)\"\n",
		abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstract[i]);
  }
  
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"\"src\\%sTcl.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		concrete[i],vtkHome,concrete_lib[i],concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > src\\%sTcl.cxx\n\n",
		vtkHome, concrete[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx \"$(OUTDIR)\"\n",
		concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concrete[i]);
  }
  
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"\"src\\%sTcl.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		abstract_h[i],vtkHome,abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > src\\%sTcl.cxx\n\n",
		vtkHome, abstract_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx \"$(OUTDIR)\"\n",
		abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstract_h[i]);
  }
  
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"\"src\\%sTcl.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		concrete_h[i],vtkHome,concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vtkHome, vtkHome, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > src\\%sTcl.cxx\n\n",
		vtkHome, concrete_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx \"$(OUTDIR)\"\n",
		concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concrete_h[i]);
  }
  
  fprintf(fp,"################################################################################\n");
}

