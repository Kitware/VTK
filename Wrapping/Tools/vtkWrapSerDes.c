// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParse.h"
#include "vtkParseData.h"
#include "vtkParseHierarchy.h"
#include "vtkParseMain.h"
#include "vtkParseString.h"
#include "vtkParseSystem.h"
#include "vtkWrap.h"
#include "vtkWrapSerDesClass.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* for Sleep() */
#include <windows.h>
#endif

/* -------------------------------------------------------------------- */
/* Get the header file for the specified class */
static const char* vtkWrapSerDes_ClassHeader(const HierarchyInfo* hinfo, const char* classname)
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
static void vtkWrapSerDes_GenerateSpecialHeaders(
  FILE* fp, FileInfo* file_info, const HierarchyInfo* hinfo)
{
  const char** types;
  int numTypes = 0;
  FunctionInfo* currentFunction;
  int i, j, k, n, m, ii, nn;
  const char* classname;
  const char* ownincfile = "";
  ClassInfo* data;
  const ValueInfo* val;
  const char** includedHeaders = NULL;
  int hasDeprecatedEntries = 0;
  size_t nIncludedHeaders = 0;

  types = (const char**)malloc(1000 * sizeof(const char*));

  /* always include vtkVariant, it is often used as a template arg
     for templated array types, and the file_info doesn't tell us
     what types each templated class is instantiated for (that info
     might be in the .cxx files, which we cannot access here) */
  types[numTypes++] = "vtkVariant";
  /* the header file for the marshalled class */
  types[numTypes++] = "vtkDeserializer";
  types[numTypes++] = "vtkInvoker";
  types[numTypes++] = "vtkSerializer";

  nn = file_info->Contents->NumberOfClasses;
  for (ii = 0; ii < nn; ii++)
  {
    data = file_info->Contents->Classes[ii];
    n = data->NumberOfFunctions;
    hasDeprecatedEntries |= (data->IsDeprecated);
    for (i = 0; i < n; i++)
    {
      currentFunction = data->Functions[i];
      hasDeprecatedEntries |= (currentFunction->IsDeprecated);
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
          if (!strcmp(val->Class, "vtkIndent"))
          {
            continue;
          }

          classname = 0;
          if (vtkWrap_IsString(val))
          {
            classname = val->Class;
          }
          else if (vtkWrap_IsObject(val) && !vtkWrap_IsRef(val))
          {
            if (!strncmp(val->Class, "vtkSmartPointer<", 16))
            {
              const size_t numCharacters = strlen(val->Class) - 1 - 16; // -1 for trailing '>'
              classname = vtkParse_CacheString(file_info->Strings, val->Class + 16, numCharacters);
            }
            else
            {
              classname = val->Class;
            }
          }
          else if (!strncmp(val->Class, "vtkVector", 9) || !strncmp(val->Class, "vtkTuple", 8) ||
            !strncmp(val->Class, "vtkColor", 8) || !strncmp(val->Class, "vtkRect", 7))
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
                  (const char**)realloc((void*)types, (numTypes + 1000) * sizeof(const char*));
              }
              types[numTypes++] = classname;
            }
          }
        }
      }
    }
  }

  if (hasDeprecatedEntries)
  {
    fprintf(fp, "#define VTK_DEPRECATION_LEVEL 0\n");
  }

  /* get our own include file (returns NULL if hinfo is NULL) */
  data = file_info->MainClass;
  if (!data && file_info->Contents->NumberOfClasses > 0)
  {
    data = file_info->Contents->Classes[0];
  }

  if (data)
  {
    ownincfile = vtkWrapSerDes_ClassHeader(hinfo, data->Name);
  }

  includedHeaders = (const char**)malloc(numTypes * sizeof(const char*));

  /* for each unique type found in the file */
  for (i = 0; i < numTypes; i++)
  {
    const char* incfile = vtkWrapSerDes_ClassHeader(hinfo, types[i]);

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

  free((void*)includedHeaders);
  includedHeaders = NULL;
  free((void*)types);
}

/* -------------------------------------------------------------------- */
/* check whether an enum type will be wrapped */
int vtkWrapSerDes_IsEnumWrapped(const HierarchyInfo* hinfo, const char* enumname)
{
  int rval = 0;
  const HierarchyEntry* entry;

  if (hinfo && enumname)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, enumname);
    if (entry && entry->IsEnum && !vtkParseHierarchy_GetProperty(entry, "WRAPEXCLUDE"))
    {
      rval = 1;
    }
  }

  return rval;
}

/* -------------------------------------------------------------------- */
/* find and mark all enum parameters by setting IsEnum=1 */
static void vtkWrapSerDes_MarkAllEnums(NamespaceInfo* contents, const HierarchyInfo* hinfo)
{
  FunctionInfo* currentFunction;
  int i, j, n, m, ii, nn;
  ClassInfo* data;
  ValueInfo* val;

  nn = contents->NumberOfClasses;
  for (ii = 0; ii < nn; ii++)
  {
    data = contents->Classes[ii];
    n = data->NumberOfFunctions;
    for (i = 0; i < n; i++)
    {
      currentFunction = data->Functions[i];
      if (!currentFunction->IsExcluded && currentFunction->Access == VTK_ACCESS_PUBLIC)
      {
        /* we start with the return value */
        val = currentFunction->ReturnValue;
        m = vtkWrap_CountWrappedParameters(currentFunction);

        /* the -1 is for the return value */
        for (j = (val ? -1 : 0); j < m; j++)
        {
          if (j >= 0)
          {
            val = currentFunction->Parameters[j];
          }

          if (vtkWrap_IsEnumMember(data, val) || vtkWrapSerDes_IsEnumWrapped(hinfo, val->Class))
          {
            val->IsEnum = 1;
          }
        }
      }
    }
  }
}

/**
 * This is the main entry point for generating object coders.
 * When called, it will print the vtkXXXSerialization.cxx file contents to "fp".
 */
int VTK_PARSE_MAIN(int argc, char* argv[])
{
  ClassInfo* classInfo = NULL;
  NamespaceInfo* contents = NULL;
  const OptionInfo* options = NULL;
  const HierarchyInfo* hinfo = NULL;
  FileInfo* file_info = NULL;
  FILE* fp = NULL;
  const char* name = NULL;
  int i = 0;
  size_t k, m = 0;
  char* name_from_file = NULL;

  /* get command-line args and parse the header file */
  file_info = vtkParse_Main(argc, argv);

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames != NULL)
  {
    hinfo =
      vtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the output file */
  fp = vtkParse_FileOpen(options->OutputFileName, "w");

#ifdef _WIN32
  if (!fp)
  {
    /* repeatedly try to open output file in case of access/sharing error */
    /* (for example, antivirus software might be scanning the output file) */
    int tries;
    for (tries = 0; !fp && tries < 5 && errno == EACCES; tries++)
    {
      Sleep(1000);
      fp = vtkParse_FileOpen(options->OutputFileName, "w");
    }
  }
#endif
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

  /* Identify all enum types that are used by methods */
  vtkWrapSerDes_MarkAllEnums(contents, hinfo);

  /* SPDX */
  fprintf(fp,
    "// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen\n"
    "// SPDX-License-Identifier: BSD-3-Clause\n");

  /* use the hierarchy file to find super classes and expand typedefs */
  int exitCode = 0;
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
    int registrarsExist = 0;
    if (contents->NumberOfClasses == 0)
    {
      // Write a placeholder registrar that always succeeds.
      vtkWrapSerDes_ExportClassRegistrars(fp, name);
      fprintf(fp,
        "int RegisterHandlers_%sSerDes(void* /*ser*/, void* /*deser*/, void* /*invoker*/)\n"
        "{\n"
        "  return 1;\n"
        "}\n",
        name);
    }
    /* generate serializers and deserializers */
    for (i = 0; i < contents->NumberOfClasses; ++i)
    {
      classInfo = contents->Classes[i];
      const int isVTKObject = vtkWrap_IsTypeOf(hinfo, classInfo->Name, "vtkObjectBase");
      if ((classInfo->Template != NULL) || (classInfo->MarshalType == VTK_MARSHAL_NONE) ||
        !isVTKObject)
      {
        continue;
      }
      if (!registrarsExist)
      {
        vtkWrapSerDes_GenerateSpecialHeaders(fp, file_info, hinfo);
        fprintf(fp, "#include \"%s.h\"\n", name);
        fprintf(fp, "//clang-format off\n");
        fprintf(fp, "#include \"vtk_nlohmannjson.h\"\n");
        fprintf(fp, "#include VTK_NLOHMANN_JSON(json.hpp)\n");
        fprintf(fp, "//clang-format on\n");
        registrarsExist = 1;
      }
      vtkWrapSerDes_Class(fp, hinfo, classInfo);
    }
    /* generate handler code for templates/unmarshalled classes */
    for (i = 0; i < contents->NumberOfClasses; ++i)
    {
      classInfo = contents->Classes[i];
      const int isVTKObject = vtkWrap_IsTypeOf(hinfo, classInfo->Name, "vtkObjectBase");
      // if headers were written, assume that the main class is wrapped and there is nothing left to
      // do.
      if (registrarsExist)
      {
        continue;
      }
      else if (!isVTKObject || (classInfo->Template != NULL) ||
        ((classInfo->MarshalType == VTK_MARSHAL_NONE)))
      {
        // For templates, the cmake code uses the file name in regsitrar.
        vtkWrapSerDes_ExportClassRegistrars(fp, name);
        fprintf(fp,
          "int RegisterHandlers_%sSerDes(void* /*ser*/, void* /*deser*/, void* /*invoker*/)\n"
          "{\n"
          "  return 1;\n"
          "}\n",
          name);
        break;
      }
    }
  }
  else
  {
    fprintf(
      stderr, "Hierarchy information unavailable. Did you forget to turn on VTK_ENABLE_WRAPPING?");
    exitCode = 1;
  }
  fclose(fp);
  free(name_from_file);
  vtkParse_Free(file_info);
  return vtkParse_FinalizeMain(exitCode);
}
