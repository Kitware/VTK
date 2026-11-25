// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapPythonNumberProtocol.h"

#include <string.h>

// NOLINTBEGIN(bugprone-unsafe-functions)

/* Overrides __rshift__ operator for vtkAlgorithm and vtkDataObject */
int vtkWrapPython_GenerateNumberProtocolDefintions(FILE* fp, const ClassInfo* classInfo)
{
  const int isVtkAlgorithm = strcmp("vtkAlgorithm", classInfo->Name) == 0;
  const int isVtkDataObject = strcmp("vtkDataObject", classInfo->Name) == 0;
  if (!(isVtkAlgorithm || isVtkDataObject))
  {
    return 0;
  }

  /*
   * define "lhs >> rhs", which is equivalent to:
   * def func(lhs: vtkDataObject, rhs: vtkAlgorithm):
   *   rhs.input_data_object = lhs
   *   return rhs
   */
  fprintf(fp,
    "static PyObject* Py%s_RShift(PyObject* lhs, PyObject* rhs)\n"
    "{\n"
    "  if (lhs == Py_None || (PySequence_Check(lhs) && PySequence_Size(lhs) == 0))\n"
    "  {\n"
    "    if (PyObject_HasAttrString(rhs, \"SetInputConnection\"))\n"
    "    {\n"
    "      PyObject* zero = PyLong_FromLong(0);\n"
    "      PyObject_CallMethod(rhs, \"RemoveAllInputConnections\", \"O\", zero);\n"
    "      Py_DECREF(zero);\n"
    "      Py_INCREF(rhs);\n"
    "      return rhs;\n"
    "    }\n"
    "  }\n"
    "  // Import the module\n"
    "  PyObject* moduleName = PyUnicode_DecodeFSDefault(\"vtkmodules.util.execution_model\");\n"
    "  PyObject* internalModule = PyImport_Import(moduleName);\n"
    "  Py_DECREF(moduleName);\n"
    "  PyObject* pipeline = nullptr;\n"
    "  if (internalModule != nullptr)\n"
    "  {\n"
    "    // Get the class from the module\n"
    "    PyObject* pipelineClass = PyObject_GetAttrString(internalModule, \"Pipeline\");\n"
    "    if (pipelineClass != nullptr)\n"
    "    {\n"
    "      // Create an instance of the class\n"
    "      auto args = PyTuple_Pack(2, lhs, rhs); // Pass any arguments required by your "
    "constructor\n"
    "      pipeline = PyObject_CallObject(pipelineClass, args);\n"
    "      Py_XDECREF(args);\n"
    "      if (pipeline == nullptr)\n"
    "      {\n"
    "        return nullptr;\n"
    "      }\n"
    "      Py_DECREF(pipelineClass);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "       return nullptr;\n"
    "    }\n"
    "    Py_DECREF(internalModule);\n"
    "  }\n"
    "  else\n"
    "  {\n"
    "    return nullptr;\n"
    "  }\n"
    "  return pipeline;\n"
    "}\n\n",
    classInfo->Name);

  return 1;
}

// NOLINTEND(bugprone-unsafe-functions)
