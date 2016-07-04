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

    /* Add QVTKInteractor as the sole exception: It is derived
     * from vtkObject but does not start with "vtk".  Given its
     * name, it would be expected to be derived from QObject. */
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
    else if (strncmp(entry->Name, "vtk", 3) != 0)
    {
      break;
    }
    else if (!vtkParseHierarchy_GetProperty(entry, "WRAP_EXCLUDE_PYTHON"))
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
  const char *ccp = NULL;
  size_t i, n;
  size_t briefmax = 255;
  int j;
  char temp[500];
  char *comment;

  if (data == file_info->MainClass && file_info->NameComment)
  {
    /* use the old VTK-style class description */
    fprintf(fp,
            "    \"%s\\n\",\n",
            vtkWrapText_QuoteString(
              vtkWrapText_FormatComment(file_info->NameComment, 70), 500));
  }
  else if (data->Comment)
  {
    strncpy(temp, data->Name, briefmax);
    temp[briefmax] = '\0';
    i = strlen(temp);
    temp[i++] = ' ';
    temp[i++] = '-';
    if (data->Comment[0] != ' ')
    {
      temp[i++] = ' ';
    }

    /* extract the brief comment, if present */
    ccp = data->Comment;
    while (i < briefmax && *ccp != '\0')
    {
      /* a blank line ends the brief comment */
      if (ccp[0] == '\n' && ccp[1] == '\n')
      {
        break;
      }
      /* fuzzy: capital letter or a new command on next line ends brief */
      if (ccp[0] == '\n' && ccp[1] == ' ' &&
          ((ccp[2] >= 'A' && ccp[2] <= 'Z') ||
           ccp[2] == '@' || ccp[2] == '\\'))
      {
        break;
      }
      temp[i] = *ccp;
      /* a sentence-ending period ends the brief comment */
      if (ccp[0] == '.' && (ccp[1] == ' ' || ccp[1] == '\n'))
      {
        i++;
        ccp++;
        while (*ccp == ' ')
        {
          ccp++;
        }
        break;
      }
      ccp++;
      i++;
    }
    /* skip all blank lines */
    while (*ccp == '\n')
    {
      ccp++;
    }
    if (*ccp == '\0')
    {
      ccp = NULL;
    }

    temp[i] = '\0';
    fprintf(fp,
            "    \"%s\\n\",\n",
            vtkWrapText_QuoteString(
              vtkWrapText_FormatComment(temp, 70), 500));
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

  if (data == file_info->MainClass &&
      (file_info->Description ||
       file_info->Caveats ||
       file_info->SeeAlso))
  {
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
  }
  else if (ccp)
  {
    ccp = vtkWrapText_FormatComment(ccp, 70);
  }

  if (ccp)
  {
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
  }

  /* for special objects, add constructor signatures to the doc */
  if (!is_vtkobject && !data->Template && !data->IsAbstract)
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
  vtkWrapText_PythonName(data->Name, classname);

  /* for vtkObjectBase objects: export New method for use by subclasses */
  fprintf(fp,
          "extern \"C\" { %s PyObject *Py%s_ClassNew(); }\n"
          "\n",
          "VTK_ABI_EXPORT", classname);

  /* declare the New methods for all the superclasses */
  supername = vtkWrapPython_GetSuperClass(data, hinfo);
  if (supername)
  {
    vtkWrapText_PythonName(supername, classname);
    fprintf(fp,
      "#ifndef DECLARED_Py%s_ClassNew\n"
      "extern \"C\" { PyObject *Py%s_ClassNew(); }\n"
      "#define DECLARED_Py%s_ClassNew\n"
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
          "PyObject *Py%s_ClassNew()\n"
          "{\n"
          "  PyVTKClass_Add(\n"
          "    &Py%s_Type, Py%s_Methods,\n",
          classname, classname, classname);

  if (strcmp(data->Name, classname) == 0)
  {
    fprintf(fp,
            "    \"%s\",\n"
            "    Py%s_Doc(),",
            classname, classname);
  }
  else
  {
    /* use of typeid() matches vtkTypeTemplate */
    fprintf(fp,
            "    typeid(%s).name(),\n"
            "    Py%s_Doc(),",
            data->Name, classname);
  }

  if (class_has_new)
  {
    fprintf(fp,
            " &Py%s_StaticNew);\n\n",
            classname);
  }
  else
  {
    fprintf(fp,
            " NULL);\n\n");
  }

  fprintf(fp,
          "  PyTypeObject *pytype = &Py%s_Type;\n\n",
          classname);

  /* if type is already ready, then return */
  fprintf(fp,
          "  if ((pytype->tp_flags & Py_TPFLAGS_READY) != 0)\n"
          "  {\n"
          "    return (PyObject *)pytype;\n"
          "  }\n\n");

  /* add any flags specific to this type */
  fprintf(fp,
          "#if !defined(VTK_PY3K) && PY_VERSION_HEX >= 0x02060000\n"
          "  pytype->tp_flags |= Py_TPFLAGS_HAVE_NEWBUFFER;\n"
          "#endif\n\n");

  /* find the first superclass that is a VTK class, create it first */
  name = vtkWrapPython_GetSuperClass(data, hinfo);
  if (name)
  {
    vtkWrapText_PythonName(name, superclassname);
    fprintf(fp,
            "  pytype->tp_base = (PyTypeObject *)Py%s_ClassNew();\n\n",
            superclassname);
  }

  /* check if any constants need to be added to the class dict */
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
            "  PyObject *d = pytype->tp_dict;\n"
            "  PyObject *o;\n"
            "\n");

    /* add any enum types defined in the class to its dict */
    vtkWrapPython_AddPublicEnumTypes(fp, "  ", "d", "o", data);

    /* add any constants defined in the class to its dict */
    vtkWrapPython_AddPublicConstants(fp, "  ", "d", "o", data);
  }

  fprintf(fp,
          "  PyType_Ready(pytype);\n"
          "  return (PyObject *)pytype;\n"
          "}\n\n");
}

/* -------------------------------------------------------------------- */
/* write out the type object */
void vtkWrapPython_GenerateObjectType(
  FILE *fp, const char *module, const char *classname)
{
  /* Generate the TypeObject */
  fprintf(fp,
    "static PyTypeObject Py%s_Type = {\n"
    "  PyVarObject_HEAD_INIT(&PyType_Type, 0)\n"
    "  \"%sPython.%s\", // tp_name\n"
    "  sizeof(PyVTKObject), // tp_basicsize\n"
    "  0, // tp_itemsize\n"
    "  PyVTKObject_Delete, // tp_dealloc\n"
    "  0, // tp_print\n"
    "  0, // tp_getattr\n"
    "  0, // tp_setattr\n"
    "  0, // tp_compare\n"
    "  PyVTKObject_Repr, // tp_repr\n",
    classname, module, classname);

  fprintf(fp,
    "  0, // tp_as_number\n"
    "  0, // tp_as_sequence\n"
    "  0, // tp_as_mapping\n"
    "  0, // tp_hash\n"
    "  0, // tp_call\n"
    "  PyVTKObject_String, // tp_str\n");

  fprintf(fp,
    "  PyObject_GenericGetAttr, // tp_getattro\n"
    "  PyObject_GenericSetAttr, // tp_setattro\n"
    "  &PyVTKObject_AsBuffer, // tp_as_buffer\n"
    "  Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,"
      " // tp_flags\n"
    "  0, // tp_doc\n"
    "  PyVTKObject_Traverse, // tp_traverse\n"
    "  0, // tp_clear\n"
    "  0, // tp_richcompare\n"
    "  offsetof(PyVTKObject, vtk_weakreflist), // tp_weaklistoffset\n");
  if (strcmp(classname, "vtkCollection") == 0)
  {
    fprintf(fp,
      "  PyvtkCollection_Iter, // tp_iter\n"
      "  0, // tp_iternext\n");
  }
  else
  {
    if(strcmp(classname, "vtkCollectionIterator") == 0)
    {
      fprintf(fp,
        "  PyvtkCollectionIterator_Iter, // tp_iter\n"
        "  PyvtkCollectionIterator_Next, // tp_iternext\n");
    }
    else
    {
      fprintf(fp,
        "  0, // tp_iter\n"
        "  0, // tp_iternext\n");
    }
  }
  fprintf(fp,
    "  0, // tp_methods\n"
    "  0, // tp_members\n"
    "  PyVTKObject_GetSet, // tp_getset\n"
    "  0, // tp_base\n"
    "  0, // tp_dict\n"
    "  0, // tp_descr_get\n"
    "  0, // tp_descr_set\n"
    "  offsetof(PyVTKObject, vtk_dict), // tp_dictoffset\n"
    "  0, // tp_init\n"
    "  0, // tp_alloc\n"
    "  PyVTKObject_New, // tp_new\n"
    "  PyObject_GC_Del, // tp_free\n"
    "  0, // tp_is_gc\n");

  /* fields set by python itself */
  fprintf(fp,
    "  0, // tp_bases\n"
    "  0, // tp_mro\n"
    "  0, // tp_cache\n"
    "  0, // tp_subclasses\n"
    "  0, // tp_weaklist\n");

  /* internal struct members */
  fprintf(fp,
    "  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED\n"
    "};\n"
    "\n");
}

/* -------------------------------------------------------------------- */
/* Wrap one class */
int vtkWrapPython_WrapOneClass(
  FILE *fp, const char *module, const char *classname,
  ClassInfo *data, FileInfo *finfo, HierarchyInfo *hinfo,
  int is_vtkobject)
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
        fp, module, classname, data->Enums[i]);
    }
  }

  /* now output all the methods are wrappable */
  vtkWrapPython_GenerateMethods(
    fp, classname, data, finfo, hinfo, is_vtkobject, 0);

  /* output the class initilization function for VTK objects */
  if (is_vtkobject)
  {
    vtkWrapPython_GenerateObjectType(
      fp, module, classname);
    vtkWrapPython_GenerateObjectNew(
      fp, classname, data, hinfo, class_has_new);
  }

  /* output the class initilization function for special objects */
  else
  {
    vtkWrapPython_GenerateSpecialType(
      fp, module, classname, data, finfo, hinfo);
  }

  /* the docstring for the class, as a static var ending in "Doc" */
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

  return 1;
}
