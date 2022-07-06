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
#include "vtkWrapPythonMethod.h"
#include "vtkWrapPythonMethodDef.h"
#include "vtkWrapPythonTemplate.h"
#include "vtkWrapPythonType.h"

#include "vtkParseExtras.h"
#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* declare the exports and imports for a VTK/Python class */
static void vtkWrapPython_ExportVTKClass(FILE* fp, ClassInfo* data, HierarchyInfo* hinfo);

/* generate the New method for a vtkObjectBase object */
static void vtkWrapPython_GenerateObjectNew(
  FILE* fp, const char* classname, ClassInfo* data, HierarchyInfo* hinfo, int class_has_new);

/* -------------------------------------------------------------------- */
/* get the true superclass */
const char* vtkWrapPython_GetSuperClass(
  ClassInfo* data, HierarchyInfo* hinfo, const char** supermodule)
{
  const char* supername = NULL;
  const char* module = NULL;
  HierarchyEntry* entry;
  int i;

  /* if there are multiple superclasses, we just need the relevant one */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    supername = data->SuperClasses[i];
    if (vtkWrap_IsClassWrapped(hinfo, supername))
    {
      if (vtkWrap_IsVTKObjectBaseType(hinfo, data->Name))
      {
        /* if class derived from vtkObjectBase, then only accept a
           superclass that is also a vtkObjectBase */
        if (vtkWrap_IsVTKObjectBaseType(hinfo, supername))
        {
          break;
        }
      }
      else
      {
        break;
      }
    }

    supername = NULL;
  }

  if (supermodule)
  {
    *supermodule = NULL;

    if (hinfo && supername)
    {
      /* get superclass module and check against our own */
      entry = vtkParseHierarchy_FindEntry(hinfo, data->Name);
      if (entry)
      {
        module = entry->Module;
      }
      entry = vtkParseHierarchy_FindEntry(hinfo, supername);
      if (entry && (!module || strcmp(entry->Module, module) != 0))
      {
        *supermodule = entry->Module;
      }
    }
  }

  return supername;
}

/* -------------------------------------------------------------------- */
/* Create the docstring for a class, and print it to fp */
void vtkWrapPython_ClassDoc(
  FILE* fp, FileInfo* file_info, ClassInfo* data, HierarchyInfo* hinfo, int is_vtkobject)
{
  char pythonname[1024];
  const char* supername;
  char* cp;
  const char* ccp = NULL;
  size_t i, n;
  size_t briefmax = 255;
  int j, m;
  char temp[500];
  char* comment;

  /* for special objects, add constructor signatures to the doc */
  /* XXX only include constructors that are wrapped */
  /* XXX exclude constructors that are type-preceded */
  /* XXX use python-style signatures with annotations */
  if (!is_vtkobject && !data->Template && !data->IsAbstract)
  {
    m = 0;
    for (j = 0; j < data->NumberOfFunctions; j++)
    {
      if (vtkWrapPython_MethodCheck(data, data->Functions[j], hinfo) &&
        vtkWrap_IsConstructor(data, data->Functions[j]))
      {
        m++;
        fprintf(fp, "\n  \"%s\\n\"",
          vtkWrapText_FormatSignature(data->Functions[j]->Signature, 70, 2000));
      }
    }
    if (m > 0)
    {
      fprintf(fp, "\"\\n\"\n");
    }
  }

  if (data == file_info->MainClass && file_info->NameComment)
  {
    /* use the old VTK-style class description */
    fprintf(fp, "  \"%s\\n\"\n",
      vtkWrapText_QuoteString(vtkWrapText_FormatComment(file_info->NameComment, 70), 500));
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
        ((ccp[2] >= 'A' && ccp[2] <= 'Z') || ccp[2] == '@' || ccp[2] == '\\'))
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
    fprintf(fp, "  \"%s\\n\"\n", vtkWrapText_QuoteString(vtkWrapText_FormatComment(temp, 70), 500));
  }
  else
  {
    fprintf(
      fp, "  \"%s - no description provided.\\n\\n\"\n", vtkWrapText_QuoteString(data->Name, 500));
  }

  /* only consider superclasses that are wrapped */
  supername = vtkWrapPython_GetSuperClass(data, hinfo, NULL);
  if (supername)
  {
    vtkWrapPython_PyTemplateName(supername, pythonname);
    fprintf(fp, "  \"Superclass: %s\\n\\n\"\n", vtkWrapText_QuoteString(pythonname, 500));
  }

  if (data == file_info->MainClass &&
    (file_info->Description || file_info->Caveats || file_info->SeeAlso))
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

    comment = (char*)malloc(n);
    cp = comment;
    *cp = '\0';
    size_t written = 0;

    if (file_info->Description)
    {
      written += snprintf(cp + written, n - written, "%s\n\n", file_info->Description);
    }

    if (file_info->Caveats)
    {
      written +=
        snprintf(cp + written, n - written, ".SECTION Caveats\n\n%s\n\n", file_info->Caveats);
    }

    if (file_info->SeeAlso)
    {
      written +=
        snprintf(cp + written, n - written, ".SECTION See Also\n\n%s\n\n", file_info->SeeAlso);
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
    i = 0;
    while (ccp[i] != '\0')
    {
      n = i;
      /* skip forward until newline */
      while (ccp[i] != '\0' && ccp[i] != '\n' && i - n < 400)
      {
        i++;
      }
      /* skip over consecutive newlines */
      while (ccp[i] == '\n' && i - n < 400)
      {
        i++;
      }

      strncpy(temp, &ccp[n], i - n);
      temp[i - n] = '\0';
      fprintf(
        fp, "  \"%s%s", vtkWrapText_QuoteString(temp, 500), ccp[i] == '\0' ? "\\n\"" : "\"\n");
    }
  }
}

/* -------------------------------------------------------------------- */
/* Declare the exports and imports for a VTK/Python class */
static void vtkWrapPython_ExportVTKClass(FILE* fp, ClassInfo* data, HierarchyInfo* hinfo)
{
  char classname[1024];
  const char* supername;
  const char* supermodule;

  /* mangle the classname if necessary */
  vtkWrapText_PythonName(data->Name, classname);

  /* for vtkObjectBase objects: export New method for use by subclasses */
  fprintf(fp, "extern \"C\" { PyObject *Py%s_ClassNew(); }\n\n", classname);

  /* declare ClassNew method for superclass, if it is in the same module */
  supername = vtkWrapPython_GetSuperClass(data, hinfo, &supermodule);
  if (supername && !supermodule)
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
  FILE* fp, const char* classname, ClassInfo* data, HierarchyInfo* hinfo, int class_has_new)
{
  char superclassname[1024];
  const char* name;
  const char* supermodule;
  int has_constants = 0;
  int has_enums = 0;
  int i;

  if (class_has_new)
  {
    fprintf(fp,
      "static vtkObjectBase *Py%s_StaticNew()\n"
      "{\n",
      classname);

    if (data->IsDeprecated)
    {
      vtkWrapPython_DeprecationWarning(
        fp, "class", data->Name, data->DeprecatedReason, data->DeprecatedVersion);
    }

    fprintf(fp,
      "  return %s::New();\n"
      "}\n"
      "\n",
      data->Name);
  }

  fprintf(fp,
    "PyObject *Py%s_ClassNew()\n"
    "{\n"
    "  PyTypeObject *pytype = PyVTKClass_Add(\n"
    "    &Py%s_Type, Py%s_Methods,\n",
    classname, classname, classname);

  if (strcmp(data->Name, classname) == 0)
  {
    fprintf(fp, "    \"%s\",\n", classname);
  }
  else
  {
    /* use of typeid() matches vtkSetGet ClassName for templated types */
    fprintf(fp, "    typeid(%s).name(),\n", data->Name);
  }

  if (class_has_new)
  {
    fprintf(fp, " &Py%s_StaticNew);\n\n", classname);
  }
  else
  {
    fprintf(fp, " nullptr);\n\n");
  }

  /* if type is already ready, then return */
  fprintf(fp,
    "  if ((pytype->tp_flags & Py_TPFLAGS_READY) != 0)\n"
    "  {\n"
    "    return (PyObject *)pytype;\n"
    "  }\n\n");

  /* add any flags specific to this type */
  fprintf(fp,
    "#ifndef VTK_PY3K\n"
    "  pytype->tp_flags |= Py_TPFLAGS_HAVE_NEWBUFFER;\n"
    "#endif\n\n");

  /* find the first superclass that is a VTK class, create it first */
  name = vtkWrapPython_GetSuperClass(data, hinfo, &supermodule);
  if (name)
  {
    vtkWrapText_PythonName(name, superclassname);
    if (!supermodule) /* superclass is in the same module */
    {
      fprintf(fp, "  pytype->tp_base = (PyTypeObject *)Py%s_ClassNew();\n\n", superclassname);
    }
    else /* superclass is in a different module */
    {
      if (strcmp(name, superclassname) == 0)
      {
        fprintf(
          fp, "  pytype->tp_base = vtkPythonUtil::FindBaseTypeObject(\"%s\");\n\n", superclassname);
      }
      else /* this occurs if superclass is templated */
      {
        /* use of typeid() matches vtkSetGet ClassName for templated types */
        fprintf(fp, "  pytype->tp_base = vtkPythonUtil::FindBaseTypeObject(typeid(%s).name());\n\n",
          name);
      }
    }
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

  /* check if any enums need to be added to the class dict */
  for (i = 0; i < data->NumberOfEnums; i++)
  {
    if (data->Enums[i]->Access == VTK_ACCESS_PUBLIC)
    {
      has_enums = 1;
      break;
    }
  }

  if (has_constants || has_enums)
  {
    fprintf(fp,
      "  PyObject *d = pytype->tp_dict;\n"
      "  PyObject *o;\n"
      "\n");
  }

  if (has_enums)
  {
    /* add any enum types defined in the class to its dict */
    vtkWrapPython_AddPublicEnumTypes(fp, "  ", "d", "o", data);
  }

  if (has_constants)
  {
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
void vtkWrapPython_GenerateObjectType(FILE* fp, const char* module, const char* classname)
{
  /* Generate the TypeObject */
  fprintf(fp,
    "#ifdef VTK_PYTHON_NEEDS_DEPRECATION_WARNING_SUPPRESSION\n"
    "#pragma GCC diagnostic ignored \"-Wdeprecated-declarations\"\n"
    "#endif\n"
    "\n"
    "static PyTypeObject Py%s_Type = {\n"
    "  PyVarObject_HEAD_INIT(&PyType_Type, 0)\n"
    "  PYTHON_PACKAGE_SCOPE \"%s.%s\", // tp_name\n"
    "  sizeof(PyVTKObject), // tp_basicsize\n"
    "  0, // tp_itemsize\n"
    "  PyVTKObject_Delete, // tp_dealloc\n"
    "#if PY_VERSION_HEX >= 0x03080000\n"
    "  0, // tp_vectorcall_offset\n"
    "#else\n"
    "  nullptr, // tp_print\n"
    "#endif\n"
    "  nullptr, // tp_getattr\n"
    "  nullptr, // tp_setattr\n"
    "  nullptr, // tp_compare\n"
    "  PyVTKObject_Repr, // tp_repr\n",
    classname, module, classname);

  fprintf(fp,
    "  nullptr, // tp_as_number\n"
    "  nullptr, // tp_as_sequence\n"
    "  nullptr, // tp_as_mapping\n"
    "  nullptr, // tp_hash\n"
    "  nullptr, // tp_call\n"
    "  PyVTKObject_String, // tp_str\n");

  fprintf(fp,
    "  PyObject_GenericGetAttr, // tp_getattro\n"
    "  PyObject_GenericSetAttr, // tp_setattro\n"
    "  &PyVTKObject_AsBuffer, // tp_as_buffer\n"
    "  Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,"
    " // tp_flags\n"
    "  Py%s_Doc, // tp_doc\n"
    "  PyVTKObject_Traverse, // tp_traverse\n"
    "  nullptr, // tp_clear\n"
    "  nullptr, // tp_richcompare\n"
    "  offsetof(PyVTKObject, vtk_weakreflist), // tp_weaklistoffset\n",
    classname);

  if (strcmp(classname, "vtkCollection") == 0)
  {
    fprintf(fp,
      "  PyvtkCollection_Iter, // tp_iter\n"
      "  nullptr, // tp_iternext\n");
  }
  else
  {
    if (strcmp(classname, "vtkCollectionIterator") == 0)
    {
      fprintf(fp,
        "  PyvtkCollectionIterator_Iter, // tp_iter\n"
        "  PyvtkCollectionIterator_Next, // tp_iternext\n");
    }
    else
    {
      fprintf(fp,
        "  nullptr, // tp_iter\n"
        "  nullptr, // tp_iternext\n");
    }
  }
  fprintf(fp,
    "  nullptr, // tp_methods\n"
    "  nullptr, // tp_members\n"
    "  PyVTKObject_GetSet, // tp_getset\n"
    "  nullptr, // tp_base\n"
    "  nullptr, // tp_dict\n"
    "  nullptr, // tp_descr_get\n"
    "  nullptr, // tp_descr_set\n"
    "  offsetof(PyVTKObject, vtk_dict), // tp_dictoffset\n"
    "  nullptr, // tp_init\n"
    "  nullptr, // tp_alloc\n"
    "  PyVTKObject_New, // tp_new\n"
    "  PyObject_GC_Del, // tp_free\n"
    "  nullptr, // tp_is_gc\n");

  /* fields set by python itself */
  fprintf(fp,
    "  nullptr, // tp_bases\n"
    "  nullptr, // tp_mro\n"
    "  nullptr, // tp_cache\n"
    "  nullptr, // tp_subclasses\n"
    "  nullptr, // tp_weaklist\n");

  /* internal struct members */
  fprintf(fp,
    "  VTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED\n"
    "};\n"
    "\n");
}

/* -------------------------------------------------------------------- */
/* Wrap one class */
int vtkWrapPython_WrapOneClass(FILE* fp, const char* module, const char* classname, ClassInfo* data,
  FileInfo* file_info, HierarchyInfo* hinfo, int is_vtkobject)
{
  int class_has_new = 0;
  int i;

  /* recursive handling of templated classes */
  if (data->Template)
  {
    return vtkWrapPython_WrapTemplatedClass(fp, data, file_info, hinfo);
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

  /* check for New() function */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    FunctionInfo* func = data->Functions[i];

    if (func->Name && !func->IsExcluded && func->Access == VTK_ACCESS_PUBLIC &&
      strcmp("New", func->Name) == 0 && func->NumberOfParameters == 0 &&
      !vtkWrap_IsInheritedMethod(data, func))
    {
      class_has_new = 1;
    }
  }

  /* create any enum types defined in the class */
  for (i = 0; i < data->NumberOfEnums; i++)
  {
    if (!data->Enums[i]->IsExcluded && data->Enums[i]->Access == VTK_ACCESS_PUBLIC)
    {
      vtkWrapPython_GenerateEnumType(fp, module, classname, data->Enums[i]);
    }
  }

  /* now output all the methods are wrappable */
  vtkWrapPython_GenerateMethods(fp, classname, data, file_info, hinfo, is_vtkobject, 0);

  /* output the class initialization function for VTK objects */
  if (is_vtkobject)
  {
    /* the docstring for the class, as a static var ending in "Doc" */
    fprintf(fp, "static const char *Py%s_Doc =\n", classname);
    vtkWrapPython_ClassDoc(fp, file_info, data, hinfo, is_vtkobject);
    fprintf(fp, ";\n\n");

    vtkWrapPython_GenerateObjectType(fp, module, classname);
    vtkWrapPython_GenerateObjectNew(fp, classname, data, hinfo, class_has_new);
  }

  /* output the class initialization function for special objects */
  else
  {
    vtkWrapPython_GenerateSpecialType(fp, module, classname, data, file_info, hinfo);
  }

  return 1;
}
