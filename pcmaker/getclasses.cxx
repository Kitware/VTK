#include "stdafx.h"
#include "pcmaker.h"
#include "pcmakerDlg.h"
#include <direct.h>
#include <fstream.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

// Switch to turn on and off widget building in the working directory
#define BUILD_WORKING_TK_WIDGET 0

// where to split the graphics library: A-O and P-Z
#define LIB_SPLIT_STRING "vtkp" 

// LT_COMMON must be last in this list
enum LibraryTypes {LT_GRAPHICS, LT_IMAGING, LT_PATENTED, LT_GEMSIP, LT_GEMSVOLUME, 
	LT_GEAE, LT_WORKING, LT_CONTRIB, LT_COMMON};

extern void AddToDepends(char *file);
extern void AddToGLibDepends(char *file);
extern void BuildDepends(CPcmakerDlg *vals);
extern void BuildGLibDepends(CPcmakerDlg *vals);
extern int GetGraphicsSplit(int classSet[]);
void makeIncrementalMakefiles(CPcmakerDlg *vals, int doAddedValue, int debugFlag);
void makeNonIncrementalMakefile(CPcmakerDlg *vals, int doAddedValue, int debugFlag);
void SetupDepends(CPcmakerDlg *vals, int debugFlag);
void SetupSplitGraphicsDepends(CPcmakerDlg *vals);
void writeGraphicsSplitInfo(CPcmakerDlg *vals);


// messy... but gets the job done 
int concreteStart[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int abstractStart[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int concreteHStart[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int abstractHStart[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int concreteEnd[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int abstractEnd[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int concreteHEnd[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int abstractHEnd[15]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};



// split info
struct DEPENDS_STRUCT
{
	int indices[100];
	int numIndices;
	char name[256];
};
extern DEPENDS_STRUCT *GLibDependsArray[];
int *SplitGraphicsIndices[40];  // allows 40 libs of >= 50!!!! 2000 files
int NumInSplitLib[40];
// keep track of how many split libs
int NumOfGraphicsLibs=0;

// classes that should only be built with tcl
char *abstractTcl[100];
char *concreteTcl[100];
const char *abstractTcl_lib[100];
const char *concreteTcl_lib[100];
int num_abstractTcl = 0;
int num_concreteTcl = 0;

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

  IS = new ifstream(fname);
  if (IS->fail())
    {
    MessageBox(NULL,"ERROR: failed to open Makefile.in for parsing.",
	       "Error",MB_OK);
    return;
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



// order of the files is important for doing the incremental build
void removeUNIXOnlyFiles(CPcmakerDlg *vals)
{
  int i, j;

  for (i = 0; i < num_concrete; i++)
    {
    if (!(strcmp(concrete[i],"vtkXRenderWindow") &&
          strcmp(concrete[i],"vtkXRenderWindowInteractor") &&
          strcmp(concrete[i],"vtkImageXViewer") &&
		  strcmp(concrete[i], "vtkXImageMapper") &&
		  strcmp(concrete[i], "vtkXImageWindow") &&
		  strcmp(concrete[i], "vtkXTextMapper") &&
          strncmp(concrete[i],"vtkDFA",6) && 
          strncmp(concrete[i],"vtkTcl",6) && 
          strncmp(concrete[i],"vtkTk",5) ))
      {
      if (vals->m_DFA && !( strncmp(concrete[i],"vtkDFA",6) && 
						                strncmp(concrete[i],"vtkTcl",6) && 
						                strncmp(concrete[i],"vtkTk",5) ))
				{
				concreteTcl[num_concreteTcl] = concrete[i];
				concreteTcl_lib[num_concreteTcl++] = concrete_lib[i];
				}

      if (i < num_concrete - 1)
        {
        memmove((void *)(concrete + i), (void *)(concrete + i + 1), sizeof(char*)*(num_concrete - i - 1) );
        memmove((void *)(concrete_lib + i), (void *)(concrete_lib + i + 1), sizeof(char*)*(num_concrete - i - 1) );
        }
      
      for ( j = 0; j <= LT_COMMON; j++)
        {
        if (concreteStart[j] > i )
          concreteStart[j]--;
        if (concreteEnd[j] > i )
          concreteEnd[j]--;
        }
      num_concrete--;
      i--;
      }
    }
  for (i = 0; i < num_abstract; i++)
    {
    if (!(strcmp(abstract[i],"vtkXRenderWindow") &&
          strcmp(abstract[i],"vtkXRenderWindowInteractor") &&
          strcmp(abstract[i],"vtkImageXViewer") &&
		  strcmp(concrete[i], "vtkXImageMapper") &&
		  strcmp(concrete[i], "vtkXImageWindow") &&
		  strcmp(concrete[i], "vtkXTextMapper") &&
          strncmp(abstract[i],"vtkDFA",6) && 
          strncmp(abstract[i],"vtkTcl",6) && 
          strncmp(abstract[i],"vtkTk",5) ))
      {
      if (vals->m_DFA && !( strncmp(abstract[i],"vtkDFA",6) && 
						                strncmp(abstract[i],"vtkTcl",6) && 
						                strncmp(abstract[i],"vtkTk",5) ))
				{
				abstractTcl[num_abstractTcl] = abstract[i];
				abstractTcl_lib[num_abstractTcl++] = abstract_lib[i];
				}

      if (i < num_abstract - 1)
        {
        memmove((void *)(abstract + i), (void *)(abstract + i + 1), sizeof(char*)*(num_abstract - i - 1) );
        memmove((void *)(abstract_lib + i), (void *)(abstract_lib + i + 1), sizeof(char*)*(num_abstract - i - 1) );
        }

      for ( j = 0; j <= LT_COMMON; j++)
        {
        if (abstractStart[j] > i )
          abstractStart[j]--;
        if (abstractEnd[j] > i )
          abstractEnd[j]--;
        }
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
    // claw: I am adding this so c++ can evaluate strings.
    fprintf(fp,"\nTcl_Interp *vtkGlobalTclInterp;\n");
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
  if (vals->m_Imaging) fprintf(fp,"extern \"C\" {int Vtktkimageviewerwidget_Init(Tcl_Interp *interp);}\n\n");
  if (vals->m_Working) fprintf(fp,"extern \"C\" {int Vtktkimagewindowwidget_Init(Tcl_Interp *interp);}\n\n");


  fprintf(fp,"\n\nint %s_Init(Tcl_Interp *interp)\n{\n",kitName);
  if (!strcmp(kitName,"Vtktcl"))
    {
    // claw: I am adding this to allow c++ to evaluate tcl commands.
    fprintf(fp,
	    "  vtkGlobalTclInterp = interp;\n");
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
    if (vals->m_Imaging) fprintf(fp,"  Vtktkimageviewerwidget_Init(interp);\n");

	if (vals->m_Working) fprintf(fp,"  Vtktkimagewindowwidget_Init(interp);\n");

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
	anindex = 0;
  for (i = 0; i < num_concrete; i++)
  {
    names[anindex++] = concrete[i];
  }
  for (i = 0; i < num_concrete_h; i++)
  {
	names[anindex++] = concrete_h[i];
  }
  for (i = 0; i < num_concreteTcl; i++)
  {
    names[anindex++] = concreteTcl[i];
  }
  
  fp = fopen(fname,"w");
  if (fp)
  {
  fprintf(fp,"#include <string.h>\n");
  fprintf(fp,"#include <tcl.h>\n\n");
  stuffit(fp,vals);
  fclose(fp);
  }
}

// writes for a specific library... only writes if there is a change
void MakeForce(char *fname, LibraryTypes whichLibrary)
{
  int i;
  FILE *fp;
  struct stat statBuff;
  ifstream *IS;
  char line[256];

  char outline[200][256];  // up to 200 lines
  int checkIndex, outIndex = 0;

  sprintf(outline[outIndex++],"#ifndef RW_STD_IOSTREAM");
  sprintf(outline[outIndex++],"#define RW_STD_IOSTREAM");
  sprintf(outline[outIndex++],"#include <string.h>");
  sprintf(outline[outIndex++],"#undef RW_STD_IOSTREAM");
  sprintf(outline[outIndex++],"#else");
  sprintf(outline[outIndex++],"#include <string.h>");
  sprintf(outline[outIndex++],"#endif");
  for (i = abstractHStart[whichLibrary]; i < abstractHEnd[whichLibrary]; i++)
		sprintf(outline[outIndex++],"#include \"%s.h\"",abstract_h[i]);
 	for (i = concreteHStart[whichLibrary]; i < concreteHEnd[whichLibrary]; i++)
		sprintf(outline[outIndex++],"#include \"%s.h\"",concrete_h[i]);

	if (stat(fname,&statBuff) != -1) // maybe what we would write is the same as what is there
    {
    IS = new ifstream(fname);
    checkIndex = 0;
    while (!IS->eof() && checkIndex < outIndex)
      {
      IS->getline(line,255);

      if (strcmp(outline[checkIndex++],line))
        break;
      }
    if ( IS->eof() && checkIndex==outIndex ) // they match
      {
      IS->close();
      delete IS;  
      return;
      }
    IS->close();
    delete IS;  
    }

  fp = fopen(fname,"w");
  if (fp)
    {
    for (i = 0; i < outIndex; i++)
      {
      fprintf(fp,"%s",outline[i]);
      if (i < outIndex - 1)
        fprintf(fp,"\n");
      }
    }
  fclose(fp);
}



void doMSCHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag);
void doBorHeader(FILE *fp, CPcmakerDlg *vals, int doAdded, int debugFlag);
void doMSCTclHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue, int debugFlag);
void doBorTclHeader(FILE *fp, CPcmakerDlg *vals, int doAdded, int debugFlag);
void doMSCJavaHeader(FILE *fp, CPcmakerDlg *vals, int doAdded, int debugFlag);
void doBorJavaHeader(FILE *fp, CPcmakerDlg *vals, int doAdded, int debugFlag);

// generate depend info for a .cxx file
// outputs the result as DEPEND
extern void OutputDepends(char *file, FILE *fp);


void UpdateStart(LibraryTypes whichLib)
  {
  concreteStart[whichLib] = num_concrete;
  abstractStart[whichLib] = num_abstract;
  concreteHStart[whichLib] = num_concrete_h;
  abstractHStart[whichLib] = num_abstract_h;
  }


void UpdateEnd(LibraryTypes whichLib)
  {
  concreteEnd[whichLib] = num_concrete;
  abstractEnd[whichLib] = num_abstract;
  concreteHEnd[whichLib] = num_concrete_h;
  abstractHEnd[whichLib] = num_abstract_h;
  }


// returns doAddedValue
int ReadMakefiles(CPcmakerDlg *vals)
{
  char fname[256];
  int doAddedValue = 0;

  UpdateStart(LT_COMMON);
  sprintf(fname,"%s\\common\\Makefile.in",vals->m_WhereVTK);
  readInMakefile(fname,strdup("common"));
  UpdateEnd(LT_COMMON);

  if (vals->m_Graphics) // also include patented if selected
    {
    UpdateStart(LT_GRAPHICS);
    sprintf(fname,"%s\\graphics\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("graphics"));

    concrete[num_concrete] = strdup("vtkOpenGLRenderer");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOpenGLTexture");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOpenGLProperty");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOpenGLActor");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOpenGLCamera");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOpenGLPolyDataMapper");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOpenGLProjectedPolyDataRayBounder");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkOpenGLLight");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkWin32OpenGLRenderWindow");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkWin32RenderWindowInteractor");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;
    concrete[num_concrete] = strdup("vtkWin32MappedInteractor");
    concrete_lib[num_concrete] = strdup("graphics");
    num_concrete++;

    if (vals->m_Patented)
      {
      sprintf(fname,"%s\\patented\\Makefile.in",vals->m_WhereVTK);
      readInMakefile(fname,strdup("patented"));
      }
    
    UpdateEnd(LT_GRAPHICS);
    }
  else if (vals->m_Patented)
    {
    UpdateStart(LT_PATENTED);
    sprintf(fname,"%s\\patented\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("patented"));
    UpdateEnd(LT_PATENTED);
    }

  if (vals->m_Imaging)
    {
    UpdateStart(LT_IMAGING);
    sprintf(fname,"%s\\imaging\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("imaging"));
	  concrete[num_concrete] = strdup("vtkImageWin32Viewer");
    concrete_lib[num_concrete] = strdup("imaging");
    num_concrete++;
    UpdateEnd(LT_IMAGING);
    }
  if (vals->m_Contrib)
    {
    UpdateStart(LT_CONTRIB);
    sprintf(fname,"%s\\contrib\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("contrib"));
    UpdateEnd(LT_CONTRIB);
    }
  if (vals->m_Working)
    {
    UpdateStart(LT_WORKING);
    sprintf(fname,"%s\\working\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("working"));
	  concrete[num_concrete] = strdup("vtkWin32ImageMapper");
    concrete_lib[num_concrete] = strdup("working");
    num_concrete++;
	  concrete[num_concrete] = strdup("vtkWin32ImageWindow");
    concrete_lib[num_concrete] = strdup("working");
    num_concrete++;
	  concrete[num_concrete] = strdup("vtkWin32TextMapper");
    concrete_lib[num_concrete] = strdup("working");
    num_concrete++;
    UpdateEnd(LT_WORKING);
    }
  if (vals->m_GEMSIP)
    {
    UpdateStart(LT_GEMSIP);
    doAddedValue = 1;
    sprintf(fname,"%s\\gemsio\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("gemsio"));
    doAddedValue = 1;
    sprintf(fname,"%s\\gemsip\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("gemsip"));
    UpdateEnd(LT_GEMSIP);
    }
  if (vals->m_GEAE)
    {
    UpdateStart(LT_GEAE);
    doAddedValue = 1;
    sprintf(fname,"%s\\geae\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("geae"));
    UpdateEnd(LT_GEAE);
    }
  if (vals->m_GEMSVOLUME)
    {
    UpdateStart(LT_GEMSVOLUME);
    doAddedValue = 1;
    sprintf(fname,"%s\\gemsvolume\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("gemsvolume"));
    UpdateEnd(LT_GEMSVOLUME);
    }

  return doAddedValue;
}



void CreateRequiredFiles(CPcmakerDlg *vals)
  {
  char fname[256];

  // we must create vtkPCForce.cxx
  sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceCommon.cxx",vals->m_WhereBuild);
  MakeForce(fname,LT_COMMON);
  sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceCommon.cxx",vals->m_WhereBuild);
  MakeForce(fname,LT_COMMON);

  if (vals->m_Graphics)
    {
	  sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceGraphics.cxx",vals->m_WhereBuild);
	  MakeForce(fname,LT_GRAPHICS);
    sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceGraphics.cxx",vals->m_WhereBuild);
	  MakeForce(fname,LT_GRAPHICS);
    }
  else if (vals->m_Patented)
    {
		sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForcePatented.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_PATENTED);
		sprintf(fname,"%s\\vtkdll\\src\\vtkPCForcePatented.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_PATENTED);
    }
  if (vals->m_Imaging)
    {
		sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceImaging.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_IMAGING);
		sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceImaging.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_IMAGING);
    }
  if (vals->m_Contrib)
    {
		sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceContrib.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_CONTRIB);
		sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceContrib.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_CONTRIB);
    }
  if (vals->m_Working)
    {
		sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceWorking.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_WORKING);
		sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceWorking.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_WORKING);
    }
  if (vals->m_GEMSIP)
    {
		sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceGemsip.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_GEMSIP);
		sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceGemsip.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_GEMSIP);
    }
  if (vals->m_GEAE)
    {
		sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceGeae.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_GEAE);
		sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceGeae.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_GEAE);
    }
  if (vals->m_GEMSVOLUME)
    {
		sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceGemsVolume.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_GEMSVOLUME);
		sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceGemsVolume.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_GEMSVOLUME);
    }

  // we must create CommonInit.cxx etc
  sprintf(fname,"%s\\Debug\\vtktcl\\src\\vtktcl.cxx",vals->m_WhereBuild);
  MakeInit(fname,"Vtktcl",vals);
  sprintf(fname,"%s\\vtktcl\\src\\vtktcl.cxx",vals->m_WhereBuild);
  MakeInit(fname,"Vtktcl",vals);
  }


// only call if m_MSComp
void makeMakefiles(CPcmakerDlg *vals)
{
  int doAddedValue;
  int total;

  doAddedValue = ReadMakefiles(vals);
  removeUNIXOnlyFiles(vals);

  CreateRequiredFiles(vals);

  // set up the progress indicator... total is approximate
  // 1st for computing depends....
  total = 10 * (1 + vals->m_Graphics + vals->m_Imaging + vals->m_Working + vals->m_Contrib + 
                    vals->m_GEMSIP + vals->m_GEMSVOLUME + vals->m_GEAE +
                    2*num_concrete + 2*num_abstract + num_abstract_h + num_concrete_h +
										2*num_concreteTcl + 2*num_abstractTcl);

  total += 2 * total / 10;

  // extra for split graphics stuff
  if (vals->m_Graphics)
    total += 10 * 2 * (concreteEnd[LT_GRAPHICS] + abstractEnd[LT_GRAPHICS] -
                        concreteStart[LT_GRAPHICS] - abstractStart[LT_GRAPHICS]);

  if (strlen(vals->m_WhereJDK) > 1)
    {
    total += 2 * (num_concrete + num_abstract + num_abstract_h + num_concrete_h);
    } 
  vals->m_Progress.SetRange(0,total);
  vals->m_Progress.SetPos(0);

  SetupDepends(vals, 2); // 2 does both Debug and not for now...

  // tcl and/or jave makefile made (called form) makeNonIncrementalMakefile()
  if (vals->m_MSComp)
    {
    makeIncrementalMakefiles(vals,doAddedValue,0);  // non-debug
    makeIncrementalMakefiles(vals,doAddedValue,1);  // debug
    }
  makeNonIncrementalMakefile(vals,doAddedValue,0);  // non-debug
  makeNonIncrementalMakefile(vals,doAddedValue,1);  // non-debug
  }


void makeNonIncrementalMakefile(CPcmakerDlg *vals, int doAddedValue, int debugFlag)
{
  char fname[256];
  FILE *ofp;

  // spit out a Makefile... did at same time as incremental if MCSomp
  if (vals->m_BorlandComp)
    {
  	if ( debugFlag )
	    sprintf(fname,"%s\\Debug\\vtkdll\\makefile",vals->m_WhereBuild);
	  else
	    sprintf(fname,"%s\\vtkdll\\makefile",vals->m_WhereBuild);
    ofp = fopen(fname,"w");
    doBorHeader(ofp, vals, doAddedValue, debugFlag);
    fclose(ofp);
    }

	if ( debugFlag )
		sprintf(fname,"%s\\Debug\\vtktcl\\makefile",vals->m_WhereBuild);
	else
		sprintf(fname,"%s\\vtktcl\\makefile",vals->m_WhereBuild);
  ofp = fopen(fname,"w");
  if (vals->m_MSComp) doMSCTclHeader(ofp, vals, doAddedValue, debugFlag);
  if (vals->m_BorlandComp) doBorTclHeader(ofp, vals, doAddedValue, debugFlag);
  fclose(ofp);

  // generate the java makefiles if requested
  if (strlen(vals->m_WhereJDK) > 1)
    {
		if ( debugFlag )
			sprintf(fname,"%s\\Debug\\vtkjava\\makefile",vals->m_WhereBuild);
		else
			sprintf(fname,"%s\\vtkjava\\makefile",vals->m_WhereBuild);
		ofp = fopen(fname,"w");
    if (vals->m_MSComp ) doMSCJavaHeader(ofp, vals, doAddedValue, debugFlag);
    if (vals->m_BorlandComp) doBorJavaHeader(ofp, vals, doAddedValue, debugFlag);
    fclose(ofp);
    }
}



// build off vtkdll directory...obj and lib directories
void makeIncrementalMakefiles(CPcmakerDlg *vals, int doAddedValue, int debugFlag)
{
  char fname[256];
  FILE *fp;
  
  // spit out a Makefile.... calls and make other makefiles
	if ( debugFlag )
		sprintf(fname,"%s\\Debug\\makefile",vals->m_WhereBuild);
	else
		sprintf(fname,"%s\\makefile",vals->m_WhereBuild);
  fp = fopen(fname,"w");

  // write the stuff at the top that we only write once
  fprintf(fp,"# VTK Parent makefile\n");

  // define ALL... and make sure directories exist
  // always has common
  fprintf(fp,"ALL : vtkLibs ");

  // Java?
  if (strlen(vals->m_WhereJDK) > 1)
    fprintf(fp,"vtkJaveLib ");

  // always TCL.. but last
  fprintf(fp,"vtkTclLib\n\n");

  fprintf(fp,"vtkLibs :\n");
  fprintf(fp,"   cd vtkdll\n");
  fprintf(fp,"   nmake OBJS\n");
  fprintf(fp,"   nmake LIBRARIES\n");
  fprintf(fp,"   cd ../\n\n");

  if (strlen(vals->m_WhereJDK) > 1)
    {
    fprintf(fp,"vtkJavaLib :\n");
	  fprintf(fp,"   cd vtkjava\n");
    fprintf(fp,"   nmake ..\\lib\\vtkjava.dll\n");
    fprintf(fp,"   nmake ..\\lib\\vtkjava.dll\n");
    fprintf(fp,"   cd ../\n\n");
    fclose(fp);
    }

  // tcl
  fprintf(fp,"vtkTclLib :\n");
  fprintf(fp,"   cd vtktcl\n");
  fprintf(fp,"   nmake ..\\lib\\vtktcl.dll\n");
  fprintf(fp,"   nmake ..\\lib\\vtktcl.dll\n");
  fprintf(fp,"   cd ../\n\n");
  fclose(fp);


  if ( debugFlag )
		sprintf(fname,"%s\\Debug\\vtkdll\\makefile",vals->m_WhereBuild);
	else
		sprintf(fname,"%s\\vtkdll\\makefile",vals->m_WhereBuild);
  fp = fopen(fname,"w");
  doMSCHeader(fp,vals,debugFlag);
  fclose(fp);
}




/******************************************************************************
  Here are the different makefile methods
*******************************************************************************/
void doMSCHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag)
{
  int i, j;
  char file[256], temp[256];

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"CPP=cl.exe\n\n");

	fprintf(fp,"OBJDIR=obj\n");
	fprintf(fp,"LIBDIR=..\\lib\n\n");

  fprintf(fp,"NONINCREMENTAL : vtkdll.dll\n\n");

  if (debugFlag)
    {
    fprintf(fp,"CPP_PROJ=/nologo /D \"STRICT\" /D \"_DEBUG\" /MDd /GX /Od /Zi /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\imaging\" /I \"%s\\graphics\" /I \"%s\\working\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
#if _MSC_VER == 1100  // version 5.0
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
		if (sysInfo.wProcessorLevel == 6) // handle the "bug"(?) using /G5
	    fprintf(fp,"CPP_PROJ=/nologo /D \"STRICT\" /MD /G6 /Ox /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\working\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
		    vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
		else
			fprintf(fp,"CPP_PROJ=/nologo /D \"STRICT\" /MD /GX /Ox /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\working\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
				vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
#else
    fprintf(fp,"CPP_PROJ=/nologo /D \"STRICT\" /MD /G5 /Ox /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\working\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
#endif
    }
  if (vals->m_Patented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"VTK_USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n",
      vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n");
    }
  if (!debugFlag && vals->m_Lean)
    {
    fprintf(fp,"/W2 /D \"VTK_LEAN_AND_MEAN\"  /Fo$(OBJDIR)\\ /c\n");
    }
  else
    {
    fprintf(fp,"/W2  /Fo$(OBJDIR)\\ /c\n");
    }

	fprintf(fp,"LINK32=link.exe\n");
  if (debugFlag)
    {
    fprintf(fp,"LINK32_FLAGS=/debug /libpath:\"%s\\lib\" \"%s\\lib\\opengl32.lib\" \"%s\\lib\\glaux.lib\" \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows\n",
		  vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=/libpath:\"%s\\lib\" \"%s\\lib\\opengl32.lib\" \"%s\\lib\\glaux.lib\" \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows\n",
    vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }

  fprintf(fp,"ALL_FLAGS= /dll /incremental:no /machine:I386\\\n");
  fprintf(fp," /out:vtkdll.dll /implib:vtkdll.lib \n\n");

  fprintf(fp,"COMMON_FLAGS= /dll /incremental:yes /machine:I386\\\n");
  fprintf(fp," /out:\"$(LIBDIR)/vtkCommon.dll\" /implib:\"$(LIBDIR)/vtkCommon.lib\" \n");

  fprintf(fp,"COMMON_OBJS= \\\n");
  fprintf(fp,"    \"$(OBJDIR)\\vtkPCForceCommon.obj\" \\\n");
  for (i = abstractStart[LT_COMMON]; i < abstractEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
  for (i = concreteStart[LT_COMMON]; i < concreteEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
  fprintf(fp,"\n");


  if ( vals->m_Graphics )
    {
    for (j = 0; j < NumOfGraphicsLibs; j++)
      {
      fprintf(fp,"GRAPHICS_FLAGS%d= /dll /incremental:yes /machine:I386\\\n",j);
      fprintf(fp," /out:\"$(LIBDIR)/vtkGraphics%d.dll\" /implib:\"$(LIBDIR)/vtkGraphics%d.lib\" \n",j,j);
      fprintf(fp,"GRAPHICS_LIBS%d=\"$(LIBDIR)/vtkCommon.lib\" ",j);
      for (i = 0; i < j; i++)
        fprintf(fp,"\"$(LIBDIR)/vtkGraphics%d.lib\" ",i);
      fprintf(fp,"\n");

      fprintf(fp,"GRAPHICS_OBJS%d= \\\n", j);
      for (i = 0; i < NumInSplitLib[ j ]; i++)
        fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",
                    GLibDependsArray[ SplitGraphicsIndices[j][i] ]->name);
      fprintf(fp,"\n");
      }
    }
  else if ( vals->m_Patented )
    {
    fprintf(fp,"PATENTED_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkPatented.dll\" /implib:\"$(LIBDIR)/vtkPatented.lib\" \n");
    fprintf(fp,"PATENTED_LIBS=\"$(LIBDIR)/vtkCommon.lib\"\n");

    fprintf(fp,"PATENTED_OBJS= \\\n");
    fprintf(fp,"    \"$(OBJDIR)\\vtkPCForcePatented.obj\" \\\n");
    for (i = abstractStart[LT_PATENTED]; i < abstractEnd[LT_PATENTED]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
    for (i = concreteStart[LT_PATENTED]; i < concreteEnd[LT_PATENTED]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
    fprintf(fp,"\n");
    }
  if ( vals->m_Imaging )
    {
    fprintf(fp,"IMAGING_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkImaging.dll\" /implib:\"$(LIBDIR)/vtkImaging.lib\" \n");
    fprintf(fp,"IMAGING_LIBS=\"$(LIBDIR)/vtkCommon.lib\" ");
    fprintf(fp,"\n");

    fprintf(fp,"IMAGING_OBJS= \\\n");
    fprintf(fp,"    \"$(OBJDIR)\\vtkPCForceImaging.obj\" \\\n");
    for (i = abstractStart[LT_IMAGING]; i < abstractEnd[LT_IMAGING]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
    for (i = concreteStart[LT_IMAGING]; i < concreteEnd[LT_IMAGING]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
    fprintf(fp,"\n");
    }
  if ( vals->m_Working )
    {
    fprintf(fp,"WORKING_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkWorking.dll\" /implib:\"$(LIBDIR)/vtkWorking.lib\" \n");
    fprintf(fp,"WORKING_LIBS=\"$(LIBDIR)/vtkCommon.lib\" \"$(LIBDIR)/vtkImaging.lib\" ");
    if ( vals->m_Graphics )
      {
      for (i = 0; i < NumOfGraphicsLibs ; i++)
        fprintf(fp,"\"$(LIBDIR)/vtkGraphics%d.lib\" ",i);
      }
    fprintf(fp,"\n");

    fprintf(fp,"WORKING_OBJS= \\\n");
    fprintf(fp,"    \"$(OBJDIR)\\vtkPCForceWorking.obj\" \\\n");
    for (i = abstractStart[LT_WORKING]; i < abstractEnd[LT_WORKING]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
    for (i = concreteStart[LT_WORKING]; i < concreteEnd[LT_WORKING]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
    fprintf(fp,"\n");
    }
  if ( vals->m_Contrib )
    {
    fprintf(fp,"CONTRIB_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkContrib.dll\" /implib:\"$(LIBDIR)/vtkContrib.lib\" \n");
    fprintf(fp,"CONTRIB_LIBS=\"$(LIBDIR)/vtkCommon.lib\" \"$(LIBDIR)/vtkImaging.lib\" ");
    if ( vals->m_Graphics )
      {
      for (i = 0; i < NumOfGraphicsLibs ; i++)
        fprintf(fp,"\"$(LIBDIR)/vtkGraphics%d.lib\" ",i);
      }
    fprintf(fp,"\n");

    fprintf(fp,"CONTRIB_OBJS= \\\n");
    fprintf(fp,"    \"$(OBJDIR)\\vtkPCForceContrib.obj\" \\\n");
    for (i = abstractStart[LT_CONTRIB]; i < abstractEnd[LT_CONTRIB]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
    for (i = concreteStart[LT_CONTRIB]; i < concreteEnd[LT_CONTRIB]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
    fprintf(fp,"\n");
    }
  if ( vals->m_GEMSIP )
    {
    fprintf(fp,"GEMSIP_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkGemsip.dll\" /implib:\"$(LIBDIR)/vtkGemsip.lib\" \n");
    fprintf(fp,"GEMSIP_LIBS=\"$(LIBDIR)/vtkCommon.lib\" \"$(LIBDIR)/vtkImaging.lib\" ");
    if ( vals->m_Graphics )
      {
      for (i = 0; i < NumOfGraphicsLibs ; i++)
        fprintf(fp,"\"$(LIBDIR)/vtkGraphics%d.lib\" ",i);
      }
    fprintf(fp,"\n");

    fprintf(fp,"GEMSIP_OBJS= \\\n");
    fprintf(fp,"    \"$(OBJDIR)\\vtkPCForceGemsip.obj\" \\\n");
    for (i = abstractStart[LT_GEMSIP]; i < abstractEnd[LT_GEMSIP]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
    for (i = concreteStart[LT_GEMSIP]; i < concreteEnd[LT_GEMSIP]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
    fprintf(fp,"\n");
    }
  if ( vals->m_GEMSVOLUME )
    {
    fprintf(fp,"GEMSVOLUME_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkGemsVolume.dll\" /implib:\"$(LIBDIR)/vtkGemsVolume.lib\" \n");
    fprintf(fp,"GEMSVOLUME_LIBS=\"$(LIBDIR)/vtkCommon.lib\" \"$(LIBDIR)/vtkImaging.lib\" ");
    if ( vals->m_Graphics )
      {
      for (i = 0; i < NumOfGraphicsLibs ; i++)
        fprintf(fp,"\"$(LIBDIR)/vtkGraphics%d.lib\" ",i);
      }
    fprintf(fp,"\n");

    fprintf(fp,"GEMSVOLUME_OBJS= \\\n");
    fprintf(fp,"    \"$(OBJDIR)\\vtkPCForceGemsVolume.obj\" \\\n");
    for (i = abstractStart[LT_GEMSVOLUME]; i < abstractEnd[LT_GEMSVOLUME]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
    for (i = concreteStart[LT_GEMSVOLUME]; i < concreteEnd[LT_GEMSVOLUME]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
    fprintf(fp,"\n");
    }
  if ( vals->m_GEAE )
    {
    fprintf(fp,"GEAE_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkGeae.dll\" /implib:\"$(LIBDIR)/vtkGeae.lib\" \n");
    fprintf(fp,"GEAE_LIBS=\"$(LIBDIR)/vtkCommon.lib\" \"$(LIBDIR)/vtkImaging.lib\" ");
    if ( vals->m_Graphics )
      {
      for (i = 0; i < NumOfGraphicsLibs ; i++)
        fprintf(fp,"\"$(LIBDIR)/vtkGraphics%d.lib\" ",i);
      }
    fprintf(fp,"\n");

    fprintf(fp,"GEAE_OBJS= \\\n");
    fprintf(fp,"    \"$(OBJDIR)\\vtkPCForceGeae.obj\" \\\n");
    for (i = abstractStart[LT_GEAE]; i < abstractEnd[LT_GEAE]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
    for (i = concreteStart[LT_GEAE]; i < concreteEnd[LT_GEAE]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
    fprintf(fp,"\n");
    }

  fprintf(fp,"OBJS : $(COMMON_OBJS) $(PATENTED_OBJS) $(IMAGING_OBJS) $(WORKING_OBJS) \\\n");
  fprintf(fp,"       $(CONTRIB_OBJS) $(GEMSIP_OBJS) $(GEMSVOLUME_OBJS) $(GEAE_OBJS) \\\n");
  if (NumOfGraphicsLibs)
    fprintf(fp,"       $(GRAPHICS_OBJS0) ");
  for (i = 1; i < NumOfGraphicsLibs; i++)
    fprintf(fp,"$(GRAPHICS_OBJS%d) ",i);
  fprintf(fp,"\n\n");

  fprintf(fp,"LIBRARIES : \"$(LIBDIR)\\vtkCommon.dll\"\\\n ");
  if ( vals->m_Graphics )
    {
    for (i = 0; i < NumOfGraphicsLibs; i++)
      fprintf(fp,"          \"$(LIBDIR)\\vtkGraphics%d.dll\"\\\n ",i);
    }
  else if ( vals->m_Patented )
    fprintf(fp,"          \"$(LIBDIR)\\vtkPatented.dll\"\\\n ");
  if ( vals->m_Imaging )
    fprintf(fp,"          \"$(LIBDIR)\\vtkImaging.dll\"\\\n ");
  if ( vals->m_Working )
    fprintf(fp,"          \"$(LIBDIR)\\vtkWorking.dll\"\\\n ");
  if ( vals->m_Contrib )
    fprintf(fp,"          \"$(LIBDIR)\\vtkContrib.dll\"\\\n ");
  if ( vals->m_GEMSIP )
    fprintf(fp,"          \"$(LIBDIR)\\vtkGemsip.dll\"\\\n ");
  if ( vals->m_GEMSVOLUME )
    fprintf(fp,"          \"$(LIBDIR)\\vtkGemsVolume.dll\"\\\n ");
  if ( vals->m_GEAE )
    fprintf(fp,"          \"$(LIBDIR)\\vtkGeae.dll\"\\\n ");
  fprintf(fp,"\n");


  fprintf(fp,"vtkdll.dll : $(DEF_FILE) $(COMMON_OBJS) $(PATENTED_OBJS) ");
  fprintf(fp," $(IMAGING_OBJS) $(WORKING_OBJS) $(CONTRIB_OBJS) ");
  fprintf(fp," $(GEMSIP_OBJS) $(GEMSVOLUME_OBJS) $(GEAE_OBJS) ");
  for (i = 0; i < NumOfGraphicsLibs; i++)
    fprintf(fp,"$(GRAPHICS_OBJS%d) ",i);
  fprintf(fp,"\n");

  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(ALL_FLAGS) $(COMMON_OBJS) $(PATENTED_OBJS) ");
  fprintf(fp," $(IMAGING_OBJS) $(WORKING_OBJS) $(CONTRIB_OBJS) ");
  fprintf(fp," $(GEMSIP_OBJS) $(GEMSVOLUME_OBJS) $(GEAE_OBJS) ");
  for (i = 0; i < NumOfGraphicsLibs; i++)
    fprintf(fp,"$(GRAPHICS_OBJS%d) ",i);
  fprintf(fp,"\n");
  fprintf(fp,"<<\n");
  fprintf(fp,"\n");

  fprintf(fp,"\"$(LIBDIR)\\vtkCommon.dll\" : $(DEF_FILE) $(COMMON_OBJS) \n");
	fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(COMMON_FLAGS) $(COMMON_OBJS)\n");
  fprintf(fp,"<<\n");
  fprintf(fp,"\n");

  if ( vals->m_Graphics )
    {
    for (i = 0; i < NumOfGraphicsLibs; i++)
      {
      fprintf(fp,"\"$(LIBDIR)\\vtkGraphics%d.dll\" : $(DEF_FILE) $(GRAPHICS_OBJS%d) \n",i,i);
	    fprintf(fp,"    $(LINK32) @<<\n");
      fprintf(fp,"  $(LINK32_FLAGS) $(GRAPHICS_FLAGS%d) $(GRAPHICS_OBJS%d) $(GRAPHICS_LIBS%d)\n",i,i,i);
      fprintf(fp,"<<\n");
      fprintf(fp,"\n");
      }
    }
  else if ( vals->m_Patented )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkPatented.dll\" : $(DEF_FILE) $(PATENTED_OBJS) %s\\GraphicsSplitInfo.txt\n",vals->m_WhereBuild);
	  fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(PATENTED_FLAGS) $(PATENTED_OBJS) $(PATENTED_LIBS)\n");
    fprintf(fp,"<<\n");
    fprintf(fp,"\n");
    }
  if ( vals->m_Imaging )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkImaging.dll\" : $(DEF_FILE) $(IMAGING_OBJS)\n");
	  fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(IMAGING_FLAGS) $(IMAGING_OBJS) $(IMAGING_LIBS)\n");
    fprintf(fp,"<<\n");
    fprintf(fp,"\n");
    }
  if ( vals->m_Working )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkWorking.dll\" : $(DEF_FILE) $(WORKING_OBJS) %s\\GraphicsSplitInfo.txt\n",vals->m_WhereBuild);
	  fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(WORKING_FLAGS) $(WORKING_OBJS) $(WORKING_LIBS)\n");
    fprintf(fp,"<<\n");
    fprintf(fp,"\n");
    }
  if ( vals->m_Contrib )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkContrib.dll\" : $(DEF_FILE) $(CONTRIB_OBJS) %s\\GraphicsSplitInfo.txt\n",vals->m_WhereBuild);
	  fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(CONTRIB_FLAGS) $(CONTRIB_OBJS) $(CONTRIB_LIBS)\n");
    fprintf(fp,"<<\n");
    fprintf(fp,"\n");
    }
  if ( vals->m_GEMSIP )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkGemsip.dll\" : $(DEF_FILE) $(GEMSIP_OBJS) %s\\GraphicsSplitInfo.txt\n",vals->m_WhereBuild);
	  fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(GEMSIP_FLAGS) $(GEMSIP_OBJS) $(GEMSIP_LIBS)\n");
    fprintf(fp,"<<\n");
    fprintf(fp,"\n");
    }
  if ( vals->m_GEMSVOLUME )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkGemsVolume.dll\" : $(DEF_FILE) $(GEMSVOLUME_OBJS) %s\\GraphicsSplitInfo.txt\n",vals->m_WhereBuild);
	  fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(GEMSVOLUME_FLAGS) $(GEMSVOLUME_OBJS) $(GEMSVOLUME_LIBS)\n");
    fprintf(fp,"<<\n");
    fprintf(fp,"\n");
    }
  if ( vals->m_GEAE )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkGeae.dll\" : $(DEF_FILE) $(GEAE_OBJS) %s\\GraphicsSplitInfo.txt\n",vals->m_WhereBuild);
	  fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(GEAE_FLAGS) $(GEAE_OBJS) $(GEAE_LIBS)\n");
    fprintf(fp,"<<\n");
    fprintf(fp,"\n");
    }

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

  if ( debugFlag )
    sprintf(temp,"%s\\Debug\\vtkdll\\src",vals->m_WhereBuild);
  else
    sprintf(temp,"%s\\vtkdll\\src",vals->m_WhereBuild);


  sprintf(file,"%s\\vtkPCForceCommon.cxx",temp);
  OutputDepends(file,fp);
  vals->m_Progress.OffsetPos(1);
  fprintf(fp,"\"$(OBJDIR)\\vtkPCForceCommon.obj\" : src\\vtkPCForceCommon.cxx $(DEPENDS) \n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceCommon.cxx\n\n");

  if ( vals->m_Graphics )
    {
    sprintf(file,"%s\\vtkPCForceGraphics.cxx",temp);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceGraphics.obj\" : src\\vtkPCForceGraphics.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceGraphics.cxx\n\n");
    }
  else if ( vals->m_Patented )
    {
    sprintf(file,"%s\\vtkPCForcePatented.cxx",temp);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForcePatented.obj\" : src\\vtkPCForcePatented.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForcePatented.cxx\n\n");
    }
  if ( vals->m_Imaging )
    {
    sprintf(file,"%s\\vtkPCForceImaging.cxx",temp);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceImaging.obj\" : src\\vtkPCForceImaging.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceImaging.cxx\n\n");
    }
  if ( vals->m_Working )
    {
    sprintf(file,"%s\\vtkPCForceWorking.cxx",temp);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceWorking.obj\" : src\\vtkPCForceWorking.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceWorking.cxx\n\n");
    }
  if ( vals->m_Contrib )
    {
    sprintf(file,"%s\\vtkPCForceContrib.cxx",temp);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceContrib.obj\" : src\\vtkPCForceContrib.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceContrib.cxx\n\n");
    }
  if ( vals->m_GEMSIP )
    {
    sprintf(file,"%s\\vtkPCForceGemsip.cxx",temp);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceGemsip.obj\" : src\\vtkPCForceGemsip.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceGemsip.cxx\n\n");
    }
  if ( vals->m_GEMSVOLUME )
    {
    sprintf(file,"%s\\vtkPCForceGemsVolume.cxx",temp);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceGemsVolume.obj\" : src\\vtkPCForceGemsVolume.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceGemsVolume.cxx\n\n");
    }
  if ( vals->m_GEAE )
    {
    sprintf(file,"%s\\vtkPCForceGeae.cxx",temp);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceGeae.obj\" : src\\vtkPCForceGeae.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceGeae.cxx\n\n");
    }

  for (i = 0; i < num_abstract; i++)
  {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\%s.obj\" : \"%s\\%s\\%s.cxx\" $(DEPENDS) \n",
	        abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\%s\\%s.cxx\" \n\n",
		      vals->m_WhereVTK,abstract_lib[i],abstract[i]);
  }
  for (i = 0; i < num_concrete; i++)
  {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\%s.obj\" : \"%s\\%s\\%s.cxx\" $(DEPENDS) \n",
  	      concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\%s\\%s.cxx\"\n\n",
	        vals->m_WhereVTK,concrete_lib[i],concrete[i]);
  }
  fprintf(fp,"################################################################################\n");
}



void doBorHeader(FILE *fp, CPcmakerDlg *vals, int doAddedValue, int debugFlag)
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

  if (debugFlag)
    {

    fprintf(fp,"-D_DEBUG -v -R \n");
    }
  else
    {
 fprintf(fp,"-v- -R- \n");
       }
  if (vals->m_Patented)
    {
    fprintf(fp,"-DVTK_USE_PATENTED -I%s\\patented\n",
      vals->m_WhereVTK);
    }
 fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL\n",
      vals->m_WhereVTK);
 fprintf(fp,"-tWM -tWD -Od -H- -VF -I%s\\include\\mfc;%s\\include;%s\\common;%s\\graphics -DWIN32\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK);
 fprintf(fp," -I%s\\imaging \n",vals->m_WhereVTK);
 fprintf(fp," -I%s\\contrib \n",vals->m_WhereVTK);
  fprintf(fp,"-P -c -w-hid -w-inl \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n");
  fprintf(fp,"LINK32=tlink32.exe\n\n");

  fprintf(fp,"LINK32_FLAGS=-L%s\\lib \\\n",
  vals->m_WhereCompiler);
  if (debugFlag)
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


// links in ALL the vtk libraries (now that they are split up)... also writes output to ../lib directory
// may only want to write the dll to that directory????
// makefile handles both incremental and non-incremental linking
void doMSCTclHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue, int debugFlag)
{
  int i;
  char file [256];

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"!MESSAGE making tcl library...\n");
  fprintf(fp,"CPP=cl.exe\n");
  fprintf(fp,"PATH=$(PATH);\"%s\\pcmaker\\cpp_parse\\Debug\"\n",
		vals->m_WhereVTK);
  fprintf(fp,"CPP_PARSE=cpp_parse.exe\n");
  fprintf(fp,"OUTDIR=obj\n\n");

	fprintf(fp,"LIBDIR=..\\lib\n\n");

  fprintf(fp,"ALL : vtktcl.dll\n\n");

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");

  if (debugFlag)
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /D \"_DEBUG\" /nologo /MDd /GX /Od /Zi /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /nologo /MD /GX /O2 /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  if (vals->m_Patented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"VTK_USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_MBCS\" \\\n",
      vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_MBCS\" \\\n");
    }
  if (doAddedValue) fprintf(fp," /I \"%s\\working\" /I \"%s\\gemsio\" /I \"%s\\gemsip\" /I \"%s\\gemsvolume\" /I \"%s\\geae\" \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  if (!debugFlag && vals->m_Lean)
    {
    fprintf(fp," /D \"VTK_LEAN_AND_MEAN\" /Fo$(OUTDIR)\\ /c \n");
    }
  else
    {
    fprintf(fp," /Fo$(OUTDIR)\\ /c \n");
    }

  if (debugFlag)
    {
    fprintf(fp,"CPP_PROJ2=/D \"STRICT\" /D \"_DEBUG\" /nologo /MDd /GX /Od /Zi /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"CPP_PROJ2=/D \"STRICT\" /nologo /MD /GX /O2 /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  if (vals->m_Patented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"VTK_USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n",
      vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n");
    }
  if (doAddedValue) fprintf(fp," /I \"%s\\working\" /I \"%s\\gemsio\" /I \"%s\\gemsip\" /I \"%s\\gemsvolume\" /I \"%s\\geae\" \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  if (!debugFlag && vals->m_Lean)
    {
    fprintf(fp," /D \"VTK_LEAN_AND_MEAN\" /Fo\"$(OUTDIR)/\" /c \n");
    }
  else
    {
    fprintf(fp," /Fo\"$(OUTDIR)/\" /c \n");
    }
  
	fprintf(fp,"LINK32=link.exe\n");
  if (debugFlag)
    {
    fprintf(fp,"LINK32_FLAGS=\"$(OUTDIR)\\vtktcl.obj\" /debug /libpath:\"%s\\lib\" \"%s\\pcmaker\\tk42.lib\" \"%s\\pcmaker\\tcl76.lib\" \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows\n",
	    vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=\"$(OUTDIR)\\vtktcl.obj\" /libpath:\"%s\\lib\" \"%s\\pcmaker\\tk42.lib\" \"%s\\pcmaker\\tcl76.lib\" \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows\n",
	    vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }

  if (vals->m_DFA)
    fprintf(fp,"FORCE_MULTIPLE=/FORCE:MULTIPLE /WARN:0\n\n");
  fprintf(fp,"MORE_FLAGS1=/dll /incremental:yes /pdb:\"$(LIBDIR)/vtktcl.pdb\" /machine:I386\\\n");
  fprintf(fp," /out:\"$(LIBDIR)/vtktcl.dll\" /implib:\"$(LIBDIR)/vtktcl.lib\" \n\n"); 
  fprintf(fp,"MORE_FLAGS2=/dll /incremental:no /pdb:vtktcl.pdb /machine:I386\\\n");
  fprintf(fp," /out:vtktcl.dll /implib:vtktcl.lib \n\n"); 
  fprintf(fp,"LIB_FLAGS=/out:vtktclobjs.lib /machine:I386\n\n"); 

  fprintf(fp,"VTKDLL_LIB=..\\vtkdll\\vtkdll.lib \n\n");

  fprintf(fp,"VTK_LIBRARIES=..\\lib\\vtkCommon.lib ");
  if (vals->m_Graphics) 
    {
    for (i = 0; i < NumOfGraphicsLibs; i++)
      fprintf(fp,"..\\lib\\vtkGraphics%d.lib  ",i);
    }
  else if (vals->m_Patented)
    fprintf(fp,"..\\lib\\vtkPatented.lib ");
  if (vals->m_Imaging) 
    fprintf(fp,"..\\lib\\vtkImaging.lib ");
  if (vals->m_Contrib)
    fprintf(fp,"..\\lib\\vtkContrib.lib ");
  if (vals->m_Working)
    fprintf(fp,"..\\lib\\vtkWorking.lib ");
  if (vals->m_GEMSIP)
    fprintf(fp,"..\\lib\\vtkGemsip.lib ");
  if (vals->m_GEAE)
    fprintf(fp,"..\\lib\\vtkGeae.lib ");
  if (vals->m_GEMSVOLUME)
    fprintf(fp,"..\\lib\\vtkGemsVolume.lib ");
     
    fprintf(fp,"\n\n");

  
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtkTclUtil.obj\" \\\n");
  if (vals->m_Graphics) fprintf(fp,"    \"$(OUTDIR)\\vtkTkRenderWidget.obj\" \\\n");
  if (vals->m_Imaging)  fprintf(fp,"    \"$(OUTDIR)\\vtkTkImageViewerWidget.obj\" \\\n");

  if (vals->m_Working)  fprintf(fp,"    \"$(OUTDIR)\\vtkTkImageWindowWidget.obj\" \\\n");

  for (i = 0; i < num_abstractTcl; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%s.obj\" \\\n",abstractTcl[i]);
  }
  for (i = 0; i < num_concreteTcl; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%s.obj\" \\\n",concreteTcl[i]);
  }
  for (i = 0; i < num_abstractTcl; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",abstractTcl[i]);
  }
  for (i = 0; i < num_concreteTcl; i++)
  {
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",concreteTcl[i]);
  }
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


  
  fprintf(fp,"vtktcl.dll : \"$(OUTDIR)\" $(DEF_FILE) \"$(OUTDIR)\\vtktcl.obj\" $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(FORCE_MULTIPLE) $(MORE_FLAGS2) $(LINK32_OBJS) $(VTKDLL_LIB)\n");
  fprintf(fp,"<<\n\n");

  fprintf(fp,"\"$(LIBDIR)\\vtktcl.dll\" : \"$(OUTDIR)\" $(DEF_FILE) \"$(OUTDIR)\\vtktcl.obj\" $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(FORCE_MULTIPLE) $(MORE_FLAGS1) $(LINK32_OBJS) $(VTK_LIBRARIES)\n");
  fprintf(fp,"<<\n\n");

  fprintf(fp,"vtktclobjs.lib : \"$(OUTDIR)\" $(DEF_FILE) $(LINK32_OBJS) \n");
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
  fprintf(fp,"\"$(OUTDIR)\\vtkTclUtil.obj\" : \"%s\\common\\vtkTclUtil.cxx\" \"$(OUTDIR)\"\n",
	  vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\common\\vtkTclUtil.cxx\"\n\n",vals->m_WhereVTK);
  if (vals->m_Graphics)
    {
    sprintf(file,"%s\\graphics\\vtkTkRenderWidget.cxx",vals->m_WhereVTK);
    OutputDepends(file,fp);
    fprintf(fp,"\"$(OUTDIR)\\vtkTkRenderWidget.obj\" : \"%s\\graphics\\vtkTkRenderWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	    vals->m_WhereVTK);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\graphics\\vtkTkRenderWidget.cxx\"\n\n",vals->m_WhereVTK);
    }
  if (vals->m_Imaging)
    {
    sprintf(file,"%s\\imaging\\vtkTkImageViewerWidget.cxx",vals->m_WhereVTK);
    OutputDepends(file,fp);
    fprintf(fp,"\"$(OUTDIR)\\vtkTkImageViewerWidget.obj\" : \"%s\\imaging\\vtkTkImageViewerWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	    vals->m_WhereVTK);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\imaging\\vtkTkImageViewerWidget.cxx\"\n\n",vals->m_WhereVTK);
    }


  if (vals->m_Working)
    {
    sprintf(file,"%s\\working\\vtkTkImageWindowWidget.cxx",vals->m_WhereVTK);
    OutputDepends(file,fp);
    fprintf(fp,"\"$(OUTDIR)\\vtkTkImageWindowWidget.obj\" : \"%s\\working\\vtkTkImageWindowWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	    vals->m_WhereVTK);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\working\\vtkTkImageWindowWidget.cxx\"\n\n",vals->m_WhereVTK);
    }


  for (i = 0; i < num_abstractTcl; i++)
  {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,abstractTcl_lib[i],abstractTcl[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OUTDIR)\\%s.obj\" : \"%s\\%s\\%s.cxx\" $(DEPENDS) \n",
  	      abstractTcl[i],vals->m_WhereVTK,abstractTcl_lib[i],abstractTcl[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ2) \"%s\\%s\\%s.cxx\"\n\n",
	        vals->m_WhereVTK,abstractTcl_lib[i],abstractTcl[i]);

    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstractTcl_lib[i],abstractTcl[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\tcl\\cpp_parse.y\" \"%s\\tcl\\hints\" \"$(OUTDIR)\"\n",
		abstractTcl[i],vals->m_WhereVTK,abstractTcl_lib[i],abstractTcl[i],vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE)  \"%s\\%s\\%s.h\"\\\n",
		vals->m_WhereVTK, abstractTcl_lib[i], abstractTcl[i]);
    fprintf(fp,"  \"%s\\tcl\\hints\" 0 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, abstractTcl[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		abstractTcl[i],abstractTcl[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstractTcl[i]);

  }
  for (i = 0; i < num_concreteTcl; i++)
  {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,concreteTcl_lib[i],concreteTcl[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OUTDIR)\\%s.obj\" : \"%s\\%s\\%s.cxx\" $(DEPENDS) \n",
  	      concreteTcl[i],vals->m_WhereVTK,concreteTcl_lib[i],concreteTcl[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ2) \"%s\\%s\\%s.cxx\"\n\n",
	        vals->m_WhereVTK,concreteTcl_lib[i],concreteTcl[i]);

    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concreteTcl_lib[i],concreteTcl[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\tcl\\cpp_parse.y\" \"%s\\tcl\\hints\" \"$(OUTDIR)\"\n",
		concreteTcl[i],vals->m_WhereVTK,concreteTcl_lib[i],concreteTcl[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
		vals->m_WhereVTK, concreteTcl_lib[i], concreteTcl[i]);
    fprintf(fp,"  \"%s\\tcl\\hints\" 1 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, concreteTcl[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		concreteTcl[i],concreteTcl[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concreteTcl[i]);
  }

  fprintf(fp,"\"$(OUTDIR)\\vtktcl.obj\" : src\\vtktcl.cxx \"$(OUTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtktcl.cxx\n\n");

  for (i = 0; i < num_abstract; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\tcl\\cpp_parse.y\" \"%s\\tcl\\hints\" \"$(OUTDIR)\"\n",
		abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE)  \"%s\\%s\\%s.h\"\\\n",
		vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  \"%s\\tcl\\hints\" 0 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstract[i]);
  }

  for (i = 0; i < num_concrete; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\tcl\\cpp_parse.y\" \"%s\\tcl\\hints\" \"$(OUTDIR)\"\n",
		concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
		vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  \"%s\\tcl\\hints\" 1 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concrete[i]);
  }

  for (i = 0; i < num_abstract_h; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\tcl\\cpp_parse.y\" \"%s\\tcl\\hints\" \"$(OUTDIR)\"\n",
		abstract_h[i],vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
		vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  \"%s\\tcl\\hints\" 0 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstract_h[i]);
  }

  for (i = 0; i < num_concrete_h; i++)
  {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    OutputDepends(file,fp);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\tcl\\cpp_parse.y\" \"%s\\tcl\\hints\" \"$(OUTDIR)\"\n",
		concrete_h[i],vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
		vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  \"%s\\tcl\\hints\" 1 > src\\%sTcl.cxx\n\n",
		vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
		concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concrete_h[i]);
  }

  fprintf(fp,"################################################################################\n");
}


void doBorTclHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue, int debugFlag)
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

  if (debugFlag)
    {

    fprintf(fp,"-D_DEBUG -v -R \n");
    }
  else
    {
 fprintf(fp,"-v- -R- \n");
       }
  if (vals->m_Patented)
    {
    fprintf(fp,"-DVTK_USE_PATENTED -I%s\\patented\n",
      vals->m_WhereVTK);
    }
 fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL\n",
      vals->m_WhereVTK);
 fprintf(fp,"-tWM -tWD -Od -H- -VF -I%s\\include\\mfc;%s\\include;%s\\common;%s\\graphics -DWIN32\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK);
 fprintf(fp," -I%s\\pcmaker\\xlib \n",vals->m_WhereVTK);
 fprintf(fp," -I%s\\imaging \n",vals->m_WhereVTK);
 fprintf(fp," -I%s\\contrib \n",vals->m_WhereVTK);
  if (doAddedValue) fprintf(fp," -I%s\\gemsio -I%s\\gemsip -I%s\\gemsvolume -I%s\\geae \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  fprintf(fp,"-P -c -w-hid -w-inl \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n"); 
  fprintf(fp,"LINK32=tlink32.exe\n\n");

  fprintf(fp,"LINK32_FLAGS=-L%s\\lib;..\\vtkdll\\obj\\vtkdll.lib;%s\\pcmaker\\tk42.lib;%s\\pcmaker\\tcl76.lib \\\n",
  vals->m_WhereCompiler,vals->m_WhereCompiler,vals->m_WhereCompiler);
  if (debugFlag)
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


// links it ALL the vtk libraries (now that they are split up)... also writes output to ../lib directory
void doMSCJavaHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue, int debugFlag)
{
  int i;
  char file[256];

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"OUTDIR=obj\n\n");

	fprintf(fp,"LIBDIR=..\\lib\n\n");

  fprintf(fp,"ALL : vtkjava.dll\n\n");

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");

  if (debugFlag)
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /D \"_DEBUG\" /nologo /MDd /GX /Od /Zi /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /nologo /MD /GX /O2 /I \"%s\\include\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" /D\\\n",
      vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  if (vals->m_Patented)
    {
    fprintf(fp," \"_WINDOWS\" /D \"VTK_USE_PATENTED\" /I \"%s\\patented\" /D \"_WINDLL\" /D \"_MBCS\" \\\n",
      vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp," \"_WINDOWS\" /D \"_WINDLL\" /D \"_MBCS\" \\\n");
    }
  if (doAddedValue) fprintf(fp," /I \"%s\\working\" /I \"%s\\gemsio\" /I \"%s\\gemsip\" /I \"%s\\gemsvolume\" /I \"%s\\geae\" \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  fprintf(fp,"/I \"%s\\include\" /I \"%s\\include\\win32\" /Fo\"$(OUTDIR)/\" /c \n",
    vals->m_WhereJDK, vals->m_WhereJDK);
  fprintf(fp,"LINK32=link.exe\n");
  if (debugFlag)
    {
    fprintf(fp,"LINK32_FLAGS=/debug /libpath:\"%s\\lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows\n",
	    vals->m_WhereCompiler, vals->m_WhereCompiler);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=/libpath:\"%s\\lib\"  \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows\n",
	    vals->m_WhereCompiler, vals->m_WhereCompiler);
    }

  fprintf(fp,"MORE_FLAGS1=/dll /incremental:yes /pdb:\"$(LIBDIR)/vtkjava.pdb\" /machine:I386\\\n");
  fprintf(fp," /out:\"$(LIBDIR)/vtkjava.dll\" /implib:\"$(LIBDIR)/vtkjava.lib\" \n"); 
  fprintf(fp,"MORE_FLAGS2=/dll /incremental:no /pdb:vtktcl.pdb /machine:I386\\\n");
  fprintf(fp," /out:vtkjava.dll /implib:vtkjava.lib \n"); 

  fprintf(fp,"VTKDLL_LIB=..\\vtkdll\\vtkdll.lib \n");

  fprintf(fp,"VTK_LIBRARIES=..\\lib\\vtkCommon.lib ");
  if (vals->m_Graphics) 
    {
    for (i = 0; i < NumOfGraphicsLibs; i++)
      fprintf(fp,"..\\lib\\vtkGraphics%d.lib  ",i);
    }
  else if (vals->m_Patented)
    fprintf(fp,"..\\lib\\vtkPatented.lib ");
  if (vals->m_Imaging) 
    fprintf(fp,"..\\lib\\vtkImaging.lib ");
  if (vals->m_Contrib)
    fprintf(fp,"..\\lib\\vtkContrib.lib ");
  if (vals->m_Working)
    fprintf(fp,"..\\lib\\vtkWorking.lib ");
  if (vals->m_GEMSIP)
    fprintf(fp,"..\\lib\\vtkGemsip.lib ");
  if (vals->m_GEAE)
    fprintf(fp,"..\\lib\\vtkGeae.lib ");
  if (vals->m_GEMSVOLUME)
    fprintf(fp,"..\\lib\\vtkGemsVolume.lib ");
     
    fprintf(fp,"\n\n");

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
  fprintf(fp,"vtkjava.dll : $(DEF_FILE) $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(MORE_FLAGS2) $(LINK32_OBJS) $(VTKDLL_LIB)\n");
  fprintf(fp,"<<\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(LIBDIR)\\vtkjava.dll\" : $(DEF_FILE) $(LINK32_OBJS)\n");
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(MORE_FLAGS1) $(LINK32_OBJS) $(VTK_LIBRARIES)\n");
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
    OutputDepends(file,fp);
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
    OutputDepends(file,fp);
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
    OutputDepends(file,fp);
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
    OutputDepends(file,fp);
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


void doBorJavaHeader(FILE *fp,CPcmakerDlg *vals, int doAddedValue, int debugFlag)
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

  if (debugFlag)
    {

    fprintf(fp,"-D_DEBUG -v -R \n");
    }
  else
    {
 fprintf(fp,"-v- -R- \n");
       }
  if (vals->m_Patented)
    {
    fprintf(fp,"-DVTK_USE_PATENTED -I%s\\patented\n",
      vals->m_WhereVTK);
    }
 fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL;VTKJAVA\n",
      vals->m_WhereVTK);
 fprintf(fp,"-tWM -tWD -Od -H- -VF -I%s\\include\\mfc;%s\\include;%s\\common;%s\\graphics -DWIN32\n",
      vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereVTK, vals->m_WhereVTK);
 fprintf(fp," -I%s\\include \n",vals->m_WhereJDK);
 fprintf(fp," -I%s\\include\\win32 \n",vals->m_WhereJDK);
 fprintf(fp," -I%s\\imaging \n",vals->m_WhereVTK);
 fprintf(fp," -I%s\\contrib \n",vals->m_WhereVTK);
  if (doAddedValue) fprintf(fp," -I%s\\gemsio -I%s\\gemsip -I%s\\gemsvolume -I%s\\geae \\\n",
    vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  fprintf(fp,"-P -c -w-hid -w-inl \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n");
  fprintf(fp,"LINK32=tlink32.exe\n\n");

  fprintf(fp,"LINK32_FLAGS=-L%s\\lib \\\n",
  vals->m_WhereCompiler);
  if (debugFlag)
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



// all files which will have OutputDepends called for should be "setup" first in thsi function
void SetupDepends(CPcmakerDlg *vals, int debugFlag)
{
  char temp[256], file[256];
  int i;


  for (i = 0; i < 2; i++)
    {
    if ( i == 0 )
      {
      if (debugFlag==0 || debugFlag==2)
        sprintf(temp,"%s\\vtkdll\\src",vals->m_WhereBuild);
      else 
        continue;
      }
    else if (debugFlag > 0)
      sprintf(temp,"%s\\Debug\\vtkdll\\src",vals->m_WhereBuild);
    else 
      continue;

    sprintf(file,"%s\\vtkPCForceCommon.cxx",temp);
    AddToDepends(file);

    if (vals->m_Graphics)
      {
      sprintf(file,"%s\\vtkPCForceGraphics.cxx",temp);
      AddToDepends(file);
      }
    else if (vals->m_Patented)
      {
      sprintf(file,"%s\\vtkPCForcePatented.cxx",temp);
      AddToDepends(file);
      }
    if (vals->m_Imaging)
      {
      sprintf(file,"%s\\vtkPCForceImaging.cxx",temp);
      AddToDepends(file);
      }
    if (vals->m_GEMSIP)
      {
      sprintf(file,"%s\\vtkPCForceGemsip.cxx",temp);
      AddToDepends(file);
      }
    if (vals->m_GEAE)
      {
      sprintf(file,"%s\\vtkPCForceGeae.cxx",temp);
      AddToDepends(file);
      }
    if (vals->m_GEMSVOLUME)
      {
      sprintf(file,"%s\\vtkPCForceGemsVolume.cxx",temp);
      AddToDepends(file);
      }
    if (vals->m_Working)
      {
      sprintf(file,"%s\\vtkPCForceWorking.cxx",temp);
      AddToDepends(file);
      }
    if (vals->m_Contrib)
      {
      sprintf(file,"%s\\vtkPCForceContrib.cxx",temp);
      AddToDepends(file);
      }
    }

  for (i = 0; i < num_abstract; i++)
    {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    AddToDepends(file);
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    AddToDepends(file);
    }
  for (i = 0; i < num_concrete; i++)
    {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    AddToDepends(file);
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    AddToDepends(file);
    }
  for (i = 0; i < num_abstractTcl; i++)
    {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,abstractTcl_lib[i],abstractTcl[i]);
    AddToDepends(file);
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstractTcl_lib[i],abstractTcl[i]);
    AddToDepends(file);
    }
  for (i = 0; i < num_concreteTcl; i++)
    {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,concreteTcl_lib[i],concreteTcl[i]);
    AddToDepends(file);
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concreteTcl_lib[i],concreteTcl[i]);
    AddToDepends(file);
    }
  for (i = 0; i < num_abstract_h; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    AddToDepends(file);
    }
  for (i = 0; i < num_concrete_h; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    AddToDepends(file);
    }

  if (vals->m_Graphics)
    {
    sprintf(file,"%s\\graphics\\vtkTkRenderWidget.cxx",vals->m_WhereVTK);
    AddToDepends(file);
    }
  if (vals->m_Imaging)
    {
    sprintf(file,"%s\\imaging\\vtkTkImageViewerWidget.cxx",vals->m_WhereVTK);
    AddToDepends(file);
    }


  if (vals->m_Working)
    {
    sprintf(file,"%s\\working\\vtkTkImageWindowWidget.cxx",vals->m_WhereVTK);
    AddToDepends(file);
    }


  // now get split information for the graphics library(ies)
  if (vals->m_Graphics)
    SetupSplitGraphicsDepends(vals);

  BuildDepends(vals);
}



void SetupSplitGraphicsDepends(CPcmakerDlg *vals)
{
  char file[256], *ptr;
  int i, j, total=0;

  if (vals->m_Graphics)
    {
    sprintf(file,"%s\\vtkdll\\src\\vtkPCForceGraphics.cxx",vals->m_WhereBuild);
    AddToGLibDepends(file);
    total++;
    }

  for (i = abstractStart[LT_GRAPHICS]; i < abstractEnd[LT_GRAPHICS]; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    AddToGLibDepends(file);
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    AddToGLibDepends(file);
    total++;
    }
  for (i = concreteStart[LT_GRAPHICS]; i < concreteEnd[LT_GRAPHICS]; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    AddToGLibDepends(file);
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    AddToGLibDepends(file);
    total++;
    }

  BuildGLibDepends(vals);

  int *classSet = new int [total];

//  FILE *fp = fopen("GraphicsSplit.txt","w");
  int numInLibrary, numInSet, numNotInLibrary = total;

  // may need to change how this works if library makeup changes considerably 
  // (in other words, maybe take smallest first instead of biggest)

  NumOfGraphicsLibs = 0;
  while ( numNotInLibrary)
    {
    numInLibrary =0;
    // build a library to have roughly 50... avoid very small library
    while( numNotInLibrary && ( numInLibrary < 50 || numNotInLibrary < 20) )
      {
      numInSet = GetGraphicsSplit(&classSet[numInLibrary]);

/*      for (i=0; i<numInSet; i++)
        fprintf(fp,"   %s\n",GLibDependsArray[ classSet[i] ]->name);*/
      numInLibrary += numInSet;
      numNotInLibrary -= numInSet;
      }
//    fprintf(fp,"\n\n");

    SplitGraphicsIndices[NumOfGraphicsLibs] = new int [numInLibrary];
    for (i = 0; i < numInLibrary; i++)
      {
      SplitGraphicsIndices[NumOfGraphicsLibs][i] = classSet[i];
       
      // remove directory info and extension
      ptr = strrchr( GLibDependsArray[ classSet[i] ]->name, '\\' );
      strncpy( file, ptr+1, strlen(ptr+1) - 4);
      file[ strlen(ptr+1) - 4 ] = NULL;
      strcpy( GLibDependsArray[ classSet[i] ]->name, file);
      }

    NumInSplitLib[NumOfGraphicsLibs] = numInLibrary;
    NumOfGraphicsLibs++;
    }
//  fclose(fp);

  delete [] classSet;

	// now compare to file on disk to decide whether or not to delete dlls
  struct stat statBuff;

	// see if file even exists	
  sprintf(file,"%s\\GraphicsSplitInfo.txt",vals->m_WhereBuild);
  if (!stat(file,&statBuff)) // does exist, so read the info
    {
    ifstream IS(file);
    int numLibs, numInLib, any_changed, changed;
    char filename[256];

    any_changed = 0;
    IS >> numLibs;
    for (i = 0; i < numLibs; i++)
      {
      changed = 0;
      IS >> numInLib;
			
      if (i < NumOfGraphicsLibs)
        {
        for (j = 0; j < numInLib; j++)
          {
          IS >> filename;
          if ( j < NumInSplitLib[i] &&
            strcmp(filename, GLibDependsArray[ SplitGraphicsIndices[i][j] ]->name) )
            changed = 1;
          }
        if (numInLib != NumInSplitLib[i])
          changed = 1;
        }
      else
        changed = 1;
      if (changed) // delete the library
        {
        any_changed = 1;
        for (j = 0; j < 2; j++)
          {
          if (j)
            sprintf(file,"%s\\lib\\vtkGraphics%d.dll",vals->m_WhereBuild,i);
          else
            sprintf(file,"%s\\Debug\\lib\\vtkGraphics%d.dll",vals->m_WhereBuild,i);
          }
        if (!stat(file,&statBuff)) // does exist, so delete it
          {
          if ( remove(file) )
            {	
            char msg[256];
            sprintf(msg,"ERROR!!!!! Unable to remove %s\n",file);
            AfxMessageBox(msg);
            exit(1);
            }
          }
        }
      }
    if (any_changed || numLibs != NumOfGraphicsLibs) // delete vtktcl.dll 's
      {
      for (j = 0; j < 2; j++);
        {
        if (j)
          sprintf(file,"%s\\lib\\vtktcl.dll",vals->m_WhereBuild);
        else
          sprintf(file,"%s\\Debug\\lib\\vtktcl.dll",vals->m_WhereBuild);
				
        if (!stat(file,&statBuff)) // does exist, so delete it
          {
          if ( remove(file) )
            {	
            char msg[256];
            sprintf(msg,"ERROR!!!!! Unable to remove %s\n",file);
            AfxMessageBox(msg);
            exit(1);
            }
          }
        }
			
      // do not rewrite the file if unchanged
      writeGraphicsSplitInfo(vals);
      }
    }
  else
    {
    // check for 40 Graphics libraries (that is a lot!)
    for (j = 0; j < 2; j++)
      {
      for (i = 0; i < 41; i++)
        {
        if (i == 40) // handle the tcl dll
          {
          if (j)
            sprintf(file,"%s\\lib\\vtktcl.dll",vals->m_WhereBuild);
          else
            sprintf(file,"%s\\Debug\\lib\\vtktcl.dll",vals->m_WhereBuild);
          }
        else
          {
          if (j)
            sprintf(file,"%s\\lib\\vtkGraphics%d.dll",vals->m_WhereBuild,i);
          else
            sprintf(file,"%s\\Debug\\lib\\vtkGraphics%d.dll",vals->m_WhereBuild,i);
          }
        if (!stat(file,&statBuff)) // does exist, so delete it
          {
          if ( remove(file) )
            {	
            char msg[256];
            sprintf(msg,"ERROR!!!!! Unable to remove %s\n",file);
            AfxMessageBox(msg);
            exit(1);
            }
          }
        }
      }
    writeGraphicsSplitInfo(vals);
    }
  }



void writeGraphicsSplitInfo(CPcmakerDlg *vals)
	{
	int i, j;
	char file[300];
  ofstream *OS;
	
  sprintf(file,"%s\\GraphicsSplitInfo.txt",vals->m_WhereBuild);
	OS = new ofstream(file);

	*OS << NumOfGraphicsLibs << endl;

	for (i = 0; i < NumOfGraphicsLibs; i++)
		{
    *OS << NumInSplitLib[i] << endl; // number in lib
		for (j = 0; j < NumInSplitLib[i]; j++)
			*OS << GLibDependsArray[ SplitGraphicsIndices[i][j] ]->name << endl;
		}
	OS->close();
	delete OS;
	}
