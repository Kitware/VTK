/* this is a CMake loadable command to wrap vtk objects into Java */

#include "cmCPluginAPI.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* do almost everything in the initial pass */
static int InitialPass(void *inf, void *mf, int argc, char *argv[])
{
  cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
  int i;
  int newArgc;
  int msize;
  int estimated;
  char **newArgv;
  int numClasses = 0;
  char **classes = 0;
  char **dependencies = 0;
  int numDep = 0;
  int numWrapped = 0;
  const char *cdir = info->CAPI->GetCurrentDirectory(mf);
  const char *def = 0;
  char *newName;
  char *jarFile = 0;
  char *target = 0;
  const char *javac = info->CAPI->GetDefinition(mf,"JAVA_COMPILE");
  const char *jar = info->CAPI->GetDefinition(mf,"JAVA_ARCHIVE");
  void *cfile = 0;
  char *args[3];
  char *jargs[5];
  int depends = 0;
  const char *libpath = info->CAPI->GetDefinition(mf,"LIBRARY_OUTPUT_PATH");
  const char *vtkpath = info->CAPI->GetDefinition(mf,"VTK_BINARY_DIR");

  if(argc < 3 )
    {
    info->CAPI->SetError(info, "called with incorrect number of arguments");
    return 0;
    }

  /* Now check and see if the value has been stored in the cache */
  /* already, if so use that value and don't look for the program */
  if(!info->CAPI->IsOn(mf,"VTK_WRAP_JAVA"))
    {
    info->CAPI->FreeArguments(newArgc, newArgv);
    return 1;
    }

  args[0] = strdup("-classpath");
  args[1] = (char *)malloc(strlen(vtkpath) + 20);
  sprintf(args[1], "%s/java", vtkpath);
  
  info->CAPI->ExpandSourceListArguments(mf, argc, (const char**)argv, 
                                        &newArgc, &newArgv, 2);
  
  /* keep the library name */
  target = strdup(newArgv[0]);
  jarFile = (char *)malloc(strlen(libpath) + strlen(newArgv[1]) + 4);
  sprintf(jarFile, "%s/%s", libpath, newArgv[1]);

  classes = (char **)malloc((newArgc -2) * sizeof(char*));
  dependencies = (char **)malloc((newArgc -2) * sizeof(char*));

  if ( jar )
    {
    dependencies[numDep++] = strdup(jarFile);
    }

  /* get the classes for this lib */
  for(i = 2; i < newArgc; ++i)
    {   
    const char *srcName = newArgv[i];
    char *className = 0;
    char *srcNameWe;
    char *srcPath;

    if ( strcmp(srcName, "DEPENDS") == 0 )
      {
      depends = 1;
      continue;
      }
    if ( depends )
      {
      dependencies[numDep++] = strdup(srcName);
      }
    else
      {
      srcNameWe = info->CAPI->GetFilenameWithoutExtension(srcName);
      srcPath   = info->CAPI->GetFilenamePath(srcName);
      
      className = (char *)malloc(strlen(srcPath) + strlen(srcNameWe) + 20);
      sprintf(className,"%s/%s.class",srcPath,srcNameWe);
      
      args[2] = strdup(srcName);
      info->CAPI->AddCustomCommand(mf, srcName, javac, 3, (const char**)args, 
                                   0, 0, 1, (const char**)&className, target);
      
      free(args[2]);
      info->CAPI->Free(srcNameWe);
      info->CAPI->Free(srcPath);
      classes[numClasses++] = strdup(className);
      free(className);
      }
    }

  jargs[0] = strdup("cvf");
  jargs[1] = jarFile;
  jargs[2] = strdup("-C");
  jargs[3] = args[1];
  jargs[4] = strdup("vtk");

  if ( jar && numClasses > 0 )
    {
    /*
      Ok, so source is bogus, so let's just pick the first
      class. Command is jar cvf ${LIBRARY_OUTPUT_PATH}/vtk.jar -C
      ${VTK_BINARY_DIR}/java vtk. It depends on all the classes. The
      output is jar file.
    */
    info->CAPI->AddCustomCommand(mf, classes[0], jar,
                                 5, (const char**)jargs, 
                                 numClasses, (const char**)classes,
                                 1, (const char**)&jarFile,
                                 target);
    }
  /* 
     Ok, now we need one to drive the whole mess. Source and target is
     same, no command and no arguments, and depends on jar file.
  */
  info->CAPI->AddCustomCommand(mf, target, "", 
                               0, 0,
                               numDep, (const char**)dependencies,
                               0, 0,
                               target);
  if ( numClasses > 0 )
    {
    for ( i = 0; i < numClasses; i ++ )
      {
      /* We should delete stuff */
      free(classes[i]);
      }
    free(classes);
    }

  if ( numDep > 0 )
    {
    for ( i = 0; i < numDep; i ++ )
      {
      /* We should delete stuff */
      free(dependencies[i]);
      }
    free(dependencies);
    }

  free(args[0]);
  free(args[1]);
  
  free(jargs[0]);
  free(jargs[2]);
  free(jargs[4]);

  free(target);

  info->CAPI->FreeArguments(newArgc, newArgv);
  return 1;
}
  
  
static void FinalPass(void *inf, void *mf) 
{
}

static void Destructor(void *inf) 
{
}

static const char* GetTerseDocumentation() 
{
  return "Create Java Archive.";
}

static const char* GetFullDocumentation()
{
  return
    "VTK_WRAP_JAVA(target resultingJarFile SourceLists ... [DEPENDS dependent files...])";
}

void CM_PLUGIN_EXPORT VTK_GENERATE_JAVA_DEPENDENCIESInit(cmLoadedCommandInfo *info)
{
  info->InitialPass = InitialPass;
  info->FinalPass = FinalPass;
  info->Destructor = Destructor;
  info->GetTerseDocumentation = GetTerseDocumentation;
  info->GetFullDocumentation = GetFullDocumentation;  
  info->m_Inherited = 0;
  info->Name = "VTK_GENERATE_JAVA_DEPENDENCIES";
}




