/* this is a CMake loadable command to wrap vtk objects into Java */

#include "cmCPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void GenerateHeaderFile(cmLoadedCommandInfo *info, 
                               const char *fullPath, const char *hdrName,
                               const char *macroName,
                               int numIncludes, const char **includes)
{
  char *tempOutputFile;  
  FILE *fout;
  int i;

  tempOutputFile = (char *)malloc(strlen(fullPath) + 5);
  sprintf(tempOutputFile,"%s.tmp",fullPath);
  fout = fopen(tempOutputFile,"w");
  if (!fout)
    {
    return;
    }

  fprintf(fout,
    "#ifndef __%s_h\n"
    "#define __%s_h\n"
    "\n"
    "#include \"vtkInstantiator.h\"\n", hdrName, hdrName);
  
  for(i=0; i < numIncludes;++i)
    {
    fprintf(fout,"#include \"%s\"\n", includes[i]);
    }
  
  /* Write the instantiator class definition. */
  fprintf(fout,
          "\n"
          "class %s %s\n"
          "{\n"
          "public:\n"
          "  %s();\n"
          "  ~%s();\n"
          "private:\n"
          "  static void ClassInitialize();\n"
          "  static void ClassFinalize();\n"
          "  static unsigned int Count;\n"
          "};\n"
          "\n", macroName, hdrName, hdrName, hdrName);
  
  /* Write the initialization instance to make sure the creation */
  /* functions get registered when this generated header is included. */
  fprintf(fout,
          "static %s %sInitializer;\n"
          "\n"
          "#endif\n",hdrName, hdrName);  

  fclose(fout);

  /* copy the file if different */
  info->CAPI->CopyFileIfDifferent(tempOutputFile, fullPath);
  info->CAPI->RemoveFile(tempOutputFile);
  free(tempOutputFile);
}

static void GenerateImplementationFile(cmLoadedCommandInfo *info, 
                                       const char *fullPath, 
                                       const char *hdrName,
                                       int numClasses, const char **classes)
{
  char *tempOutputFile;  
  FILE *fout;
  int i;
  
  tempOutputFile = (char *)malloc(strlen(fullPath) + 5);
  sprintf(tempOutputFile,"%s.tmp",fullPath);
  fout = fopen(tempOutputFile,"w");
  if (!fout)
    {
    return;
    }

  /* Include the instantiator class header. */
  fprintf(fout,
          "#include \"%s.h\"\n"
          "\n", hdrName);
  
  /* Write the extern declarations for all the creation functions. */
  for(i=0; i < numClasses; ++i)
    {
    if (classes[i])
      {
      fprintf(fout,
              "extern vtkObject* vtkInstantiator%sNew();\n", 
              classes[i]);
      }
    }
  
  /* Write the ClassInitialize method to register all the creation functions.*/
  fprintf(fout,
          "\n"
          "void %s::ClassInitialize()\n"
          "{\n", hdrName);
  
  for(i=0; i < numClasses; ++i)
    {
    if (classes[i])
      {
      fprintf(fout,
              "  vtkInstantiator::RegisterInstantiator(\"%s\", vtkInstantiator%sNew);\n", classes[i], classes[i]);
      }
    }
  
  /* Write the ClassFinalize method to unregister all the creation functions.*/
  fprintf(fout,
          "}\n"
          "\n"
          "void %s::ClassFinalize()\n"
          "{\n", hdrName);
  
  for(i=0; i < numClasses; ++i)
    {
    if (classes[i])
      {
      fprintf(fout,
              "  vtkInstantiator::UnRegisterInstantiator(\"%s\", vtkInstantiator%sNew);\n",classes[i],classes[i]);
      }
    }
  
  /* Write the constructor and destructor of the initializer class to */
  /* call the ClassInitialize and ClassFinalize methods at the right */
  /* time. */
  fprintf(fout,
          "}\n"
          "\n" 
          "%s::%s()\n"
          "{\n"
          "  if(++%s::Count == 1)\n"
          "    { %s::ClassInitialize(); }\n"
          "}\n", hdrName, hdrName, hdrName, hdrName);
  fprintf(fout,
          "\n" 
          "%s::~%s()\n"
          "{\n"
          "  if(--%s::Count == 0)\n"
          "    { %s::ClassFinalize(); }\n"
          "}\n"
          "\n"
          "// Number of translation units that include this class's header.\n"
          "// Purposely not initialized.  Default is static initialization to 0.\n"
          "unsigned int %s::Count;\n",
          hdrName, hdrName, hdrName, hdrName, hdrName);
  fclose(fout);

  /* copy the file if different */
  info->CAPI->CopyFileIfDifferent(tempOutputFile, fullPath);
  info->CAPI->RemoveFile(tempOutputFile);
  free(tempOutputFile);
}

/* do almost everything in the initial pass */
static int InitialPass(void *inf, void *mf, int argc, char *argv[])
{
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  int i;
  int newArgc;
  char **newArgv;
  const char *filePath = info->CAPI->GetCurrentOutputDirectory(mf);
  const char *headerPath = filePath;
  int includesMode = 0;
  const char *ExportMacro = 0;
  int numClasses = 0;
  char **classes = 0;
  int numIncludes = 0;
  char **includes = 0;
  char *fullName;
  void *cfile;

  /* make sure we have the correct number of arguments */
  if(argc < 3 )
    {
    return 0;
    }
  
  /* expand any source lists */
  info->CAPI->ExpandSourceListArguments(mf, argc, argv, 
                                        &newArgc, &newArgv, 2);
  
  /* make sure we allocate enough memory */
  classes = (char **)malloc(sizeof(char *)*newArgc);
  includes = (char **)malloc(sizeof(char *)*newArgc);

  /* Find the path of the files to be generated. */
  for(i=2; i < newArgc; ++i)
    {
    if(strcmp(newArgv[i],"HEADER_LOCATION") == 0)
      {
      includesMode = 0;
      if(++i < newArgc)
        {
        headerPath = newArgv[i];
        }
      else
        {
        info->CAPI->SetError(info,"HEADER_LOCATION option used without value.");
        return 0;
        }
      }
    else if (strcmp(newArgv[i],"EXPORT_MACRO") == 0)
      {
      includesMode = 0;
      if (++i < newArgc)
        {
        ExportMacro = newArgv[i];
        }
      else
        {
        info->CAPI->SetError(info, "EXPORT_MACRO option used without value.");
        return 0;
        }
      }
    else if (strcmp(newArgv[i],"INCLUDES") == 0)
      {
      includesMode = 1;
      }
    /* If not an option, it must be another input source list name or */
    /* an include file. */
    else
      {
      if(!includesMode)
        {
        classes[numClasses] = newArgv[i];
        numClasses++;
        }
      else
        {
        includes[numIncludes] = newArgv[i];
        numIncludes++;
        }
      }
    }
  
  if(!ExportMacro)
    {
    info->CAPI->SetError(info, "No EXPORT_MACRO option given.");
    return 0;
    }
  
  for (i = 0; i < numClasses; ++i)
    {
    const char *srcName = info->CAPI->GetFilenameWithoutExtension(classes[i]);
    void *sf = info->CAPI->GetSource(mf,classes[i]);

    /* Wrap-excluded and abstract classes do not have a New() method. */
    /* vtkIndent and vtkTimeStamp are special cases and are not */
    /* vtkObject subclasses. */
    if(
      (sf && 
       (info->CAPI->SourceFileGetPropertyAsBool(sf,"WRAP_EXCLUDE") ||
        info->CAPI->SourceFileGetPropertyAsBool(sf,"ABSTRACT"))) ||
      !strcmp(srcName,"vtkIndent") || 
      !strcmp(srcName,"vtkTimeStamp"))
      {
      /* remove this class from the list */
      classes[i] = 0;
      }
    else
      {
      classes[i] = info->CAPI->GetFilenameWithoutExtension(classes[i]);
      }
    info->CAPI->Free((char*)srcName);
    }    
  
  /* Generate the header */
  fullName = malloc(strlen(newArgv[0]) + strlen(headerPath) + 10);
  sprintf(fullName,"%s/%s.h",headerPath,newArgv[0]);
  GenerateHeaderFile(info, fullName, newArgv[0], ExportMacro, 
                     numIncludes, includes);
  free(fullName);
  
  /* Generate the implementation  */
  fullName = malloc(strlen(newArgv[0]) + strlen(filePath) + 10);
  sprintf(fullName,"%s.cxx",newArgv[0]);
  info->CAPI->AddDefinition(mf, newArgv[1], fullName);  
  sprintf(fullName,"%s/%s.cxx",filePath,newArgv[0]);
  GenerateImplementationFile(info, fullName, newArgv[0], numClasses, classes);
  free(fullName);
  
  /* free the classes */
  for (i = 0; i < numClasses; ++i)
    {
    if (classes[i])
      {
      info->CAPI->Free(classes[i]);
      }
    }
  
  /* Add the generated source file into the source list. */
  cfile = info->CAPI->CreateSourceFile();
  info->CAPI->SourceFileSetProperty(cfile,"WRAP_EXCLUDE","1");
  info->CAPI->SourceFileSetProperty(cfile,"ABSTRACT","0");
  info->CAPI->SourceFileSetName2(cfile, newArgv[0], 
                                 info->CAPI->GetCurrentOutputDirectory(mf),
                                 "cxx",0);
  info->CAPI->AddSource(mf,cfile);
  info->CAPI->DestroySourceFile(cfile);
  info->CAPI->FreeArguments(newArgc, newArgv);
  free(classes);
  free(includes);
  return 1;
}

static const char* GetTerseDocumentation() 
{
  return "Register classes for creation by vtkInstantiator";
}

static const char* GetFullDocumentation()
{
  return
    "VTK_MAKE_INSTANTIATOR(className outSourceList\n"
    "                      src-list1 [src-list2 ..]\n"
    "                      EXPORT_MACRO exportMacro\n"
    "                      [HEADER_LOCATION dir]\n"
    "                      [INCLUDES [file1 file2 ..]])\n"
    "Generates a new class with the given name and adds its files to the\n"
    "given outSourceList.  It registers the classes from the other given\n"
    "source lists with vtkInstantiator when it is loaded.  The output\n"
    "source list should be added to the library with the classes it\n"
    "registers.\n"
    "The EXPORT_MACRO argument must be given and followed by the export\n"
    "macro to use when generating the class (ex. VTK_COMMON_EXPORT).\n"
    "The HEADER_LOCATION option must be followed by a path.  It specifies\n"
    "the directory in which to place the generated class's header file.\n"
    "The generated class implementation files always go in the build\n"
    "directory corresponding to the CMakeLists.txt file containing\n"
    "the command.  This is the default location for the header.\n"
    "The INCLUDES option can be followed by a list of zero or more files.\n"
    "These files will be #included by the generated instantiator header,\n"
    "and can be used to gain access to the specified exportMacro in the\n"
    " C++ code.";
}

void CM_PLUGIN_EXPORT VTK_MAKE_INSTANTIATOR2Init(cmLoadedCommandInfo *info)
{
  info->InitialPass = InitialPass;
  info->m_Inherited = 0;
  info->GetTerseDocumentation = GetTerseDocumentation;
  info->GetFullDocumentation = GetFullDocumentation;  
  info->Name = "VTK_MAKE_INSTANTIATOR2";
}




