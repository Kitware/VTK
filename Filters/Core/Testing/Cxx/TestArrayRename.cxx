/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestArrayRename.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkArrayRename.h"
#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

int TestPointCellData()
{
  int errors = 0;

  vtkNew<vtkArrayRename> renamer;
  std::string originalName = "First";
  std::string originalName2 = "Second";
  std::string newName = "Result";

  int i = 2;
  int j = 2;
  int k = 2;
  vtkNew<vtkImageData> image;
  image->SetDimensions(i, j, k);

  vtkNew<vtkIntArray> array1;
  array1->SetName(originalName.c_str());
  vtkNew<vtkIntArray> array2;
  array2->SetName(originalName2.c_str());
  for (vtkIdType idx = 0; idx < i * j * k; idx++)
  {
    array1->InsertNextValue(idx);
    array2->InsertNextValue(-idx);
  }

  vtkCellData* cellData = image->GetCellData();
  cellData->AddArray(array1);
  cellData->AddArray(array2);
  vtkPointData* pointData = image->GetPointData();
  pointData->AddArray(array1);
  pointData->AddArray(array2);

  renamer->SetInputData(image);

  // Test specific (point data) API
  renamer->SetPointArrayName(originalName.c_str(), newName.c_str());
  renamer->Update();
  if (renamer->GetNumberOfPointArrays() != 2)
  {
    std::cerr << "error : Wrong number of point arrays after renaming" << std::endl;
    errors++;
  }
  vtkImageData* outputImage = vtkImageData::SafeDownCast(renamer->GetOutput());
  vtkAbstractArray* outArray = outputImage->GetPointData()->GetAbstractArray(newName.c_str());
  if (!outArray)
  {
    std::cerr << "error : Cannot find array in output with name " << newName << std::endl;
    errors++;
  }

  // test generic API.
  renamer->SetArrayName(vtkDataObject::CELL, originalName2.c_str(), newName.c_str());
  renamer->Update();
  if (renamer->GetNumberOfArrays(vtkDataObject::CELL) != 2)
  {
    std::cerr << "error : Wrong number of cell arrays after renaming" << std::endl;
    errors++;
  }

  return errors;
}

int TestRowData()
{
  int errors = 0;

  std::string originalName = "First";
  std::string newName = "Result";
  vtkNew<vtkArrayRename> renamer;
  vtkNew<vtkTable> table;
  vtkNew<vtkStringArray> strings;
  strings->InsertNextValue("stringValue");
  strings->SetName(originalName.c_str());
  table->SetNumberOfRows(1);
  table->GetRowData()->AddArray(strings);

  renamer->SetInputData(table);
  renamer->SetRowArrayName(0, newName.c_str());
  renamer->Update();
  if (originalName != renamer->GetRowArrayOriginalName(0))
  {
    std::cerr << "error : wrong original name stored" << std::endl;
    errors++;
  }
  if (newName != renamer->GetRowArrayNewName(0))
  {
    std::cerr << "error : wrong new name stored" << std::endl;
    errors++;
  }

  return errors;
}

int TestArrayRename(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;
  errors += TestPointCellData();
  errors += TestRowData();
  return errors;
}
