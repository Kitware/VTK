/*=========================================================================

  Program:   Visualization Toolkit
  Module:    getclasses.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "stdafx.h"
#include "pcmaker.h"
#include "pcmakerDlg.h"
#include <direct.h>
#include <fstream.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


// where to split the graphics library: A-O and P-Z
#define LIB_SPLIT_STRING "vtkp" 


// LT_COMMON must be last in this list
enum LibraryTypes {LT_GRAPHICS, LT_IMAGING, LT_PATENTED, LT_CONTRIB, LT_LOCAL, LT_COMMON};


extern void CheckAndAddToDepends(char *file, const char *vtkHome, 
                                 char *extraPtr[], int extra_num);
extern char **ReturnDepends(int &num);
extern void OutputPCDepends(char *file, FILE *fp, const char *vtkHome, 
			    char *extraPtr[], int extra_num);
extern void AddToGLibDepends(char *file);
extern void BuildGLibDepends(CPcmakerDlg *vals);
extern int GetGraphicsSplit(int classSet[]);
void makeIncrementalMakefiles(CPcmakerDlg *vals, int debugFlag);
void makeNonIncrementalMakefile(CPcmakerDlg *vals, int debugFlag);
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




char *extraPtr[100];
int extra_num;




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

int bbuilder = 0; // JCP 0 if borland 5.0x, 1 if borland builder


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
  while (strcmp(line,"WIN32_OBJ") && strcmp(line,"@MAKEINCLUDE@"))
    {
    if ((line[0] == 'v')&&(line[1] == 't'))
      {
      concrete[num_concrete] = strdup(line);
      concrete_lib[num_concrete] = libname;
      num_concrete++;
      }
    *IS >> line;
    }

  // look for win32 classes
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
void CreateToolkitsH(CPcmakerDlg *vals)
{
  char *fname = new char [strlen(vals->m_WhereBuild)+100];
  FILE *ofp;
  
  // create the vtkToolkits.h file
  sprintf(fname,"%s\\vtkToolkits.h",vals->m_WhereBuild);
  ofp = fopen(fname,"w");
  if (ofp)
    {
    fprintf(ofp,"/* generated file from pcmaker.exe */\n");
    if (vals->m_Patented) 
      {
      fprintf(ofp,"#define VTK_USE_PATENTED 1\n");
      }
    if (vals->m_Local) 
      {
      fprintf(ofp,"#define VTK_USE_LOCAL 1\n");
      }
    if (vals->m_Contrib) 
      {
      fprintf(ofp,"#define VTK_USE_CONTRIB 1\n");
      }
    if (vals->m_Imaging) 
      {
      fprintf(ofp,"#define VTK_USE_IMAGING 1\n");
      }
    if (vals->m_Graphics) 
      {
      fprintf(ofp,"#define VTK_USE_GRAPHICS 1\n");
      }
    if (vals->adlg.m_UseMPI) 
      {
      fprintf(ofp,"#define VTK_USE_MPI 1\n");
      }
    fclose(ofp);    
    }
  else
    {
    char msg[256];
    sprintf(msg,"ERROR!!!!! Unable to create vtkToolkits.h\n");
    AfxMessageBox(msg);
    exit(1);
    }

  // create the vtkConfigure.h file only if it isn't already there
  sprintf(fname,"%s\\vtkConfigure.h",vals->m_WhereBuild);
  ofp = fopen(fname,"r");
  const char* AnsiDefine = "#define VTK_USE_ANSI_STDLIB\n";
  bool writeFilep = true;
  if(ofp)
    {
    char lineIn[1024];
    fgets(lineIn, 1024, ofp);
    // if the second line is the ansi flag, and ansi is on, then do not
    // write the file again, as it needs no change
    fgets(lineIn, 1024, ofp);
    if(vals->m_AnsiCpp)
      {
      if(strcmp(lineIn, AnsiDefine) == 0)
        {
        writeFilep = false;
        }
      }
    fclose(ofp);
    }
  if (!ofp || writeFilep)
    {
    ofp = fopen(fname,"w");
    if (ofp)
      {
      fprintf(ofp,"/* generated file from pcmaker.exe */\n");
      if(vals->m_AnsiCpp)
        {
        fprintf(ofp,"%s", AnsiDefine);
        }
      fclose(ofp);    
      }
    else
      {
      char msg[256];
      sprintf(msg,"ERROR!!!!! Unable to create vtkConfigure.h\n");
      AfxMessageBox(msg);
      exit(1);
      }
    }
}


// order of the files is important for doing the incremental build
void removeExtraFiles(CPcmakerDlg *vals)
{
  int i, j, k;
  int numReq, oldReq, required;
  ifstream *IS;
  char aFile[300];
  char **reqList;
  
  // does a reqlist exist ?
  FILE *fp = fopen("C:/reqlist.txt","r");
  if (!fp)
    {
    return;
    }
  fclose(fp);
  
  // first load the list of required files
  IS = new ifstream("C:/reqlist.txt");
  numReq = 0;
  while (!IS->eof() && !IS->fail())
    {
    *IS >> aFile;
    CheckAndAddToDepends(aFile, vals->m_WhereVTK, extraPtr, extra_num);
    }

  // find this entry in DependsStructArray
  oldReq = 0;
  reqList = ReturnDepends(numReq);

  // iterate to collect extras
  while (oldReq != numReq) 
    {
    oldReq = numReq;
    for (i = 0; i < numReq; i++)
      {
      sprintf(aFile,"%s.cxx",reqList[i]);
      CheckAndAddToDepends(aFile, vals->m_WhereVTK, extraPtr, extra_num);
      }
    // find this entry in DependsStructArray
    reqList = ReturnDepends(numReq);
    }

  // for each file
  for (i = 0; i < num_concrete; i++)
    {
    // is it a required file ?
    required = 0;
    for (k = 0; k < numReq; k++)
      {
      if (!strcmp(concrete[i],reqList[k]))
        {
        required = 1;
        }
      }
    if (!required)
      {
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
    // is it a required file ?
    required = 0;
    for (k = 0; k < numReq; k++)
      {
      if (!strcmp(abstract[i],reqList[k]))
        {
        required = 1;
        }
      }
    if (!required)
      {
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
  for (i = 0; i < num_concrete_h; i++)
    {
    // is it a required file ?
    required = 0;
    for (k = 0; k < numReq; k++)
      {
      if (!strcmp(concrete_h[i],reqList[k]))
        {
        required = 1;
        }
      }
    if (!required)
      {
      if (i < num_concrete_h - 1)
        {
        memmove((void *)(concrete_h + i), (void *)(concrete_h + i + 1), sizeof(char*)*(num_concrete_h - i - 1) );
        memmove((void *)(concrete_h_lib + i), (void *)(concrete_h_lib + i + 1), sizeof(char*)*(num_concrete_h - i - 1) );
        }
      
      for ( j = 0; j <= LT_COMMON; j++)
        {
        if (concreteHStart[j] > i )
          concreteHStart[j]--;
        if (concreteHEnd[j] > i )
          concreteHEnd[j]--;
        }
      num_concrete_h--;
      i--;
      }
    }
  for (i = 0; i < num_abstract_h; i++)
    {
    // is it a required file ?
    required = 0;
    for (k = 0; k < numReq; k++)
      {
      if (!strcmp(abstract_h[i],reqList[k]))
        {
        required = 1;
        }
      }
    if (!required)
      {
      if (i < num_abstract_h - 1)
        {
        memmove((void *)(abstract_h + i), (void *)(abstract_h + i + 1), sizeof(char*)*(num_abstract_h - i - 1) );
        memmove((void *)(abstract_h_lib + i), (void *)(abstract_h_lib + i + 1), sizeof(char*)*(num_abstract_h - i - 1) );
        }
      for ( j = 0; j <= LT_COMMON; j++)
        {
        if (abstractHStart[j] > i )
          abstractHStart[j]--;
        if (abstractHEnd[j] > i )
          abstractHEnd[j]--;
        }
      num_abstract_h--;
      i--;
      }
    }
}

/* warning this code is also in getclasses.cxx under pcmaker */
void stuffitPython(FILE *fp, CPcmakerDlg *vals)
{
  int i;
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(fp,"extern  \"C\" {__declspec( dllexport) PyObject *PyVTKClass_%sNew(char *); }\n",names[i]);
    }

  fprintf(fp,"\nstatic PyMethodDef Py%s_ClassMethods[] = {\n",
	  kitName);
  fprintf(fp,"{NULL, NULL}};\n\n");
  
  fprintf(fp,"extern  \"C\" {__declspec( dllexport) void init%s();}\n\n",kitName);


  /* module init function */
  fprintf(fp,"void init%s()\n{\n",kitName);
  fprintf(fp,"  PyObject *m, *d, *c;\n\n");
  fprintf(fp,"  static char modulename[] = \"%s\";\n",kitName);
  fprintf(fp,"  m = Py_InitModule(modulename, Py%s_ClassMethods);\n",
	  kitName);
  
  fprintf(fp,"  d = PyModule_GetDict(m);\n");
  fprintf(fp,"  if (!d) Py_FatalError(\"can't get dictionary for module %s!\");\n\n",
	  kitName);

  for (i = 0; i < anindex; i++)
    {
    fprintf(fp,"  if ((c = PyVTKClass_%sNew(modulename)))\n",names[i]);
    fprintf(fp,"    if (-1 == PyDict_SetItemString(d, \"%s\", c))\n",
	    names[i]);
    fprintf(fp,"      Py_FatalError(\"can't add class %s to dictionary!\");\n\n",
	    names[i]);
    }
  fprintf(fp,"}\n\n");
};


void MakePythonInit(char *fname, char *argv1, CPcmakerDlg *vals)
{
  int i;
  FILE *fp;

  kitName = strdup(argv1);
 
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
  for (i = 0; i < num_abstract; i++)
    {
    names[anindex++] = abstract[i];
    }
  for (i = 0; i < num_abstract_h; i++)
    {
    names[anindex++] = abstract_h[i];
    }  
  
  fp = fopen(fname,"w");
  if (fp)
    {
    fprintf(fp,"#include <string.h>\n");
    fprintf(fp,"#include \"Python.h\"\n\n");
    stuffitPython(fp,vals);
    fclose(fp);
    }
}

// warning this code is also in kit_init.cxx under tcl
void stuffitTcl(FILE *fp, CPcmakerDlg *vals)
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
    // fprintf(fp,"\nTcl_Interp *vtkGlobalTclInterp;\n");
    }
  fprintf(fp,"extern void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);\n");
  fprintf(fp,"\n\nextern \"C\" {__declspec(dllexport) int %s_SafeInit(Tcl_Interp *interp);}\n\n",
	  kitName);
  fprintf(fp,"\n\nextern \"C\" {__declspec(dllexport) int %s_Init(Tcl_Interp *interp);}\n\n",
	  kitName);

  /* the main declaration */
  fprintf(fp,"\n\nint %s_SafeInit(Tcl_Interp *interp)\n{\n",kitName);
  fprintf(fp,"  return %s_Init(interp);\n}\n",kitName);

  /* prototype for tkRenderWidget */
  if (vals->m_Graphics) fprintf(fp,"extern \"C\" {int Vtktkrenderwidget_Init(Tcl_Interp *interp);}\n\n");
  if (vals->m_Imaging) 
    {
    fprintf(fp,"extern \"C\" {int Vtktkimageviewerwidget_Init(Tcl_Interp *interp);}\n\n");
    fprintf(fp,"extern \"C\" {int Vtktkimagewindowwidget_Init(Tcl_Interp *interp);}\n\n");
    }

  fprintf(fp,"\n\nint %s_Init(Tcl_Interp *interp)\n{\n",kitName);
  if (!strcmp(kitName,"Vtktcl"))
    {
    fprintf(fp,
	    "  vtkTclInterpStruct *info = new vtkTclInterpStruct;\n");
    fprintf(fp,
            "  info->Number = 0; info->InDelete = 0; info->DebugOn = 0;\n");
    fprintf(fp,
            "\n");
    fprintf(fp,
            "\n");
    // claw: I am adding this to allow c++ to evaluate tcl commands.
//    fprintf(fp,
//	    "  vtkGlobalTclInterp = interp;\n");
    fprintf(fp,
	    "  Tcl_InitHashTable(&info->InstanceLookup, TCL_STRING_KEYS);\n");
    fprintf(fp,
	    "  Tcl_InitHashTable(&info->PointerLookup, TCL_STRING_KEYS);\n");
    fprintf(fp,
	    "  Tcl_InitHashTable(&info->CommandLookup, TCL_STRING_KEYS);\n");
    fprintf(fp,
            "  Tcl_SetAssocData(interp,\"vtk\",NULL,(ClientData *)info);\n");

    /* create special vtkCommand command */
    fprintf(fp,"  Tcl_CreateCommand(interp,\"vtkCommand\",vtkCommand,\n		    (ClientData *)info, NULL);\n\n");


    /* add a class exit method to the vtkWin32RenderWindowInteractor to do the
       right thing with tcl and c++ */
    if (vals->m_Graphics) 
      {
      fprintf(fp, 
	      "  vtkWin32RenderWindowInteractor\n  \t::SetClassExitMethod((void (*)(void *))Tcl_Exit, 0);\n\n");
      }


    /* initialize the tkRenderWidget */
    if (vals->m_Graphics) fprintf(fp,"  Vtktkrenderwidget_Init(interp);\n");
    if (vals->m_Imaging) 
      {
      fprintf(fp,"  Vtktkimageviewerwidget_Init(interp);\n");
      fprintf(fp,"  Vtktkimagewindowwidget_Init(interp);\n");
      }
    }
  
  for (i = 0; i < anindex; i++)
    {
    fprintf(fp,"  vtkTclCreateNew(interp,\"%s\", %sNewCommand,\n",
	    names[i], names[i]);
    fprintf(fp,"                  %sCommand);\n",names[i]);
    }

  fprintf(fp,"  return TCL_OK;\n}\n");
}




void MakeTclInit(char *fname, char *argv1, CPcmakerDlg *vals)
{
  int i;
  FILE *fp;


  /* we have to make sure that the name is the correct case */
  kitName = strdup(argv1);
  if (kitName[0] > 90) kitName[0] -= 32;
  for (i = 1; i < (int)strlen(kitName); i++)
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
  
  fp = fopen(fname,"w");
  if (fp)
    {
    fprintf(fp,"#include \"vtkTclUtil.h\"\n");
    fprintf(fp,"#include \"vtkWin32RenderWindowInteractor.h\"\n\n");
    stuffitTcl(fp,vals);
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
void doBorHeader(FILE *fp, CPcmakerDlg *vals, int debugFlag);
void doMSCTclHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag);
void doBorTclHeader(FILE *fp, CPcmakerDlg *vals, int debugFlag);
void doMSCJavaHeader(FILE *fp, CPcmakerDlg *vals, int debugFlag);
void doBorJavaHeader(FILE *fp, CPcmakerDlg *vals, int debugFlag);
void doMSCPythonHeader(FILE *fp, CPcmakerDlg *vals, int debugFlag);
void doBorPythonHeader(FILE *fp, CPcmakerDlg *vals, int debugFlag);


// generate depend info for a .cxx file
// outputs the result as DEPEND


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




void ReadMakefiles(CPcmakerDlg *vals)
{
  char fname[256];


  // also adds to extraPtr and extra_num
  extra_num = 1;
  extraPtr[0] = strdup(vals->m_WhereBuild);

  // do graphics first (if present) and LT_COMMON last for
  // doing split obj.lib's in Tcl and Jave
  if (vals->m_Graphics) 
    {
    UpdateStart(LT_GRAPHICS);
    sprintf(fname,"%s\\graphics\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("graphics"));
    UpdateEnd(LT_GRAPHICS);
    }
  
  if (vals->m_Patented)
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
    UpdateEnd(LT_IMAGING);
    }
  if (vals->m_Contrib)
    {
    UpdateStart(LT_CONTRIB);
    sprintf(fname,"%s\\contrib\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("contrib"));
    if (vals->adlg.m_UseMPI)
      {
      concrete[num_concrete] = strdup("vtkMPIController");
      concrete_lib[num_concrete] = strdup("contrib");
      num_concrete++;
      }
    UpdateEnd(LT_CONTRIB);
    }
  if (vals->m_Local)
    {
    UpdateStart(LT_LOCAL);
    sprintf(fname,"%s\\local\\Makefile.in",vals->m_WhereVTK);
    readInMakefile(fname,strdup("local"));
    UpdateEnd(LT_LOCAL);
    }

  UpdateStart(LT_COMMON);
  sprintf(fname,"%s\\common\\Makefile.in",vals->m_WhereVTK);
  readInMakefile(fname,strdup("common"));
  UpdateEnd(LT_COMMON);
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
  if (vals->m_Patented)
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
  if (vals->m_Local)
    {
    sprintf(fname,"%s\\Debug\\vtkdll\\src\\vtkPCForceLocal.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_LOCAL);
    sprintf(fname,"%s\\vtkdll\\src\\vtkPCForceLocal.cxx",vals->m_WhereBuild);
    MakeForce(fname,LT_LOCAL);
    }

  if (vals->m_BuildPython)
    {
    // we must create CommonInit.cxx etc
    sprintf(fname,"%s\\Debug\\vtkpython\\src\\vtkpython.cxx",vals->m_WhereBuild);
    MakePythonInit(fname,"vtkpython",vals);
    sprintf(fname,"%s\\vtkpython\\src\\vtkpython.cxx",vals->m_WhereBuild);
    MakePythonInit(fname,"vtkpython",vals);
    }

  if (vals->m_BuildTcl)
    {
    // we must create CommonInit.cxx etc
    sprintf(fname,"%s\\Debug\\vtktcl\\src\\vtktcl.cxx",vals->m_WhereBuild);
    MakeTclInit(fname,"Vtktcl",vals);
    sprintf(fname,"%s\\vtktcl\\src\\vtktcl.cxx",vals->m_WhereBuild);
    MakeTclInit(fname,"Vtktcl",vals);
    }
}




// only call if m_MSComp
void makeMakefiles(CPcmakerDlg *vals)
{
  int total;
  FILE *ifp;   // JCP physical check for Borland Builder
  char fname[128];


  // create vtkToolkits.h
  CreateToolkitsH(vals);
  
  if (vals->m_BorlandComp)
    { 
    // Determine whether this is 5.0x or builder (3.0 we hope...)
    sprintf(fname,"%s\\bin\\ilink32.exe",vals->m_WhereCompiler);
    ifp = fopen(fname,"r");
    if (ifp)
      {
      bbuilder = 1;  
     fclose(ifp);
    sprintf(fname,"%s\\lib\\release\\vcl35.lib",vals->m_WhereCompiler);
    ifp = fopen(fname,"r");
    if (ifp)
      {
      bbuilder = 3;  
     fclose(ifp);

      } else {
    	sprintf(fname,"%s\\lib\\release\\vcl40.lib",vals->m_WhereCompiler);
    	ifp = fopen(fname,"r");
    	if (ifp)
    	  {
    	  bbuilder = 4;  
    	 fclose(ifp);
    		  }
         }
     }                            
    }        // JCP end of physical check for Borland Builder. 

  ReadMakefiles(vals);
  removeExtraFiles(vals);
  CreateRequiredFiles(vals);


  // set up the progress indicator... total is approximate
  // 1st for computing depends....
  total = 2 * (1 + vals->m_Graphics + vals->m_Imaging + vals->m_Contrib +
	       vals->m_Local + 
               num_concrete + num_abstract + num_abstract_h + num_concrete_h);


  // extra for split graphics stuff
  if (vals->m_Graphics)
    total += 5 * 2 * (concreteEnd[LT_GRAPHICS] + abstractEnd[LT_GRAPHICS] -
		      concreteStart[LT_GRAPHICS] - abstractStart[LT_GRAPHICS]);

  if (vals->m_BuildTcl)
    {
    total += 2 * (num_concrete + num_abstract + num_abstract_h + num_concrete_h);
    } 
  if (vals->m_BuildJava && strlen(vals->m_WhereJDK) > 1)
    {
    total += 2 * (num_concrete + num_abstract + num_abstract_h + num_concrete_h);
    } 
  if (vals->m_BuildPython)
    {
    total += 2 * (num_concrete + num_abstract + num_abstract_h + num_concrete_h);
    } 
  vals->m_Progress.SetRange(0,total);
  vals->m_Progress.SetPos(0);


  if (vals->m_Graphics && vals->m_MSComp)
    SetupSplitGraphicsDepends(vals);


  // tcl and/or java makefile made (called form) makeNonIncrementalMakefile()
  if (vals->m_MSComp)
    {
    makeIncrementalMakefiles(vals,0);  // non-debug
    makeIncrementalMakefiles(vals,1);  // debug
    }
  makeNonIncrementalMakefile(vals,0);  // non-debug
  makeNonIncrementalMakefile(vals,1);  // debug
}




void makeNonIncrementalMakefile(CPcmakerDlg *vals, int debugFlag)
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
    doBorHeader(ofp, vals, debugFlag);
    fclose(ofp);
    }


  if (vals->m_BuildTcl)
    {
    if ( debugFlag )
      sprintf(fname,"%s\\Debug\\vtktcl\\makefile",vals->m_WhereBuild);
    else
      sprintf(fname,"%s\\vtktcl\\makefile",vals->m_WhereBuild);
    ofp = fopen(fname,"w");
    if (vals->m_MSComp) doMSCTclHeader(ofp, vals, debugFlag);
    if (vals->m_BorlandComp) doBorTclHeader(ofp, vals, debugFlag);
    fclose(ofp);
    }

  // generate the java makefiles if requested
  if (vals->m_BuildJava && strlen(vals->m_WhereJDK) > 1)
    {
    if ( debugFlag )
      sprintf(fname,"%s\\Debug\\vtkjava\\makefile",vals->m_WhereBuild);
    else
      sprintf(fname,"%s\\vtkjava\\makefile",vals->m_WhereBuild);
    ofp = fopen(fname,"w");
    if (vals->m_MSComp ) doMSCJavaHeader(ofp, vals, debugFlag);
    if (vals->m_BorlandComp) doBorJavaHeader(ofp, vals, debugFlag);
    fclose(ofp);
    }

  // generate the python makefiles if requested
  if (vals->m_BuildPython)
    {
    if ( debugFlag )
      sprintf(fname,"%s\\Debug\\vtkpython\\makefile",vals->m_WhereBuild);
    else
      sprintf(fname,"%s\\vtkpython\\makefile",vals->m_WhereBuild);
    ofp = fopen(fname,"w");
    if (vals->m_MSComp ) doMSCPythonHeader(ofp, vals, debugFlag);
    if (vals->m_BorlandComp) doBorPythonHeader(ofp, vals, debugFlag);
    fclose(ofp);
    }
}






// build off vtkdll directory...obj and lib directories
void makeIncrementalMakefiles(CPcmakerDlg *vals, int debugFlag)
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
  if (vals->m_BuildJava &&strlen(vals->m_WhereJDK) > 1)
    {
    fprintf(fp,"vtkJavaLib ");
    }

  if (vals->m_BuildPython)
    {
    fprintf(fp,"vtkPythonLib ");
    }

  // TCL.. but last
  if (vals->m_BuildTcl)
    {
    fprintf(fp,"vtkTclLib  ");
    }

  fprintf(fp,"\n\n");

  fprintf(fp,"vtkLibs :\n");
  fprintf(fp,"   cd vtkdll\n");
  fprintf(fp,"   nmake OBJS\n");
  fprintf(fp,"   nmake LIBRARIES\n");
  fprintf(fp,"   cd ../\n\n");


  if (vals->m_BuildJava && strlen(vals->m_WhereJDK) > 1)
    {
    fprintf(fp,"vtkJavaLib :\n");
    fprintf(fp,"   cd vtkjava\n");
    fprintf(fp,"   nmake ..\\lib\\%sjava.dll\n",vals->adlg.m_LibPrefix);
    fprintf(fp,"   nmake ..\\lib\\%sjava.dll\n",vals->adlg.m_LibPrefix);
    fprintf(fp,"   cd ../\n\n");
    }

  if (vals->m_BuildPython)
    {
    fprintf(fp,"vtkPythonLib :\n");
    fprintf(fp,"   cd vtkpython\n");
    if (debugFlag)
      {
      fprintf(fp,"   nmake ..\\lib\\%spython_d.dll\n",vals->adlg.m_LibPrefix);
      fprintf(fp,"   nmake ..\\lib\\%spython_d.dll\n",vals->adlg.m_LibPrefix);
      }
    else
      {
      fprintf(fp,"   nmake ..\\lib\\%spython.dll\n",vals->adlg.m_LibPrefix);
      fprintf(fp,"   nmake ..\\lib\\%spython.dll\n",vals->adlg.m_LibPrefix);
      }
    fprintf(fp,"   cd ../\n\n");
    }

  // tcl
  if (vals->m_BuildTcl)
    {
    fprintf(fp,"vtkTclLib :\n");
    fprintf(fp,"   cd vtktcl\n");
    fprintf(fp,"   nmake ..\\lib\\%stcl.dll\n",vals->adlg.m_LibPrefix);
    fprintf(fp,"   nmake ..\\lib\\%stcl.dll\n",vals->adlg.m_LibPrefix);
    fprintf(fp,"   cd ../\n\n");
    }

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


  fprintf(fp,"NONINCREMENTAL : %sdll.dll\n\n",vals->adlg.m_LibPrefix);


  if (debugFlag)
    {
    fprintf(fp,"CPP_PROJ=/nologo /D \"STRICT\" /D \"_DEBUG\" /MDd /W3 /Od /Zi /I \"%s\\include\" /I \"%s\" /I \"%s\\common\" /I \"%s\\imaging\" /I \"%s\\graphics\" /D \"NDEBUG\" /D \"WIN32\" \\\n",
            vals->m_WhereCompiler, vals->m_WhereBuild, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    /* would like to use compiler option /Ox but this option includes /Og which
       causes several problems in subexpressions and loop optimizations.  Therefore
       we include all the compiler options that /Ox uses except for /Og */
    fprintf(fp,"CPP_PROJ=/nologo /D \"STRICT\" /MD /W3 /Ob1 /Oi /Ot /Oy /Gs /I \"%s\\include\" /I \"%s\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /D \"NDEBUG\" /D \"WIN32\" \\\n",
	    vals->m_WhereCompiler, vals->m_WhereBuild, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  if (vals->m_Patented) 
    {
    fprintf(fp," /I \"%s\\patented\" \\\n",vals->m_WhereVTK);
    }
  if (vals->m_Contrib) 
    {
    fprintf(fp," /I \"%s\\contrib\" \\\n",vals->m_WhereVTK);
    }
  if (vals->adlg.m_UseMPI) 
    {
    fprintf(fp," /I \"%s\" \\\n",vals->adlg.m_WhereMPIInclude);
    }
  
  fprintf(fp," %s /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_MBCS\" /D \"VTKDLL\"\\\n",
	  vals->adlg.m_EXTRA_CFLAGS);


  if (!debugFlag && vals->m_Lean)
    {
    fprintf(fp,"/D \"VTK_LEAN_AND_MEAN\"  /Fo$(OBJDIR)\\ /c\n");
    }
  else
    {
    fprintf(fp,"/Fo$(OBJDIR)\\ /c\n");
    }


  char mpilibs[1024];
  sprintf(mpilibs,"");
  if (vals->adlg.m_UseMPI) 
    {
    // what libraries to link in for MPI ?
    FILE *testfp;
    char fname[1024];
    char libs[1024];
    sprintf(fname,"%s\\mpich.lib",vals->adlg.m_WhereMPILibrary);
    testfp = fopen(fname,"r");
    if (testfp)
      {
      fclose(testfp);
      sprintf(fname,"%s\\pmpich.lib",vals->adlg.m_WhereMPILibrary);
      testfp = fopen(fname,"r");
      if (testfp)
        {
        sprintf(libs,"mpich.lib pmpich.lib");
        }
      else
        {
        sprintf(libs,"mpich.lib");
        }
      }
    else
      {
      sprintf(fname,"%s\\mpi.lib",vals->adlg.m_WhereMPILibrary);
      testfp = fopen(fname,"r");
      if (testfp)
        {
        sprintf(libs,"mpi.lib");
        fclose(testfp);
        }
      else
        {
        sprintf(libs,"");
        }
      }
    sprintf(mpilibs,"/libpath:\"%s\" %s",
            vals->adlg.m_WhereMPILibrary, libs);
    }

  fprintf(fp,"LINK32=link.exe\n");
  if (debugFlag)
    {
    fprintf(fp,"LINK32_FLAGS= %s /debug %s /libpath:\"%s\\lib\" \"%s\\lib\\opengl32.lib\" \"%s\\lib\\glaux.lib\" \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows\n",
	    vals->adlg.m_EXTRA_LINK_FLAGS, mpilibs, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS= %s %s /libpath:\"%s\\lib\" \"%s\\lib\\opengl32.lib\" \"%s\\lib\\glaux.lib\" \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows\n",
	    vals->adlg.m_EXTRA_LINK_FLAGS, mpilibs, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }

  fprintf(fp,"ALL_FLAGS= /dll /incremental:no /machine:I386\\\n");
  fprintf(fp," /out:%sdll.dll /implib:%sdll.lib \n\n",vals->adlg.m_LibPrefix,
	  vals->adlg.m_LibPrefix);

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
  if ( vals->m_Patented )
    {
    fprintf(fp,"PATENTED_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkPatented.dll\" /implib:\"$(LIBDIR)/vtkPatented.lib\" \n");
    fprintf(fp,"PATENTED_LIBS=\"$(LIBDIR)/vtkCommon.lib\" \"$(LIBDIR)/vtkImaging.lib\" ");
    if ( vals->m_Graphics )
      {
      for (i = 0; i < NumOfGraphicsLibs ; i++)
        fprintf(fp,"\"$(LIBDIR)/vtkGraphics%d.lib\" ",i);
      }
    fprintf(fp,"\n");


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
  if ( vals->m_Local)
    {
    fprintf(fp,"LOCAL_FLAGS= /dll /incremental:yes /machine:I386\\\n");
    fprintf(fp," /out:\"$(LIBDIR)/vtkLocal.dll\" /implib:\"$(LIBDIR)/vtkLocal.lib\" \n");
    fprintf(fp,"LOCAL_LIBS=\"$(LIBDIR)/vtkCommon.lib\" \"$(LIBDIR)/vtkImaging.lib\" ");
    if ( vals->m_Graphics )
      {
      for (i = 0; i < NumOfGraphicsLibs ; i++)
        fprintf(fp,"\"$(LIBDIR)/vtkGraphics%d.lib\" ",i);
      }
    fprintf(fp,"\n");


    fprintf(fp,"LOCAL_OBJS= \\\n");
    fprintf(fp,"    \"$(OBJDIR)\\vtkPCForceLocal.obj\" \\\n");
    for (i = abstractStart[LT_LOCAL]; i < abstractEnd[LT_LOCAL]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",abstract[i]);
    for (i = concreteStart[LT_LOCAL]; i < concreteEnd[LT_LOCAL]; i++)
      fprintf(fp,"    \"$(OBJDIR)\\%s.obj\" \\\n",concrete[i]);
    fprintf(fp,"\n");
    }


  fprintf(fp,"OBJS : $(COMMON_OBJS) $(PATENTED_OBJS) $(IMAGING_OBJS) \\\n");
  fprintf(fp,"       $(CONTRIB_OBJS) $(LOCAL_OBJS) \\\n");
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
  if ( vals->m_Imaging )
    fprintf(fp,"          \"$(LIBDIR)\\vtkImaging.dll\"\\\n ");
  if ( vals->m_Patented )
    fprintf(fp,"          \"$(LIBDIR)\\vtkPatented.dll\"\\\n ");
  if ( vals->m_Contrib )
    fprintf(fp,"          \"$(LIBDIR)\\vtkContrib.dll\"\\\n ");
  if ( vals->m_Local )
    fprintf(fp,"          \"$(LIBDIR)\\vtkLocal.dll\"\\\n ");
  fprintf(fp,"\n");




  fprintf(fp,"%sdll.dll : $(DEF_FILE) $(COMMON_OBJS) $(PATENTED_OBJS) ",
	  vals->adlg.m_LibPrefix);
  fprintf(fp," $(IMAGING_OBJS) $(CONTRIB_OBJS) $(LOCAL_OBJS) ");
  for (i = 0; i < NumOfGraphicsLibs; i++)
    fprintf(fp,"$(GRAPHICS_OBJS%d) ",i);
  fprintf(fp,"\n");


  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(ALL_FLAGS) $(COMMON_OBJS) $(PATENTED_OBJS) ");
  fprintf(fp," $(IMAGING_OBJS) $(CONTRIB_OBJS) $(LOCAL_OBJS) ");
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
  if ( vals->m_Patented )
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
  if ( vals->m_Contrib )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkContrib.dll\" : $(DEF_FILE) $(CONTRIB_OBJS) %s\\GraphicsSplitInfo.txt\n",vals->m_WhereBuild);
    fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(CONTRIB_FLAGS) $(CONTRIB_OBJS) $(CONTRIB_LIBS)\n");
    fprintf(fp,"<<\n");
    fprintf(fp,"\n");
    }
  if ( vals->m_Local )
    {
    fprintf(fp,"\"$(LIBDIR)\\vtkLocal.dll\" : $(DEF_FILE) $(LOCAL_OBJS) %s\\GraphicsSplitInfo.txt\n",vals->m_WhereBuild);
    fprintf(fp,"    $(LINK32) @<<\n");
    fprintf(fp,"  $(LINK32_FLAGS) $(LOCAL_FLAGS) $(LOCAL_OBJS) $(LOCAL_LIBS)\n");
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
  OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
  vals->m_Progress.OffsetPos(1);
  fprintf(fp,"\"$(OBJDIR)\\vtkPCForceCommon.obj\" : src\\vtkPCForceCommon.cxx $(DEPENDS) \n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceCommon.cxx\n\n");


  if ( vals->m_Graphics )
    {
    sprintf(file,"%s\\vtkPCForceGraphics.cxx",temp);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceGraphics.obj\" : src\\vtkPCForceGraphics.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceGraphics.cxx\n\n");
    }
  if ( vals->m_Imaging )
    {
    sprintf(file,"%s\\vtkPCForceImaging.cxx",temp);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceImaging.obj\" : src\\vtkPCForceImaging.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceImaging.cxx\n\n");
    }
  if ( vals->m_Patented )
    {
    sprintf(file,"%s\\vtkPCForcePatented.cxx",temp);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForcePatented.obj\" : src\\vtkPCForcePatented.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForcePatented.cxx\n\n");
    }
  if ( vals->m_Contrib )
    {
    sprintf(file,"%s\\vtkPCForceContrib.cxx",temp);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceContrib.obj\" : src\\vtkPCForceContrib.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceContrib.cxx\n\n");
    }
  if ( vals->m_Local )
    {
    sprintf(file,"%s\\vtkPCForceLocal.cxx",temp);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\vtkPCForceLocal.obj\" : src\\vtkPCForceLocal.cxx $(DEPENDS) \n");
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkPCForceLocal.cxx\n\n");
    }


  for (i = 0; i < num_abstract; i++)
    {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\%s.obj\" : \"%s\\%s\\%s.cxx\" $(DEPENDS) \n",
	    abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\%s\\%s.cxx\" \n\n",
	    vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    }
  for (i = 0; i < num_concrete; i++)
    {
    sprintf(file,"%s\\%s\\%s.cxx",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"$(OBJDIR)\\%s.obj\" : \"%s\\%s\\%s.cxx\" $(DEPENDS) \n",
	    concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\%s\\%s.cxx\"\n\n",
	    vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    }
  fprintf(fp,"################################################################################\n");
}





void doBorHeader(FILE *fp, CPcmakerDlg *vals, int debugFlag)
{
  int i;

  fprintf(fp,"# VTK Borland makefile\n");
  fprintf(fp,".autodepend\n");
  fprintf(fp,"OUTDIR=obj\n");
  fprintf(fp,"WHEREVTK=%s\n",vals->m_WhereVTK);
  fprintf(fp,"WHEREBUILD=%s\n",vals->m_WhereBuild);
  fprintf(fp,"WHERECOMP=%s\n\n",vals->m_WhereCompiler);
  fprintf(fp,"CPP=BCC32.exe +CPP_PROJ.CFG\n\n");

  fprintf(fp,"ALL : %sdll.dll  %sdll.lib\n\n",
	  vals->adlg.m_LibPrefix, vals->adlg.m_LibPrefix);

  fprintf(fp,"\"obj\" :\n");
  fprintf(fp,"    if not exist \"obj$(NULL)\" mkdir \"obj\"\n");
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
    fprintf(fp,"-I$(WHEREVTK)\\patented\n");
    }
  if (vals->m_Contrib)
    {
    fprintf(fp,"-I$(WHEREVTK)\\contrib\n");
    }

  fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL;STRICT\n");

  // JCP take Borland Builder specific action ...
 if (bbuilder) 
   {
   fprintf(fp,"-tWM -tWD -Od -H- -VF -I$(WHERECOMP)\\include;$(WHERECOMP)\\include\\rw;$(WHERECOMP)\\include\\vcl;$(WHEREVTK)\\common;$(WHEREVTK)\\graphics -DWIN32\n");
   } 
 else 
   {
   fprintf(fp,"-tWM -tWD -Od -H- -VF -I$(WHERECOMP)\\include;$(WHEREVTK)\\common;$(WHEREVTK)\\graphics -DWIN32\n");
   }

  fprintf(fp," -I$(WHEREBUILD) \n");
  fprintf(fp," -I$(WHEREVTK)\\imaging \n");
  fprintf(fp," -I$(WHEREVTK)\\contrib \n");
  fprintf(fp," -I$(WHEREVTK)\\local \n");
  fprintf(fp," -I$(WHEREVTK) \n");
  fprintf(fp,"-P -c -w-hid -w-inl -w-ccc -w-aus \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n");

  // JCP take Borland Builder specific action ...
  if (bbuilder) 
	{
    fprintf(fp,"LINK32=ilink32.exe\n\n");
	} 
  else 
	{
    fprintf(fp,"LINK32=tlink32.exe\n\n");
	}

  fprintf(fp,"LINK32_FLAGS=-L$(WHERECOMP)\\lib \\\n");
  if (debugFlag)
    {
    fprintf(fp,"  -v \\\n");
    }
  else
    {
    fprintf(fp,"  -v- \\\n");
    }

  // JCP take Borland Builder specific action ...
  if (bbuilder) 
	{
	fprintf(fp,"  -aa -V4.0 -m -n -Tpp -Gi -Gl -Gn\n");
	} 
  else 
	{
	fprintf(fp,"  -Tpd -aa -V4.0 -Gm  -w-inq -m -n\n");
	}

  fprintf(fp,"DEPLINK32_OBJS= \\\n");
  fprintf(fp,"    $(OUTDIR)\\vtkPCForceCommon.obj");
  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp," \\\n    $(OUTDIR)\\%s.obj",abstract[i]);
    }
  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp," \\\n    $(OUTDIR)\\%s.obj",concrete[i]);
    }
  fprintf(fp,"\n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    $(WHERECOMP)\\lib\\c0d32.obj+ \\\n");
  fprintf(fp,"    $(OUTDIR)\\vtkPCForceCommon.obj");
  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%s.obj",abstract[i]);
    }
  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%s.obj",concrete[i]);
    }
  fprintf(fp,", \\\n $<,$*,  \\\n");   // this is the target and map file name

  // JCP take Borland Builder specific action ...
  if (bbuilder == 3) 
	{
   if (debugFlag)
      {
	 fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcl35.lib \\\n");
      } else {
	 fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcl35.lib \\\n");
      }
	fprintf(fp,"    $(WHERECOMP)\\lib\\vcl.lib \\\n");
	}
  // JCP take Borland Builder specific action ...
  if (bbuilder == 4) 
	{
   if (debugFlag)
      {
	fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcl40.lib \\\n");
	fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcle40.lib \\\n");
      } else {
	fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcl40.lib \\\n");
	fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcle40.lib \\\n");
       }
	fprintf(fp,"    $(WHERECOMP)\\lib\\vcl.lib \\\n");
      fprintf(fp,"    $(WHERECOMP)\\lib\\cp32mt.lib \\\n");
	}

  fprintf(fp,"    $(WHERECOMP)\\lib\\import32.lib \\\n");
 if (bbuilder < 4) 
	{
  fprintf(fp,"    $(WHERECOMP)\\lib\\cw32mt.lib \\\n");
      }
  fprintf(fp,"\n");

  fprintf(fp,"%sdll.dll : \"obj\" $(DEF_FILE) $(DEPLINK32_OBJS)\n",
	  vals->adlg.m_LibPrefix);

  fprintf(fp,"    $(LINK32) @&&|\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"| \n");
 
  fprintf(fp,"%sdll.lib : %sdll.dll \n", vals->adlg.m_LibPrefix,
	  vals->adlg.m_LibPrefix);
  fprintf(fp,"      implib -w $@ %sdll.dll \n",
	  vals->adlg.m_LibPrefix);
 
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"\n");

  
  fprintf(fp,"$(OUTDIR)\\vtkPCForceCommon.obj : src\\vtkPCForceCommon.cxx \n");
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-o$(OUTDIR)\\vtkPCForceCommon.obj src\\vtkPCForceCommon.cxx \n\n");
  fprintf(fp,"|  \n");
  
  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp,"$(OUTDIR)\\%s.obj : $(WHEREVTK)\\%s\\%s.cxx \n",
	    abstract[i],abstract_lib[i],abstract[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%s.obj $(WHEREVTK)\\%s\\%s.cxx \n\n",
	    abstract[i],abstract_lib[i],abstract[i]);
    fprintf(fp,"|  \n");
    }
  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp,"$(OUTDIR)\\%s.obj : $(WHEREVTK)\\%s\\%s.cxx \n",
	    concrete[i],concrete_lib[i],concrete[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%s.obj $(WHEREVTK)\\%s\\%s.cxx \n\n",
	    concrete[i],concrete_lib[i],concrete[i]);
    fprintf(fp,"|  \n");
    }


  fprintf(fp,"################################################################################\n");
}


// links in ALL the vtk libraries (now that they are split up)... also writes output to ../lib directory
// may only want to write the dll to that directory????
// makefile handles both incremental and non-incremental linking
void doMSCTclHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag)
{
  int i;
  char file [256];


  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"!MESSAGE making tcl library...\n");
  fprintf(fp,"CPP=cl.exe\n");
  fprintf(fp,"PATH=$(PATH);\"%s\\pcmaker\\\"\n",vals->m_WhereVTK);
  fprintf(fp,"CPP_PARSE=vtkWrapTcl.exe\n");
  fprintf(fp,"OUTDIR=obj\n\n");

  fprintf(fp,"LIBDIR=..\\lib\n\n");

  fprintf(fp,"ALL : %stcl.dll\n\n",vals->adlg.m_LibPrefix);

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");

  if (debugFlag)
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /D \"_DEBUG\" /nologo /MDd /W3 /Od /Zi /I \"%s\\include\" /I \"%s\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\" %s \\\n",
	    vals->m_WhereCompiler, vals->m_WhereBuild, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->adlg.m_EXTRA_CFLAGS);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /nologo /MD /W3 /O2 /I \"%s\\include\" /I \"%s\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\" %s \\\n",
	    vals->m_WhereCompiler, vals->m_WhereBuild, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->adlg.m_EXTRA_CFLAGS);
    }


  if (strlen(vals->adlg.m_WhereTcl) > 1)
    {
    fprintf(fp," /I \"%s\\win\" /I \"%s\\xlib\" /I \"%s\\generic\" /I \"%s\\win\" /I \"%s\\generic\" /I \"%s\\include\" ",
            vals->TkRoot, vals->TkRoot, vals->TkRoot, vals->TclRoot, vals->TclRoot, vals->TclRoot);
    }
  else
    {
    fprintf(fp," /I \"%s\\pcmaker\\xlib\" ",vals->m_WhereVTK);
    }

  if (vals->adlg.m_UseMPI) 
    {
    fprintf(fp," /I \"%s\" \\\n",vals->adlg.m_WhereMPIInclude);
    }

  if (vals->m_Patented) fprintf(fp," /I \"%s\\patented\" \\\n",
				vals->m_WhereVTK);
  if (vals->m_Contrib) fprintf(fp," /I \"%s\\contrib\" \\\n",vals->m_WhereVTK);

  if (vals->m_Local) fprintf(fp," /I \"%s\\local\" \\\n",vals->m_WhereVTK);

  fprintf(fp," /D \"_WINDLL\" /D \"_MBCS\" \\\n");


  if (!debugFlag && vals->m_Lean)
    {
    fprintf(fp," /D \"VTK_LEAN_AND_MEAN\" /Fo$(OUTDIR)\\ /c \n");
    }
  else
    {
    fprintf(fp," /Fo$(OUTDIR)\\ /c \n");
    }


  //fprintf(fp,"CPP_PROJ2=$(CPP_PROJ) /D \"VTKDLL\"\n");
  fprintf(fp,"CPP_PROJ2=$(CPP_PROJ) \n");
  
  fprintf(fp,"LINK32=link.exe\n");
  if (debugFlag)
    {
    fprintf(fp,"LINK32_FLAGS=\"$(OUTDIR)\\vtktcl.obj\" /debug /libpath:\"%s\\lib\"  \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows",
	    vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=\"$(OUTDIR)\\vtktcl.obj\" /libpath:\"%s\\lib\" \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows",
	    vals->m_WhereCompiler, vals->m_WhereCompiler, vals->m_WhereCompiler);
    }


  if (strlen(vals->adlg.m_WhereTcl) > 1)
    {
    fprintf(fp," \"%s\" \"%s\" \n",vals->adlg.m_WhereTk, vals->adlg.m_WhereTcl);
    }
  else
    {
    fprintf(fp," \"%s\\pcmaker\\tk82.lib\" \"%s\\pcmaker\\tcl82.lib\" \n",
	    vals->m_WhereVTK, vals->m_WhereVTK);
    }
  fprintf(fp,"MORE_FLAGS1=/dll /incremental:yes /pdb:\"$(LIBDIR)/%stcl.pdb\" /machine:I386\\\n",vals->adlg.m_LibPrefix);
  fprintf(fp," /out:\"$(LIBDIR)/%stcl.dll\" /implib:\"$(LIBDIR)/%stcl.lib\" \n\n",vals->adlg.m_LibPrefix,vals->adlg.m_LibPrefix); 
  fprintf(fp,"MORE_FLAGS2=/dll /incremental:no /pdb:%stcl.pdb /machine:I386\\\n",vals->adlg.m_LibPrefix);
  fprintf(fp," /out:%stcl.dll /implib:%stcl.lib \n\n",vals->adlg.m_LibPrefix
	  ,vals->adlg.m_LibPrefix); 
  fprintf(fp,"LIB_FLAGS=/machine:I386\n\n"); 


  fprintf(fp,"TCLOBJLIBS=vtktclotherobjs.lib vtktclgraphicsobjs.lib\n\n");


  fprintf(fp,"VTKDLL_LIB=..\\vtkdll\\%sdll.lib \n\n",
	  vals->adlg.m_LibPrefix);


  fprintf(fp,"VTK_LIBRARIES=..\\lib\\vtkCommon.lib ");
  if (vals->m_Graphics) 
    {
    for (i = 0; i < NumOfGraphicsLibs; i++)
      fprintf(fp,"..\\lib\\vtkGraphics%d.lib  ",i);
    }
  if (vals->m_Imaging) 
    fprintf(fp,"..\\lib\\vtkImaging.lib ");
  if (vals->m_Patented)
    fprintf(fp,"..\\lib\\vtkPatented.lib ");
  if (vals->m_Contrib)
    fprintf(fp,"..\\lib\\vtkContrib.lib ");     
  if (vals->m_Local)
    fprintf(fp,"..\\lib\\vtkLocal.lib ");     
  fprintf(fp,"\n\n");


  // All the graphics Tcl objects
  if (vals->m_Graphics)
    {
    fprintf(fp,"GRAPHICSTCL_OBJS= \\\n");
    fprintf(fp,"    \"$(OUTDIR)\\vtkTkRenderWidget.obj\" \\\n");
    for (i = abstractStart[LT_GRAPHICS]; i < abstractEnd[LT_GRAPHICS]; i++)
      fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",abstract[i]);


    for (i = concreteStart[LT_GRAPHICS]; i < concreteEnd[LT_GRAPHICS]; i++)
      fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",concrete[i]);


    for (i = abstractHStart[LT_GRAPHICS]; i < abstractHEnd[LT_GRAPHICS]; i++)
      fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",abstract_h[i]);


    for (i = concreteHStart[LT_GRAPHICS]; i < concreteHEnd[LT_GRAPHICS]; i++)
      fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",concrete_h[i]);
    fprintf(fp,"\n");
    }




  // Now all the TCL objects other than graphics (and DFA stuff)
  fprintf(fp,"OTHERTCL_OBJS= \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtkTclUtil.obj\" \\\n");
  if (vals->m_Imaging)  
    {
    fprintf(fp,"    \"$(OUTDIR)\\vtkTkImageViewerWidget.obj\" \\\n");
    fprintf(fp,"    \"$(OUTDIR)\\vtkTkImageWindowWidget.obj\" \\\n");
    }


  for (i = (vals->m_Graphics ? abstractEnd[LT_GRAPHICS] : 0); 
       i < abstractEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",abstract[i]);


  for (i = (vals->m_Graphics ? concreteEnd[LT_GRAPHICS] : 0); 
       i < concreteEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",concrete[i]);


  for (i = (vals->m_Graphics ? abstractHEnd[LT_GRAPHICS] : 0);
       i < abstractHEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",abstract_h[i]);


  for (i = (vals->m_Graphics ? concreteHEnd[LT_GRAPHICS] : 0);
       i < concreteHEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OUTDIR)\\%sTcl.obj\" \\\n",concrete_h[i]);
  fprintf(fp,"\n");
  
  fprintf(fp,"%stcl.dll : \"$(OUTDIR)\" $(DEF_FILE) \"$(OUTDIR)\\vtktcl.obj\" $(TCLOBJLIBS)\n",vals->adlg.m_LibPrefix);
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(MORE_FLAGS2) $(TCLOBJLIBS) $(VTKDLL_LIB)\n");
  fprintf(fp,"<<\n\n");


  fprintf(fp,"\"$(LIBDIR)\\%stcl.dll\" : \"$(OUTDIR)\" $(DEF_FILE) \"$(OUTDIR)\\vtktcl.obj\" $(TCLOBJLIBS)\n",vals->adlg.m_LibPrefix);
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(MORE_FLAGS1) $(TCLOBJLIBS) $(VTK_LIBRARIES)\n");
  fprintf(fp,"<<\n\n");


  if (vals->m_Graphics)
    {
    fprintf(fp,"vtktclgraphicsobjs.lib : \"$(OUTDIR)\" $(DEF_FILE) $(GRAPHICSTCL_OBJS) \n");
    fprintf(fp,"    lib.exe @<<\n");
    fprintf(fp,"  /out:vtktclgraphicsobjs.lib $(LIB_FLAGS) $(GRAPHICSTCL_OBJS)\n");
    fprintf(fp,"<<\n\n");
    }


  fprintf(fp,"vtktclotherobjs.lib : \"$(OUTDIR)\" $(DEF_FILE) $(OTHERTCL_OBJS) \n");
  fprintf(fp,"    lib.exe @<<\n");
  fprintf(fp,"  /out:vtktclotherobjs.lib $(LIB_FLAGS) $(OTHERTCL_OBJS)\n");
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
  fprintf(fp,"\"$(OUTDIR)\\vtkTclUtil.obj\" : \"%s\\common\\vtkTclUtil.cxx\" \"%s\\common\\vtkTclUtil.h\" \"%s\\common\\vtkCommand.h\" \"$(OUTDIR)\"\n",
	  vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\common\\vtkTclUtil.cxx\"\n\n",vals->m_WhereVTK);
  if (vals->m_Graphics)
    {
    sprintf(file,"%s\\graphics\\vtkTkRenderWidget.cxx",vals->m_WhereVTK);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    fprintf(fp,"\"$(OUTDIR)\\vtkTkRenderWidget.obj\" : \"%s\\graphics\\vtkTkRenderWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	    vals->m_WhereVTK);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\graphics\\vtkTkRenderWidget.cxx\"\n\n",vals->m_WhereVTK);
    }
  if (vals->m_Imaging)
    {
    sprintf(file,"%s\\imaging\\vtkTkImageViewerWidget.cxx",vals->m_WhereVTK);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    fprintf(fp,"\"$(OUTDIR)\\vtkTkImageViewerWidget.obj\" : \"%s\\imaging\\vtkTkImageViewerWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	    vals->m_WhereVTK);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\imaging\\vtkTkImageViewerWidget.cxx\"\n\n",vals->m_WhereVTK);
    sprintf(file,"%s\\imaging\\vtkTkImageWindowWidget.cxx",vals->m_WhereVTK);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    fprintf(fp,"\"$(OUTDIR)\\vtkTkImageWindowWidget.obj\" : \"%s\\imaging\\vtkTkImageWindowWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	    vals->m_WhereVTK);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\imaging\\vtkTkImageWindowWidget.cxx\"\n\n",vals->m_WhereVTK);
    }


  fprintf(fp,"\"$(OUTDIR)\\vtktcl.obj\" : src\\vtktcl.cxx \"$(OUTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtktcl.cxx\n\n");


  for (i = 0; i < num_abstract; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\pcmaker\\vtkWrapTcl.exe\" \"%s\\wrap\\hints\" \"$(OUTDIR)\"\n",
	    abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE)  \"%s\\%s\\%s.h\"\\\n",
	    vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  \"%s\\wrap\\hints\" 0 > src\\%sTcl.cxx\n\n",
	    vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstract[i]);
    }


  for (i = 0; i < num_concrete; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\pcmaker\\vtkWrapTcl.exe\" \"%s\\wrap\\hints\" \"$(OUTDIR)\"\n",
	    concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
	    vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  \"%s\\wrap\\hints\" 1 > src\\%sTcl.cxx\n\n",
	    vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concrete[i]);
    }


  for (i = 0; i < num_abstract_h; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\pcmaker\\vtkWrapTcl.exe\" \"%s\\wrap\\hints\" \"$(OUTDIR)\"\n",
	    abstract_h[i],vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
	    vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  \"%s\\wrap\\hints\" 0 > src\\%sTcl.cxx\n\n",
	    vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",abstract_h[i]);
    }


  for (i = 0; i < num_concrete_h; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sTcl.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkTclUtil.h\" \"%s\\pcmaker\\vtkWrapTcl.exe\" \"%s\\wrap\\hints\" \"$(OUTDIR)\"\n",
	    concrete_h[i],vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
	    vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  \"%s\\wrap\\hints\" 1 > src\\%sTcl.cxx\n\n",
	    vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sTcl.obj\" : src\\%sTcl.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sTcl.cxx\n\n",concrete_h[i]);
    }


  fprintf(fp,"################################################################################\n");
}




void doBorTclHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag)
{
  int i;
  fprintf(fp,"# VTK Borland makefile\n");
  fprintf(fp,".autodepend\n");
  fprintf(fp,"OUTDIR=obj\n\n");
  fprintf(fp,"WHEREVTK=%s\n",vals->m_WhereVTK);
  fprintf(fp,"WHEREBUILD=%s\n",vals->m_WhereBuild);
  fprintf(fp,"WHERECOMP=%s\n\n",vals->m_WhereCompiler);
  fprintf(fp,"CPP=BCC32.exe +CPP_PROJ.CFG\n\n");
  fprintf(fp,"ALL : %stcl.dll\n\n",vals->adlg.m_LibPrefix);


  fprintf(fp,"$(OUTDIR) ::\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
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
    fprintf(fp,"-I$(WHEREVTK)\\patented\n");
    }
  if (vals->m_Contrib)
    {
    fprintf(fp,"-I$(WHEREVTK)\\contrib\n");
    }
  fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL;STRICT\n");
  fprintf(fp,"-tWM -tWD -Od -H- -VF  -I$(WHERECOMP)\\include;$(WHERECOMP)\\include\\rw;$(WHEREVTK)\\common;$(WHEREVTK)\\graphics -DWIN32\n");
  fprintf(fp," -I$(WHEREVTK)\\pcmaker\\xlib \n");
  fprintf(fp," -I$(WHEREVTK)\\imaging \n");
  fprintf(fp," -I$(WHEREVTK)\\contrib \n");
  fprintf(fp," -I$(WHEREBUILD) \n");
  fprintf(fp," -I$(WHEREVTK)\\local \n");
  fprintf(fp,"-P -c -w-hid -w-inl -w-ccc -w-aus \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n");

  // JCP take Borland Builder specific action ...
  if (bbuilder) 
	{
	fprintf(fp,"LINK32=ilink32.exe\n\n");
	} 
  else 
	{
	fprintf(fp,"LINK32=tlink32.exe\n\n");
	}

  fprintf(fp,"LINK32_FLAGS=-L$(WHERECOMP)\\lib;..\\vtkdll\\%sdll.lib;$(WHEREVTK)\\pcmaker\\tk82.lib;$(WHEREVTK)\\pcmaker\\tcl82.lib \\\n",
	  vals->adlg.m_LibPrefix);
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
  fprintf(fp,"    $(OUTDIR)\\vtkTclUtil.obj \\\n");
  fprintf(fp,"    $(OUTDIR)\\vtktcl.obj \\\n");
  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp,"    $(OUTDIR)\\%sTcl.obj \\\n",abstract[i]);
    }
  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp,"    $(OUTDIR)\\%sTcl.obj \\\n",concrete[i]);
    }
  for (i = 0; i < num_abstract_h; i++)
    {
    fprintf(fp,"    $(OUTDIR)\\%sTcl.obj \\\n",abstract_h[i]);
    }
  for (i = 0; i < num_concrete_h; i++)
    {
    fprintf(fp,"    $(OUTDIR)\\%sTcl.obj \\\n",concrete_h[i]);
    }
  fprintf(fp,"\n");
  fprintf(fp,"LINK32_OBJS= \\\n");
  fprintf(fp,"    $(WHERECOMP)\\lib\\c0d32.obj+ \\\n");


  fprintf(fp,"    $(OUTDIR)\\vtkTclUtil.obj+ \\\n");
  fprintf(fp,"    $(OUTDIR)\\vtktcl.obj");
  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%sTcl.obj",abstract[i]);
    }
  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%sTcl.obj",concrete[i]);
    }
  for (i = 0; i < num_abstract_h; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%sTcl.obj",abstract_h[i]);
    }
  for (i = 0; i < num_concrete_h; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%sTcl.obj",concrete_h[i]);
    }
  fprintf(fp,", \\\n $<,$*,  \\\n");   // this is the target and map file name
  // JCP take Borland Builder specific action ...
  if (bbuilder == 3) 
	{
   if (debugFlag)
      {
	 fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcl35.lib \\\n");
      } else {
	 fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcl35.lib \\\n");
      }
	fprintf(fp,"    $(WHERECOMP)\\lib\\vcl.lib \\\n");
	}
  // JCP take Borland Builder specific action ...
  if (bbuilder == 4) 
	{
   if (debugFlag)
      {
	fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcl40.lib \\\n");
	fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcle40.lib \\\n");
      } else {
	fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcl40.lib \\\n");
	fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcle40.lib \\\n");
       }
	fprintf(fp,"    $(WHERECOMP)\\lib\\vcl.lib \\\n");
      fprintf(fp,"    $(WHERECOMP)\\lib\\cp32mt.lib \\\n");
	}

  fprintf(fp,"    $(WHERECOMP)\\lib\\import32.lib \\\n");
 if (bbuilder < 4) 
	{
  fprintf(fp,"    $(WHERECOMP)\\lib\\cw32mt.lib \\\n");
      }
  fprintf(fp,"    ..\\vtkdll\\%sdll.lib \\\n",vals->adlg.m_LibPrefix);
  fprintf(fp,"    $(WHEREVTK)\\pcmaker\\tk82.lib \\\n");

  fprintf(fp,"    $(WHEREVTK)\\pcmaker\\tcl82.lib \\\n");

  fprintf(fp," \n");
  fprintf(fp,"%stcl.dll : obj $(DEF_FILE) $(DEPLINK32_OBJS) obj\n",
	  vals->adlg.m_LibPrefix);
  fprintf(fp,"    $(LINK32) @&&|\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"|\n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"\n");
  fprintf(fp,"$(OUTDIR)\\vtkTclUtil.obj : $(WHEREVTK)\\common\\vtkTclUtil.cxx $(WHEREVTK)\\common\\vtkTclUtil.h $(WHEREVTK)\\common\\vtkCommand.h\n");
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-o$(OUTDIR)\\vtkTclUtil.obj  $(WHEREVTK)\\common\\vtkTclUtil.cxx\n\n");
  fprintf(fp,"|  \n");
  fprintf(fp,"$(OUTDIR)\\vtktcl.obj : src\\vtktcl.cxx \n");
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-o$(OUTDIR)\\vtktcl.obj  src\\vtktcl.cxx\n\n");
  fprintf(fp,"|  \n");


  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp,"src\\%sTcl.cxx : $(WHEREVTK)\\%s\\%s.h \n",
	    abstract[i],abstract_lib[i],abstract[i]);
    fprintf(fp,"  copy &&| \n");
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkWrapTcl $(WHEREVTK)\\%s\\%s.h\\\n",
	    abstract_lib[i], abstract[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 0 > src\\%sTcl.cxx\n\n",
	    abstract[i]);
    fprintf(fp,"| vtktcl.bat \n  vtktcl \n\n");
    fprintf(fp,"$(OUTDIR)\\%sTcl.obj : src\\%sTcl.cxx \n",
	    abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%sTcl.obj src\\%sTcl.cxx\n\n",abstract[i],abstract[i]);
    fprintf(fp,"|  \n");
    }


  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp,"src\\%sTcl.cxx : $(WHEREVTK)\\%s\\%s.h \n",
	    concrete[i],concrete_lib[i],concrete[i]);
    fprintf(fp,"  copy &&| \n");
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkWrapTcl $(WHEREVTK)\\%s\\%s.h\\\n",
	    concrete_lib[i], concrete[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 1 > src\\%sTcl.cxx\n\n",
	    concrete[i]);
    fprintf(fp,"| vtktcl.bat \n  vtktcl \n\n");
    fprintf(fp,"$(OUTDIR)\\%sTcl.obj : src\\%sTcl.cxx \n",
	    concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%sTcl.obj src\\%sTcl.cxx\n\n",concrete[i],concrete[i]);
    fprintf(fp,"|  \n");
    }


  for (i = 0; i < num_abstract_h; i++)
    {
    fprintf(fp,"src\\%sTcl.cxx : $(WHEREVTK)\\%s\\%s.h \n",
	    abstract_h[i],abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"  copy &&| \n");
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkWrapTcl $(WHEREVTK)\\%s\\%s.h\\\n",
	    abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 0 > src\\%sTcl.cxx\n\n",
	    abstract_h[i]);
    fprintf(fp,"| vtktcl.bat \n  vtktcl \n\n");
    fprintf(fp,"$(OUTDIR)\\%sTcl.obj : src\\%sTcl.cxx \n",
	    abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%sTcl.obj src\\%sTcl.cxx\n\n",abstract_h[i],abstract_h[i]);
    fprintf(fp,"|  \n");
    }


  for (i = 0; i < num_concrete_h; i++)
    {
    fprintf(fp,"src\\%sTcl.cxx : $(WHEREVTK)\\%s\\%s.h \n",
	    concrete_h[i],concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"  copy &&| \n");
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkWrapTcl $(WHEREVTK)\\%s\\%s.h\\\n",
	    concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 1 > src\\%sTcl.cxx\n\n",
	    concrete_h[i]);
    fprintf(fp,"| vtktcl.bat \n  vtktcl \n\n");
    fprintf(fp,"$(OUTDIR)\\%sTcl.obj : src\\%sTcl.cxx \n",
	    concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%sTcl.obj src\\%sTcl.cxx\n\n",concrete_h[i],concrete_h[i]);
    fprintf(fp,"|  \n");
    }


  fprintf(fp,"################################################################################\n");
}




// links it ALL the vtk libraries (now that they are split up)... also writes output to ../lib directory
void doMSCJavaHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag)
{
  int i;
  char file[256];


  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"CPP=cl.exe\n\n");
  fprintf(fp,"OUTDIR=obj\n\n");


  fprintf(fp,"LIBDIR=..\\lib\n\n");


  fprintf(fp,"ALL : %sjava.dll\n\n",vals->adlg.m_LibPrefix);


  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");


  if (debugFlag)
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /D \"_DEBUG\" /nologo /MDd /W3 /Od /Zi /I \"%s\\include\" /I \"%s\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\local\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" \\\n",
	    vals->m_WhereCompiler, vals->m_WhereBuild, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /nologo /MD /W3 /O2 /I \"%s\\include\" /I \"%s\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /I \"%s\\contrib\" /I \"%s\\contrib\" /I \"%s\\pcmaker\\xlib\" /D \"NDEBUG\" /D \"WIN32\" \\\n",
	    vals->m_WhereCompiler, vals->m_WhereBuild, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  if (vals->m_Patented)
    {
    fprintf(fp,"/I \"%s\\patented\" \\\n",vals->m_WhereVTK);
    }
  if (vals->m_Contrib)
    {
    fprintf(fp,"/I \"%s\\contrib\" \\\n",vals->m_WhereVTK);
    }

  fprintf(fp," /D \"_WINDOWS\" /D \"_WINDLL\" /D \"_MBCS\" \\\n");
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


  fprintf(fp,"MORE_FLAGS1=/dll /incremental:yes /pdb:\"$(LIBDIR)/%sjava.pdb\" /machine:I386\\\n",vals->adlg.m_LibPrefix);
  fprintf(fp," /out:\"$(LIBDIR)/%sjava.dll\" /implib:\"$(LIBDIR)/%sjava.lib\" \n",vals->adlg.m_LibPrefix,vals->adlg.m_LibPrefix); 
  fprintf(fp,"MORE_FLAGS2=/dll /incremental:no /pdb:%sjava.pdb /machine:I386\\\n",vals->adlg.m_LibPrefix);
  fprintf(fp," /out:%sjava.dll /implib:%sjava.lib \n",vals->adlg.m_LibPrefix,
	  vals->adlg.m_LibPrefix); 

  fprintf(fp,"VTKDLL_LIB=..\\vtkdll\\%sdll.lib \n",vals->adlg.m_LibPrefix);

  fprintf(fp,"VTK_LIBRARIES=..\\lib\\vtkCommon.lib ");
  if (vals->m_Graphics) 
    {
    for (i = 0; i < NumOfGraphicsLibs; i++)
      fprintf(fp,"..\\lib\\vtkGraphics%d.lib  ",i);
    }
  if (vals->m_Imaging) 
    fprintf(fp,"..\\lib\\vtkImaging.lib ");
  if (vals->m_Patented)
    fprintf(fp,"..\\lib\\vtkPatented.lib ");
  if (vals->m_Contrib)
    fprintf(fp,"..\\lib\\vtkContrib.lib ");     
  if (vals->m_Local)
    fprintf(fp,"..\\lib\\vtkLocal.lib ");     
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
  fprintf(fp,"%sjava.dll : $(DEF_FILE) $(LINK32_OBJS)\n",vals->adlg.m_LibPrefix);
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(MORE_FLAGS2) $(LINK32_OBJS) $(VTKDLL_LIB)\n");
  fprintf(fp,"<<\n");
  fprintf(fp,"\n");
  fprintf(fp,"\"$(LIBDIR)\\%sjava.dll\" : $(DEF_FILE) $(LINK32_OBJS)\n",
	  vals->adlg.m_LibPrefix);
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
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sJava.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
	    abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\vtkWrapJava %s\\%s\\%s.h\\\n",
	    vals->m_WhereVTK, vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\wrap\\hints 0 > src\\%sJava.cxx\n\n",
	    vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"   %s\\pcmaker\\vtkParseJava %s\\%s\\%s.h\\\n",
	    vals->m_WhereVTK, vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  %s\\wrap\\hints 0 > vtk\\%s.java\n\n",
	    vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sJava.obj\" : src\\%sJava.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",abstract[i]);
    }


  for (i = 0; i < num_concrete; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sJava.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
	    concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\vtkWrapJava %s\\%s\\%s.h\\\n",
	    vals->m_WhereVTK, vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\wrap\\hints 1 > src\\%sJava.cxx\n\n",
	    vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"   %s\\pcmaker\\vtkParseJava %s\\%s\\%s.h\\\n",
	    vals->m_WhereVTK, vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  %s\\wrap\\hints 1 > vtk\\%s.java\n\n",
	    vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sJava.obj\" : src\\%sJava.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",concrete[i]);
    }


  for (i = 0; i < num_abstract_h; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sJava.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
	    abstract_h[i],vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\vtkWrapJava %s\\%s\\%s.h\\\n",
	    vals->m_WhereVTK, vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\wrap\\hints 0 > src\\%sJava.cxx\n\n",
	    vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"   %s\\pcmaker\\vtkParseJava %s\\%s\\%s.h\\\n",
	    vals->m_WhereVTK, vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  %s\\wrap\\hints 0 > vtk\\%s.java\n\n",
	    vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sJava.obj\" : src\\%sJava.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",abstract_h[i]);
    }


  for (i = 0; i < num_concrete_h; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sJava.cxx\" : %s\\%s\\%s.h \"$(OUTDIR)\"\n",
	    concrete_h[i],vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\vtkWrapJava %s\\%s\\%s.h\\\n",
	    vals->m_WhereVTK, vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\wrap\\hints 1 > src\\%sJava.cxx\n\n",
	    vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"   %s\\pcmaker\\vtkParseJava %s\\%s\\%s.h\\\n",
	    vals->m_WhereVTK, vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  %s\\wrap\\hints 1 > vtk\\%s.java\n\n",
	    vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sJava.obj\" : src\\%sJava.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sJava.cxx\n\n",concrete_h[i]);
    }


  fprintf(fp,"################################################################################\n");
}



void doBorJavaHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag)
{
  int i;

  fprintf(fp,"# VTK Borland makefile\n");
  fprintf(fp,".autodepend\n");
  fprintf(fp,"OUTDIR=obj\n\n");
  fprintf(fp,"WHEREVTK=%s\n",vals->m_WhereVTK);
  fprintf(fp,"WHEREJDK=%s\n\n",vals->m_WhereJDK);
  fprintf(fp,"WHEREBUILD=%s\n",vals->m_WhereBuild);
  fprintf(fp,"WHERECOMP=%s\n\n",vals->m_WhereCompiler);
  fprintf(fp,"CPP=BCC32.exe +CPP_PROJ.CFG\n\n");
  fprintf(fp,"ALL : %sjava.dll\n\n",vals->adlg.m_LibPrefix);

  fprintf(fp,"$(OUTDIR) ::\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)$(NULL)\" mkdir \"$(OUTDIR)\"\n");
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
    fprintf(fp,"-I$(WHEREVTK)\\patented\n");
    }
  if (vals->m_Contrib)
    {
    fprintf(fp,"-I$(WHEREVTK)\\contrib\n");
    }
  fprintf(fp,"-D_WINDOWS;_WINDLL;_USRDLL;VTKDLL;_RTLDLL;VTKJAVA;STRICT");

  // JCP take Borland Builder specific action ...
  if (!bbuilder) fprintf(fp,";__int64"); 

  fprintf(fp,"\n-tWM -tWD -Od -H- -VF -I$(WHERECOMP)\\include;$(WHERECOMP)\\include\\rw;$(WHEREVTK)\\common;$(WHEREVTK)\\graphics -DWIN32\n");
  fprintf(fp," -I$(WHEREJDK)\\include \n");
  fprintf(fp," -I$(WHEREJDK)\\include\\win32 \n");
  fprintf(fp," -I$(WHEREVTK)\\imaging \n");
  fprintf(fp," -I$(WHEREVTK)\\contrib \n");
  fprintf(fp," -I$(WHEREVTK)\\local \n");
  fprintf(fp," -I$(WHEREBUILD) \n");
  fprintf(fp,"-P -c -w-hid -w-inl -w-ccc -w-aus \n");
  fprintf(fp,"| CPP_PROJ.CFG \n\n");
  // JCP take Borland Builder specific action ...
  if (bbuilder) 
	{
	fprintf(fp,"LINK32=ilink32.exe\n\n");
	} 
  else 
	{ 
	fprintf(fp,"LINK32=tlink32.exe\n\n");
	}

  fprintf(fp,"LINK32_FLAGS=-L$(WHERECOMP)\\lib \\\n");
  if (debugFlag)
    {
    fprintf(fp,"  -v \\\n");
    }
  else
    {
    fprintf(fp,"  -v- \\\n");
    }
  // JCP take Borland Builder specific action ...
  if (bbuilder) 
	{
	fprintf(fp,"  -Tpd -aa -V4.0 -m -n\n");
	} 
  else 
	{
	fprintf(fp,"  -Tpd -aa -V4.0 -Gm  -w-inq -m -n\n");
	}

  fprintf(fp,"DEPLINK32_OBJS= \\\n");

  // JCP take Borland Builder specific action ...
  if (bbuilder == 4) 
	{
	  fprintf(fp,"    $(WHERECOMP)\\lib\\c0pkg32.obj \\\n");
	}
    else {
	  fprintf(fp,"    $(WHERECOMP)\\lib\\c0pkg32.obj \\\n");
	}
  fprintf(fp,"    $(OUTDIR)\\vtkJavaUtil.obj \\\n");
  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp,"    $(OUTDIR)\\%sJava.obj \\\n",abstract[i]);
    }
  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp,"    $(OUTDIR)\\%sJava.obj \\\n",concrete[i]);
    }
  for (i = 0; i < num_abstract_h; i++)
    {
    fprintf(fp,"    $(OUTDIR)\\%sJava.obj \\\n",abstract_h[i]);
    }
  for (i = 0; i < num_concrete_h; i++)
    {
    fprintf(fp,"    $(OUTDIR)\\%sJava.obj \\\n",concrete_h[i]);
    }
  fprintf(fp,"\n");
  fprintf(fp,"LINK32_OBJS= \\\n");

  fprintf(fp,"    $(WHERECOMP)\\lib\\c0d32.obj");

  fprintf(fp,"    $(OUTDIR)\\vtkJavaUtil.obj");
  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%sJava.obj",abstract[i]);
    }
  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%sJava.obj",concrete[i]);
    }
  for (i = 0; i < num_abstract_h; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%sJava.obj",abstract_h[i]);
    }
  for (i = 0; i < num_concrete_h; i++)
    {
    fprintf(fp,"+ \\\n    $(OUTDIR)\\%sJava.obj",concrete_h[i]);
    }
  fprintf(fp,", \\\n $<,$*,  \\\n");   // this is the target and map file name
  fprintf(fp,"    $(WHEREJDK)\\lib\\javai.lib \\\n");
  // JCP take Borland Builder specific action ...
  if (bbuilder == 3) 
	{
   if (debugFlag)
      {
	 fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcl35.lib \\\n");
      } else {
	 fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcl35.lib \\\n");
      }
	fprintf(fp,"    $(WHERECOMP)\\lib\\vcl.lib \\\n");
	}
  // JCP take Borland Builder specific action ...
  if (bbuilder == 4) 
	{
   if (debugFlag)
      {
	fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcl40.lib \\\n");
	fprintf(fp,"    $(WHERECOMP)\\lib\\debug\\vcle40.lib \\\n");
      } else {
	fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcl40.lib \\\n");
	fprintf(fp,"    $(WHERECOMP)\\lib\\release\\vcle40.lib \\\n");
       }
	fprintf(fp,"    $(WHERECOMP)\\lib\\vcl.lib \\\n");
      fprintf(fp,"    $(WHERECOMP)\\lib\\cp32mt.lib \\\n");
	}

  fprintf(fp,"    $(WHERECOMP)\\lib\\import32.lib \\\n");
  if (bbuilder < 4) 
    {
      fprintf(fp,"    $(WHERECOMP)\\lib\\cw32mt.lib \\\n");
    }
  fprintf(fp,"    ..\\vtkdll\\%sdll.lib \\\n",vals->adlg.m_LibPrefix);
  fprintf(fp,"\n");
  fprintf(fp,"%sjava.dll : obj $(DEF_FILE) $(DEPLINK32_OBJS)\n",
	  vals->adlg.m_LibPrefix);
  fprintf(fp,"    $(LINK32) @&&| \n");
  fprintf(fp,"  $(LINK32_FLAGS) $(LINK32_OBJS)\n");
  fprintf(fp,"|  \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_OBJS)}.obj:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".c{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cpp{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,".cxx{$(CPP_SBRS)}.sbr:\n");
  fprintf(fp,"   $(CPP) @&&| \n");
  fprintf(fp,"-o$(OUTDIR)\\$@ $< \n");
  fprintf(fp,"| \n");
  fprintf(fp,"\n");
  fprintf(fp,"################################################################################\n");
  fprintf(fp,"\n");
  fprintf(fp,"$(OUTDIR)\\vtkJavaUtil.obj : $(WHEREVTK)\\common\\vtkJavaUtil.cxx \n");
  fprintf(fp,"  $(CPP) @&&|\n");
  fprintf(fp,"-o$(OUTDIR)\\vtkJavaUtil.obj  $(WHEREVTK)\\common\\vtkJavaUtil.cxx\n\n");
  fprintf(fp,"|  \n");


  for (i = 0; i < num_abstract; i++)
    {
    fprintf(fp,"src\\%sJava.cxx : $(WHEREVTK)\\%s\\%s.h \n",
	    abstract[i],abstract_lib[i],abstract[i]);
    fprintf(fp,"  copy &&| \n");


    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkWrapJava $(WHEREVTK)\\%s\\%s.h\\\n",
	    abstract_lib[i], abstract[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 0 > src\\%sJava.cxx\n\n",
	    abstract[i]);
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkParseJava $(WHEREVTK)\\%s\\%s.h\\\n",
	    abstract_lib[i], abstract[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 0 > vtk\\%s.java\n\n",
	    abstract[i]);
    fprintf(fp,"| vtkjava.bat \n  vtkjava \n\n");


    fprintf(fp,"$(OUTDIR)\\%sJava.obj : src\\%sJava.cxx \n",
	    abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%sJava.obj src\\%sJava.cxx\n\n",
	    abstract[i],abstract[i]);
    fprintf(fp,"|  \n");
    }


  for (i = 0; i < num_concrete; i++)
    {
    fprintf(fp,"src\\%sJava.cxx : $(WHEREVTK)\\%s\\%s.h \n",
	    concrete[i],concrete_lib[i],concrete[i]);
    fprintf(fp,"  copy &&| \n");


    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkWrapJava $(WHEREVTK)\\%s\\%s.h\\\n",
	    concrete_lib[i], concrete[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 1 > src\\%sJava.cxx\n\n",
	    concrete[i]);
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkParseJava $(WHEREVTK)\\%s\\%s.h\\\n",
	    concrete_lib[i], concrete[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 1 > vtk\\%s.java\n\n",
	    concrete[i]);
    fprintf(fp,"| vtkjava.bat \n  vtkjava \n\n");


    fprintf(fp,"$(OUTDIR)\\%sJava.obj : src\\%sJava.cxx \n",
	    concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%sJava.obj src\\%sJava.cxx\n\n",
	    concrete[i],concrete[i]);
    fprintf(fp,"|  \n");
    }


  for (i = 0; i < num_abstract_h; i++)
    {
    fprintf(fp,"src\\%sJava.cxx : $(WHEREVTK)\\%s\\%s.h \n",
	    abstract_h[i],abstract_h_lib[i],abstract_h[i]);
    fprintf(fp,"  copy &&| \n");
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkWrapJava $(WHEREVTK)\\%s\\%s.h\\\n",
	    abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 0 > src\\%sJava.cxx\n\n",
	    abstract_h[i]);
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkParseJava $(WHEREVTK)\\%s\\%s.h\\\n",
	    abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 0 > vtk\\%s.java\n\n",
	    abstract_h[i]);
    fprintf(fp,"| vtkjava.bat \n  vtkjava \n\n");


    fprintf(fp,"$(OUTDIR)\\%sJava.obj : src\\%sJava.cxx \n",
	    abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%sJava.obj src\\%sJava.cxx\n\n",
	    abstract_h[i],abstract_h[i]);
    fprintf(fp,"|  \n");
    }


  for (i = 0; i < num_concrete_h; i++)
    {
    fprintf(fp,"src\\%sJava.cxx : $(WHEREVTK)\\%s\\%s.h \n",
	    concrete_h[i],concrete_h_lib[i],concrete_h[i]);
    fprintf(fp,"  copy &&| \n");
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkWrapJava $(WHEREVTK)\\%s\\%s.h\\\n",
	    concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 1 > src\\%sJava.cxx\n\n",
	    concrete_h[i]);
    fprintf(fp,"   $(WHEREVTK)\\pcmaker\\vtkParseJava $(WHEREVTK)\\%s\\%s.h\\\n",
	    concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  $(WHEREVTK)\\wrap\\hints 1 > vtk\\%s.java\n\n",
	    concrete_h[i]);
    fprintf(fp,"| vtkjava.bat \n  vtkjava \n\n");


    fprintf(fp,"$(OUTDIR)\\%sJava.obj : src\\%sJava.cxx \n",
	    concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) @&&|\n");
    fprintf(fp,"-o$(OUTDIR)\\%sJava.obj src\\%sJava.cxx\n\n",
	    concrete_h[i],concrete_h[i]);
    fprintf(fp,"|  \n");
    }


  fprintf(fp,"################################################################################\n");
}


// links in ALL the vtk libraries (now that they are split up)... also writes output to ../lib directory
// may only want to write the dll to that directory????
// makefile handles both incremental and non-incremental linking
void doMSCPythonHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag)
{
  int i;
  char file [256];
  char targetName[256];

  fprintf(fp,"# VTK Generic makefile\n");
  fprintf(fp,"!MESSAGE making python library...\n");
  fprintf(fp,"CPP=cl.exe\n");
  fprintf(fp,"PATH=$(PATH);\"%s\\pcmaker\\\"\n",vals->m_WhereVTK);
  fprintf(fp,"CPP_PARSE=vtkWrapPython.exe\n");
  fprintf(fp,"OUTDIR=obj\n\n");

  fprintf(fp,"LIBDIR=..\\lib\n\n");

  if (debugFlag)
    {
    fprintf(fp,"ALL : %spython_d.dll\n\n",vals->adlg.m_LibPrefix);
    sprintf(targetName,"%spython_d",vals->adlg.m_LibPrefix);
    }
  else
    {
    fprintf(fp,"ALL : %spython.dll\n\n",vals->adlg.m_LibPrefix);
    sprintf(targetName,"%spython",vals->adlg.m_LibPrefix);
    }

  fprintf(fp,"\"$(OUTDIR)\" :\n");
  fprintf(fp,"    if not exist \"$(OUTDIR)/$(NULL)\" mkdir \"$(OUTDIR)\"\n");
  fprintf(fp,"\n");

  if (debugFlag)
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /D \"_DEBUG\" /nologo /MDd /W3 /Od /Zi /I \"%s\\include\" /I \"%s\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\" \\\n",
	    vals->m_WhereCompiler, vals->m_WhereBuild, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }
  else
    {
    fprintf(fp,"CPP_PROJ=/D \"STRICT\" /nologo /MD /W3 /O2 /I \"%s\\include\" /I \"%s\" /I \"%s\\common\" /I \"%s\\graphics\" /I \"%s\\imaging\" /D \"NDEBUG\" /D \"WIN32\" /D \"_WINDOWS\" \\\n",
	    vals->m_WhereCompiler, vals->m_WhereBuild, vals->m_WhereVTK, vals->m_WhereVTK, vals->m_WhereVTK);
    }

  fprintf(fp," /I \"%s\\Include\" ", vals->m_WherePy);

  // Python.h needs config.h, which is automatically generated by configure.
  // A predefined Win32 config.h is found in python/PC. 

  fprintf(fp," /I \"%s\\PC\" ", vals->m_WherePy);

  if (vals->m_Patented) fprintf(fp," /I \"%s\\patented\" \\\n",
				vals->m_WhereVTK);
  if (vals->m_Contrib) fprintf(fp," /I \"%s\\contrib\" \\\n",
			       vals->m_WhereVTK);
  if (vals->m_Local) fprintf(fp," /I \"%s\\local\" \\\n",vals->m_WhereVTK);

  fprintf(fp," /D \"_WINDLL\" /D \"_MBCS\" \\\n");


  if (!debugFlag && vals->m_Lean)
    {
    fprintf(fp," /D \"VTK_LEAN_AND_MEAN\" /Fo$(OUTDIR)\\ /c \n");
    }
  else
    {
    fprintf(fp," /Fo$(OUTDIR)\\ /c \n");
    }


  //fprintf(fp,"CPP_PROJ2=$(CPP_PROJ) /D \"VTKDLL\"\n");
  fprintf(fp,"CPP_PROJ2=$(CPP_PROJ) \n");
  
  fprintf(fp,"LINK32=link.exe\n");
  if (debugFlag)
    {
    fprintf(fp,"LINK32_FLAGS=\"$(OUTDIR)\\vtkpython.obj\" /debug /libpath:\"%s\\lib\"  \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows %s",
	    vals->m_WhereCompiler, vals->m_WhereCompiler, 
            vals->m_WhereCompiler, vals->adlg.m_EXTRA_LINK_FLAGS);
    }
  else
    {
    fprintf(fp,"LINK32_FLAGS=\"$(OUTDIR)\\vtkpython.obj\" /libpath:\"%s\\lib\" \"%s\\lib\\gdi32.lib\" \"%s\\lib\\user32.lib\" /nologo /version:1.3 /subsystem:windows %s",
	    vals->m_WhereCompiler, vals->m_WhereCompiler, 
            vals->m_WhereCompiler, vals->adlg.m_EXTRA_LINK_FLAGS);
    }

  fprintf(fp," /libpath:\"%s\\PCbuild\" /libpath:\"%s\\libs\" \n",
          vals->m_WherePy,vals->m_WherePy);
  fprintf(fp,"MORE_FLAGS1=/dll /incremental:yes /pdb:\"$(LIBDIR)/%s.pdb\" /machine:I386\\\n",targetName);
  fprintf(fp," /out:\"$(LIBDIR)/%s.dll\" /implib:\"$(LIBDIR)/%s.lib\" \n\n",targetName,targetName); 
  fprintf(fp,"MORE_FLAGS2=/dll /incremental:no /pdb:%s.pdb /machine:I386\\\n", targetName);
  fprintf(fp," /out:%s.dll /implib:%s.lib \n\n", targetName, targetName); 
  fprintf(fp,"LIB_FLAGS=/machine:I386\n\n"); 


  fprintf(fp,"PYTHONOBJLIBS=vtkpythonotherobjs.lib vtkpythongraphicsobjs.lib\n\n");


  fprintf(fp,"VTKDLL_LIB=..\\vtkdll\\%sdll.lib \n\n",
	  vals->adlg.m_LibPrefix);


  fprintf(fp,"VTK_LIBRARIES=..\\lib\\vtkCommon.lib ");
  if (vals->m_Graphics) 
    {
    for (i = 0; i < NumOfGraphicsLibs; i++)
      fprintf(fp,"..\\lib\\vtkGraphics%d.lib  ",i);
    }
  if (vals->m_Patented)
    fprintf(fp,"..\\lib\\vtkPatented.lib ");
  if (vals->m_Imaging) 
    fprintf(fp,"..\\lib\\vtkImaging.lib ");
  if (vals->m_Contrib)
    fprintf(fp,"..\\lib\\vtkContrib.lib ");     
  if (vals->m_Local)
    fprintf(fp,"..\\lib\\vtkLocal.lib ");     
  fprintf(fp,"\n\n");


  // All the graphics Python objects
  if (vals->m_Graphics)
    {
    fprintf(fp,"GRAPHICSPYTHON_OBJS= \\\n");
    //fprintf(fp,"    \"$(OUTDIR)\\vtkTkRenderWidget.obj\" \\\n");
    for (i = abstractStart[LT_GRAPHICS]; i < abstractEnd[LT_GRAPHICS]; i++)
      fprintf(fp,"    \"$(OUTDIR)\\%sPython.obj\" \\\n",abstract[i]);


    for (i = concreteStart[LT_GRAPHICS]; i < concreteEnd[LT_GRAPHICS]; i++)
      fprintf(fp,"    \"$(OUTDIR)\\%sPython.obj\" \\\n",concrete[i]);


    for (i = abstractHStart[LT_GRAPHICS]; i < abstractHEnd[LT_GRAPHICS]; i++)
      fprintf(fp,"    \"$(OUTDIR)\\%sPython.obj\" \\\n",abstract_h[i]);


    for (i = concreteHStart[LT_GRAPHICS]; i < concreteHEnd[LT_GRAPHICS]; i++)
      fprintf(fp,"    \"$(OUTDIR)\\%sPython.obj\" \\\n",concrete_h[i]);
    fprintf(fp,"\n");
    }




  // Now all the PYTHON objects other than graphics
  fprintf(fp,"OTHERPYTHON_OBJS= \\\n");
  fprintf(fp,"    \"$(OUTDIR)\\vtkPythonUtil.obj\" \\\n");
  if (vals->m_Imaging)  
    {
    //fprintf(fp,"    \"$(OUTDIR)\\vtkTkImageViewerWidget.obj\" \\\n");
    //fprintf(fp,"    \"$(OUTDIR)\\vtkTkImageWindowWidget.obj\" \\\n");
    }


  for (i = (vals->m_Graphics ? abstractEnd[LT_GRAPHICS] : 0); 
       i < abstractEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OUTDIR)\\%sPython.obj\" \\\n",abstract[i]);


  for (i = (vals->m_Graphics ? concreteEnd[LT_GRAPHICS] : 0); 
       i < concreteEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OUTDIR)\\%sPython.obj\" \\\n",concrete[i]);


  for (i = (vals->m_Graphics ? abstractHEnd[LT_GRAPHICS] : 0);
       i < abstractHEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OUTDIR)\\%sPython.obj\" \\\n",abstract_h[i]);


  for (i = (vals->m_Graphics ? concreteHEnd[LT_GRAPHICS] : 0);
       i < concreteHEnd[LT_COMMON]; i++)
    fprintf(fp,"    \"$(OUTDIR)\\%sPython.obj\" \\\n",concrete_h[i]);
  fprintf(fp,"\n");
  
  fprintf(fp,"%s.dll : \"$(OUTDIR)\" $(DEF_FILE) \"$(OUTDIR)\\vtkpython.obj\" $(PYTHONOBJLIBS)\n", targetName);
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(MORE_FLAGS2) $(PYTHONOBJLIBS) $(VTKDLL_LIB)\n");
  fprintf(fp,"<<\n\n");


  fprintf(fp,"\"$(LIBDIR)\\%s.dll\" : \"$(OUTDIR)\" $(DEF_FILE) \"$(OUTDIR)\\vtkpython.obj\" $(PYTHONOBJLIBS)\n", targetName);
  fprintf(fp,"    $(LINK32) @<<\n");
  fprintf(fp,"  $(LINK32_FLAGS) $(MORE_FLAGS1) $(PYTHONOBJLIBS) $(VTK_LIBRARIES)\n");
  fprintf(fp,"<<\n\n");


  if (vals->m_Graphics)
    {
    fprintf(fp,"vtkpythongraphicsobjs.lib : \"$(OUTDIR)\" $(DEF_FILE) $(GRAPHICSPYTHON_OBJS) \n");
    fprintf(fp,"    lib.exe @<<\n");
    fprintf(fp,"  /out:vtkpythongraphicsobjs.lib $(LIB_FLAGS) $(GRAPHICSPYTHON_OBJS)\n");
    fprintf(fp,"<<\n\n");
    }


  fprintf(fp,"vtkpythonotherobjs.lib : \"$(OUTDIR)\" $(DEF_FILE) $(OTHERPYTHON_OBJS) \n");
  fprintf(fp,"    lib.exe @<<\n");
  fprintf(fp,"  /out:vtkpythonotherobjs.lib $(LIB_FLAGS) $(OTHERPYTHON_OBJS)\n");
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
  fprintf(fp,"\"$(OUTDIR)\\vtkPythonUtil.obj\" : \"%s\\common\\vtkPythonUtil.cxx\" \"$(OUTDIR)\"\n",
	  vals->m_WhereVTK);
  fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\common\\vtkPythonUtil.cxx\"\n\n",vals->m_WhereVTK);
  if (vals->m_Graphics)
    {
    //sprintf(file,"%s\\graphics\\vtkTkRenderWidget.cxx",vals->m_WhereVTK);
    //OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    //fprintf(fp,"\"$(OUTDIR)\\vtkTkRenderWidget.obj\" : \"%s\\graphics\\vtkTkRenderWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	  //  vals->m_WhereVTK);
    //fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\graphics\\vtkTkRenderWidget.cxx\"\n\n",vals->m_WhereVTK);
    }
  if (vals->m_Imaging)
    {
    //sprintf(file,"%s\\imaging\\vtkTkImageViewerWidget.cxx",vals->m_WhereVTK);
    //OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    //fprintf(fp,"\"$(OUTDIR)\\vtkTkImageViewerWidget.obj\" : \"%s\\imaging\\vtkTkImageViewerWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	  //  vals->m_WhereVTK);
    //fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\imaging\\vtkTkImageViewerWidget.cxx\"\n\n",vals->m_WhereVTK);
    //sprintf(file,"%s\\imaging\\vtkTkImageWindowWidget.cxx",vals->m_WhereVTK);
    //OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    //fprintf(fp,"\"$(OUTDIR)\\vtkTkImageWindowWidget.obj\" : \"%s\\imaging\\vtkTkImageWindowWidget.cxx\" $(DEPENDS) \"$(OUTDIR)\"\n",
	  //  vals->m_WhereVTK);
    //fprintf(fp,"  $(CPP) $(CPP_PROJ) \"%s\\imaging\\vtkTkImageWindowWidget.cxx\"\n\n",vals->m_WhereVTK);
    }


  fprintf(fp,"\"$(OUTDIR)\\vtkpython.obj\" : src\\vtkpython.cxx \"$(OUTDIR)\"\n");
  fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\vtkpython.cxx\n\n");


  for (i = 0; i < num_abstract; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_lib[i],abstract[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sPython.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkPythonUtil.h\" \"%s\\wrap\\vtkParse.y\" \"%s\\wrap\\hints\" \"$(OUTDIR)\"\n",
	    abstract[i],vals->m_WhereVTK,abstract_lib[i],abstract[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE)  \"%s\\%s\\%s.h\"\\\n",
	    vals->m_WhereVTK, abstract_lib[i], abstract[i]);
    fprintf(fp,"  \"%s\\wrap\\hints\" 0 > src\\%sPython.cxx\n\n",
	    vals->m_WhereVTK, abstract[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sPython.obj\" : src\\%sPython.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    abstract[i],abstract[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sPython.cxx\n\n",abstract[i]);
    }


  for (i = 0; i < num_concrete; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_lib[i],concrete[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sPython.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkPythonUtil.h\" \"%s\\wrap\\vtkParse.y\" \"%s\\wrap\\hints\" \"$(OUTDIR)\"\n",
	    concrete[i],vals->m_WhereVTK,concrete_lib[i],concrete[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
	    vals->m_WhereVTK, concrete_lib[i], concrete[i]);
    fprintf(fp,"  \"%s\\wrap\\hints\" 1 > src\\%sPython.cxx\n\n",
	    vals->m_WhereVTK, concrete[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sPython.obj\" : src\\%sPython.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    concrete[i],concrete[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sPython.cxx\n\n",concrete[i]);
    }


  for (i = 0; i < num_abstract_h; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sPython.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkPythonUtil.h\" \"%s\\wrap\\vtkParse.y\" \"%s\\wrap\\hints\" \"$(OUTDIR)\"\n",
	    abstract_h[i],vals->m_WhereVTK,abstract_h_lib[i],abstract_h[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
	    vals->m_WhereVTK, abstract_h_lib[i], abstract_h[i]);
    fprintf(fp,"  \"%s\\wrap\\hints\" 0 > src\\%sPython.cxx\n\n",
	    vals->m_WhereVTK, abstract_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sPython.obj\" : src\\%sPython.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    abstract_h[i],abstract_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sPython.cxx\n\n",abstract_h[i]);
    }


  for (i = 0; i < num_concrete_h; i++)
    {
    sprintf(file,"%s\\%s\\%s.h",vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i]);
    OutputPCDepends(file,fp,vals->m_WhereVTK, extraPtr, extra_num);
    vals->m_Progress.OffsetPos(1);
    fprintf(fp,"\"src\\%sPython.cxx\" : \"%s\\%s\\%s.h\" \"%s\\common\\vtkPythonUtil.h\" \"%s\\wrap\\vtkParse.y\" \"%s\\wrap\\hints\" \"$(OUTDIR)\"\n",
	    concrete_h[i],vals->m_WhereVTK,concrete_h_lib[i],concrete_h[i],vals->m_WhereVTK,vals->m_WhereVTK,vals->m_WhereVTK);
    fprintf(fp," $(CPP_PARSE) \"%s\\%s\\%s.h\"\\\n",
	    vals->m_WhereVTK, concrete_h_lib[i], concrete_h[i]);
    fprintf(fp,"  \"%s\\wrap\\hints\" 1 > src\\%sPython.cxx\n\n",
	    vals->m_WhereVTK, concrete_h[i]);
    fprintf(fp,"\"$(OUTDIR)\\%sPython.obj\" : src\\%sPython.cxx $(DEPENDS) \"$(OUTDIR)\"\n",
	    concrete_h[i],concrete_h[i]);
    fprintf(fp,"  $(CPP) $(CPP_PROJ) src\\%sPython.cxx\n\n",concrete_h[i]);
    }


  fprintf(fp,"################################################################################\n");
}

// links in ALL the vtk libraries (now that they are split up)... also writes output to ../lib directory
// may only want to write the dll to that directory????
// makefile handles both incremental and non-incremental linking
void doBorPythonHeader(FILE *fp,CPcmakerDlg *vals, int debugFlag)
{
  fprintf(fp,"### Not Implemented Yet\n");

  fprintf(fp,"################################################################################\n");
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
	sprintf(file,"%s\\lib\\%stcl.dll",vals->m_WhereBuild,
		vals->adlg.m_LibPrefix);
      else
	sprintf(file,"%s\\Debug\\lib\\%stcl.dll",vals->m_WhereBuild,
		vals->adlg.m_LibPrefix);
				
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
            sprintf(file,"%s\\lib\\%stcl.dll",vals->m_WhereBuild,
		    vals->adlg.m_LibPrefix);
          else
            sprintf(file,"%s\\Debug\\lib\\%stcl.dll",vals->m_WhereBuild,
		    vals->adlg.m_LibPrefix);
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
