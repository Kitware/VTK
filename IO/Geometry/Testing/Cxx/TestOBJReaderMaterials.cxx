// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Verifies that vtkOBJReader does something sensible w/rt materials.
// .SECTION Description
//

#include "vtkOBJReader.h"

#include "vtkCellData.h"
#include "vtkStringArray.h"
#include "vtkTestUtilities.h"

int TestOBJReaderMaterials(int argc, char* argv[])
{
  int retVal = 0;

  // Create the reader.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/obj_with_materials.obj");
  vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  vtkPolyData* data = reader->GetOutput();

  if (!data)
  {
    std::cerr << "Could not read data" << std::endl;
    return 1;
  }
  vtkStringArray* mna =
    vtkStringArray::SafeDownCast(data->GetFieldData()->GetAbstractArray("MaterialNames"));
  if (!mna)
  {
    std::cerr << "missing material names array" << std::endl;
    return 1;
  }
  vtkIntArray* mia =
    vtkIntArray::SafeDownCast(data->GetCellData()->GetAbstractArray("MaterialIds"));
  if (!mia)
  {
    std::cerr << "missing material id array" << std::endl;
    return 1;
  }
  if (data->GetNumberOfCells() != 2)
  {
    std::cerr << "wrong number of cells" << std::endl;
    return 1;
  }
  int matid = mia->GetVariantValue(1).ToInt();
  std::string matname = mna->GetVariantValue(matid).ToString();
  if (matname != "Air")
  {
    std::cerr << "wrong material for" << std::endl;
    return 1;
  }

  return retVal;
}
