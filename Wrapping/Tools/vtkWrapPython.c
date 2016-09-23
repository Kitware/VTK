/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPython.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWrapPythonClass.h"
#include "vtkWrapPythonConstant.h"
#include "vtkWrapPythonEnum.h"
#include "vtkWrapPythonMethodDef.h"
#include "vtkWrapPythonNamespace.h"

#include "vtkWrap.h"
#include "vtkParseMain.h"
#include "vtkParseExtras.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* -------------------------------------------------------------------- */
/* the main entry method, called by vtkParse.y */
void vtkParseOutput(FILE *fp, FileInfo *data);


/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* get the header file for the named VTK class */
static const char *vtkWrapPython_ClassHeader(
  HierarchyInfo *hinfo, const char *classname);

/* get the module for the named VTK class */
static const char *vtkWrapPython_ClassModule(
  HierarchyInfo *hinfo, const char *classname);

/* print out headers for any special types used by methods */
static void vtkWrapPython_GenerateSpecialHeaders(
  FILE *fp, FileInfo *file_info, HierarchyInfo *hinfo);


/* -------------------------------------------------------------------- */
/* Get the header file for the specified class */
static const char *vtkWrapPython_ClassHeader(
  HierarchyInfo *hinfo, const char *classname)
{
  /* to allow special types to be used when "hinfo" is not available
   * (this is necessary until VTK exports the hierarchy files) */
  static const char *headers[][2] = {
    { "vtkArrayCoordinates", "vtkArrayCoordinates.h" },
    { "vtkArrayExtents", "vtkArrayExtents.h" },
    { "vtkArrayExtentsList", "vtkArrayExtentsList.h" },
    { "vtkArrayRange", "vtkArrayRange.h" },
    { "vtkArraySort", "vtkArraySort.h" },
    { "vtkArrayWeights", "vtkArrayWeights.h" },
    { "vtkAtom", "vtkAtom.h" },
    { "vtkBond", "vtkBond.h" },
    { "vtkTimeStamp", "vtkTimeStamp.h" },
    { "vtkVariant", "vtkVariant.h" },
    { "vtkStdString", "vtkStdString.h" },
    { "vtkUnicodeString", "vtkUnicodeString.h" },
    { "vtkTuple", "vtkVector.h" },
    { "vtkVector", "vtkVector.h" },
    { "vtkVector2", "vtkVector.h" },
    { "vtkVector2i", "vtkVector.h" },
    { "vtkVector2f", "vtkVector.h" },
    { "vtkVector2d", "vtkVector.h" },
    { "vtkVector3", "vtkVector.h" },
    { "vtkVector3i", "vtkVector.h" },
    { "vtkVector3f", "vtkVector.h" },
    { "vtkVector3d", "vtkVector.h" },
    { "vtkRect", "vtkRect.h" },
    { "vtkRecti", "vtkRect.h" },
    { "vtkRectf", "vtkRect.h" },
    { "vtkRectd", "vtkRect.h" },
    { "vtkColor", "vtkColor.h" },
    { "vtkColor3", "vtkColor.h" },
    { "vtkColor3ub", "vtkColor.h" },
    { "vtkColor3f", "vtkColor.h" },
    { "vtkColor3d", "vtkColor.h" },
    { "vtkColor4", "vtkColor.h" },
    { "vtkColor4ub", "vtkColor.h" },
    { "vtkColor4f", "vtkColor.h" },
    { "vtkColor4d", "vtkColor.h" },
    { "vtkAMRBox", "vtkAMRBox.h" },
    { "vtkEdgeBase", "vtkGraph.h" },
    { "vtkEdgeType", "vtkGraph.h" },
    { "vtkInEdgeType", "vtkGraph.h" },
    { "vtkOutEdgeType", "vtkGraph.h" },
    { NULL, NULL }
  };

  HierarchyEntry *entry;
  int i;
  size_t n;

  /* if "hinfo" is present, use it to find the file */
  if (hinfo)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      return entry->HeaderFile;
    }
  }

  /* otherwise, use the hard-coded entries */
  n = vtkParse_IdentifierLength(classname);
  for (i = 0; headers[i][0]; i++)
  {
    if (strlen(headers[i][0]) == n &&
        strncmp(classname, headers[i][0], n) == 0)
    {
      return headers[i][1];
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Get the module for the specified class */
static const char *vtkWrapPython_ClassModule(
  HierarchyInfo *hinfo, const char *classname)
{
  HierarchyEntry *entry;

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
/* generate includes for any special types that are used */
static void vtkWrapPython_GenerateSpecialHeaders(
  FILE *fp, FileInfo *file_info, HierarchyInfo *hinfo)
{
  const char **types;
  int numTypes = 0;
  FunctionInfo *currentFunction;
  int i, j, k, n, m, ii, nn;
  unsigned int aType;
  const char *classname;
  const char *ownincfile = "";
  ClassInfo *data;

  types = (const char **)malloc(1000*sizeof(const char *));

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
      if (currentFunction->Access == VTK_ACCESS_PUBLIC)
      {
        classname = "void";
        aType = VTK_PARSE_VOID;
        if (currentFunction->ReturnValue)
        {
          classname = currentFunction->ReturnValue->Class;
          aType = currentFunction->ReturnValue->Type;
        }

        m = vtkWrap_CountWrappedParameters(currentFunction);

        for (j = -1; j < m; j++)
        {
          if (j >= 0)
          {
            classname = currentFunction->Parameters[j]->Class;
            aType = currentFunction->Parameters[j]->Type;
          }
          /* we don't require the header file if it is just a pointer */
          if ((aType & VTK_PARSE_INDIRECT) != VTK_PARSE_POINTER)
          {
            if ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_STRING)
            {
              classname = "vtkStdString";
            }
            else if ((aType & VTK_PARSE_BASE_TYPE) == VTK_PARSE_UNICODE_STRING)
            {
              classname = "vtkUnicodeString";
            }
            else if ((aType & VTK_PARSE_BASE_TYPE) != VTK_PARSE_OBJECT)
            {
              classname = 0;
            }
          }
          else
          {
            classname = 0;
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
                types = (const char **)realloc((char **)types,
                  (numTypes + 1000)*sizeof(const char *));
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
    ownincfile = vtkWrapPython_ClassHeader(hinfo, data->Name);
  }

  /* for each unique type found in the file */
  for (i = 0; i < numTypes; i++)
  {
    const char *incfile;
    incfile = vtkWrapPython_ClassHeader(hinfo, types[i]);

    if (incfile)
    {
      /* make sure it doesn't share our header file */
      if (ownincfile == 0 || strcmp(incfile, ownincfile) != 0)
      {
        fprintf(fp,
               "#include \"%s\"\n",
                incfile);
      }
    }
  }

  free((char **)types);
}

/* -------------------------------------------------------------------- */
/* This is the main entry point for the python wrappers.  When called,
 * it will print the vtkXXPython.c file contents to "fp".  */

#define MAX_WRAPPED_CLASSES 256

int main(int argc, char *argv[])
{
  ClassInfo *wrappedClasses[MAX_WRAPPED_CLASSES];
  unsigned char wrapAsVTKObject[MAX_WRAPPED_CLASSES];
  ClassInfo *data = NULL;
  NamespaceInfo *contents;
  OptionInfo *options;
  HierarchyInfo *hinfo = NULL;
  FileInfo *file_info;
  FILE *fp;
  const char *module = "vtkCommonCore";
  const char *name;
  char *name_from_file = NULL;
  int numberOfWrappedClasses = 0;
  int numberOfWrappedNamespaces = 0;
  int wrapped_anything = 0;
  int i, j;
  size_t k, m;
  int is_vtkobject;

  /* pre-define a macro to identify the language */
  vtkParse_DefineMacro("__VTK_WRAP_PYTHON__", 0);

  /* get command-line args and parse the header file */
  file_info = vtkParse_Main(argc, argv);

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the output file */
  fp = fopen(options->OutputFileName, "w");

  if (!fp)
  {
    fprintf(stderr, "Error opening output file %s\n", options->OutputFileName);
    exit(1);
  }

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hinfo = vtkParseHierarchy_ReadFiles(
      options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the filename without the extension */
  name = file_info->FileName;
  m = strlen(name);
  for (k = m; k > 0; k--)
  {
    if (name[k] == '.') { break; }
  }
  if (k > 0) { m = k; }
  for (k = m; k > 0; k--)
  {
    if (!((name[k-1] >= 'a' && name[k-1] <= 'z') ||
          (name[k-1] >= 'A' && name[k-1] <= 'Z') ||
          (name[k-1] >= '0' && name[k-1] <= '9') ||
          name[k-1] == '_')) { break; }
  }
  name_from_file = (char *)malloc(m - k + 1);
  strncpy(name_from_file, &name[k], m - k);
  name_from_file[m-k] = '\0';
  name = name_from_file;

  /* get the global namespace */
  contents = file_info->Contents;

  /* use the hierarchy file to expand typedefs */
  if (hinfo)
  {
    for (i = 0; i < contents->NumberOfClasses; i++)
    {
      vtkWrap_ApplyUsingDeclarations(contents->Classes[i], file_info, hinfo);
    }
    for (i = 0; i < contents->NumberOfClasses; i++)
    {
      vtkWrap_ExpandTypedefs(contents->Classes[i], file_info, hinfo);
    }
  }

  /* the VTK_WRAPPING_CXX tells header files where they're included from */
  fprintf(fp,
          "// python wrapper for %s\n//\n"
          "#define VTK_WRAPPING_CXX\n",
          name);

  /* unless this is vtkObjectBase.h, define VTK_STREAMS_FWD_ONLY */
  if (strcmp("vtkObjectBase", name) != 0)
  {
    /* Block inclusion of full streams.  */
    fprintf(fp,
            "#define VTK_STREAMS_FWD_ONLY\n");
  }

  /* lots of important utility functions are defined in vtkPythonArgs.h */
  fprintf(fp,
          "#include \"vtkPythonArgs.h\"\n"
          "#include \"vtkPythonOverload.h\"\n"
          "#include \"vtkConfigure.h\"\n"
          "#include <cstddef>\n"
          "#include <sstream>\n");

  /* vtkPythonCommand is needed to wrap vtkObject.h */
  if (strcmp("vtkObject", name) == 0)
  {
    fprintf(fp,
          "#include \"vtkPythonCommand.h\"\n");
  }

  /* generate includes for any special types that are used */
  vtkWrapPython_GenerateSpecialHeaders(fp, file_info, hinfo);

  /* the header file for the wrapped class */
  fprintf(fp,
          "#include \"%s.h\"\n\n",
          name);

  /* do the export of the main entry point */
  fprintf(fp,
          "extern \"C\" { %s void PyVTKAddFile_%s(PyObject *); }\n",
          "VTK_ABI_EXPORT", name);

  /* get the module that is being wrapped */
  data = file_info->MainClass;
  if (!data && file_info->Contents->NumberOfClasses > 0)
  {
    data = file_info->Contents->Classes[0];
  }
  if (data && hinfo)
  {
    module = vtkWrapPython_ClassModule(hinfo, data->Name);
  }

  /* Identify all enum types that are used by methods */
  vtkWrapPython_MarkAllEnums(file_info->Contents, hinfo);

  /* Wrap any enum types defined in the global namespace */
  for (i = 0; i < contents->NumberOfEnums; i++)
  {
    vtkWrapPython_GenerateEnumType(fp, module, NULL, contents->Enums[i]);
  }

  /* Wrap any namespaces */
  for (i = 0; i < contents->NumberOfNamespaces; i++)
  {
    if (contents->Namespaces[i]->NumberOfConstants > 0)
    {
      vtkWrapPython_WrapNamespace(fp, module, contents->Namespaces[i]);
      numberOfWrappedNamespaces++;
    }
  }

  /* Check for all special classes before any classes are wrapped */
  for (i = 0; i < contents->NumberOfClasses; i++)
  {
    data = contents->Classes[i];

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
        FunctionInfo *func = data->Functions[j];
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
    is_vtkobject = wrapAsVTKObject[i];

    /* if "hinfo" is present, wrap everything, else just the main class */
    if (hinfo || data == file_info->MainClass)
    {
      if (vtkWrapPython_WrapOneClass(
            fp, module, data->Name, data, file_info, hinfo, is_vtkobject))
      {
        /* re-index wrapAsVTKObject for wrapped classes */
        wrapAsVTKObject[numberOfWrappedClasses] = (is_vtkobject ? 1 : 0);
        wrappedClasses[numberOfWrappedClasses++] = data;
      }
    }
  }

  /* The function for adding everything to the module dict */
  wrapped_anything = (numberOfWrappedClasses ||
                      numberOfWrappedNamespaces ||
                      contents->NumberOfConstants);
  fprintf(fp,
          "void PyVTKAddFile_%s(\n"
          "  PyObject *%s)\n"
          "{\n"
          "%s",
          name,
          (wrapped_anything ? "dict" : ""),
          (wrapped_anything ? "  PyObject *o;\n" : ""));

  /* Add all of the namespaces */
  for (j = 0; j < contents->NumberOfNamespaces; j++)
  {
    if (contents->Namespaces[j]->NumberOfConstants > 0)
    {
      fprintf(fp,
            "  o = PyVTKNamespace_%s();\n"
            "  if (o && PyDict_SetItemString(dict, \"%s\", o) != 0)\n"
            "  {\n"
            "    Py_DECREF(o);\n"
            "  }\n"
            "\n",
            contents->Namespaces[j]->Name,
            contents->Namespaces[j]->Name);
    }
  }

  /* Add all of the classes that have been wrapped */
  for (i = 0; i < numberOfWrappedClasses; i++)
  {
    data = wrappedClasses[i];
    is_vtkobject = wrapAsVTKObject[i];

    if (data->Template)
    {
      /* Template generator */
      fprintf(fp,
             "  o = Py%s_TemplateNew();\n"
            "\n",
            data->Name);

      /* Add template specializations to dict */
      fprintf(fp,
             "  if (o)\n"
             "  {\n"
             "    PyObject *l = PyObject_CallMethod(o, (char *)\"values\", 0);\n"
             "    Py_ssize_t n = PyList_GET_SIZE(l);\n"
             "    for (Py_ssize_t i = 0; i < n; i++)\n"
             "    {\n"
             "      PyObject *ot = PyList_GET_ITEM(l, i);\n"
             "      const char *nt = NULL;\n"
             "      if (PyType_Check(ot))\n"
             "      {\n"
             "        nt = ((PyTypeObject *)ot)->tp_name;\n"
             "      }\n"
             "      else if (PyCFunction_Check(ot))\n"
             "      {\n"
             "        nt = ((PyCFunctionObject *)ot)->m_ml->ml_name;\n"
             "      }\n"
             "      if (nt)\n"
             "      {\n"
             "        PyDict_SetItemString(dict, nt, ot);\n"
             "      }\n"
             "    }\n"
             "    Py_DECREF(l);\n"
             "  }\n"
             "\n");
    }
    else if (is_vtkobject)
    {
      /* Class is derived from vtkObjectBase */
      fprintf(fp,
            "  o = Py%s_ClassNew();\n"
            "\n",
            data->Name);
    }
    else
    {
      /* Classes that are not derived from vtkObjectBase */
      fprintf(fp,
            "  o = Py%s_TypeNew();\n"
            "\n",
            data->Name);
    }

    fprintf(fp,
            "  if (o && PyDict_SetItemString(dict, \"%s\", o) != 0)\n"
            "  {\n"
            "    Py_DECREF(o);\n"
            "  }\n"
            "\n",
            data->Name);
  }

  /* add any enum types defined in the file */
  vtkWrapPython_AddPublicEnumTypes(fp, "  ", "dict", "o", contents);

  /* add any constants defined in the file */
  vtkWrapPython_AddPublicConstants(fp, "  ", "dict", "o", contents);

  /* close the AddFile function */
  fprintf(fp,
          "}\n\n");

  free(name_from_file);

  vtkParse_Free(file_info);

  return 0;
}
