/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapJavaScript.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"
#include "vtkParseMain.h"
#include "vtkParseSystem.h"
#include "vtkWrap.h"
#include "vtkWrapJavaScriptClass.h"
#include "vtkWrapJavaScriptConstant.h"
#include "vtkWrapJavaScriptEnum.h"
#include "vtkWrapJavaScriptNamespace.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)
// NOLINTBEGIN(bugprone-multi-level-implicit-pointer-conversion)

/* -------------------------------------------------------------------- */
/* This is the main entry point for the javascript wrappers.  When called,
 * it will print the vtkXXEmbinding.cxx file contents to "fp".  */

// NOLINTNEXTLINE(modernize-macro-to-enum)
#define MAX_WRAPPED_CLASSES 256

#ifdef NDEBUG
#define DLOG(...)
#else
#define DLOG(...) printf(__VA_ARGS__);
#endif

/* -------------------------------------------------------------------- */
/* Get the module for the specified class */
static const char* vtkWrapJavaScript_ClassModule(HierarchyInfo* hinfo, const char* classname)
{
  HierarchyEntry* entry;

  /* if "hinfo" is present, use it to find the file */
  if (hinfo)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      return entry->Module;
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Get the header file for the specified class */
static const char* vtkWrapJavaScript_ClassHeader(HierarchyInfo* hinfo, const char* classname)
{
  HierarchyEntry* entry;

  /* if "hinfo" is present, use it to find the file */
  if (hinfo)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      return entry->HeaderFile;
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* generate includes for any special types that are used */
static void vtkWrapJavaScript_GenerateSpecialHeaders(
  FILE* fp, FileInfo* file_info, HierarchyInfo* hinfo)
{
  const char** types;
  int numTypes = 0;
  FunctionInfo* currentFunction;
  int i, j, k, n, m, ii, nn;
  const char* classname;
  const char* ownincfile = "";
  ClassInfo* data;
  ValueInfo* val;
  const char** includedHeaders = NULL;
  size_t nIncludedHeaders = 0;

  types = (const char**)malloc(1000 * sizeof(const char*));

  /* always include vtkVariant, it is often used as a template arg
     for templated array types, and the file_info doesn't tell us
     what types each templated class is instantiated for (that info
     might be in the .cxx files, which we cannot access here) */
  types[numTypes++] = "vtkVariant";

  nn = file_info->Contents->NumberOfClasses;
  for (ii = 0; ii < nn; ii++)
  {
    data = file_info->Contents->Classes[ii];
    n = data->NumberOfFunctions;
    for (i = 0; i < n; i++)
    {
      currentFunction = data->Functions[i];
      if (currentFunction->Access == VTK_ACCESS_PUBLIC && !currentFunction->IsExcluded &&
        strcmp(currentFunction->Class, data->Name) == 0)
      {
        m = vtkWrap_CountWrappedParameters(currentFunction);

        for (j = -1; j < m; j++)
        {
          if (j >= 0)
          {
            val = currentFunction->Parameters[j];
          }
          else
          {
            val = currentFunction->ReturnValue;
          }
          if (vtkWrap_IsVoid(val))
          {
            continue;
          }

          classname = 0;
          /* the IsScalar check is used because the wrappers don't need the
             header for objects passed via a pointer (but they need the header
             for objects passed by reference) */
          if (vtkWrap_IsString(val))
          {
            classname = val->Class;
          }
          else if (vtkWrap_IsObject(val) && !vtkWrap_IsRef(val))
          {
            classname = val->Class;
          }
          else if (vtkWrap_IsRef(val))
          {
            classname = val->Class;
          }
          /* we already include our own header */
          if (classname && strcmp(classname, data->Name) != 0)
          {
            for (k = 0; k < numTypes; k++)
            {
              /* make a unique list of all classes found */
              if (strcmp(classname, types[k]) == 0)
              {
                break;
              }
            }

            if (k == numTypes)
            {
              if (numTypes > 0 && (numTypes % 1000) == 0)
              {
                types =
                  (const char**)realloc((char**)types, (numTypes + 1000) * sizeof(const char*));
              }
              types[numTypes++] = classname;
            }
          }
        }
      }
    }
  }

  /* get our own include file (returns NULL if hinfo is NULL) */
  data = file_info->MainClass;
  if (!data && file_info->Contents->NumberOfClasses > 0)
  {
    data = file_info->Contents->Classes[0];
  }

  if (data)
  {
    ownincfile = vtkWrapJavaScript_ClassHeader(hinfo, data->Name);
  }

  includedHeaders = (const char**)malloc(numTypes * sizeof(const char*));

  /* for each unique type found in the file */
  for (i = 0; i < numTypes; i++)
  {
    const char* incfile;
    incfile = vtkWrapJavaScript_ClassHeader(hinfo, types[i]);

    if (incfile)
    {
      /* make sure it hasn't been included before. */
      size_t nHeader;
      int uniqueInclude = 1;
      for (nHeader = 0; nHeader < nIncludedHeaders; ++nHeader)
      {
        if (!strcmp(incfile, includedHeaders[nHeader]))
        {
          uniqueInclude = 0;
        }
      }

      /* ignore duplicate includes. */
      if (!uniqueInclude)
      {
        continue;
      }

      includedHeaders[nIncludedHeaders] = incfile;
      ++nIncludedHeaders;

      /* make sure it doesn't share our header file */
      if (ownincfile == 0 || strcmp(incfile, ownincfile) != 0)
      {
        fprintf(fp, "#include \"%s\"\n", incfile);
      }
    }
  }

  free((char**)includedHeaders);
  includedHeaders = NULL;

  /* special case for the way vtkGenericDataArray template is used */
  if (data && strcmp(data->Name, "vtkGenericDataArray") == 0)
  {
    fprintf(fp,
      "#include \"vtkSOADataArrayTemplate.h\"\n"
      "#include \"vtkAOSDataArrayTemplate.h\"\n"
      "#ifdef VTK_USE_SCALED_SOA_ARRAYS\n"
      "#include \"vtkScaledSOADataArrayTemplate.h\"\n"
      "#endif\n");
  }

  free((char**)types);
}

//------------------------------------------------------------------------------
/**
 * Add arguments used to generate the file to ease debugging
 */
void vtkWrapJavaScript_DecorateHeader(FILE* f, int argc, char** argv)
{
  fprintf(f, "// This file was auto-generated using :\n");
  fprintf(f, "/*\n");
  // if we've EMSDK_NODE, print it, otherwise use node.
  const char* emsdk_node = getenv("EMSDK_NODE");
  const char* cmake_cross_compiling_emulator = emsdk_node ? emsdk_node : "node";
  fprintf(f, "%s ", cmake_cross_compiling_emulator);
  for (int i = 0; i < argc - 1; i++)
  {
    fprintf(f, "%s ", argv[i]);
#ifdef _WIN32
    fprintf(f, "`\n ");
#else
    fprintf(f, "\\\n ");
#endif
  }
  fprintf(f, "%s\n", argv[argc - 1]);
  fprintf(f, "*/\n");
}

//------------------------------------------------------------------------------
/* print the parsed structures */
int VTK_PARSE_MAIN(int argc, char* argv[])
{
  unsigned char wrapAsVTKObject[MAX_WRAPPED_CLASSES];
  ClassInfo* data = NULL;
  NamespaceInfo* contents;
  const OptionInfo* options;
  HierarchyInfo* hinfo = NULL;
  FileInfo* file_info;
  FILE* fp;
  const char* module = "vtkCommonCore";
  const char* name;
  char* name_from_file = NULL;
  int numberOfWrappedClasses = 0;
  int numberOfWrappedNamespaces = 0;
  int wrapped_anything = 0;
  int i, j;
  size_t k, m;
  int is_vtkobject;

  /* pre-define a macro to identify the language */
  vtkParse_DefineMacro("__EMSCRIPTEN__", 0);

  /* get command-line args and parse the header file */
  file_info = vtkParse_Main(argc, argv);

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hinfo =
      vtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the output file */
  fp = vtkParse_FileOpen(options->OutputFileName, "w");
  if (!fp)
  {
    int e = errno;
    char* etext = strerror(e);
    etext = (etext ? etext : "Unknown error");
    fprintf(stderr, "Error %d opening output file %s: %s\n", e, options->OutputFileName, etext);
    return vtkParse_FinalizeMain(1);
  }

  /* get the filename without the extension */
  name = file_info->FileName;
  m = strlen(name);
  for (k = m; k > 0; k--)
  {
    if (name[k] == '.')
    {
      break;
    }
  }
  if (k > 0)
  {
    m = k;
  }
  for (k = m; k > 0; k--)
  {
    if (!((name[k - 1] >= 'a' && name[k - 1] <= 'z') ||
          (name[k - 1] >= 'A' && name[k - 1] <= 'Z') ||
          (name[k - 1] >= '0' && name[k - 1] <= '9') || name[k - 1] == '_'))
    {
      break;
    }
  }
  name_from_file = (char*)malloc(m - k + 1);
  strncpy(name_from_file, &name[k], m - k);
  name_from_file[m - k] = '\0';
  name = name_from_file;

  /* get the global namespace */
  contents = file_info->Contents;

  /* use the hierarchy file to find super classes and expand typedefs */
  if (hinfo)
  {
    for (i = 0; i < contents->NumberOfClasses; i++)
    {
      vtkWrap_MergeSuperClasses(contents->Classes[i], file_info, hinfo);
    }
    for (i = 0; i < contents->NumberOfClasses; i++)
    {
      vtkWrap_ExpandTypedefs(contents->Classes[i], file_info, hinfo);
    }
  }

  /* Decorate the header with a command line showing how it was generated */
  fprintf(fp, "// JavaScript wrapper for %s with embind \n//\n", name);
  vtkWrapJavaScript_DecorateHeader(fp, argc, argv);

  /* Include the smart_ptr_trait specialization for vtkSmartPointer */
  fprintf(fp,
    "#include \"vtkEmbindSmartPointerTrait.h\"\n"
    "#include <emscripten.h>\n"
    "#include <string>\n");

  /* generate includes for any special types that are used */
  vtkWrapJavaScript_GenerateSpecialHeaders(fp, file_info, hinfo);

  /* the header file for the wrapped class */
  fprintf(fp, "#include \"%s.h\"\n\n", name);

  /* get the module that is being wrapped */
  data = file_info->MainClass;
  if (!data && file_info->Contents->NumberOfClasses > 0)
  {
    data = file_info->Contents->Classes[0];
  }
  if (data && hinfo)
  {
    module = vtkWrapJavaScript_ClassModule(hinfo, data->Name);
  }

  /* Identify all enum types that are used by methods */
  vtkWrapJavaScript_MarkAllEnums(file_info->Contents, hinfo);

  /* Wrap any enum types defined in the global namespace */
  vtkWrapJavaScript_GenerateEnumTypes(fp, module, NULL, "  ", file_info->Contents);

  /* Wrap any constants defined in the global namespace */
  vtkWrapJavaScript_GenerateConstants(fp, module, name, "  ", file_info->Contents);

  /* Wrap any namespaces */
  for (i = 0; i < contents->NumberOfNamespaces; i++)
  {
    if (contents->Namespaces[i]->NumberOfConstants > 0)
    {
      vtkWrapJavaScript_Namespace(fp, module, contents->Namespaces[i]);
      numberOfWrappedNamespaces++;
    }
  }

  /* Check for all special classes before any classes are wrapped */
  for (i = 0; i < contents->NumberOfClasses; i++)
  {
    data = contents->Classes[i];
    if (data->IsDeprecated)
    {
      // skip deprecated classes.
      continue;
    }

    /* guess whether type is a vtkobject */
    is_vtkobject = (data == file_info->MainClass ? 1 : 0);
    if (hinfo)
    {
      is_vtkobject = vtkWrap_IsTypeOf(hinfo, data->Name, "vtkObjectBase");
    }

    if (!is_vtkobject)
    {
      /* mark class as abstract only if it has pure virtual methods */
      /* (does not check for inherited pure virtual methods) */
      data->IsAbstract = 0;
      for (j = 0; j < data->NumberOfFunctions; j++)
      {
        FunctionInfo* func = data->Functions[j];
        if (func && func->IsPureVirtual)
        {
          data->IsAbstract = 1;
          break;
        }
      }
    }

    wrapAsVTKObject[i] = (is_vtkobject ? 1 : 0);
  }

  /* Wrap all of the classes in the file */
  for (i = 0; i < contents->NumberOfClasses; i++)
  {
    data = contents->Classes[i];
    if (data->IsDeprecated)
    {
      // skip deprecated classes.
      continue;
    }
    if (data->IsExcluded)
    {
      continue;
    }

    is_vtkobject = wrapAsVTKObject[i];

    /* if "hinfo" is present, wrap everything, else just the main class */
    if (hinfo || data == file_info->MainClass)
    {
      if (vtkWrapJavaScript_WrapOneClass(
            fp, module, data->Name, data, file_info, hinfo, is_vtkobject))
      {
        /* re-index wrapAsVTKObject for wrapped classes */
        wrapAsVTKObject[numberOfWrappedClasses] = (is_vtkobject ? 1 : 0);
      }
    }
  }

  /* close the file */
  fclose(fp);

  /* free data structures */
  free(name_from_file);
  if (hinfo)
  {
    vtkParseHierarchy_Free(hinfo);
  }
  vtkParse_Free(file_info);
  wrapped_anything = numberOfWrappedClasses + numberOfWrappedNamespaces;
  if (wrapped_anything > 0)
  {
    DLOG("Wrapped %d classes, %d namespaces", numberOfWrappedClasses, numberOfWrappedNamespaces);
  }
  return vtkParse_FinalizeMain(0);
}

// NOLINTEND(bugprone-multi-level-implicit-pointer-conversion)
// NOLINTEND(bugprone-unsafe-functions)
