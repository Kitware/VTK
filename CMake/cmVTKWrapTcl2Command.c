/* this is a CMake loadable command to wrap vtk objects into Tcl */

#include "cmCPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct 
{
  char *LibraryName;
  int NumberWrapped;
  void **SourceFiles;
  char **HeaderFiles;
} cmVTKWrapTclData;

/* this roputine creates the init file */
static void CreateInitFile(cmLoadedCommandInfo *info,
                           void *mf, const char *libName, 
                           int numConcrete, const char **concrete, 
                           int numCommands, const char **commands) 
{
  /* we have to make sure that the name is the correct case */
  char *kitName = info->CAPI->Capitalized(libName);
  int i;
  char *tempOutputFile;  
  char *outFileName = 
    (char *)malloc(strlen(info->CAPI->GetCurrentOutputDirectory(mf)) + 
                   strlen(libName) + 10);
  char **capcommands = (char **)malloc(numCommands*sizeof(char *));
  FILE *fout;
  
  sprintf(outFileName,"%s/%sInit.cxx",
          info->CAPI->GetCurrentOutputDirectory(mf), libName);
  
  tempOutputFile = (char *)malloc(strlen(outFileName) + 5);
  sprintf(tempOutputFile,"%s.tmp",outFileName);
  fout = fopen(tempOutputFile,"w");
  if (!fout)
    {
    return;
    }
  
  /* capitalized commands just once */
  for (i = 0; i < numCommands; i++)
    {
    capcommands[i] = info->CAPI->Capitalized(commands[i]);
    }
  
  fprintf(fout,"#include \"vtkTclUtil.h\"\n");
  fprintf(fout,"#include \"vtkVersion.h\"\n");
  fprintf(fout,"#define VTK_TCL_TO_STRING(x) VTK_TCL_TO_STRING0(x)\n");
  fprintf(fout,"#define VTK_TCL_TO_STRING0(x) #x\n");

  fprintf(fout,
          "extern \"C\"\n"
          "{\n"
          "#if (TCL_MAJOR_VERSION == 8) && (TCL_MINOR_VERSION >= 4) && (TCL_RELEASE_LEVEL >= TCL_FINAL_RELEASE)\n"
          "  typedef int (*vtkTclCommandType)(ClientData, Tcl_Interp *,int, CONST84 char *[]);\n"
          "#else\n"
          "  typedef int (*vtkTclCommandType)(ClientData, Tcl_Interp *,int, char *[]);\n"
          "#endif\n"
          "}\n"
          "\n");

  for (i = 0; i < numConcrete; i++)
    {
    fprintf(fout,"int %sCommand(ClientData cd, Tcl_Interp *interp,\n             int argc, char *argv[]);\n",concrete[i]);
    fprintf(fout,"ClientData %sNewCommand();\n",concrete[i]);
    }
  
  if (!strcmp(kitName,"Vtkcommontcl"))
    {
    fprintf(fout,"int vtkCreateCommand(ClientData cd, Tcl_Interp *interp,\n"
            "               int argc, char *argv[]);\n");
    fprintf(fout,"\nTcl_HashTable vtkInstanceLookup;\n");
    fprintf(fout,"Tcl_HashTable vtkPointerLookup;\n");
    fprintf(fout,"Tcl_HashTable vtkCommandLookup;\n");
    }
  else
    {
    fprintf(fout,"\nextern Tcl_HashTable vtkInstanceLookup;\n");
    fprintf(fout,"extern Tcl_HashTable vtkPointerLookup;\n");
    fprintf(fout,"extern Tcl_HashTable vtkCommandLookup;\n");
    }
  fprintf(fout,"extern void vtkTclDeleteObjectFromHash(void *);\n");  
  fprintf(fout,"extern void vtkTclListInstances(Tcl_Interp *interp, ClientData arg);\n");
  
  for (i = 0; i < numCommands; i++)
    {
    fprintf(fout,
            "\nextern \"C\" {int VTK_EXPORT %s_Init(Tcl_Interp *interp);}\n",
            capcommands[i]);
    }
  
  fprintf(fout,
          "\n\nextern \"C\" {int VTK_EXPORT %s_SafeInit(Tcl_Interp *interp);}\n",
    kitName);
  fprintf(fout,
          "\nextern \"C\" {int VTK_EXPORT %s_Init(Tcl_Interp *interp);}\n",
    kitName);
  
  /* create an extern ref to the generic delete function */
  fprintf(fout,"\nextern void vtkTclGenericDeleteObject(ClientData cd);\n");
  
  if (!strcmp(kitName,"Vtkcommontcl"))
    {
    fprintf(fout,
            "extern \"C\"\n{\nvoid vtkCommonDeleteAssocData(ClientData cd)\n");
    fprintf(fout,"  {\n");
    fprintf(fout,"  vtkTclInterpStruct *tis = static_cast<vtkTclInterpStruct*>(cd);\n");
    fprintf(fout,"  delete tis;\n  }\n}\n");
    }
  
  /* the main declaration */
  fprintf(fout,
          "\n\nint VTK_EXPORT %s_SafeInit(Tcl_Interp *interp)\n{\n",kitName);
  fprintf(fout,"  return %s_Init(interp);\n}\n",kitName);
  
  fprintf(fout,"\n\nint VTK_EXPORT %s_Init(Tcl_Interp *interp)\n{\n",
          kitName);
  if (!strcmp(kitName,"Vtkcommontcl"))
    {
    fprintf(fout,
      "  vtkTclInterpStruct *info = new vtkTclInterpStruct;\n");
    fprintf(fout,
            "  info->Number = 0; info->InDelete = 0; info->DebugOn = 0;\n");
    fprintf(fout,"\n");
    fprintf(fout,"\n");
    fprintf(fout,
      "  Tcl_InitHashTable(&info->InstanceLookup, TCL_STRING_KEYS);\n");
    fprintf(fout,
      "  Tcl_InitHashTable(&info->PointerLookup, TCL_STRING_KEYS);\n");
    fprintf(fout,
      "  Tcl_InitHashTable(&info->CommandLookup, TCL_STRING_KEYS);\n");
    fprintf(fout,
            "  Tcl_SetAssocData(interp,(char *) \"vtk\",NULL,(ClientData *)info);\n");
    fprintf(fout,
            "  Tcl_CreateExitHandler(vtkCommonDeleteAssocData,(ClientData *)info);\n");
    
    /* create special vtkCommand command */
    fprintf(fout,"  Tcl_CreateCommand(interp,(char *) \"vtkCommand\",\n"
            "                    reinterpret_cast<vtkTclCommandType>(vtkCreateCommand),\n"
            "                    (ClientData *)NULL, NULL);\n\n");
    }
  
  for (i = 0; i < numCommands; i++)
    {
    fprintf(fout,"  %s_Init(interp);\n", capcommands[i]);
    }
  fprintf(fout,"\n");

  for (i = 0; i < numConcrete; i++)
    {
    fprintf(fout,"  vtkTclCreateNew(interp,(char *) \"%s\", %sNewCommand,\n",
      concrete[i], concrete[i]);
    fprintf(fout,"                  %sCommand);\n",concrete[i]);
    }
  
  fprintf(fout,"  char pkgName[]=\"%s\";\n", libName);
  fprintf(fout,"  char pkgVers[]=VTK_TCL_TO_STRING(VTK_MAJOR_VERSION)"
               " \".\" "
               "VTK_TCL_TO_STRING(VTK_MINOR_VERSION);\n");
  fprintf(fout,"  Tcl_PkgProvide(interp, pkgName, pkgVers);\n");
  fprintf(fout,"  return TCL_OK;\n}\n");
  fclose(fout);

  /* copy the file if different */
  info->CAPI->CopyFileIfDifferent(tempOutputFile, outFileName);
  info->CAPI->RemoveFile(tempOutputFile);
  for ( i = 0; i < numCommands; i++ )
    {
    info->CAPI->Free(capcommands[i]);
    }
  free(capcommands);
  free(tempOutputFile);
  info->CAPI->Free(kitName);
  free(outFileName);
}

/* do almost everything in the initial pass */
static int InitialPass(void *inf, void *mf, int argc, char *argv[])
{
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  int i;
  int newArgc;
  char **newArgv;
  int doing_sources = 1;
  char **sources = 0;
  char **commands = 0;
  int numSources = 0;
  int numCommands = 0;
  int numConcrete = 0;
  char **concrete = 0;
  int numWrapped = 0;
  cmVTKWrapTclData *cdata = 
    (cmVTKWrapTclData *)malloc(sizeof(cmVTKWrapTclData));
  
  if(argc < 3 )
    {
    info->CAPI->SetError(info, "called with incorrect number of arguments");
    return 0;
    }
  
  if (!strcmp(argv[1],"SOURCES"))
    {
    info->CAPI->ExpandSourceListArguments(mf, argc, argv, &newArgc, 
                                          &newArgv, 3);
    }
  else
    {
    info->CAPI->ExpandSourceListArguments(mf, argc, argv, 
                                          &newArgc, &newArgv, 2);
    }
  
  /* Now check and see if the value has been stored in the cache */
  /* already, if so use that value and don't look for the program */
  if(!info->CAPI->IsOn(mf,"VTK_WRAP_TCL"))
    {
    info->CAPI->FreeArguments(newArgc, newArgv);
    return 1;
    }

  /* extract the sources and commands parameters */
  sources = (char **)malloc(sizeof(char *)*newArgc);
  commands = (char **)malloc(sizeof(char *)*newArgc);
  concrete = (char **)malloc(sizeof(char *)*newArgc);
  cdata->SourceFiles = (void **)malloc(sizeof(void *)*newArgc);
  cdata->HeaderFiles = (char **)malloc(sizeof(char *)*newArgc);
  
  for(i = 1; i < newArgc; ++i)
    {   
    if(!strcmp(newArgv[i],"SOURCES"))
      {
      doing_sources = 1;
      }
    else if (!strcmp(newArgv[i],"COMMANDS"))
      {
      doing_sources = 0;
      }
    else
      { 
      if(doing_sources)
        {
        sources[numSources] = newArgv[i];
        numSources++;
        }
      else
        {
        commands[numCommands] = newArgv[i];
        numCommands++;
        }
      }
    }
  
  /* get the list of classes for this library */
  if (numSources)
    {
    /* what is the current source dir */
    const char *cdir = info->CAPI->GetCurrentDirectory(mf);
    int sourceListSize = 0;
    char *sourceListValue = 0;
    void *cfile = 0;
    char *newName;

    /* was the list already populated */
    const char *def = info->CAPI->GetDefinition(mf, sources[0]);

    /* Calculate size of source list.  */
    /* Start with list of source files.  */
    sourceListSize = info->CAPI->GetTotalArgumentSize(newArgc,newArgv);
    /* Add enough to extend the name of each class. */
    sourceListSize += numSources*strlen("Tcl.cxx");
    /* Add enough to include the def and init file.  */
    sourceListSize += def?strlen(def):0;
    sourceListSize += strlen(";Init.cxx");

    /* Allocate and initialize the source list.  */
    sourceListValue = (char *)malloc(sourceListSize);
    if (def)
      {
      sprintf(sourceListValue,"%s;%sInit.cxx",def,argv[0]);
      }
    else
      {
      sprintf(sourceListValue,"%sInit.cxx",argv[0]);
      }
    
    for(i = 1; i < numSources; ++i)
      {   
      void *curr = info->CAPI->GetSource(mf,sources[i]);
      
      /* if we should wrap the class */
      if (!curr || 
          !info->CAPI->SourceFileGetPropertyAsBool(curr,"WRAP_EXCLUDE"))
        {
        void *file = info->CAPI->CreateSourceFile();
        char *srcName;
        char *hname;
        char *pathName;
        srcName = info->CAPI->GetFilenameWithoutExtension(sources[i]);
        pathName = info->CAPI->GetFilenamePath(sources[i]);
        if (curr)
          {
          int abst = info->CAPI->SourceFileGetPropertyAsBool(curr,"ABSTRACT");
          info->CAPI->SourceFileSetProperty(file,"ABSTRACT",
                                            (abst ? "1" : "0"));
          if (!abst)
            {
            concrete[numConcrete] = strdup(srcName);
            numConcrete++;
            }
         }
        else
          {
          concrete[numConcrete] = strdup(srcName);
          numConcrete++;
          }
        newName = (char *)malloc(strlen(srcName)+4);
        sprintf(newName,"%sTcl",srcName);
        info->CAPI->SourceFileSetName2(file, newName, 
                             info->CAPI->GetCurrentOutputDirectory(mf),
                             "cxx",0);
        
        if (strlen(pathName) > 1)
          {
          hname = (char *)malloc(strlen(pathName) + strlen(srcName) + 4);
          sprintf(hname,"%s/%s.h",pathName,srcName);
          }
        else
          {
          hname = (char *)malloc(strlen(cdir) + strlen(srcName) + 4);
          sprintf(hname,"%s/%s.h",cdir,srcName);
          }
        /* add starting depends */
        info->CAPI->SourceFileAddDepend(file,hname);
        info->CAPI->AddSource(mf,file);
        cdata->SourceFiles[numWrapped] = file;
        cdata->HeaderFiles[numWrapped] = hname;
        numWrapped++;
        strcat(sourceListValue,";");
        strcat(sourceListValue,newName);
        strcat(sourceListValue,".cxx");        
        free(newName);
        info->CAPI->Free(srcName);
        info->CAPI->Free(pathName);
        }
      }
    /* add the init file */
    cfile = info->CAPI->CreateSourceFile();
    info->CAPI->SourceFileSetProperty(cfile,"ABSTRACT","0");
    newName = (char *)malloc(strlen(argv[0]) + 5);
    sprintf(newName,"%sInit",argv[0]);
    CreateInitFile(info,mf,argv[0],numConcrete,concrete,numCommands,commands);
    info->CAPI->SourceFileSetName2(cfile, newName, 
                                   info->CAPI->GetCurrentOutputDirectory(mf),
                                   "cxx",0);
    free(newName);
    info->CAPI->AddSource(mf,cfile);
    info->CAPI->DestroySourceFile(cfile);
    info->CAPI->AddDefinition(mf, sources[0], sourceListValue);
    free(sourceListValue);
    }

  /* store key data in the CLientData for the final pass */
  cdata->NumberWrapped = numWrapped;
  cdata->LibraryName = strdup(argv[0]);
  info->CAPI->SetClientData(info,cdata);
  
  free(sources);
  free(commands);
  info->CAPI->FreeArguments(newArgc, newArgv);
  for (i = 0; i < numConcrete; ++i)
    {
    free(concrete[i]);
    }
  free(concrete);
  return 1;
}


static void FinalPass(void *inf, void *mf) 
{
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  /* get our client data from initial pass */
  cmVTKWrapTclData *cdata = 
    (cmVTKWrapTclData *)info->CAPI->GetClientData(info);

  /* first we add the rules for all the .h to Tcl.cxx files */
  const char *wtcl = "${VTK_WRAP_TCL_EXE}";
  const char *hints = info->CAPI->GetDefinition(mf,"VTK_WRAP_HINTS");
  const char *args[4];
  const char *depends[2];
  int i;
  int numDepends, numArgs;
  const char *cdir = info->CAPI->GetCurrentDirectory(mf);
  
  /* If the first pass terminated early, we have nothing to do.  */
  if(!cdata)
    {
    return;
    }
  
  /* wrap all the .h files */
  depends[0] = wtcl;
  numDepends = 1;
  if (hints)
    {
    depends[1] = hints;
    numDepends++;
    }
  for(i = 0; i < cdata->NumberWrapped; i++)
    {
    char *res;
    const char *srcName = info->CAPI->SourceFileGetSourceName(cdata->SourceFiles[i]);
    args[0] = cdata->HeaderFiles[i];
    numArgs = 1;
    if (hints)
      {
      args[1] = hints;
      numArgs++;
      }
    args[numArgs] = 
      (info->CAPI->SourceFileGetPropertyAsBool(cdata->SourceFiles[i],"ABSTRACT") ?"0" :"1");
    numArgs++;
    res = (char *)malloc(strlen(info->CAPI->GetCurrentOutputDirectory(mf)) + 
                         strlen(srcName) + 6);
    sprintf(res,"%s/%s.cxx",info->CAPI->GetCurrentOutputDirectory(mf),srcName);
    args[numArgs] = res;
    numArgs++;
    info->CAPI->AddCustomCommand(mf, args[0],
                       wtcl, numArgs, args, numDepends, depends, 
                       1, &res, cdata->LibraryName);
    free(res);
    }
}

static void Destructor(void *inf) 
{
  int i;
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  /* get our client data from initial pass */
  cmVTKWrapTclData *cdata = 
    (cmVTKWrapTclData *)info->CAPI->GetClientData(info);
  if (cdata)
    {
    for (i = 0; i < cdata->NumberWrapped; ++i)
      {              
      info->CAPI->DestroySourceFile(cdata->SourceFiles[i]);
      free(cdata->HeaderFiles[i]);
      }
    free(cdata->SourceFiles);
    free(cdata->HeaderFiles);
    free(cdata->LibraryName);
    free(cdata);
    }
}

static const char* GetTerseDocumentation() 
{
  return "Create Tcl Wrappers for VTK classes.";
}

static const char* GetFullDocumentation()
{
  return
    "VTK_WRAP_TCL(resultingLibraryName [SOURCES] SourceListName SourceLists ... [COMMANDS CommandName1 CommandName2 ...])";
}

void CM_PLUGIN_EXPORT VTK_WRAP_TCL2Init(cmLoadedCommandInfo *info)
{
  info->InitialPass = InitialPass;
  info->FinalPass = FinalPass;
  info->Destructor = Destructor;
  info->m_Inherited = 0;
  info->GetTerseDocumentation = GetTerseDocumentation;
  info->GetFullDocumentation = GetFullDocumentation;  
  info->Name = "VTK_WRAP_TCL2";
}
