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
void stuffit(FILE *fp, CPcmakerDlg *vals)
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

  /* prototype for tkRenderWidget */
  if (vals->m_Graphics) fprintf(fp,"extern \"C\" {int Vtktkrenderwidget_Init(Tcl_Interp *interp);}\n\n");
  
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
    /* initialize the tkRenderWidget */
    if (vals->m_Graphics) fprintf(fp,"  Vtktkrenderwidget_Init(interp);\n");
  }
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(fp,"  Tcl_CreateCommand(interp,\"%s\",vtk%sNewInstanceCommand,\n		    (ClientData *)NULL,\n		    (Tcl_CmdDeleteProc *)NULL);\n\n",
	    names[i],kitName);
    }

  fprintf(fp,"  return TCL_OK;\n}\n");
}


void MakeInit(char *fname, char *argv1, CPcmakerDlg *vals)
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
  stuffit(fp,vals);
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
     fprintf(fp,"#ifndef RW_STD_IOSTREAM\n");
     fprintf(fp,"#define RW_STD_IOSTREAM\n");
     fprintf(fp,"#include <string.h>\n");
     fprintf(fp,"#undef RW_STD_IOSTREAM\n");
     fprintf(fp,"#else\n");
     fprintf(fp,"#include <string.h>\n");
     fprintf(fp,"#endif\n");
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

void doMSCHeader(FILE *fp, CPcmakerDlg *vals, int doAdded);
void doBorHeader(FILE *fp, CPcmakerDlg *vals, int doAdded);
void doMSCTclHeader(FILE *fp, CPcmakerDlg *vals, int doAdded);
void doBorTclHeader(FILE *fp, CPcmakerDlg *vals, int doAdded);
void doMSCJavaHeader(FILE *fp, CPcmakerDlg *vals, int doAdded);
void doBorJavaHeader(FILE *fp, CPcmakerDlg *vals, int doAdded);

// generate depend info for a .cxx file
// outputs the result as DEPEND
extern void OutputDepends(char *file, FILE *fp, const char *vtkHome);

void makeMakefile(CPcmakerDlg *vals)
{
  char fname[256];
  FILE *ofp;
  int doAddedValue = 0;
  int total;
  
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
    sprintf(fname,"%s\\volume\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("volume"));
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

  if (vals->m_GEMSVOLUME)
  {
    concrete[num_concrete] = strdup("vtkOglrPolyDepthMapper");
    concrete_lib[num_concrete] = strdup("volume");
    num_concrete++;
  }
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
  MakeInit(fname,"Vtktcl",vals);

  // we must create vtkPCForce.cxx
  sprintf(fname,"%s\\vtkdll\\vtkPCForce.cxx",vals->m_WhereBuild);
  MakeForce(fname);

  // set up the progress indicator
  total = 1 + 2*num_concrete + 2*num_abstract + num_abstract_h + num_concrete_h;
  if (strlen(vals->m_WhereJDK) > 1)
    {
    total = total + num_concrete + num_abstract + num_abstract_h + num_concrete_h;
    } 
  vals->m_Progress.SetRange(0,total);
  vals->m_Progress.SetPos(0);

  // spit out a Makefile
  sprintf(fname,"%s\\vtkdll\\makefile",vals->m_WhereBuild);
  ofp = fopen(fname,"w");
  if (vals->m_MSComp) doMSCHeader(ofp, vals, doAddedValue);
  if (vals->m_BorlandComp) doBorHeader(ofp, vals, doAddedValue);
  fclose(ofp);

  sprintf(fname,"%s\\vtktcl\\makefile",vals->m_WhereBuild);
  ofp = fopen(fname,"w");
  if (vals->m_MSComp) doMSCTclHeader(ofp, vals, doAddedValue);
  if (vals->m_BorlandComp) doBorTclHeader(ofp, vals, doAddedValue);
  fclose(ofp);

  // generate the java makefiles if requested
  if (strlen(vals->m_WhereJDK) > 1)
    {
    sprintf(fname,"%s\\vtkjava\\makefile",vals->m_WhereBuild);
    ofp = fopen(fname,"w");
    if (vals->m_MSComp ) doMSCJavaHeader(ofp, vals, doAddedValue);
    if (vals->m_BorlandComp) doBorJavaHeader(ofp, vals, doAddedValue);
    fclose(ofp);
    }
}



/******************************************************************************
  Here are the different makefile methods
*******************************************************************************/

void doMSCHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue)
{
  int i;
  char file[256];

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"OUTDIR=obj\n\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtkdll.dll\"\n\n");

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");
  if (vals->m_Debug)
    {
    fprintf(fp,"CPP_PROJ=/nologo /D \"_DEBUG\" /MTd /GX /Od /Zi /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\imaging\" /I \"%s\\graphics\" /I \"%s\\volume\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/nologo /MT /G5 /Ox /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\volume\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  if (vals->m_Patented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n",
      vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n");
    }
  fprintf(fp," /Fo\"$(OUTDIR)/\" /c \n");
  fprintf(fp,"LINK32=link.exe\n");
  if (vals->m_Debug)
    {
    fprintf(fp,"LINK32_FLAGS=/debug /libpath:\"%s\\mfc\\lib\" /libpath:\"%s\\lib\" \"%s\\lib\\opengl32.lib\" \"%s\\lib\\glaux.lib\" /nologo /version:1.3 /subsystem:windows\\\n",
    vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=/libpath:\"%s\\mfc\\lib\" /libpath:\"%s\\lib\" \"%s\\lib\\opengl32.lib\" \"%s\\lib\\glaux.lib\" /nologo /version:1.3 /subsystem:windows\\\n",
    vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler);
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
  if (vals->m_Debug)
    {
    fprintf(fp,"	$(CPP) /D \"_DEBUG\" /nologo /MTd /GX /Od /Zi /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\"\\\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK,vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"	$(CPP) /nologo /MT /GX /O2 /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\"\\\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK,vals->m_WhereVTK);
    }
  fprintf(fp," /D \"_WINDLL\" /D \"_MBCS\" /D \"_USRDLL\" /D \"VTKDLL\"\\\n");
  fprintf(fp," /Fo\"$(OUTDIR)/\" /c %s\\vtkdll\\StdAfx.cpp \\\n",
	  vals->m_WhereVTK);
  fprintf(fp,"	\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\StdAfx.obj\" : %s\\vtkdll\\StdAfx.cpp \"$(OUTDIR)\"\n",
	  vals->m_WhereVTK);
  fprintf(fp,"   $(BuildCmds)\n");
  fprintf(fp,"\n");
  sprintf(file,"%s\\vtkdll\\vtkPCForce.cxx",vals->m_WhereBuild);
  OutputDepends(file,fp,vals->m_WhereVTK);
  vals->m_Progress.OffsetPos(1);
  fprintf(fp,"\"$(OUTDIR)\\vtkPCForce.obj\" : vtkPCForce.cxx $(DEPENDS) \"$(OUTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) vtkPCForce.cxx\n\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkdll.obj\" : %s\\vtkdll\\vtkdll.cpp \"$(OUTDIR)\"\n",
	    vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) %s\\vtkdll\\vtkdll.cpp\n\n",vals->m_WhereVTK);

  for (i = 0; i < num_abstract; i++)
  {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OUTDIR)\\%s.obj\" : %s\\%s\\%s.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) %s\\%s\\%s.cxx\n\n",
		vals->m_WhereVTK,abstract_lib[i],abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OUTDIR)\\%s.obj\" : %s\\%s\\%s.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) %s\\%s\\%s.cxx\n\n",
	    vals->m_WhereVTK,concrete_lib[i],concrete[i]);
  }
  fprintf(fp,"################################################################################\n");
}

void doBorHeader(FILE *fp, CPcmakerDlg *vals, int doAddedValue)
{
  int i;

  fprintf(fp,"# VTK Borland makefile\n");
  fprintf(fp,"OUTDIR=%s\\vtkdll\\obj\n\n",vals->m_WhereBuild);
  fprintf(fp,"CPP=BCC32.exe +CPP_PROJ.CFG\n\n");
  fprintf(fp,"ALL : vtkdll.dll\n\n");

  fprintf(fp,"\"obj\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"obj\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"  echo Make Config\n");
    fprintf(fp,"    Copy &&| \n");

  if (vals->m_Debug)
    {

    fprintf(fp,"-D_DEBUG -v -R \n");
    }
  else
    {
 fprintf(fp,"-v- -R- \n");
       }
  if (vals->m_Patented)
    {
    fprintf(fp,"-DUSE_PATENTED -I%s\\patented\n",
      vals->m_WhereVTK);
    }
 fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL\n",
      vals->m_WhereVTK);
 fprintf(fp,"-tWM -tWD -Od -H- -VF -I%s\\include\\mfc;%s\\include;%s\\common;%s\\graphics;%s\\volume -DWIN32\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
 fprintf(fp," -I%s\\imaging \n",vals->m_WhereVTK);
 fprintf(fp," -I%s\\contrib \n",vals->m_WhereVTK);
  fprintf(fp,"-P -c -w-hid -w-inl \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n");
  fprintf(fp,"LINK32=tlink32.exe\n\n");

  fprintf(fp,"LINK32_FLAGS=-L%s\\lib \\\n",
  vals->m_WhereCompiler);
  if (vals->m_Debug)
    {
    fprintf(fp,"  -v \\\n");
    }
   else
    {
    fprintf(fp,"  -v- \\\n");
    }
  fprintf(fp,"  -Tpd -aa -V4.0 -Gm  -w-inq -m -n\n");
  fprintf(fp,"DEPLINK32_OBJS= \\\n");
  fprintf(fp,"    obj\\StdAfx.obj \\\n");
  fprintf(fp,"    obj\\vtkdll.obj \\\n");
  fprintf(fp,"    obj\\vtkPCForce.obj");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp," \\\n    obj\\%s.obj",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp," \\\n    obj\\%s.obj",concrete[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    %s\\lib\\c0d32.obj+ \\\n",vals->m_WhereCompiler);
  fprintf(fp,"    obj\\StdAfx.obj+ \\\n");
  fprintf(fp,"    obj\\vtkdll.obj+ \\\n");
  fprintf(fp,"    obj\\vtkPCForce.obj");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%s.obj",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%s.obj",concrete[i]);
  }
    fprintf(fp,", \\\n $<,$*,  \\\n");   // this is the target and map file name

    fprintf(fp,"    %s\\lib\\bfc40.lib+ \\\n",vals->m_WhereCompiler);
    fprintf(fp,"    %s\\lib\\bfcs40.lib+ \\\n",vals->m_WhereCompiler);
    fprintf(fp,"    %s\\lib\\import32.lib+ \\\n",vals->m_WhereCompiler);
     fprintf(fp,"   %s\\lib\\cw32.lib+ \\\n",vals->m_WhereCompiler);
    fprintf(fp,"    %s\\lib\\cw32mt.lib+ \\\n",vals->m_WhereCompiler);
    fprintf(fp,"    %s\\lib\\gl\\glaux.lib \\\n",vals->m_WhereCompiler);
 fprintf(fp,"\n");
  fprintf(fp,"vtkdll.dll : \"obj\" $(DEF_FILE) $(DEPLINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @&&|\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"| $@\n");
  fprintf(fp,"obj\\vtkdll.lib : vtkdll.dll \n");
  fprintf(fp,"      implib $@ vtkdll.dll \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"\n");

  fprintf(fp,"obj\\StdAfx.obj : %s\\vtkdll\\StdAfx.cpp \n",
	  vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) @&&| \n");
  fprintf(fp,"-I%s\\vtkdll -oobj\\StdAfx.obj %s\\vtkdll\\StdAfx.cpp \n",vals->m_WhereVTK,vals->m_WhereVTK);
  fprintf(fp,"|  \n");
  fprintf(fp,"obj\\vtkPCForce.obj : vtkPCForce.cxx \n");
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-oobj\\vtkPCForce.obj vtkPCForce.cxx \n\n");
  fprintf(fp,"|  \n");
  fprintf(fp,"obj\\vtkdll.obj : %s\\vtkdll\\vtkdll.cpp \n",
	    vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-I%s\\vtkdll -oobj\\vtkdll.obj %s\\vtkdll\\vtkdll.cpp \n\n",vals->m_WhereVTK,vals->m_WhereVTK);
  fprintf(fp,"|  \n");

  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"obj\\%s.obj : %s\\%s\\%s.cxx \n",
	    abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
  fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-oobj\\%s.obj %s\\%s\\%s.cxx \n\n",
		abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
  fprintf(fp,"|  \n");
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"obj\\%s.obj : %s\\%s\\%s.cxx \n",
	    concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
  fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-oobj\\%s.obj %s\\%s\\%s.cxx \n\n",
	    concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
  fprintf(fp,"|  \n");
  }

  fprintf(fp,"################################################################################\n");
}

void doMSCTclHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue)
{
  int i;
  char file [256];

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"OUTDIR=obj\n\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtktcl.dll\"\n\n");

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");

  if (vals->m_Debug)
    {
    fprintf(fp,"CPP_PROJ=/D \"_DEBUG\" /nologo /MTd /GX /Od /Zi /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\"  /I \"%s\\volume\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/nologo /MT /GX /O2 /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\"  /I \"%s\\volume\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  if (vals->m_Patented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" \\\n",
      vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" \\\n");
    }
  if (doAddedValue) fprintf(fp," /I \"%s\\gemsio\" /I \"%s\\gemsip\" /I \"%s\\gemsvolume\" /I \"%s\\volume\" \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  fprintf(fp," /Fo\"$(OUTDIR)/\" /c \n");
  fprintf(fp,"LINK32=link.exe\n");
  if (vals->m_Debug)
    {
    fprintf(fp,"LINK32_FLAGS=\"$(OUTDIR)\\vtktcldll.obj\" \"$(OUTDIR)\\vtktcl.obj\" \"$(OUTDIR)\\vtktclobjs.lib\" /debug /libpath:\"%s\\mfc\\lib\" /libpath:\"%s\\lib\" nafxcwd.lib ..\\vtkdll\\obj\\vtkdll.lib \"%s\\pcmaker\\tk42.lib\" \"%s\\pcmaker\\tcl76.lib\" /nologo /version:1.3 /subsystem:windows\\\n",
	    vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=\"$(OUTDIR)\\vtktcldll.obj\" \"$(OUTDIR)\\vtktcl.obj\" \"$(OUTDIR)\\vtktclobjs.lib\" /libpath:\"%s\\mfc\\lib\" /libpath:\"%s\\lib\" nafxcw.lib ..\\vtkdll\\obj\\vtkdll.lib \"%s\\pcmaker\\tk42.lib\" \"%s\\pcmaker\\tcl76.lib\" /nologo /version:1.3 /subsystem:windows\\\n",
	    vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  fprintf(fp," /dll /incremental:no /pdb:\"$(OUTDIR)/vtktcl.pdb\" /machine:I386\\\n");
  fprintf(fp," /out:\"$(OUTDIR)/vtktcl.dll\" /implib:\"$(OUTDIR)/vtktcl.lib\" \n"); 
  fprintf(fp,"LIB_FLAGS=/out:\"$(OUTDIR)/vtktclobjs.lib\" /machine:I386\n\n"); 

  
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtkTclUtil.obj\" \\\n");
  if (vals->m_Graphics) fprintf(fp,"    \"$(OUTDIR)\\vtkTkRenderWidget.obj\" \\\n");
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
  fprintf(fp,"\"$(OUTDIR)\\vtktcl.dll\" : \"$(OUTDIR)\" $(DEF_FILE) \"$(OUTDIR)\\vtktcldll.obj\" \"$(OUTDIR)\\vtktcl.obj\" \"$(OUTDIR)\\vtktclobjs.lib\" \n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS)\n");
  fprintf(fp,"<<\n\n");
  fprintf(fp,"\"$(OUTDIR)\\vtktclobjs.lib\" : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS) \n");
  fprintf(fp,"    lib.exe @<<\n");
  fprintf(fp,"  $(LIB_FLAGS) $(LINK32_OBJS)\n");
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
	  vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\common\\vtkTclUtil.cxx\"\n\n",vals->m_WhereVTK);
  if (vals->m_Graphics)
    {
    sprintf(file,"%s\\graphics\\vtkTkRenderWidget.cxx",vals->m_WhereVTK);
    OutputDepends(file,fp,vals->m_WhereVTK);
    fprintf(fp,"\"$(OUTDIR)\\vtkTkRenderWidget.obj\" : %s\\graphics\\vtkTkRenderWidget.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    vals->m_WhereVTK);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\graphics\\vtkTkRenderWidget.cxx\"\n\n",vals->m_WhereVTK);
    }
  fprintf(fp,"\"$(OUTDIR)\\vtktcl.obj\" : src\\vtktcl.cxx \"$(OUTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtktcl.cxx\n\n");
  fprintf(fp,"\"$(OUTDIR)\\vtktcldll.obj\" : \"%s\\vtkdll\\vtktcldll.cpp\" \"$(OUTDIR)\"\n",vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\vtkdll\\vtktcldll.cpp\"\n\n",vals->m_WhereVTK);

  for (i = 0; i < num_abstract; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"$(OUTDIR)\"\n",
		abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstract[i]);
  }

  for (i = 0; i < num_concrete; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concrete[i]);
  }

  for (i = 0; i < num_abstract_h; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		abstract_h[i],vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstract_h[i]);
  }

  for (i = 0; i < num_concrete_h; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		concrete_h[i],vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concrete_h[i]);
  }

  fprintf(fp,"################################################################################\n");
}

void doBorTclHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue)
{
  int i;
  fprintf(fp,"# VTK Borland makefile\n");
  fprintf(fp,"OUTDIR=%s\\vtktcl\\obj\n\n",vals->m_WhereBuild);
  fprintf(fp,"CPP=BCC32.exe +CPP_PROJ.CFG\n\n");
  fprintf(fp,"ALL : vtktcl.dll\n\n");

  fprintf(fp,"obj ::\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"obj\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"  echo Make Config\n");
    fprintf(fp,"    Copy &&| \n");

  if (vals->m_Debug)
    {

    fprintf(fp,"-D_DEBUG -v -R \n");
    }
  else
    {
 fprintf(fp,"-v- -R- \n");
       }
  if (vals->m_Patented)
    {
    fprintf(fp,"-DUSE_PATENTED -I%s\\patented\n",
      vals->m_WhereVTK);
    }
 fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL\n",
      vals->m_WhereVTK);
 fprintf(fp,"-tWM -tWD -Od -H- -VF -I%s\\include\\mfc;%s\\include;%s\\common;%s\\graphics;%s\\volume -DWIN32\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
 fprintf(fp," -I%s\\pcmaker\\xlib \n",vals->m_WhereVTK);
 fprintf(fp," -I%s\\imaging \n",vals->m_WhereVTK);
 fprintf(fp," -I%s\\contrib \n",vals->m_WhereVTK);
  if (doAddedValue) fprintf(fp," -I%s\\gemsio -I%s\\gemsip -I%s\\gemsvolume -I%s\\volume \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  fprintf(fp,"-P -c -w-hid -w-inl \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n");
  fprintf(fp,"LINK32=tlink32.exe\n\n");

  fprintf(fp,"LINK32_FLAGS=-L%s\\lib;..\\vtkdll\\obj\\vtkdll.lib;%s\\pcmaker\\tk42.lib;%s\\pcmaker\\tcl76.lib \\\n",
  vals->m_WhereCompiler,vals->m_WhereCompiler,vals->m_WhereCompiler);
  if (vals->m_Debug)
    {
    fprintf(fp,"  -v \\\n");
    }
   else
    {
    fprintf(fp,"  -v- \\\n");
    }
  fprintf(fp,"  -Tpd -aa -V4.0 -Gm  -w-inq -m -n\n");
  fprintf(fp,"DEPLINK32_OBJS= \\\n");
  fprintf(fp,"    obj\\vtkTclUtil.obj \\\n");
  fprintf(fp,"    obj\\vtktcl.obj \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    obj\\%sTcl.obj \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    obj\\%sTcl.obj \\\n",concrete[i]);
  }
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"    obj\\%sTcl.obj \\\n",abstract_h[i]);
  }
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"    obj\\%sTcl.obj \\\n",concrete_h[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    obj\\vtkTclUtil.obj+ \\\n");
  fprintf(fp,"    obj\\vtktcl.obj");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%sTcl.obj",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%sTcl.obj",concrete[i]);
  }
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%sTcl.obj",abstract_h[i]);
  }
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%sTcl.obj",concrete_h[i]);
  }
  fprintf(fp," \n");
  fprintf(fp,"vtktcl.dll : obj $(DEF_FILE) $(DEPLINK32_OBJS) obj\n");
  fprintf(fp,"    $(LINK32) @&&|\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"|\n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"\n");
  fprintf(fp,"obj\\vtkTclUtil.obj : %s\\common\\vtkTclUtil.cxx \n",
	  vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-oobj\\vtkTclUtil.obj  %s\\common\\vtkTclUtil.cxx\n\n",vals->m_WhereVTK,vals->m_WhereVTK);
   fprintf(fp,"|  \n");
  fprintf(fp,"obj\\vtktcl.obj : src\\vtktcl.cxx \n");
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-oobj\\vtktcl.obj  src\\vtktcl.cxx\n\n");
   fprintf(fp,"|  \n");

  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"src\\%sTcl.cxx : %s\\%s\\%s.h \n",
		abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"obj\\%sTcl.obj : src\\%sTcl.cxx \n",
		abstract[i],abstract[i]);
  fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-oobj\\%sTcl.obj src\\%sTcl.cxx\n\n",abstract[i],abstract[i]);
   fprintf(fp,"|  \n");
  }

  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"src\\%sTcl.cxx : %s\\%s\\%s.h \n",
		concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"obj\\%sTcl.obj : src\\%sTcl.cxx \n",
		concrete[i],concrete[i]);
  fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-oobj\\%sTcl.obj src\\%sTcl.cxx\n\n",concrete[i],concrete[i]);
   fprintf(fp,"|  \n");
  }

  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"src\\%sTcl.cxx : %s\\%s\\%s.h \n",
		abstract_h[i],vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 0 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"obj\\%sTcl.obj : src\\%sTcl.cxx \n",
		abstract_h[i],abstract_h[i]);
  fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-oobj\\%sTcl.obj src\\%sTcl.cxx\n\n",abstract_h[i],abstract_h[i]);
   fprintf(fp,"|  \n");
  }

  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"src\\%sTcl.cxx : %s\\%s\\%s.h \n",
		concrete_h[i],vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\cpp_parse\\Debug\\cpp_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints 1 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"obj\\%sTcl.obj : src\\%sTcl.cxx \n",
		concrete_h[i],concrete_h[i]);
  fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-oobj\\%sTcl.obj src\\%sTcl.cxx\n\n",concrete_h[i],concrete_h[i]);
   fprintf(fp,"|  \n");
  }

  fprintf(fp,"################################################################################\n");
}

void doMSCJavaHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue)
{
  int i;
  char file[256];

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"OUTDIR=obj\n\n");
  fprintf(fp,"ALL : \"$(OUTDIR)\\vtkjava.dll\"\n\n");

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");

  if (vals->m_Debug)
    {
    fprintf(fp,"CPP_PROJ=/D \"_DEBUG\" /nologo /MTd /GX /Od /Zi /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\"  /I \"%s\\volume\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/nologo /MT /GX /O2 /I \"%s\\mfc\\include\" /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\"  /I \"%s\\volume\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  if (vals->m_Patented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" \\\n",
      vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_USRDLL\" /D \"_MBCS\" \\\n");
    }
  if (doAddedValue) fprintf(fp," /I \"%s\\gemsio\" /I \"%s\\gemsip\" /I \"%s\\gemsvolume\" /I \"%s\\volume\" \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  fprintf(fp,"/I \"%s\\include\" /I \"%s\\include\\win32\" /Fo\"$(OUTDIR)/\" /c \n",
    vals->m_WhereJDK, vals->m_WhereJDK);
  fprintf(fp,"LINK32=link.exe\n");
  if (vals->m_Debug)
    {
    fprintf(fp,"LINK32_FLAGS=/debug /libpath:\"%s\\mfc\\lib\" /libpath:\"%s\\lib\" nafxcwd.lib ..\\vtkdll\\obj\\vtkdll.lib /nologo /version:1.3 /subsystem:windows\\\n",
	    vals->m_WhereCompiler, vals->m_WhereCompiler);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=/libpath:\"%s\\mfc\\lib\" /libpath:\"%s\\lib\" nafxcw.lib ..\\vtkdll\\obj\\vtkdll.lib /nologo /version:1.3 /subsystem:windows\\\n",
	    vals->m_WhereCompiler, vals->m_WhereCompiler);
    }
  fprintf(fp," /dll /incremental:no /pdb:\"$(OUTDIR)/vtkjava.pdb\" /machine:I386\\\n");
  fprintf(fp," /out:\"$(OUTDIR)/vtkjava.dll\" /implib:\"$(OUTDIR)/vtkjava.lib\" \n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtkJavaUtil.obj\" \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sJava.obj\" \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sJava.obj\" \\\n",concrete[i]);
  }
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sJava.obj\" \\\n",abstract_h[i]);
  }
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sJava.obj\" \\\n",concrete_h[i]);
  }
  fprintf(fp,"\n");
  fprintf(fp,"\"$(OUTDIR)\\vtkjava.dll\" : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS)\n");
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
  fprintf(fp,"\"$(OUTDIR)\\vtkJavaUtil.obj\" : \"%s\\common\\vtkJavaUtil.cxx\" \"$(OUTDIR)\"\n",
	  vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\common\\vtkJavaUtil.cxx\"\n\n",vals->m_WhereVTK);

  for (i = 0; i < num_abstract; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sJava.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\java_wrap\\Debug\\java_wrap %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints > src\\%sJava.cxx\n\n",
		vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\java_parse\\Debug\\java_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints > vtk\\%s.java\n\n",
		vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sJava.obj\" : src\\%sJava.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",abstract[i]);
  }

  for (i = 0; i < num_concrete; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sJava.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\java_wrap\\Debug\\java_wrap %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints > src\\%sJava.cxx\n\n",
		vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\java_parse\\Debug\\java_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints > vtk\\%s.java\n\n",
		vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sJava.obj\" : src\\%sJava.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",concrete[i]);
  }

  for (i = 0; i < num_abstract_h; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sJava.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		abstract_h[i],vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\java_wrap\\Debug\\java_wrap %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints > src\\%sJava.cxx\n\n",
		vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\java_parse\\Debug\\java_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints > vtk\\%s.java\n\n",
		vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sJava.obj\" : src\\%sJava.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",abstract_h[i]);
  }

  for (i = 0; i < num_concrete_h; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    OutputDepends(file,fp,vals->m_WhereVTK);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sJava.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
		concrete_h[i],vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\java_wrap\\Debug\\java_wrap %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints > src\\%sJava.cxx\n\n",
		vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\java_parse\\Debug\\java_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints > vtk\\%s.java\n\n",
		vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sJava.obj\" : src\\%sJava.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",concrete_h[i]);
  }

  fprintf(fp,"################################################################################\n");
}

void doBorJavaHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue)
{
  int i;

  fprintf(fp,"# VTK Borland makefile\n");
  fprintf(fp,"OUTDIR=%s\\vtkjava\\obj\n\n",vals->m_WhereBuild);
  fprintf(fp,"CPP=BCC32.exe +CPP_PROJ.CFG\n\n");
  fprintf(fp,"ALL : vtkjava.dll\n\n");

  fprintf(fp,"obj ::\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"obj\"\n");
  fprintf(fp,"\n");
  fprintf(fp,"  echo Make Config\n");
    fprintf(fp,"    Copy &&| \n");

  if (vals->m_Debug)
    {

    fprintf(fp,"-D_DEBUG -v -R \n");
    }
  else
    {
 fprintf(fp,"-v- -R- \n");
       }
  if (vals->m_Patented)
    {
    fprintf(fp,"-DUSE_PATENTED -I%s\\patented\n",
      vals->m_WhereVTK);
    }
 fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL;VTKJAVA\n",
      vals->m_WhereVTK);
 fprintf(fp,"-tWM -tWD -Od -H- -VF -I%s\\include\\mfc;%s\\include;%s\\common;%s\\graphics;%s\\volume -DWIN32\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
 fprintf(fp," -I%s\\include \n",vals->m_WhereJDK);
 fprintf(fp," -I%s\\include\\win32 \n",vals->m_WhereJDK);
 fprintf(fp," -I%s\\imaging \n",vals->m_WhereVTK);
 fprintf(fp," -I%s\\contrib \n",vals->m_WhereVTK);
  if (doAddedValue) fprintf(fp," -I%s\\gemsio -I%s\\gemsip -I%s\\gemsvolume -I%s\\volume \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  fprintf(fp,"-P -c -w-hid -w-inl \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n");
  fprintf(fp,"LINK32=tlink32.exe\n\n");

  fprintf(fp,"LINK32_FLAGS=-L%s\\lib \\\n",
  vals->m_WhereCompiler);
  if (vals->m_Debug)
    {
    fprintf(fp,"  -v \\\n");
    }
   else
    {
    fprintf(fp,"  -v- \\\n");
    }
  fprintf(fp,"  -Tpd -aa -V4.0 -Gm  -w-inq -m -n\n");
  fprintf(fp,"DEPLINK32_OBJS= \\\n");
  fprintf(fp,"    obj\\vtkJavaUtil.obj \\\n");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"    obj\\%sJava.obj \\\n",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"    obj\\%sJava.obj \\\n",concrete[i]);
  }
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"    obj\\%sJava.obj \\\n",abstract_h[i]);
  }
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"    obj\\%sJava.obj \\\n",concrete_h[i]);
  }
  fprintf(fp,"\n");
    fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    obj\\vtkJavaUtil.obj");
  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%sJava.obj",abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%sJava.obj",concrete[i]);
  }
  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%sJava.obj",abstract_h[i]);
  }
  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"+ \\\n    obj\\%sJava.obj",concrete_h[i]);
  }
    fprintf(fp,", \\\n $<,$*,  \\\n");   // this is the target and map file name
    fprintf(fp,"    %s\\java\\lib\\javai.lib \\\n",vals->m_WhereCompiler);
  fprintf(fp,"\n");
  fprintf(fp,"vtkjava.dll : obj $(DEF_FILE) $(DEPLINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @&&| \n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"|  \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-oobj\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"\n");
  fprintf(fp,"obj\\vtkJavaUtil.obj : %s\\common\\vtkJavaUtil.cxx \n",
	  vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-oobj\\vtkJavaUtil.obj  %s\\common\\vtkJavaUtil.cxx\n\n",vals->m_WhereVTK);
   fprintf(fp,"|  \n");

  for (i = 0; i < num_abstract; i++)
  {
    fprintf(fp,"src\\%sJava.cxx : %s\\%s\\%s.h \n",
		abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\java_wrap\\Debug\\java_wrap %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints > src\\%sJava.cxx\n\n",
		vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\java_parse\\Debug\\java_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\tcl\\hints > vtk\\%s.java\n\n",
		vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"obj\\%sJava.obj : src\\%sJava.cxx \n",
		abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) @&&|");
    fprintf(fp,"-oobj\\%sJava.obj  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",abstract[i],abstract[i]);
    fprintf(fp,"|  \n");
  }

  for (i = 0; i < num_concrete; i++)
  {
    fprintf(fp,"src\\%sJava.cxx : %s\\%s\\%s.h \n",
		concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\java_wrap\\Debug\\java_wrap %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints > src\\%sJava.cxx\n\n",
		vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\java_parse\\Debug\\java_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\tcl\\hints > vtk\\%s.java\n\n",
		vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"obj\\%sJava.obj : src\\%sJava.cxx \n",
		concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) @&&|");
    fprintf(fp,"-oobj\\%sJava.obj  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",
   	concrete[i],concrete[i]);
     fprintf(fp,"|  \n");
  }

  for (i = 0; i < num_abstract_h; i++)
  {
    fprintf(fp,"src\\%sJava.cxx\" : %s\\%s\\%s.h \n",
		abstract_h[i],vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\java_wrap\\Debug\\java_wrap %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints > src\\%sJava.cxx\n\n",
		vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\java_parse\\Debug\\java_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\tcl\\hints > vtk\\%s.java\n\n",
		vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"obj\\%sJava.obj : src\\%sJava.cxx \n",
		abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) @&&|");
    fprintf(fp,"-oobj\\%sJava.obj  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",abstract[i],abstract[i]);
    fprintf(fp,"|  \n");
  }

  for (i = 0; i < num_concrete_h; i++)
  {
    fprintf(fp,"src\\%sJava.cxx : %s\\%s\\%s.h \n",
		concrete_h[i],vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\java_wrap\\Debug\\java_wrap %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints > src\\%sJava.cxx\n\n",
		vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\java_parse\\Debug\\java_parse %s\\%s\\%s.h\\\n",
		vals->m_WhereVTK, vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\tcl\\hints > vtk\\%s.java\n\n",
		vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"obj\\%sJava.obj : src\\%sJava.cxx \n",
		concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) @&&|");
    fprintf(fp,"-oobj\\%sJava.obj  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",abstract[i],abstract[i]);
    fprintf(fp,"|  \n");
  }

  fprintf(fp,"################################################################################\n");
}

