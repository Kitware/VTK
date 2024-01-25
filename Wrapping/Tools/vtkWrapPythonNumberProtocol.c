// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkWrapPythonNumberProtocol.h"

#include <string.h>

/* Overrides __rshift__ operator for vtkAlgorithm and vtkDataObject */
int vtkWrapPython_GenerateNumberProtocolDefintions(FILE* fp, ClassInfo* classInfo)
{
  const int isVtkAlgorithm = strcmp("vtkAlgorithm", classInfo->Name) == 0;
  const int isVtkDataObject = strcmp("vtkDataObject", classInfo->Name) == 0;
  if (!(isVtkAlgorithm || isVtkDataObject))
  {
    return 0;
  }
  fprintf(fp,
    "static PyNumberMethods Py%s_NumberMethods[] = {\n"
    "  {\n"
    "    nullptr, // nb_add\n"
    "    nullptr, // nb_subtract\n"
    "    nullptr, // nb_multiply\n"
    "    nullptr, // nb_remainder\n"
    "    nullptr, // nb_divmod\n"
    "    nullptr, // nb_power\n"
    "    nullptr, // nb_negative\n"
    "    nullptr, // nb_positive\n"
    "    nullptr, // nb_absolute\n"
    "    nullptr, // nb_bool\n"
    "    nullptr, // nb_invert\n"
    "    nullptr, // nb_lshift\n",
    classInfo->Name);
  if (isVtkAlgorithm)
  {
    // lhs >> rhs is equivalent to:
    // def func(lhs: vtkAlgorithm, rhs: vtkAlgorithm):
    //   rhs.input_connection = lhs.output_port
    //   return rhs
    // The generated code uses wrapped C function directly without going through python properties.
    fprintf(fp,
      "    [](PyObject* lhs, PyObject* rhs) -> PyObject*\n"
      "    {\n"
      "      auto placeholder = PyTuple_New(0);\n"
      "      auto args = PyTuple_Pack(1, PyvtkAlgorithm_GetOutputPort(lhs, placeholder));\n"
      "      PyvtkAlgorithm_SetInputConnection(rhs, args);\n"
      "      Py_DECREF(args);\n"
      "      Py_DECREF(placeholder);\n"
      "      Py_INCREF(rhs); // keeps rhs alive.\n"
      "      return rhs;\n"
      "    }, // nb_rshift\n");
  }
  else if (isVtkDataObject)
  {
    // lhs >> rhs is equivalent to:
    // def func(lhs: vtkDataObject, rhs: vtkAlgorithm):
    //   rhs.input_data_object = lhs
    //   return rhs
    // Go through PyObject_SetAttrString because
    // vtkDataObjectPython.cxx can't access PyvtkAlgorithm_SetInputDataObject.
    fprintf(fp,
      "    [](PyObject* lhs, PyObject* rhs) -> PyObject*\n"
      "    {\n"
      "      PyObject_SetAttrString(rhs, \"input_data_object\", lhs);\n"
      "      Py_INCREF(rhs); // keeps rhs alive.\n"
      "      return rhs;\n"
      "    }, // nb_rshift\n");
  }
  fprintf(fp,
    "    nullptr, // nb_and\n"
    "    nullptr, // nb_xor\n"
    "    nullptr, // nb_or\n"
    "    nullptr, // nb_int\n"
    "    nullptr, // nb_reserved;\n"
    "    nullptr, // nb_float\n"
    "    nullptr, // nb_inplace_add\n"
    "    nullptr, // nb_inplace_subtract\n"
    "    nullptr, // nb_inplace_multiply\n"
    "    nullptr, // nb_inplace_remainder\n"
    "    nullptr, // nb_inplace_power\n"
    "    nullptr, // nb_inplace_lshift\n"
    "    nullptr, // nb_inplace_rshift\n"
    "    nullptr, // nb_inplace_and\n"
    "    nullptr, // nb_inplace_xor\n"
    "    nullptr, // nb_inplace_or\n"
    "    nullptr, // nb_floor_divide\n"
    "    nullptr, // nb_true_divide\n"
    "    nullptr, // nb_inplace_floor_divide\n"
    "    nullptr, // nb_inplace_true_divide\n"
    "    nullptr, // nb_index\n"
    "    nullptr, // nb_matrix_multiply\n"
    "    nullptr, // nb_inplace_matrix_multiply\n"
    "  },\n"
    "  { // sentinel\n"
    "    nullptr, // nb_add\n"
    "    nullptr, // nb_subtract\n"
    "    nullptr, // nb_multiply\n"
    "    nullptr, // nb_remainder\n"
    "    nullptr, // nb_divmod\n"
    "    nullptr, // nb_power\n"
    "    nullptr, // nb_negative\n"
    "    nullptr, // nb_positive\n"
    "    nullptr, // nb_absolute\n"
    "    nullptr, // nb_bool\n"
    "    nullptr, // nb_invert\n"
    "    nullptr, // nb_lshift\n"
    "    nullptr, // nb_rshift\n"
    "    nullptr, // nb_and\n"
    "    nullptr, // nb_xor\n"
    "    nullptr, // nb_or\n"
    "    nullptr, // nb_int\n"
    "    nullptr, // nb_reserved;\n"
    "    nullptr, // nb_float\n"
    "    nullptr, // nb_inplace_add\n"
    "    nullptr, // nb_inplace_subtract\n"
    "    nullptr, // nb_inplace_multiply\n"
    "    nullptr, // nb_inplace_remainder\n"
    "    nullptr, // nb_inplace_power\n"
    "    nullptr, // nb_inplace_lshift\n"
    "    nullptr, // nb_inplace_rshift\n"
    "    nullptr, // nb_inplace_and\n"
    "    nullptr, // nb_inplace_xor\n"
    "    nullptr, // nb_inplace_or\n"
    "    nullptr, // nb_floor_divide\n"
    "    nullptr, // nb_true_divide\n"
    "    nullptr, // nb_inplace_floor_divide\n"
    "    nullptr, // nb_inplace_true_divide\n"
    "    nullptr, // nb_index\n"
    "    nullptr, // nb_matrix_multiply\n"
    "    nullptr, // nb_inplace_matrix_multiply\n"
    "  }\n"
    "};\n");
  return 1;
}
