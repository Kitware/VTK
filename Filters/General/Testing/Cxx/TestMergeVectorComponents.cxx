/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMergeVectorComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMergeVectorComponents.h"
#include "vtkPointData.h"
#include "vtkSphereSource.h"

int TestMergeVectorComponents(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  // Create a sphere
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetCenter(0.0, 0.0, 0.0);
  sphereSource->SetRadius(5.0);
  // Make the surface smooth.
  sphereSource->SetPhiResolution(100);
  sphereSource->SetThetaResolution(100);
  sphereSource->Update();
  auto dataset = sphereSource->GetOutput();

  // test point data by creating 3 point data related arrays and merging them
  vtkNew<vtkDoubleArray> xPD;
  xPD->SetNumberOfValues(dataset->GetNumberOfPoints());
  xPD->SetName("xPD");
  xPD->Fill(0);
  dataset->GetPointData()->AddArray(xPD);

  vtkNew<vtkDoubleArray> yPD;
  yPD->SetNumberOfValues(dataset->GetNumberOfPoints());
  yPD->SetName("yPD");
  yPD->Fill(1);
  dataset->GetPointData()->AddArray(yPD);

  vtkNew<vtkDoubleArray> zPD;
  zPD->SetNumberOfValues(dataset->GetNumberOfPoints());
  zPD->SetName("zPD");
  zPD->Fill(2);
  dataset->GetPointData()->AddArray(zPD);

  vtkNew<vtkMergeVectorComponents> mergeFilterPD;
  mergeFilterPD->SetInputData(dataset);
  mergeFilterPD->SetXArrayName("xPD");
  mergeFilterPD->SetYArrayName("yPD");
  mergeFilterPD->SetZArrayName("zPD");
  mergeFilterPD->SetAttributeType(vtkDataObject::POINT);
  mergeFilterPD->SetOutputVectorName("vectorPD");
  mergeFilterPD->Update();

  auto outputPD = mergeFilterPD->GetPolyDataOutput();
  auto vectorPD = outputPD->GetPointData()->GetArray("vectorPD");

  for (vtkIdType i = 0; i < outputPD->GetNumberOfPoints(); ++i)
  {
    if (vectorPD->GetComponent(i, 0) != xPD->GetValue(i))
    {
      return EXIT_FAILURE;
    }
    if (vectorPD->GetComponent(i, 1) != yPD->GetValue(i))
    {
      return EXIT_FAILURE;
    }
    if (vectorPD->GetComponent(i, 2) != zPD->GetValue(i))
    {
      return EXIT_FAILURE;
    }
  }

  // test point data by creating 3 point data related arrays of different type and merging them
  vtkNew<vtkDoubleArray> xPD2;
  xPD2->SetNumberOfValues(dataset->GetNumberOfPoints());
  xPD2->SetName("xPD2");
  xPD2->Fill(0);
  dataset->GetPointData()->AddArray(xPD2);

  vtkNew<vtkIntArray> yPD2;
  yPD2->SetNumberOfValues(dataset->GetNumberOfPoints());
  yPD2->SetName("yPD2");
  yPD2->Fill(1);
  dataset->GetPointData()->AddArray(yPD2);

  vtkNew<vtkCharArray> zPD2;
  zPD2->SetNumberOfValues(dataset->GetNumberOfPoints());
  zPD2->SetName("zPD2");
  zPD2->Fill(2);
  dataset->GetPointData()->AddArray(zPD2);

  vtkNew<vtkMergeVectorComponents> mergeFilterPD2;
  mergeFilterPD2->SetInputData(dataset);
  mergeFilterPD2->SetXArrayName("xPD2");
  mergeFilterPD2->SetYArrayName("yPD2");
  mergeFilterPD2->SetZArrayName("zPD2");
  mergeFilterPD2->SetAttributeType(vtkDataObject::POINT);
  mergeFilterPD2->SetOutputVectorName("vectorPD2");
  mergeFilterPD2->Update();

  auto outputPD2 = mergeFilterPD2->GetPolyDataOutput();
  auto vectorPD2 = outputPD2->GetPointData()->GetArray("vectorPD2");

  for (vtkIdType i = 0; i < outputPD2->GetNumberOfPoints(); ++i)
  {
    if (vectorPD2->GetComponent(i, 0) != xPD2->GetValue(i))
    {
      return EXIT_FAILURE;
    }
    if (vectorPD2->GetComponent(i, 1) != yPD2->GetValue(i))
    {
      return EXIT_FAILURE;
    }
    if (vectorPD2->GetComponent(i, 2) != zPD2->GetValue(i))
    {
      return EXIT_FAILURE;
    }
  }

  // test cell data by creating 3 cell data related arrays and merging them
  vtkNew<vtkDoubleArray> xCD;
  xCD->SetNumberOfValues(dataset->GetNumberOfPolys());
  xCD->SetName("xCD");
  xCD->Fill(0);
  dataset->GetCellData()->AddArray(xCD);

  vtkNew<vtkDoubleArray> yCD;
  yCD->SetNumberOfValues(dataset->GetNumberOfPolys());
  yCD->SetName("yCD");
  yCD->Fill(1);
  dataset->GetCellData()->AddArray(yCD);

  vtkNew<vtkDoubleArray> zCD;
  zCD->SetNumberOfValues(dataset->GetNumberOfPolys());
  zCD->SetName("zCD");
  zCD->Fill(2);
  dataset->GetCellData()->AddArray(zCD);

  vtkNew<vtkMergeVectorComponents> mergeFilterCD;
  mergeFilterCD->SetInputData(dataset);
  mergeFilterCD->SetXArrayName("xCD");
  mergeFilterCD->SetYArrayName("yCD");
  mergeFilterCD->SetZArrayName("zCD");
  mergeFilterCD->SetAttributeType(vtkDataObject::CELL);
  mergeFilterCD->SetOutputVectorName("vectorCD");
  mergeFilterCD->Update();

  auto outputCD = mergeFilterCD->GetPolyDataOutput();
  auto vectorCD = outputCD->GetCellData()->GetArray("vectorCD");

  for (vtkIdType i = 0; i < outputCD->GetNumberOfCells(); ++i)
  {
    if (vectorCD->GetComponent(i, 0) != xCD->GetValue(i))
    {
      return EXIT_FAILURE;
    }
    if (vectorCD->GetComponent(i, 1) != yCD->GetValue(i))
    {
      return EXIT_FAILURE;
    }
    if (vectorCD->GetComponent(i, 2) != zCD->GetValue(i))
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
