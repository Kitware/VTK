// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWrapPythonType.h"
#include "vtkWrapPythonClass.h"
#include "vtkWrapPythonConstant.h"
#include "vtkWrapPythonEnum.h"
#include "vtkWrapPythonMethod.h"
#include "vtkWrapPythonMethodDef.h"
#include "vtkWrapPythonTemplate.h"

#include "vtkParseExtras.h"
#include "vtkWrap.h"
#include "vtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

/* -------------------------------------------------------------------- */
/* A struct for special types to store info about the type, it is fairly
 * small because not many operators or special features are wrapped */
typedef struct
{
  int has_print;    /* there is "<<" stream operator */
  int has_compare;  /* there are comparison operators e.g. "<" */
  int has_sequence; /* the [] operator takes a single integer */
  int has_setitem;  /* the [] operator can also be assigned to */
  int has_number;   /* there are arithmetic operators e.g. "+" */
  int number_ops;   /* bitmask of the VTK_PYTHON_OP_* flags */
} SpecialTypeInfo;

/* bit flags for SpecialTypeInfo::number_ops */
enum
{
  VTK_PYTHON_OP_ADD = 1,
  VTK_PYTHON_OP_SUBTRACT = 2,
  VTK_PYTHON_OP_MULTIPLY = 4,
  VTK_PYTHON_OP_DIVIDE = 8,
  VTK_PYTHON_OP_NEGATIVE = 16
};

/* -------------------------------------------------------------------- */
/* The following functions are for generating code for special types,
 * i.e. types that are not derived from vtkObjectBase */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/* check if class has a wrapped constructor, and return its name if so */
static const char* vtkWrapPython_WrappedConstructor(
  ClassInfo* data, const HierarchyInfo* hinfo, size_t* np)
{
  const char* constructor = data->Name;
  size_t n, m;
  int i;

  /* remove namespaces and template parameters from the
   * class name to get the constructor name */
  m = vtkParse_UnscopedNameLength(constructor);
  while (constructor[m] == ':' && constructor[m + 1] == ':')
  {
    constructor += m + 2;
    m = vtkParse_UnscopedNameLength(constructor);
  }
  for (n = 0; n < m; n++)
  {
    if (constructor[n] == '<')
    {
      break;
    }
  }

  /* check if a public constructor exists */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    const FunctionInfo* theFunc = data->Functions[i];

    if (theFunc->Name && strncmp(theFunc->Name, constructor, n) == 0 && theFunc->Name[n] == '\0' &&
      !theFunc->Template && vtkWrapPython_MethodCheck(data, theFunc, hinfo))
    {
      *np = n;
      return constructor;
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------- */
/* generate function for printing a special object */
static void vtkWrapPython_NewDeleteProtocol(
  FILE* fp, const char* classname, ClassInfo* data, const HierarchyInfo* hinfo)
{
  size_t n = 0;
  const char* constructor = NULL;

  if (!data->IsAbstract)
  {
    constructor = vtkWrapPython_WrappedConstructor(data, hinfo, &n);
  }

  /* the "new" method */
  if (constructor)
  {
    fprintf(fp,
      "static PyObject *\n"
      "Py%s_New(PyTypeObject *, PyObject *args, PyObject *kwds)\n"
      "{\n"
      "  if (kwds && PyDict_Size(kwds))\n"
      "  {\n"
      "    PyErr_SetString(PyExc_TypeError,\n"
      "                    \"this function takes no keyword arguments\");\n"
      "    return nullptr;\n"
      "  }\n"
      "\n"
      "  return Py%s_%*.*s(nullptr, args);\n"
      "}\n"
      "\n",
      classname, classname, (int)n, (int)n, constructor);
  }
  else
  {
    fprintf(fp,
      "static PyObject *\n"
      "Py%s_New(PyTypeObject * /*unused*/, PyObject * /*unused*/, PyObject * /*unused*/)\n"
      "{\n"
      "  PyErr_SetString(PyExc_TypeError,\n"
      "                  \"this class cannot be instantiated\");\n"
      "\n"
      "  return nullptr;\n"
      "}\n"
      "\n",
      classname);
  }

  /* the delete method */
  if (vtkWrap_HasPublicDestructor(data))
  {
    fprintf(fp,
      "static void Py%s_Delete(PyObject *self)\n"
      "{\n"
      "  // Instances of a heap type hold a reference to their type (taken by\n"
      "  // PyObject_GC_New); release it after the instance is freed.\n"
      "  PyTypeObject *tp = Py_TYPE(self);\n"
      "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
      "  delete static_cast<%s *>(obj->vtk_ptr);\n"
      "  PyObject_Del(self);\n"
      "  Py_DECREF((PyObject *)tp);\n"
      "}\n"
      "\n",
      classname, data->Name);
  }
  else
  {
    fprintf(fp,
      "static void Py%s_Delete(PyObject *self)\n"
      "{\n"
      "  PyTypeObject *tp = Py_TYPE(self);\n"
      "  PyObject_Del(self);\n"
      "  Py_DECREF((PyObject *)tp);\n"
      "}\n"
      "\n",
      classname);
  }
}

/* -------------------------------------------------------------------- */
/* generate function for printing a special object */
static void vtkWrapPython_PrintProtocol(
  FILE* fp, const char* classname, const ClassInfo* data, FileInfo* finfo, SpecialTypeInfo* info)
{
  int i;
  const FunctionInfo* func;

  /* look in the file for "operator<<" for printing */
  for (i = 0; i < finfo->Contents->NumberOfFunctions; i++)
  {
    func = finfo->Contents->Functions[i];
    if (func->Name && func->IsOperator && !func->IsDeleted && !func->IsExcluded &&
      strcmp(func->Name, "operator<<") == 0)
    {
      if (func->NumberOfParameters == 2 &&
        (func->Parameters[0]->Type & VTK_PARSE_UNQUALIFIED_TYPE) == VTK_PARSE_OSTREAM_REF &&
        vtkWrap_IsSpecialObject(func->Parameters[1]) &&
        !vtkWrap_IsNonConstRef(func->Parameters[1]) &&
        strcmp(func->Parameters[1]->Class, data->Name) == 0)
      {
        info->has_print = 1;
      }
    }
  }

  /* the str function */
  if (info->has_print)
  {
    fprintf(fp,
      "static PyObject *Py%s_String(PyObject *self)\n"
      "{\n"
      "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
      "  std::ostringstream os;\n"
      "  if (obj->vtk_ptr)\n"
      "  {\n"
      "    os << *static_cast<const %s *>(obj->vtk_ptr);\n"
      "  }\n"
      "  const std::string &s = os.str();\n"
      "  return PyUnicode_FromStringAndSize(s.data(), s.size());\n"
      "}\n"
      "\n",
      classname, data->Name);
  }
}

/* -------------------------------------------------------------------- */
/* Check if type is a vtkTuple derivative (has sequence-like [] access) */
static int vtkWrapPython_IsTupleType(ClassInfo* data, const HierarchyInfo* hinfo)
{
  return vtkWrap_IsTypeOf(hinfo, data->Name, "vtkTuple");
}

/* -------------------------------------------------------------------- */
/* generate function for comparing special objects */
static void vtkWrapPython_RichCompareProtocol(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, const HierarchyInfo* hinfo, SpecialTypeInfo* info)
{
  static const char* compare_consts[6] = { "Py_LT", "Py_LE", "Py_EQ", "Py_NE", "Py_GT", "Py_GE" };
  static const char* compare_tokens[6] = { "<", "<=", "==", "!=", ">", ">=" };
  int compare_ops = 0;
  int i, n;
  int is_tuple = 0;
  const FunctionInfo* func;

  /* look for comparison operator methods */
  n = data->NumberOfFunctions + finfo->Contents->NumberOfFunctions;
  for (i = 0; i < n; i++)
  {
    if (i < data->NumberOfFunctions)
    {
      /* member function */
      func = data->Functions[i];
      if (func->NumberOfParameters != 1 || !vtkWrap_IsSpecialObject(func->Parameters[0]) ||
        strcmp(func->Parameters[0]->Class, data->Name) != 0)
      {
        continue;
      }
    }
    else
    {
      /* non-member function: both args must be of our type */
      func = finfo->Contents->Functions[i - data->NumberOfFunctions];
      if (func->NumberOfParameters != 2 || !vtkWrap_IsSpecialObject(func->Parameters[0]) ||
        strcmp(func->Parameters[0]->Class, data->Name) != 0 ||
        !vtkWrap_IsSpecialObject(func->Parameters[1]) ||
        strcmp(func->Parameters[1]->Class, data->Name) != 0)
      {
        continue;
      }
    }
    if (func->IsOperator && func->Name != NULL && !func->IsDeleted && !func->IsExcluded)
    {
      if (strcmp(func->Name, "operator<") == 0)
      {
        compare_ops = (compare_ops | (1 << 0));
      }
      else if (strcmp(func->Name, "operator<=") == 0)
      {
        compare_ops = (compare_ops | (1 << 1));
      }
      else if (strcmp(func->Name, "operator==") == 0)
      {
        compare_ops = (compare_ops | (1 << 2));
      }
      else if (strcmp(func->Name, "operator!=") == 0)
      {
        compare_ops = (compare_ops | (1 << 3));
      }
      else if (strcmp(func->Name, "operator>") == 0)
      {
        compare_ops = (compare_ops | (1 << 4));
      }
      else if (strcmp(func->Name, "operator>=") == 0)
      {
        compare_ops = (compare_ops | (1 << 5));
      }
    }
  }

  /* If the type is a vtkTuple derivative, generate == and != via
   * element-wise comparison at the Python level.  This handles types
   * where the template operator== from vtkTuple.h is not visible
   * to the wrapper. */
  is_tuple = vtkWrapPython_IsTupleType(data, hinfo);
  if (is_tuple && !(compare_ops & (1 << 2)))
  {
    /* add EQ and NE if not already found */
    compare_ops |= ((1 << 2) | (1 << 3));
  }

  /* the compare function */
  if (compare_ops != 0)
  {
    info->has_compare = 1;

    fprintf(fp, "static int Py%s_Check(PyObject *ob);\n\n", classname);

    fprintf(fp,
      "static PyObject *Py%s_RichCompare(\n"
      "  PyObject *o1, PyObject *o2, int opid)\n"
      "{\n",
      classname);

    /* For EQ/NE using sequence protocol, we work at the Python level */
    if (is_tuple && !(compare_ops & ~((1 << 2) | (1 << 3))))
    {
      /* Only EQ/NE via sequence, no C++ operators at all */
      fprintf(fp,
        "  int result = -1;\n"
        "\n"
        "  if (!Py%s_Check(o1) || !Py%s_Check(o2))\n"
        "  {\n"
        "    Py_INCREF(Py_NotImplemented);\n"
        "    return Py_NotImplemented;\n"
        "  }\n"
        "\n"
        "  if (opid == Py_EQ || opid == Py_NE)\n"
        "  {\n"
        "    Py_ssize_t n = PySequence_Size(o1);\n"
        "    Py_ssize_t m = PySequence_Size(o2);\n"
        "    result = 1; // assume equal\n"
        "    if (n != m) { result = 0; }\n"
        "    else\n"
        "    {\n"
        "      for (Py_ssize_t i = 0; i < n; i++)\n"
        "      {\n"
        "        PyObject *v1 = PySequence_GetItem(o1, i);\n"
        "        PyObject *v2 = PySequence_GetItem(o2, i);\n"
        "        int eq = PyObject_RichCompareBool(v1, v2, Py_EQ);\n"
        "        Py_DECREF(v1);\n"
        "        Py_DECREF(v2);\n"
        "        if (eq < 0) { return nullptr; }\n"
        "        if (!eq) { result = 0; break; }\n"
        "      }\n"
        "    }\n"
        "    if (opid == Py_NE) { result = !result; }\n"
        "  }\n",
        classname, classname);
    }
    else
    {
      /* Has C++ operators: use the existing pointer-based approach */
      fprintf(fp,
        "  PyObject *n1 = nullptr;\n"
        "  PyObject *n2 = nullptr;\n"
        "  const %s *so1 = nullptr;\n"
        "  const %s *so2 = nullptr;\n"
        "  int result = -1;\n"
        "\n",
        data->Name, data->Name);

      for (i = 1; i <= 2; i++)
      {
        fprintf(fp,
          "  if (Py%s_Check(o%d))\n"
          "  {\n"
          "    PyVTKSpecialObject *s%d = (PyVTKSpecialObject *)o%d;\n"
          "    so%d = static_cast<const %s *>(s%d->vtk_ptr);\n"
          "  }\n"
          "  else\n"
          "  {\n"
          "    so%d = static_cast<const %s *>(\n"
          "      vtkPythonUtil::GetPointerFromSpecialObject(\n"
          "        o%d, \"%s\", &n%d));\n"
          "    if (so%d == nullptr)\n"
          "    {\n"
          "      PyErr_Clear();\n"
          "      Py_INCREF(Py_NotImplemented);\n"
          "      return Py_NotImplemented;\n"
          "    }\n"
          "  }\n"
          "\n",
          classname, i, i, i, i, data->Name, i, i, data->Name, i, classname, i, i);
      }

      /* the switch statement for all possible compare ops */
      fprintf(fp,
        "  switch (opid)\n"
        "  {\n");

      for (i = 0; i < 6; i++)
      {
        if (((compare_ops >> i) & 1) != 0)
        {
          if (is_tuple && (i == 2 || i == 3))
          {
            /* Use sequence-based EQ/NE for consistency */
            if (i == 2)
            {
              fprintf(fp,
                "    case Py_EQ:\n"
                "    {\n"
                "      result = 1;\n"
                "      for (int j = 0; j < so1->GetSize(); j++)\n"
                "      {\n"
                "        if ((*so1)[j] != (*so2)[j]) { result = 0; break; }\n"
                "      }\n"
                "      break;\n"
                "    }\n");
            }
            else
            {
              fprintf(fp,
                "    case Py_NE:\n"
                "    {\n"
                "      result = 0;\n"
                "      for (int j = 0; j < so1->GetSize(); j++)\n"
                "      {\n"
                "        if ((*so1)[j] != (*so2)[j]) { result = 1; break; }\n"
                "      }\n"
                "      break;\n"
                "    }\n");
            }
          }
          else
          {
            fprintf(fp,
              "    case %s:\n"
              "      result = ((*so1) %s (*so2));\n"
              "      break;\n",
              compare_consts[i], compare_tokens[i]);
          }
        }
        else
        {
          fprintf(fp,
            "    case %s:\n"
            "      break;\n",
            compare_consts[i]);
        }
      }

      fprintf(fp,
        "  }\n"
        "\n");

      /* delete temporary objects, there will be at most one */
      fprintf(fp,
        "  if (n1)\n"
        "  {\n"
        "    Py_DECREF(n1);\n"
        "  }\n"
        "  else if (n2)\n"
        "  {\n"
        "    Py_DECREF(n2);\n"
        "  }\n"
        "\n");
    }

    /* return the result */
    fprintf(fp,
      "  if (result == -1)\n"
      "  {\n"
      "    Py_RETURN_NOTIMPLEMENTED;\n"
      "  }\n"
      "\n"
      "  // avoids aliasing issues with Py_INCREF(Py_False)\n"
      "  return PyBool_FromLong((long)result);\n"
      "}\n"
      "\n");
  }
}

/* -------------------------------------------------------------------- */
/* generate functions for indexing into special objects */
static void vtkWrapPython_SequenceProtocol(FILE* fp, const char* classname, ClassInfo* data,
  const HierarchyInfo* hinfo, SpecialTypeInfo* info)
{
  int i;
  FunctionInfo* func;
  const FunctionInfo* getItemFunc = 0;
  const FunctionInfo* setItemFunc = 0;

  /* look for [] operator */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    func = data->Functions[i];

    if (func->Name && func->IsOperator && !func->IsDeleted && !func->IsExcluded &&
      strcmp(func->Name, "operator[]") == 0 && vtkWrapPython_MethodCheck(data, func, hinfo))
    {
      if (func->NumberOfParameters == 1 && func->ReturnValue &&
        vtkWrap_IsInteger(func->Parameters[0]))
      {
        if (!setItemFunc && vtkWrap_IsNonConstRef(func->ReturnValue))
        {
          setItemFunc = func;
        }
        if (!getItemFunc || (func->IsConst && !getItemFunc->IsConst))
        {
          getItemFunc = func;
        }
      }
    }
  }

  if (getItemFunc && getItemFunc->SizeHint)
  {
    info->has_sequence = 1;
    info->has_setitem = (setItemFunc != 0);

    fprintf(fp,
      "static Py_ssize_t Py%s_SequenceSize(PyObject *self)\n"
      "{\n"
      "  void *vp = vtkPythonArgs::GetSelfSpecialPointer(self);\n"
      "  %s *op = static_cast<%s *>(vp);\n"
      "\n"
      "  return static_cast<Py_ssize_t>(op->%s);\n"
      "}\n\n",
      classname, data->Name, data->Name, getItemFunc->SizeHint);

    fprintf(fp,
      "static PyObject *Py%s_SequenceItem(PyObject *self, Py_ssize_t i)\n"
      "{\n"
      "  void *vp = vtkPythonArgs::GetSelfSpecialPointer(self);\n"
      "  %s *op = static_cast<%s *>(vp);\n"
      "\n",
      classname, data->Name, data->Name);

    vtkWrapPython_DeclareVariables(fp, data, getItemFunc);

    fprintf(fp,
      "  temp0 = static_cast<%s>(i);\n"
      "\n"
      "  if (temp0 < 0 || temp0 >= op->%s)\n"
      "  {\n"
      "    PyErr_SetString(PyExc_IndexError, \"index out of range\");\n"
      "  }\n"
      "  else\n"
      "  {\n",
      vtkWrap_GetTypeName(getItemFunc->Parameters[0]), getItemFunc->SizeHint);

    fprintf(fp, "  ");
    vtkWrap_DeclareVariable(
      fp, data, getItemFunc->ReturnValue, "tempr", -1, VTK_WRAP_RETURN | VTK_WRAP_NOSEMI);

    fprintf(fp,
      " = %s(*op)[temp0];\n"
      "\n",
      (vtkWrap_IsRef(getItemFunc->ReturnValue) ? "&" : ""));

    vtkWrapPython_ReturnValue(fp, data, getItemFunc->ReturnValue, 1);

    fprintf(fp,
      "  }\n"
      "\n"
      "  return result;\n"
      "}\n\n");

    if (setItemFunc)
    {
      fprintf(fp,
        "static int Py%s_SequenceSetItem(\n"
        "  PyObject *self, Py_ssize_t i, PyObject *arg1)\n"
        "{\n"
        "  void *vp = vtkPythonArgs::GetSelfSpecialPointer(self);\n"
        "  %s *op = static_cast<%s *>(vp);\n"
        "\n",
        classname, data->Name, data->Name);

      vtkWrap_DeclareVariable(fp, data, setItemFunc->Parameters[0], "temp", 0, VTK_WRAP_ARG);
      vtkWrap_DeclareVariable(fp, data, setItemFunc->ReturnValue, "temp", 1, VTK_WRAP_ARG);

      fprintf(fp,
        "  int result = -1;\n"
        "\n"
        "  temp0 = static_cast<%s>(i);\n"
        "\n"
        "  if (temp0 < 0 || temp0 >= op->%s)\n"
        "  {\n"
        "    PyErr_SetString(PyExc_IndexError, \"index out of range\");\n"
        "  }\n"
        "  else if (",
        vtkWrap_GetTypeName(setItemFunc->Parameters[0]), getItemFunc->SizeHint);

      vtkWrapPython_GetSingleArgument(fp, data, 1, setItemFunc->ReturnValue, 1);

      fprintf(fp,
        ")\n"
        "  {\n"
        "    (*op)[temp0] = %stemp1;\n"
        "\n",
        ((vtkWrap_IsRef(getItemFunc->ReturnValue) && vtkWrap_IsObject(getItemFunc->ReturnValue))
            ? "*"
            : ""));

      fprintf(fp,
        "    if (PyErr_Occurred() == nullptr)\n"
        "    {\n"
        "      result = 0;\n"
        "    }\n"
        "  }\n"
        "\n"
        "  return result;\n"
        "}\n\n");
    }

    /* generate mp_subscript to support slice indexing */
    fprintf(fp,
      "static PyObject *Py%s_MappingSubscript(PyObject *self, PyObject *key)\n"
      "{\n"
      "  void *vp = vtkPythonArgs::GetSelfSpecialPointer(self);\n"
      "  %s *op = static_cast<%s *>(vp);\n"
      "  Py_ssize_t size = static_cast<Py_ssize_t>(op->%s);\n"
      "\n"
      "  if (PyIndex_Check(key))\n"
      "  {\n"
      "    Py_ssize_t i = PyNumber_AsSsize_t(key, PyExc_IndexError);\n"
      "    if (i == -1 && PyErr_Occurred())\n"
      "    {\n"
      "      return nullptr;\n"
      "    }\n"
      "    if (i < 0)\n"
      "    {\n"
      "      i += size;\n"
      "    }\n"
      "    return Py%s_SequenceItem(self, i);\n"
      "  }\n"
      "  else if (PySlice_Check(key))\n"
      "  {\n"
      "    Py_ssize_t start, stop, step, slicelen;\n"
      "    if (PySlice_GetIndicesEx(key, size, &start, &stop, &step, &slicelen) < 0)\n"
      "    {\n"
      "      return nullptr;\n"
      "    }\n"
      "    PyObject *result = PyTuple_New(slicelen);\n"
      "    for (Py_ssize_t j = 0; j < slicelen; j++)\n"
      "    {\n"
      "      PyObject *item = Py%s_SequenceItem(self, start + j * step);\n"
      "      if (item == nullptr)\n"
      "      {\n"
      "        Py_DECREF(result);\n"
      "        return nullptr;\n"
      "      }\n"
      "      PyTuple_SetItem(result, j, item);\n"
      "    }\n"
      "    return result;\n"
      "  }\n"
      "  PyErr_SetString(PyExc_TypeError, \"indices must be integers or slices\");\n"
      "  return nullptr;\n"
      "}\n"
      "\n",
      classname, data->Name, data->Name, getItemFunc->SizeHint, classname, classname);
  }
}

/* -------------------------------------------------------------------- */
/* generate functions for arithmetic on special objects */
static void vtkWrapPython_NumberProtocol(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, const HierarchyInfo* hinfo, SpecialTypeInfo* info)
{
  int i, n;
  const FunctionInfo* func;
  int number_ops = 0;
  int use_sequence = 0; /* use Python-level sequence protocol for ops */

  /* scan both member functions and file-level free functions */
  n = data->NumberOfFunctions + finfo->Contents->NumberOfFunctions;
  for (i = 0; i < n; i++)
  {
    int is_member;
    if (i < data->NumberOfFunctions)
    {
      func = data->Functions[i];
      is_member = 1;
    }
    else
    {
      func = finfo->Contents->Functions[i - data->NumberOfFunctions];
      is_member = 0;
    }
    if (!func->IsOperator || !func->Name || func->IsDeleted || func->IsExcluded || func->Template)
    {
      continue;
    }

    /* check for binary arithmetic operators */
    {
      int is_binary = 0;
      if (is_member)
      {
        /* member binary: one parameter of the same special type */
        if (func->NumberOfParameters == 1 && vtkWrap_IsSpecialObject(func->Parameters[0]) &&
          strcmp(func->Parameters[0]->Class, data->Name) == 0)
        {
          is_binary = 1;
        }
      }
      else
      {
        /* non-member binary: two parameters of the same special type */
        if (func->NumberOfParameters == 2 && vtkWrap_IsSpecialObject(func->Parameters[0]) &&
          strcmp(func->Parameters[0]->Class, data->Name) == 0 &&
          vtkWrap_IsSpecialObject(func->Parameters[1]) &&
          strcmp(func->Parameters[1]->Class, data->Name) == 0)
        {
          is_binary = 1;
        }
      }

      if (is_binary)
      {
        if (strcmp(func->Name, "operator+") == 0)
        {
          number_ops |= VTK_PYTHON_OP_ADD;
        }
        else if (strcmp(func->Name, "operator-") == 0)
        {
          number_ops |= VTK_PYTHON_OP_SUBTRACT;
        }
        else if (strcmp(func->Name, "operator*") == 0)
        {
          number_ops |= VTK_PYTHON_OP_MULTIPLY;
        }
        else if (strcmp(func->Name, "operator/") == 0)
        {
          number_ops |= VTK_PYTHON_OP_DIVIDE;
        }
      }
    }

    /* check for unary negation */
    if (strcmp(func->Name, "operator-") == 0)
    {
      if (is_member && func->NumberOfParameters == 0)
      {
        /* member unary: no parameters */
        number_ops |= VTK_PYTHON_OP_NEGATIVE;
      }
      else if (!is_member && func->NumberOfParameters == 1 &&
        vtkWrap_IsSpecialObject(func->Parameters[0]) &&
        strcmp(func->Parameters[0]->Class, data->Name) == 0)
      {
        /* non-member unary: one parameter of our type */
        number_ops |= VTK_PYTHON_OP_NEGATIVE;
      }
    }
  }

  /* If no C++ operators found, check if this is a vtkTuple derivative.
   * For vtkTuple types, generate arithmetic using the Python sequence
   * protocol (element-wise operations via PySequence_GetItem). This handles
   * cases where the C++ operators are defined via macros that the wrapper
   * parser cannot see. */
  if (number_ops == 0)
  {
    if (vtkWrapPython_IsTupleType(data, hinfo))
    {
      number_ops = VTK_PYTHON_OP_ADD | VTK_PYTHON_OP_SUBTRACT | VTK_PYTHON_OP_MULTIPLY |
        VTK_PYTHON_OP_DIVIDE | VTK_PYTHON_OP_NEGATIVE;
      use_sequence = 1;
    }
    else
    {
      return;
    }
  }

  info->has_number = 1;
  info->number_ops = number_ops;

  /* forward declare Check if not already done by RichCompare */
  if (!info->has_compare)
  {
    fprintf(fp, "static int Py%s_Check(PyObject *ob);\n\n", classname);
  }

  if (use_sequence)
  {
    /* Generate a helper that performs an element-wise binary operation
     * using the Python sequence protocol.  When both operands are the
     * same tuple type, the operation is element-wise.  When one operand
     * is a scalar, it is broadcast to each element.  The result is
     * constructed by passing the element tuple as a single argument
     * to the constructor (matching the const T* init overload). */
    fprintf(fp,
      "static PyObject *Py%s_SeqBinOp(\n"
      "  PyObject *o1, PyObject *o2,\n"
      "  PyObject *(*op)(PyObject *, PyObject *))\n"
      "{\n"
      "  int o1v = Py%s_Check(o1);\n"
      "  int o2v = Py%s_Check(o2);\n"
      "  if (!o1v && !o2v) { Py_RETURN_NOTIMPLEMENTED; }\n"
      "  PyObject *vec = o1v ? o1 : o2;\n"
      "  Py_ssize_t n = PySequence_Size(vec);\n"
      "  if (n < 0) { return nullptr; }\n"
      "  PyObject *elems = PyTuple_New(n);\n"
      "  if (!elems) { return nullptr; }\n"
      "  for (Py_ssize_t i = 0; i < n; i++)\n"
      "  {\n"
      "    PyObject *a = o1v ? PySequence_GetItem(o1, i) : o1;\n"
      "    PyObject *b = o2v ? PySequence_GetItem(o2, i) : o2;\n"
      "    PyObject *r = (a && b) ? op(a, b) : nullptr;\n"
      "    if (o1v) { Py_XDECREF(a); }\n"
      "    if (o2v) { Py_XDECREF(b); }\n"
      "    if (!r)\n"
      "    {\n"
      "      Py_DECREF(elems);\n"
      "      // Element op failed: let Python try the other operand's reflected method.\n"
      "      PyErr_Clear();\n"
      "      Py_RETURN_NOTIMPLEMENTED;\n"
      "    }\n"
      "    PyTuple_SetItem(elems, i, r);\n"
      "  }\n"
      "  PyObject *args = PyTuple_Pack(1, elems);\n"
      "  Py_DECREF(elems);\n"
      "  PyObject *result = PyObject_CallObject((PyObject *)Py_TYPE(vec), args);\n"
      "  Py_DECREF(args);\n"
      "  return result;\n"
      "}\n\n",
      classname, classname, classname);
  }

  /* generate binary operator functions */
  static const struct
  {
    int mask;
    const char* name;     /* function name suffix */
    const char* op_token; /* C++ operator */
    const char* py_func;  /* PyNumber_* helper for sequence path */
  } binops[] = {
    { VTK_PYTHON_OP_ADD, "Add", "+", "PyNumber_Add" },
    { VTK_PYTHON_OP_SUBTRACT, "Subtract", "-", "PyNumber_Subtract" },
    { VTK_PYTHON_OP_MULTIPLY, "Multiply", "*", "PyNumber_Multiply" },
    { VTK_PYTHON_OP_DIVIDE, "Divide", "/", "PyNumber_TrueDivide" },
  };

  for (size_t k = 0; k < sizeof(binops) / sizeof(binops[0]); k++)
  {
    if (!(number_ops & binops[k].mask))
    {
      continue;
    }
    if (use_sequence)
    {
      fprintf(fp,
        "static PyObject *Py%s_%s(PyObject *o1, PyObject *o2)\n"
        "{\n"
        "  return Py%s_SeqBinOp(o1, o2, %s);\n"
        "}\n\n",
        classname, binops[k].name, classname, binops[k].py_func);
    }
    else
    {
      fprintf(fp,
        "static PyObject *Py%s_%s(PyObject *o1, PyObject *o2)\n"
        "{\n"
        "  const %s *so1 = nullptr;\n"
        "  const %s *so2 = nullptr;\n"
        "  PyObject *n1 = nullptr;\n"
        "  PyObject *n2 = nullptr;\n"
        "\n"
        "  if (Py%s_Check(o1)) {\n"
        "    so1 = static_cast<const %s *>(((PyVTKSpecialObject *)o1)->vtk_ptr);\n"
        "  } else {\n"
        "    so1 = static_cast<const %s *>(\n"
        "      vtkPythonUtil::GetPointerFromSpecialObject(o1, \"%s\", &n1));\n"
        "    if (!so1) { PyErr_Clear(); Py_RETURN_NOTIMPLEMENTED; }\n"
        "  }\n"
        "  if (Py%s_Check(o2)) {\n"
        "    so2 = static_cast<const %s *>(((PyVTKSpecialObject *)o2)->vtk_ptr);\n"
        "  } else {\n"
        "    so2 = static_cast<const %s *>(\n"
        "      vtkPythonUtil::GetPointerFromSpecialObject(o2, \"%s\", &n2));\n"
        "    if (!so2) { PyErr_Clear(); Py_XDECREF(n1); Py_RETURN_NOTIMPLEMENTED; }\n"
        "  }\n"
        "\n"
        "  %s tempr = (*so1) %s (*so2);\n"
        "  Py_XDECREF(n1); Py_XDECREF(n2);\n"
        "  return PyVTKSpecialObject_CopyNew(\"%s\", &tempr);\n"
        "}\n\n",
        classname, binops[k].name, data->Name, data->Name, classname, data->Name, data->Name,
        classname, classname, data->Name, data->Name, classname, data->Name, binops[k].op_token,
        classname);
    }
  }

  if (number_ops & VTK_PYTHON_OP_NEGATIVE)
  {
    if (use_sequence)
    {
      fprintf(fp,
        "static PyObject *Py%s_Negative(PyObject *o1)\n"
        "{\n"
        "  Py_ssize_t n = PySequence_Size(o1);\n"
        "  if (n < 0) { return nullptr; }\n"
        "  PyObject *elems = PyTuple_New(n);\n"
        "  if (!elems) { return nullptr; }\n"
        "  for (Py_ssize_t i = 0; i < n; i++)\n"
        "  {\n"
        "    PyObject *v = PySequence_GetItem(o1, i);\n"
        "    PyObject *r = PyNumber_Negative(v);\n"
        "    Py_DECREF(v);\n"
        "    if (!r) { Py_DECREF(elems); return nullptr; }\n"
        "    PyTuple_SetItem(elems, i, r);\n"
        "  }\n"
        "  PyObject *args = PyTuple_Pack(1, elems);\n"
        "  Py_DECREF(elems);\n"
        "  PyObject *result = PyObject_CallObject((PyObject *)Py_TYPE(o1), args);\n"
        "  Py_DECREF(args);\n"
        "  return result;\n"
        "}\n\n",
        classname);
    }
    else
    {
      fprintf(fp,
        "static PyObject *Py%s_Negative(PyObject *o1)\n"
        "{\n"
        "  const %s *so1 = static_cast<const %s *>(\n"
        "    ((PyVTKSpecialObject *)o1)->vtk_ptr);\n"
        "\n"
        "  %s tempr = -(*so1);\n"
        "  return PyVTKSpecialObject_CopyNew(\"%s\", &tempr);\n"
        "}\n\n",
        classname, data->Name, data->Name, data->Name, classname);
    }
  }
}

/* -------------------------------------------------------------------- */
/* generate function for hashing special objects */
static void vtkWrapPython_HashProtocol(FILE* fp, const char* classname, const ClassInfo* data)
{
  /* the hash function, defined only for specific types */
  fprintf(fp, "static Py_hash_t Py%s_Hash(PyObject *self)\n", classname);

  if (strcmp(data->Name, "vtkTimeStamp") == 0)
  {
    /* hash for vtkTimeStamp is just the timestamp itself */
    fprintf(fp,
      "{\n"
      "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
      "  const vtkTimeStamp *op = static_cast<const vtkTimeStamp *>(obj->vtk_ptr);\n"
      "  vtkMTimeType mtime = *op;\n"
      "  long h = (long)mtime;\n"
      "  if (h != -1) { return h; }\n"
      "  return -2;\n"
      "}\n"
      "\n");
  }
  else if (strcmp(data->Name, "vtkVariant") == 0)
  {
    /* hash for vtkVariant is cached to avoid recomputation, this is
     * safe because vtkVariant is an immutable object, and is necessary
     * because computing the hash for vtkVariant is very expensive */
    fprintf(fp,
      "{\n"
      "  PyVTKSpecialObject *obj = (PyVTKSpecialObject *)self;\n"
      "  const vtkVariant *op = static_cast<const vtkVariant *>(obj->vtk_ptr);\n"
      "  long h = obj->vtk_hash;\n"
      "  if (h != -1)\n"
      "  {\n"
      "    return h;\n"
      "  }\n"
      "  h = vtkPythonUtil::VariantHash(op);\n"
      "  obj->vtk_hash = h;\n"
      "  return h;\n"
      "}\n"
      "\n");
  }
  else
  {
    /* if hash is not implemented, raise an exception */
    fprintf(fp,
      "{\n"
      "  return PyObject_HashNotImplemented(self);\n"
      "}\n"
      "\n");
  }
}

/* -------------------------------------------------------------------- */
/* generate extra functions for a special object */
static void vtkWrapPython_SpecialTypeProtocols(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, const HierarchyInfo* hinfo, SpecialTypeInfo* info)
{
  /* clear all info about the type */
  info->has_print = 0;
  info->has_compare = 0;
  info->has_sequence = 0;
  info->has_setitem = 0;
  info->has_number = 0;
  info->number_ops = 0;

  vtkWrapPython_NewDeleteProtocol(fp, classname, data, hinfo);
  vtkWrapPython_PrintProtocol(fp, classname, data, finfo, info);
  vtkWrapPython_RichCompareProtocol(fp, classname, data, finfo, hinfo, info);
  vtkWrapPython_SequenceProtocol(fp, classname, data, hinfo, info);
  vtkWrapPython_NumberProtocol(fp, classname, data, finfo, hinfo, info);
  vtkWrapPython_HashProtocol(fp, classname, data);
}

/* -------------------------------------------------------------------- */
/* For classes that aren't derived from vtkObjectBase, check to see if
 * they are wrappable */
int vtkWrapPython_IsSpecialTypeWrappable(const ClassInfo* data)
{
  /* wrapping templates is only possible after template instantiation */
  if (data->Template)
  {
    return 0;
  }

  /* restrict wrapping to classes that have a "vtk" prefix */
  if (strncmp(data->Name, "vtk", 3) != 0)
  {
    return 0;
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* write out a special type object */
void vtkWrapPython_GenerateSpecialType(FILE* fp, const char* module, const char* classname,
  ClassInfo* data, FileInfo* finfo, const HierarchyInfo* hinfo)
{
  char supername[1024];
  const char* supermodule;
  const char* name;
  SpecialTypeInfo info;
  const char* constructor = NULL;
  const char* constructorSignature = NULL;
  size_t n = 0;
  int i;
  int has_constants = 0;
  int has_superclass = 0;
  int has_copycons = 0;

  /* remove namespaces and template parameters from the
   * class name to get the constructor name */
  if (!data->IsAbstract)
  {
    constructor = vtkWrapPython_WrappedConstructor(data, hinfo, &n);
  }

  /* get the superclass */
  name = vtkWrapPython_GetSuperClass(data, finfo, hinfo, &supermodule);
  if (name)
  {
    has_superclass = 1;
    vtkWrapText_PythonName(name, supername);
  }

  /* generate all constructor methods */
  if (constructor)
  {
    constructorSignature = vtkWrapPython_GenerateMethods(fp, classname, data, finfo, hinfo, 0, 1);
  }

  /* the docstring for the class, as a static var ending in "Doc" */
  fprintf(fp, "\nstatic const char *Py%s_Doc =\n", classname);
  if (constructorSignature)
  {
    fprintf(
      fp, "\n  \"%s\\n\"\"\\n\"\n", vtkWrapText_FormatSignature(constructorSignature, 70, 2000));
  }
  vtkWrapPython_ClassDoc(fp, finfo, data, hinfo);
  fprintf(fp, ";\n\n");

  /* generate all functions and protocols needed for the type */
  vtkWrapPython_SpecialTypeProtocols(fp, classname, data, finfo, hinfo, &info);

  /* Generate the type's slot table and spec, used by PyVTKSpecialType_AddHeapType. */
  fprintf(fp, "static PyTypeObject *Py%s_Type = nullptr;\n\n", classname);

  fprintf(fp,
    "static PyType_Slot Py%s_Slots[] = {\n"
    "  { Py_tp_dealloc, (void *)Py%s_Delete },\n"
    "  { Py_tp_repr, (void *)PyVTKSpecialObject_Repr },\n"
    "  { Py_tp_hash, (void *)Py%s_Hash },\n"
    "  { Py_tp_getattro, (void *)PyObject_GenericGetAttr },\n"
    "  { Py_tp_doc, (void *)Py%s_Doc },\n"
    "  { Py_tp_new, (void *)Py%s_New },\n",
    classname, classname, classname, classname, classname);

  if (info.has_print)
  {
    fprintf(fp, "  { Py_tp_str, (void *)Py%s_String },\n", classname);
  }
  else if (info.has_sequence)
  {
    fprintf(fp, "  { Py_tp_str, (void *)PyVTKSpecialObject_SequenceString },\n");
  }

  if (info.has_compare)
  {
    fprintf(fp, "  { Py_tp_richcompare, (void *)Py%s_RichCompare },\n", classname);
  }

  if (info.has_sequence)
  {
    fprintf(fp,
      "  { Py_sq_length, (void *)Py%s_SequenceSize },\n"
      "  { Py_sq_item, (void *)Py%s_SequenceItem },\n"
      "  { Py_mp_length, (void *)Py%s_SequenceSize },\n"
      "  { Py_mp_subscript, (void *)Py%s_MappingSubscript },\n",
      classname, classname, classname, classname);
    if (info.has_setitem)
    {
      fprintf(fp, "  { Py_sq_ass_item, (void *)Py%s_SequenceSetItem },\n", classname);
    }
  }

  if (info.has_number)
  {
    if (info.number_ops & VTK_PYTHON_OP_ADD)
    {
      fprintf(fp, "  { Py_nb_add, (void *)Py%s_Add },\n", classname);
    }
    if (info.number_ops & VTK_PYTHON_OP_SUBTRACT)
    {
      fprintf(fp, "  { Py_nb_subtract, (void *)Py%s_Subtract },\n", classname);
    }
    if (info.number_ops & VTK_PYTHON_OP_MULTIPLY)
    {
      fprintf(fp, "  { Py_nb_multiply, (void *)Py%s_Multiply },\n", classname);
    }
    if (info.number_ops & VTK_PYTHON_OP_DIVIDE)
    {
      fprintf(fp, "  { Py_nb_true_divide, (void *)Py%s_Divide },\n", classname);
    }
    if (info.number_ops & VTK_PYTHON_OP_NEGATIVE)
    {
      fprintf(fp, "  { Py_nb_negative, (void *)Py%s_Negative },\n", classname);
    }
  }

  fprintf(fp,
    "  { 0, nullptr }\n"
    "};\n"
    "\n"
    "static PyType_Spec Py%s_Spec = {\n"
    "  PYTHON_PACKAGE_SCOPE \"%s.%s\", // tp_name\n"
    "  sizeof(PyVTKSpecialObject), // tp_basicsize\n"
    "  0, // tp_itemsize\n"
    "  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags\n"
    "  Py%s_Slots\n"
    "};\n"
    "\n",
    classname, module, classname, classname);

  /* need a check function for some protocols */
  if (info.has_compare || info.has_number)
  {
    fprintf(fp,
      "static int Py%s_Check(PyObject *ob)\n"
      "{\n"
      "  return (Py_TYPE(ob) == Py%s_Type || PyType_IsSubtype(Py_TYPE(ob), Py%s_Type));\n"
      "}\n\n",
      classname, classname, classname);
  }

  /* generate the copy constructor helper function */
  if (constructor && vtkWrap_HasPublicCopyConstructor(data))
  {
    has_copycons = 1;

    fprintf(fp,
      "static void *Py%s_CCopy(const void *obj)\n"
      "{\n"
      "  if (obj)\n"
      "  {\n"
      "    return new %s(*static_cast<const %s*>(obj));\n"
      "  }\n"
      "  return nullptr;\n"
      "}\n"
      "\n",
      classname, data->Name, data->Name);
  }

  /* export New method for use by subclasses */
  fprintf(fp,
    "#ifndef DECLARED_Py%s_TypeNew\n"
    "extern \"C\" { %s PyObject *Py%s_TypeNew(); }\n"
    "#define DECLARED_Py%s_TypeNew\n"
    "#endif\n\n",
    classname, "VTK_ABI_HIDDEN", classname, classname);

  /* import New method of the superclass */
  if (has_superclass && !supermodule)
  {
    fprintf(fp,
      "#ifndef DECLARED_Py%s_TypeNew\n"
      "extern \"C\" { PyObject *Py%s_TypeNew(); }\n"
      "#define DECLARED_Py%s_TypeNew\n"
      "#endif\n",
      supername, supername, supername);
  }

  /* the method for adding the VTK extras to the type,
   * the unused "const char *" arg is the module name */
  fprintf(fp,
    "PyObject *Py%s_TypeNew()\n"
    "{\n"
    "  if (Py%s_Type != nullptr)\n"
    "  {\n"
    "    return (PyObject *)Py%s_Type;\n"
    "  }\n"
    "\n"
    "  PyObject *base = nullptr;\n",
    classname, classname, classname);

  /* create the superclass first; it becomes the base of this heap type */
  if (has_superclass)
  {
    if (!supermodule) /* superclass is in the same module */
    {
      fprintf(fp, "  base = Py%s_TypeNew();\n", supername);
    }
    else /* superclass is in a different module */
    {
      fprintf(
        fp, "  base = (PyObject *)vtkPythonUtil::FindSpecialTypeObject(\"%s\");\n", supername);
    }
  }

  fprintf(fp,
    "\n"
    "  Py%s_Type = PyVTKSpecialType_AddHeapType(\n"
    "    &Py%s_Spec, base,\n"
    "    Py%s_Methods,\n"
    "    Py%s_GetSets,\n",
    classname, classname, classname, classname);

  if (has_copycons)
  {
    fprintf(fp,
      "    Py%s_%*.*s_Methods,\n"
      "    &Py%s_CCopy);\n",
      classname, (int)n, (int)n, constructor, classname);
  }
  else if (constructor)
  {
    fprintf(fp,
      "    Py%s_%*.*s_Methods,\n"
      "    nullptr);\n",
      classname, (int)n, (int)n, constructor);
  }
  else
  {
    fprintf(fp,
      "    nullptr,\n"
      "    nullptr);\n");
  }

  fprintf(fp,
    "  if (Py%s_Type == nullptr)\n"
    "  {\n"
    "    return nullptr;\n"
    "  }\n\n",
    classname);

  /* check whether the class has any constants as members */
  for (i = 0; i < data->NumberOfConstants; i++)
  {
    if (data->Constants[i]->Access == VTK_ACCESS_PUBLIC)
    {
      has_constants = 1;
    }
  }

  if (has_constants)
  {
    fprintf(fp,
      "  PyObject *d = (PyObject *)Py%s_Type;\n"
      "  PyObject *o;\n\n",
      classname);

    /* add any enum types defined in the class to its dict */
    vtkWrapPython_AddPublicEnumTypes(fp, "  ", "d", "o", data);

    /* add any constants defined in the class to its dict */
    vtkWrapPython_AddPublicConstants(fp, "  ", "d", "o", data);
  }

  fprintf(fp,
    "  PyType_Modified(Py%s_Type);\n"
    "  return (PyObject *)Py%s_Type;\n"
    "}\n\n",
    classname, classname);
}

// NOLINTEND(bugprone-unsafe-functions)
