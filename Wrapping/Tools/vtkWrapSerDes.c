// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParseData.h"
#include "vtkWrapSerDesClass.h"

#include "vtkParse.h"
#include "vtkParseHierarchy.h"
#include "vtkParseMain.h"
#include "vtkParseSystem.h"
#include "vtkWrap.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* for Sleep() */
#include <windows.h>
#endif

/**
 * This is the main entry point for generating object coders.
 * When called, it will print the vtkXXXSerialization.cxx file contents to "fp".
 */
int VTK_PARSE_MAIN(int argc, char* argv[])
{
  ClassInfo* classInfo = NULL;
  NamespaceInfo* contents = NULL;
  const OptionInfo* options = NULL;
  HierarchyInfo* hinfo = NULL;
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
        "int RegisterHandlers_%sSerDes(void* /*ser*/, void* /*deser*/)\n"
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
      else if (classInfo->MarshalType == VTK_MARSHAL_MANUAL_MODE)
      {
        vtkWrapSerDes_ExportClassRegistrars(fp, name);
        fprintf(fp,
          "extern \"C\"\n"
          "{\n"
          "  int RegisterHandlers_%sSerDesHelper(void* ser, void* deser);\n"
          "}\n"
          "int RegisterHandlers_%sSerDes(void* ser, void* deser)\n"
          "{\n"
          "  return RegisterHandlers_%sSerDesHelper(ser, deser);\n"
          "}\n",
          classInfo->Name, classInfo->Name, classInfo->Name);
        registrarsExist = 1;
      }
      else if (!registrarsExist)
      {
        /* the header file for the marshalled class */
        fprintf(fp, "#define VTK_DEPRECATION_LEVEL 0\n");
        fprintf(fp, "#include \"vtkDeserializer.h\"\n");
        fprintf(fp, "#include \"vtkSerializer.h\"\n");
        fprintf(fp, "#include \"vtkSmartPointer.h\"\n");
        fprintf(fp, "#include \"vtkStdString.h\"\n");
        fprintf(fp, "#include \"vtk_nlohmannjson.h\"\n");
        fprintf(fp, "#include VTK_NLOHMANN_JSON(json.hpp)\n");
        fprintf(fp, "#include \"%s.h\"\n\n", name);
        registrarsExist = 1;
      }
      if (classInfo->MarshalType == VTK_MARSHAL_AUTO_MODE)
      {
        vtkWrapSerDes_Class(fp, hinfo, classInfo);
      }
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
          "int RegisterHandlers_%sSerDes(void* /*ser*/, void* /*deser*/)\n"
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
