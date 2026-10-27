// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWrapPythonClass.h"
#include "vtkParseProperties.h"
#include "vtkWrapPythonConstant.h"
#include "vtkWrapPythonEnum.h"
#include "vtkWrapPythonMethod.h"
#include "vtkWrapPythonMethodDef.h"
#include "vtkWrapPythonNumberProtocol.h"
#include "vtkWrapPythonProperty.h"
#include "vtkWrapPythonTemplate.h"
#include "vtkWrapPythonType.h"

#include "vtkParseExtras.h"
#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* declare the exports and imports for a VTK/Python class */
static void vtkWrapPython_ExportVTKClass(
  FILE* fp, const ClassInfo* data, FileInfo* file_info, const HierarchyInfo* hinfo);

/* generate the New method for a vtkObjectBase object */
static void vtkWrapPython_GenerateObjectNew(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* file_info, const HierarchyInfo* hinfo, int class_has_new);

/* -------------------------------------------------------------------- */
/* get the true superclass */
const char* vtkWrapPython_GetSuperClass(
  const ClassInfo* data, FileInfo* file_info, const HierarchyInfo* hinfo, const char** supermodule)
{
  const char* supername = NULL;
  const char* module = NULL;
  const HierarchyEntry* entry;
  int i;

  /* if there are multiple superclasses, we just need the relevant one */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    /* for templated superclass, ensure args are fully resolved */
    supername = vtkParseHierarchy_ExpandTypedefsInTemplateArgs(
      hinfo, data->SuperClasses[i], file_info->Strings, NULL);

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
  FILE* fp, FileInfo* file_info, const ClassInfo* data, const HierarchyInfo* hinfo)
{
  char pythonname[1024];
  const char* supername;
  char* cp;
  const char* ccp = NULL;
  size_t i, n;
  size_t briefmax = 255;
  char temp[500];
  char* comment;

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
  supername = vtkWrapPython_GetSuperClass(data, file_info, hinfo, NULL);
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
      (void)written;
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
    if (data->Name && strcmp(data->Name, "vtkAlgorithm") == 0)
    {
      fprintf(fp, "\n  \"vtkAlgorithm defines two additional methods in Python: \\n\\n\"\n");
      fprintf(
        fp, "  \"update(): This method updates the pipeline connected to this algorithm\\n\"\n");
      fprintf(fp, "  \"and returns an Output object with an output property. This property\\n\"\n");
      fprintf(
        fp, "  \"provides either a single data object (for algorithms with single output\\n\"\n");
      fprintf(fp, "  \"or a tuple (for algorithms with multiple outputs).\\n\\n\"\n");
      fprintf(
        fp, "  \"__call__() (or just ()): This method takes a data object as input (or\\n\"\n");
      fprintf(fp, "  \"a tuple for repeatable inputs for algorithms such as append)\\n\"\n");
      fprintf(fp, "  \"and returns the output the same way as update()\\n\\n\"\n");
      fprintf(fp, "  \"vtkAlgorithm also implements the >> operator which can be used to\\n\"\n");
      fprintf(
        fp, "  \"connect algorithms to form pipelines. The >> operator returns a Pipeline\\n\"\n");
      fprintf(
        fp, "  \"object which can be used to execute the pipeline with the update() and\\n\"\n");
      fprintf(fp, "  \"__call__() methods.\\n\"");
    }
  }
}

/* -------------------------------------------------------------------- */
/* Declare the exports and imports for a VTK/Python class */
static void vtkWrapPython_ExportVTKClass(
  FILE* fp, const ClassInfo* data, FileInfo* file_info, const HierarchyInfo* hinfo)
{
  char classname[1024];
  const char* supername;
  const char* supermodule;

  /* mangle the classname if necessary */
  vtkWrapText_PythonName(data->Name, classname);

  /* for vtkObjectBase objects: export New method for use by subclasses */
  fprintf(fp, "extern \"C\" { %s PyObject *Py%s_ClassNew(); }\n\n", "VTK_ABI_HIDDEN", classname);

  /* declare ClassNew method for superclass, if it is in the same module */
  supername = vtkWrapPython_GetSuperClass(data, file_info, hinfo, &supermodule);
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
static void vtkWrapPython_GenerateObjectNew(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* file_info, const HierarchyInfo* hinfo, int class_has_new)
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
    "  static PyTypeObject *pytype = nullptr;\n"
    "  if (pytype != nullptr)\n"
    "  {\n"
    "    return (PyObject *)pytype;\n"
    "  }\n"
    "\n"
    "  PyObject *base = nullptr;\n",
    classname);

  /* find the first superclass that is a VTK class, create it first; it becomes
   * the base of this heap type (the whole hierarchy must be heap types) */
  name = vtkWrapPython_GetSuperClass(data, file_info, hinfo, &supermodule);
  if (name)
  {
    vtkWrapText_PythonName(name, superclassname);
    if (!supermodule) /* superclass is in the same module */
    {
      fprintf(fp, "  base = Py%s_ClassNew();\n", superclassname);
    }
    else /* superclass is in a different module */
    {
      if (strcmp(name, superclassname) == 0)
      {
        fprintf(
          fp, "  base = (PyObject *)vtkPythonUtil::FindBaseTypeObject(\"%s\");\n", superclassname);
      }
      else /* this occurs if superclass is templated */
      {
        /* use of typeid() matches vtkSetGet ClassName for templated types */
        fprintf(
          fp, "  base = (PyObject *)vtkPythonUtil::FindBaseTypeObject(typeid(%s).name());\n", name);
      }
    }
  }

  fprintf(fp,
    "\n"
    "  pytype = PyVTKClass_AddHeapType(\n"
    "    &Py%s_Spec, base, Py%s_Methods,\n",
    classname, classname);

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
    fprintf(fp, "    &Py%s_StaticNew);\n", classname);
  }
  else
  {
    fprintf(fp, "    nullptr);\n");
  }

  fprintf(fp,
    "  if (pytype == nullptr)\n"
    "  {\n"
    "    return nullptr;\n"
    "  }\n\n");

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
      "  PyObject *d = (PyObject *)pytype;\n"
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
    "  PyVTKClass_AddCombinedGetSetDefinitions(pytype, Py%s_GetSets);\n"
    "  PyType_Modified(pytype);\n"
    "  return (PyObject *)pytype;\n"
    "}\n\n",
    classname);
}

/* -------------------------------------------------------------------- */
/* write out the type object */
void vtkWrapPython_GenerateObjectType(
  FILE* fp, const char* module, const char* classname, const int hasNumberProtocol)
{
  /* Generate a PyType_Spec and slot table, used by PyVTKClass_AddHeapType. */
  fprintf(fp,
    "static PyType_Slot Py%s_Slots[] = {\n"
    "  { Py_tp_dealloc, (void *)PyVTKObject_Delete },\n"
    "  { Py_tp_repr, (void *)PyVTKObject_Repr },\n"
    "  { Py_tp_str, (void *)PyVTKObject_String },\n"
    "  { Py_tp_getattro, (void *)PyObject_GenericGetAttr },\n"
    "  { Py_tp_setattro, (void *)PyObject_GenericSetAttr },\n"
    "  { Py_tp_doc, (void *)Py%s_Doc },\n"
    "  { Py_tp_traverse, (void *)PyVTKObject_Traverse },\n"
    "  { Py_tp_getset, (void *)PyVTKObject_GetSet },\n"
    "  { Py_tp_members, (void *)PyVTKObject_Members },\n"
    "  { Py_tp_init, (void *)PyVTKObject_Init },\n"
    "  { Py_tp_new, (void *)PyVTKObject_New },\n"
    "  { Py_bf_getbuffer, (void *)PyVTKObject_GetBuffer },\n"
    "  { Py_bf_releasebuffer, (void *)PyVTKObject_ReleaseBuffer },\n",
    classname, classname);

  if (hasNumberProtocol)
  {
    /* only nb_rshift (the ">>" pipeline operator) is implemented */
    fprintf(fp, "  { Py_nb_rshift, (void *)Py%s_RShift },\n", classname);
  }
  if (strcmp(classname, "vtkAlgorithm") == 0)
  {
    fprintf(fp, "  { Py_tp_call, (void *)PyvtkAlgorithm_Call },\n");
  }
  if (strcmp(classname, "vtkCollection") == 0)
  {
    fprintf(fp,
      "  { Py_sq_length, (void *)PyVTKObject_AsSequence_Length },\n"
      "  { Py_sq_item, (void *)PyVTKObject_AsSequence_GetItem },\n"
      "  { Py_sq_contains, (void *)PyVTKObject_AsSequence_Contains },\n"
      "  { Py_tp_iter, (void *)PyvtkCollection_Iter },\n");
  }
  else if (strcmp(classname, "vtkCollectionIterator") == 0)
  {
    fprintf(fp,
      "  { Py_tp_iter, (void *)PyvtkCollectionIterator_Iter },\n"
      "  { Py_tp_iternext, (void *)PyvtkCollectionIterator_Next },\n");
  }

  fprintf(fp,
    "  { 0, nullptr }\n"
    "};\n"
    "\n"
    "static PyType_Spec Py%s_Spec = {\n"
    "  PYTHON_PACKAGE_SCOPE \"%s.%s\", // tp_name\n"
    "  sizeof(PyVTKObject), // tp_basicsize\n"
    "  0, // tp_itemsize\n"
    "  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE, // tp_flags\n"
    "  Py%s_Slots\n"
    "};\n"
    "\n",
    classname, module, classname, classname);
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
    vtkWrapPython_ExportVTKClass(fp, data, file_info, hinfo);
  }

  /* check for New() function */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    const FunctionInfo* func = data->Functions[i];

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

  /* The call to generate methods below erases some occurrences, so parse all properties before
   * methods are generated */
  ClassProperties* properties = vtkParseProperties_Create(data, hinfo);

  /* now output all the methods are wrappable */
  vtkWrapPython_GenerateMethods(fp, classname, data, file_info, hinfo, is_vtkobject, 0);

  /* now output number protocol definitions where acceptable */
  int hasNumberProtocol = vtkWrapPython_GenerateNumberProtocolDefintions(fp, data);

  /* now output all the property getters and setters */
  vtkWrapPython_GenerateProperties(fp, classname, data, hinfo, properties);

  /* Free properties */
  vtkParseProperties_Free(properties);

  /* output the class initialization function for VTK objects */
  if (is_vtkobject)
  {
    /* the docstring for the class, as a static var ending in "Doc" */
    fprintf(fp, "static const char *Py%s_Doc =\n", classname);
    vtkWrapPython_ClassDoc(fp, file_info, data, hinfo);
    fprintf(fp, ";\n\n");

    vtkWrapPython_GenerateObjectType(fp, module, classname, hasNumberProtocol);
    vtkWrapPython_GenerateObjectNew(fp, classname, data, file_info, hinfo, class_has_new);
  }

  /* output the class initialization function for special objects */
  else
  {
    vtkWrapPython_GenerateSpecialType(fp, module, classname, data, file_info, hinfo);
  }

  return 1;
}

// NOLINTEND(bugprone-unsafe-functions)
