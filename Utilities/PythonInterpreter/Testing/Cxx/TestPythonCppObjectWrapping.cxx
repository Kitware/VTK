// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPythonInterpreter.h"
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPythonUtil.h>
#include <vtkSmartPyObject.h>

int TestPythonCppObjectWrapping(int, char*[])
{
  // Create a C++ object
  vtkNew<vtkIntArray> array;
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(5);
  array->Fill(0);
  array->SetTuple1(2, 5);

  // Initialize interpreter and GIL
  vtkPythonInterpreter::Initialize();
  vtkPythonScopeGilEnsurer gilEnsurer(true, true);

  // Import vtk in Python
  vtkPythonInterpreter::RunSimpleString("import vtkmodules.vtkCommonCore");

  // Create the python object from the C++ object
  vtkSmartPyObject pyDataFromCpp;
  pyDataFromCpp.TakeReference(vtkPythonUtil::GetObjectFromPointer(array.GetPointer()));

  // Make it visible from python
  PyObject* mainModule = PyImport_AddModule("__main__");
  if (!mainModule)
  {
    std::cerr << "Could not retrieve the __main__ module." << std::endl;
    return EXIT_FAILURE;
  }
  PyObject* mainDict = PyModule_GetDict(mainModule);
  if (!mainDict)
  {
    std::cerr << "Could not retrieve the main dictionnary." << std::endl;
    return EXIT_FAILURE;
  }
  if (PyDict_SetItemString(mainDict, "pyDataFromPython", pyDataFromCpp) != 0)
  {
    std::cerr << "Failed to set pyDataFromPython from pyDataFromCpp." << std::endl;
    return EXIT_FAILURE;
  }

  // Run Python script
  std::string script;
  script += "val = pyDataFromPython.GetTuple1(2)\n";
  script += "pyDataFromPython.SetTuple1(2, val - 2)\n";
  vtkPythonInterpreter::RunSimpleString(script.c_str());
  vtkPythonInterpreter::Finalize();

  if (array->GetTuple1(2) != 3)
  {
    std::cerr << "Wrong array value. Got " << array->GetTuple1(2) << ", should be 3."
              << "C++ and Python may not reference the same object." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
