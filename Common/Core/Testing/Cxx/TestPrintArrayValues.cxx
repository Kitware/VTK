// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkTypeUInt32Array.h"

#include <sstream>

int TestPrintArrayValues(int, char*[])
{
  {
    vtkNew<vtkTypeUInt32Array> uint32Array;
    for (int i = 0; i < 10; ++i)
    {
      uint32Array->InsertNextValue(i);
    }
    std::ostringstream oss;
    uint32Array->PrintValues(oss);
    if (oss.str() != "0 1 2 3 4 5 6 7 8 9 ")
    {
      std::cerr << oss.str() << " != 0 1 2 3 4 5 6 7 8 9\n";
      return EXIT_FAILURE;
    }
  }
  {
    vtkNew<vtkStringArray> stringArray;
    stringArray->InsertNextValue("VTK");
    stringArray->InsertNextValue("vtk");
    std::ostringstream oss;
    stringArray->PrintValues(oss);
    if (oss.str() != "VTK vtk ")
    {
      std::cerr << oss.str() << " != VTK vtk\n";
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
