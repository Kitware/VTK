/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWrapPythonClass.c

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
#include "vtkWrapPythonTemplate.h"
#include "vtkWrapPythonType.h"

#include "vtkWrap.h"
#include "vtkWrapText.h"
#include "vtkParseExtras.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* declare the exports and imports for a VTK/Python class */
static void vtkWrapPython_ExportVTKClass(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo);

/* generate the New method for a vtkObjectBase object */
static void vtkWrapPython_GenerateObjectNew(
  FILE *fp, const char *classname, ClassInfo *data,
  HierarchyInfo *hinfo, int class_has_new);


/* -------------------------------------------------------------------- */
/* get the true superclass */
const char *vtkWrapPython_GetSuperClass(
  ClassInfo *data, HierarchyInfo *hinfo)
{
  int i;
  const char *supername = NULL;
  const char *name;
  const char **args;
  const char *defaults[2] = { NULL, NULL };
  char *cp;

  for (i = 0; i < data->NumberOfSuperClasses; i++)
    {
    supername = data->SuperClasses[i];

    if (strncmp(supername, "vtkTypeTemplate<", 16) == 0)
      {
      vtkParse_DecomposeTemplatedType(supername, &name, 2, &args, defaults);
      cp = (char *)malloc(strlen(args[1])+1);
      strcpy(cp, args[1]);
      vtkParse_FreeTemplateDecomposition(name, 2, args);
      supername = cp;
      }

    // Add QVTKInteractor as the sole exception: It is derived
    // from vtkObject but does not start with "vtk".  Given its
    // name, it would be expected to be derived from QObject.
    if (vtkWrap_IsVTKObjectBaseType(hinfo, data->Name) ||
        strcmp(data->Name, "QVTKInteractor") == 0)
      {
      if (vtkWrap_IsClassWrapped(hinfo, supername) &&
          vtkWrap_IsVTKObjectBaseType(hinfo, supername))
        {
        return supername;
        }
      }
    else if (vtkWrapPython_HasWrappedSuperClass(hinfo, data->Name, NULL))
      {
      return supername;
      }
    }

  return NULL;
}

/* -------------------------------------------------------------------- */
/* check whether the superclass of the specified class is wrapped */
int vtkWrapPython_HasWrappedSuperClass(
  HierarchyInfo *hinfo, const char *classname, int *is_external)
{
  HierarchyEntry *entry;
  const char *module;
  const char *header;
  const char *name;
  const char *supername;
  int result = 0;
  int depth = 0;

  if (is_external)
    {
    *is_external = 0;
    }

  if (!hinfo)
    {
    return 0;
    }

  name = classname;
  entry = vtkParseHierarchy_FindEntry(hinfo, name);
  if (!entry)
    {
    return 0;
    }

  module = entry->Module;
  header = entry->HeaderFile;
  while (entry->NumberOfSuperClasses == 1)
    {
    supername = vtkParseHierarchy_TemplatedSuperClass(entry, name, 0);
    if (name != classname)
      {
      free((char *)name);
      }
    name = supername;
    entry = vtkParseHierarchy_FindEntry(hinfo, name);
    if (!entry)
      {
      break;
      }

    /* check if superclass is in a different module */
    if (is_external && depth == 0 && strcmp(entry->Module, module) != 0)
      {
      *is_external = 1;
      }
    depth++;

    /* the order of these conditions is important */
    if (entry->IsTypedef)
      {
      break;
      }
    else if (!vtkParseHierarchy_GetProperty(entry, "WRAP_EXCLUDE"))
      {
      result = 1;
      break;
      }
    else if (strncmp(entry->Name, "vtk", 3) != 0)
      {
      break;
      }
    else if (vtkParseHierarchy_GetProperty(entry, "WRAP_SPECIAL"))
      {
      result = 1;
      break;
      }
    else if (strcmp(entry->HeaderFile, header) != 0)
      {
      break;
      }
    }

  if (name != classname)
    {
    free((char *)name);
    }

  return result;
}


/* -------------------------------------------------------------------- */
/* Create the docstring for a class, and print it to fp */
void vtkWrapPython_ClassDoc(
  FILE *fp, FileInfo *file_info, ClassInfo *data, HierarchyInfo *hinfo,
  int is_vtkobject)
{
  char pythonname[1024];
  const char *supername;
  char *cp;
  const char *ccp;
  size_t i, n;
  int j;
  char temp[500];
  char *comment;

  if (file_info->NameComment)
    {
    fprintf(fp,
            "    \"%s\\n\",\n",
            vtkWrapText_QuoteString(
              vtkWrapText_FormatComment(file_info->NameComment, 70), 500));
    }
  else
    {
    fprintf(fp,
            "    \"%s - no description provided.\\n\\n\",\n",
            vtkWrapText_QuoteString(data->Name, 500));
    }

  /* only consider superclasses that are wrapped */
  supername = vtkWrapPython_GetSuperClass(data, hinfo);
  if (supername)
    {
    vtkWrapPython_PyTemplateName(supername, pythonname);
    fprintf(fp,
            "    \"Superclass: %s\\n\\n\",\n",
            vtkWrapText_QuoteString(pythonname, 500));
    }

  n = 100;
  if (file_info->Description)
    {
    n += strlen(file_info->Description);
    }

  if (file_info->Caveats)
    {
    n += strlen(file_info->Caveats);
    }

  if (file_info->SeeAlso)
    {
    n += strlen(file_info->SeeAlso);
    }

  comment = (char *)malloc(n);
  cp = comment;
  *cp = '\0';

  if (file_info->Description)
    {
    strcpy(cp, file_info->Description);
    cp += strlen(cp);
    *cp++ = '\n'; *cp++ = '\n'; *cp = '\0';
    }

  if (file_info->Caveats)
    {
    sprintf(cp, ".SECTION Caveats\n\n");
    cp += strlen(cp);
    strcpy(cp, file_info->Caveats);
    cp += strlen(cp);
    *cp++ = '\n'; *cp++ = '\n'; *cp = '\0';
    }

  if (file_info->SeeAlso)
    {
    sprintf(cp, ".SECTION See Also\n\n");
    cp += strlen(cp);
    strcpy(cp, file_info->SeeAlso);
    cp += strlen(cp);
    *cp = '\0';
    }

  ccp = vtkWrapText_FormatComment(comment, 70);
  free(comment);

  n = (strlen(ccp) + 400-1)/400;
  for (i = 0; i < n; i++)
    {
    strncpy(temp, &ccp[400*i], 400);
    temp[400] = '\0';
    if (i < n-1)
      {
      fprintf(fp,
              "    \"%s\",\n",
              vtkWrapText_QuoteString(temp, 500));
      }
    else
      { /* just for the last time */
      fprintf(fp,
              "    \"%s\\n\",\n",
              vtkWrapText_QuoteString(temp, 500));
      }
    }

  /* for special objects, add constructor signatures to the doc */
  if (!is_vtkobject && !data->Template)
    {
    for (j = 0; j < data->NumberOfFunctions; j++)
      {
      if (vtkWrapPython_MethodCheck(data, data->Functions[j], hinfo) &&
          vtkWrap_IsConstructor(data, data->Functions[j]))
        {
        fprintf(fp,"    \"%s\\n\",\n",
                vtkWrapText_FormatSignature(
                  data->Functions[j]->Signature, 70, 2000));
        }
      }
    }
}


/* -------------------------------------------------------------------- */
/* Declare the exports and imports for a VTK/Python class */
static void vtkWrapPython_ExportVTKClass(
  FILE *fp, ClassInfo *data, HierarchyInfo *hinfo)
{
  char classname[1024];
  const char *supername;

  /* mangle the classname if necessary */
  vtkWrapPython_PythonicName(data->Name, classname);

  /* for vtkObjectBase objects: export New method for use by subclasses */
  fprintf(fp,
          "extern \"C\" { %s PyObject *PyVTKClass_%sNew(const char *); }\n"
          "\n",
          "VTK_PYTHON_EXPORT", classname);

  /* declare the New methods for all the superclasses */
  supername = vtkWrapPython_GetSuperClass(data, hinfo);
  if (supername)
    {
    vtkWrapPython_PythonicName(supername, classname);
    fprintf(fp,
      "#ifndef DECLARED_PyVTKClass_%sNew\n"
      "extern \"C\" { PyObject *PyVTKClass_%sNew(const char *); }\n"
      "#define DECLARED_PyVTKClass_%sNew\n"
      "#endif\n",
      classname, classname, classname);
    }
}

/* -------------------------------------------------------------------- */
/* generate the New method for a vtkObjectBase object */
static void vtkWrapPython_GenerateObjectNew(
  FILE *fp, const char *classname, ClassInfo *data,
  HierarchyInfo *hinfo, int class_has_new)
{
  char superclassname[1024];
  const char *name;
  int has_constants = 0;
  int i;

  if (class_has_new)
    {
    fprintf(fp,
            "static vtkObjectBase *Py%s_StaticNew()\n"
            "{\n"
            "  return %s::New();\n"
            "}\n"
            "\n",
            classname, data->Name);
    }

  fprintf(fp,
          "PyObject *PyVTKClass_%sNew(const char *modulename)\n"
          "{\n",
          classname);

  if (class_has_new)
    {
    fprintf(fp,
            "  PyObject *cls = PyVTKClass_New(&Py%s_StaticNew,\n",
            classname);
    }
  else
    {
    fprintf(fp,
            "  PyObject *cls = PyVTKClass_New(NULL,\n");
    }

  if (strcmp(data->Name, classname) == 0)
    {
    fprintf(fp,
            "    Py%s_Methods,\n"
            "    \"%s\", modulename,\n"
            "    NULL, NULL,\n"
            "    Py%s_Doc(),",
            classname, classname, classname);
    }
  else
    {
    /* use of typeid() matches vtkTypeTemplate */
    fprintf(fp,
            "    Py%s_Methods,\n"
            "    typeid(%s).name(), modulename,\n"
            "    \"%s\", \"%s\",\n"
            "    Py%s_Doc(),",
            classname, data->Name, classname, classname, classname);
    }

  /* find the first superclass that is a VTK class */
  name = vtkWrapPython_GetSuperClass(data, hinfo);
  if (name)
    {
    vtkWrapPython_PythonicName(name, superclassname);
    fprintf(fp, "\n"
            "    PyVTKClass_%sNew(modulename));\n",
            superclassname);
    }
  else
    {
    fprintf(fp, "0);\n");
    }

  for (i = 0; i < data->NumberOfConstants; i++)
    {
    if (data->Constants[i]->Access == VTK_ACCESS_PUBLIC)
      {
      has_constants = 1;
      break;
      }
    }

  if (has_constants)
    {
    fprintf(fp,
            "\n"
            "  if (cls)\n"
            "    {\n"
            "    PyObject *d = PyVTKClass_GetDict(cls);\n"
            "    PyObject *o;\n"
            "\n");

    /* add any enum types defined in the class to its dict */
    vtkWrapPython_AddPublicEnumTypes(fp, "    ", "d", "o", data);

    /* add any constants defined in the class to its dict */
    vtkWrapPython_AddPublicConstants(fp, "    ", "d", "o", data);

    fprintf(fp,
            "    }\n"
            "\n");
    }

  fprintf(fp,
          "  return cls;\n"
          "}\n"
          "\n");
}

/* -------------------------------------------------------------------- */
/* Wrap one class */
int vtkWrapPython_WrapOneClass(
  FILE *fp, const char *classname, ClassInfo *data,
  FileInfo *finfo, HierarchyInfo *hinfo, int is_vtkobject)
{
  int class_has_new = 0;
  int i;

  /* recursive handling of templated classes */
  if (data->Template)
    {
    return vtkWrapPython_WrapTemplatedClass(fp, data, finfo, hinfo);
    }

  /* verify wrappability */
  if (!is_vtkobject && !vtkWrapPython_IsSpecialTypeWrappable(data))
    {
    return 0;
    }

  /* declare items to be exported or imported */
  if (is_vtkobject)
    {
    vtkWrapPython_ExportVTKClass(fp, data, hinfo);
    }

  /* prototype for the docstring function */
  fprintf(fp,
          "\n"
          "static const char **Py%s_Doc();\n"
          "\n",
          classname);

  /* check for New() function */
  for (i = 0; i < data->NumberOfFunctions; i++)
    {
    if (data->Functions[i]->Name &&
        data->Functions[i]->Access == VTK_ACCESS_PUBLIC &&
        strcmp("New",data->Functions[i]->Name) == 0 &&
        data->Functions[i]->NumberOfParameters == 0)
      {
      class_has_new = 1;
      }
    }

  /* create any enum types defined in the class */
  for (i = 0; i < data->NumberOfEnums; i++)
    {
    if (data->Enums[i]->Access == VTK_ACCESS_PUBLIC)
      {
      vtkWrapPython_GenerateEnumType(
        fp, classname, data->Enums[i]);
      }
    }

  /* now output all the methods are wrappable */
  if (is_vtkobject || !data->IsAbstract)
    {
    vtkWrapPython_GenerateMethods(
      fp, classname, data, finfo, hinfo, is_vtkobject, 0);
    }

  /* output the class initilization function for VTK objects */
  if (is_vtkobject)
    {
    vtkWrapPython_GenerateObjectNew(
      fp, classname, data, hinfo, class_has_new);
    }

  /* output the class initilization function for special objects */
  else if (!data->IsAbstract)
    {
    vtkWrapPython_GenerateSpecialType(
      fp, classname, data, finfo, hinfo);
    }

  /* the docstring for the class, as a static var ending in "Doc" */
  if (is_vtkobject || !data->IsAbstract)
    {
    fprintf(fp,
            "const char **Py%s_Doc()\n"
            "{\n"
            "  static const char *docstring[] = {\n",
            classname);

    vtkWrapPython_ClassDoc(fp, finfo, data, hinfo, is_vtkobject);

    fprintf(fp,
            "    NULL\n"
            "  };\n"
            "\n"
            "  return docstring;\n"
            "}\n"
            "\n");
    }

  return 1;
}
