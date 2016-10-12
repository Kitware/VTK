/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataSetSurfaceMultiBlockFieldData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cstdio>

#include <vtkDataSetSurfaceFilter.h>
#include <vtkFieldData.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStructuredGrid.h>
#include <vtkUnstructuredGrid.h>

// Test to ensure that field data is copied for different data types in vtkDataSetSurface

namespace
{

//----------------------------------------------------------------------------
int TestDataSet(vtkDataSet* ds, int expectedValue)
{
  vtkNew<vtkDataSetSurfaceFilter> surfacer;
  surfacer->SetInputData(ds);
  surfacer->Update();
  if (!surfacer->GetOutput())
  {
    std::cout << "No output!\n";
    return EXIT_FAILURE;
  }

  vtkFieldData* fieldData = surfacer->GetOutput()->GetFieldData();
  const char* className = ds->GetClassName();
  if (fieldData == NULL || fieldData->GetNumberOfArrays() == 0)
  {
    std::cerr << "No field data was associated with data set type " << className << "\n";
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Have field data for surface from data set type " << className << "\n";

    vtkIntArray* array = vtkArrayDownCast<vtkIntArray>(fieldData->GetArray(0));
    if (!array)
    {
      std::cerr << "Field data array was not of type vtkIntArray for data set type"
                << className << "\n";
      return EXIT_FAILURE;
    }
    else if (array->GetNumberOfTuples() < 1)
    {
      std::cerr << "No tuples in field data array for surface from data set type "
                << className << "\n";
      return EXIT_FAILURE;
    }
    else
    {
      int value = 0;
      array->GetTypedTuple(0, &value);

      std::cout << "Block value " << value << "\n";
      if (value != expectedValue)
      {
        std::cerr << "Unexpected block field array value " << value
                  << " for surface from data set type " << className
                  << ". Expected " << expectedValue << "\n";
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

//----------------------------------------------------------------------------
void AddFieldData(vtkDataSet* ds, int id)
{
  vtkNew<vtkIntArray> array;
  array->SetName("ID");
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(1);
  array->SetTypedTuple(0, &id);

  ds->GetFieldData()->AddArray(array.GetPointer());
}

//----------------------------------------------------------------------------
int TestImageData()
{
 // Create image data
  vtkNew<vtkImageData> imageData;
  imageData->Initialize();
  imageData->SetSpacing(1, 1, 1);
  imageData->SetOrigin(0, 0, 0);
  imageData->SetDimensions(10, 10, 10);

  int id = 1;
  AddFieldData(imageData.GetPointer(), id);

  // Add point data
  vtkNew<vtkFloatArray> pa;
  pa->SetName("pd");
  pa->SetNumberOfComponents(1);
  pa->SetNumberOfTuples(10 * 10 * 10);

  imageData->GetPointData()->AddArray(pa.GetPointer());

  return TestDataSet(imageData.GetPointer(), id);
}

//----------------------------------------------------------------------------
int TestPolyData()
{
  // Create polydata
  vtkNew<vtkPolyData> polyData;
  polyData->Initialize();

  int id = 2;
  AddFieldData(polyData.GetPointer(), id);

  return TestDataSet(polyData.GetPointer(), id);
}

//----------------------------------------------------------------------------
int TestStructuredGrid()
{
  // Create structured grid data
  vtkNew<vtkStructuredGrid> structuredGrid;
  structuredGrid->Initialize();

  int id = 3;
  AddFieldData(structuredGrid.GetPointer(), id);

  return TestDataSet(structuredGrid.GetPointer(), id);
}

//----------------------------------------------------------------------------
int TestUnstructuredGrid()
{
  // Create unstructured grid data
  vtkNew<vtkUnstructuredGrid> unstructuredGrid;
  unstructuredGrid->Initialize();

  int id = 4;
  AddFieldData(unstructuredGrid.GetPointer(), id);

  return TestDataSet(unstructuredGrid.GetPointer(), id);
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int TestDataSetSurfaceFieldData(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (TestImageData() != EXIT_SUCCESS)
  {
    std::cerr << "TestImageData failed\n";
    return EXIT_FAILURE;
  }

  if (TestPolyData() != EXIT_SUCCESS)
  {
    std::cerr << "TestPolyData failed\n";
    return EXIT_FAILURE;
  }

  if (TestStructuredGrid() != EXIT_SUCCESS)
  {
    std::cerr << "TestStructuredGrid failed\n";
    return EXIT_FAILURE;
  }

  if (TestUnstructuredGrid() != EXIT_SUCCESS)
  {
    std::cerr << "TestUnstructuredGrid failed\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
