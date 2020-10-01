/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestXMLLegacyFileReadIdTypeArrays.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellData.h"
#include "vtkIdTypeArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

namespace
{
bool CheckArray(vtkDataSetAttributes* dsa, const char* aname)
{
  auto array = dsa->GetArray(aname);
  if (array == nullptr)
  {
    vtkLogF(ERROR, "missing array '%s'", aname);
    return false;
  }
  if (vtkIdTypeArray::SafeDownCast(array) == nullptr)
  {
    vtkLogF(
      ERROR, "array '%s' is of type '%s', and not vtkIdTypeArray", aname, array->GetClassName());
    return false;
  }
  return true;
}
}

int TestXMLLegacyFileReadIdTypeArrays(int argc, char* argv[])
{
  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  std::string filename = testing->GetDataRoot();
  filename += "/Data/xml-without-idtype-tag.vtu";

  vtkNew<vtkXMLUnstructuredGridReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();

  auto ug = reader->GetOutput();
  if (CheckArray(ug->GetPointData(), "GlobalNodeId") &&
    CheckArray(ug->GetPointData(), "PedigreeNodeId") &&
    CheckArray(ug->GetCellData(), "GlobalElementId") &&
    CheckArray(ug->GetCellData(), "PedigreeElementId"))
  {
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
