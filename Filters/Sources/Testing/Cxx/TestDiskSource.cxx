/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiskSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkDiskSource.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>

int TestDiskSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkDiskSource> diskSource
    = vtkSmartPointer<vtkDiskSource>::New();
  diskSource->SetCircumferentialResolution(8);
  diskSource->SetRadialResolution(8);

  diskSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  randomSequence->Next();
  double innerRadius = randomSequence->GetValue();

  randomSequence->Next();
  double outerRadius = randomSequence->GetValue();

  if(innerRadius > outerRadius)
    {
    std::swap(innerRadius, outerRadius);
    }

  diskSource->SetInnerRadius(innerRadius);
  diskSource->SetOuterRadius(outerRadius);

  diskSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = diskSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  diskSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  innerRadius = randomSequence->GetValue();

  randomSequence->Next();
  outerRadius = randomSequence->GetValue();

  if(innerRadius > outerRadius)
    {
    std::swap(innerRadius, outerRadius);
    }

  diskSource->SetInnerRadius(innerRadius);
  diskSource->SetOuterRadius(outerRadius);

  diskSource->Update();

  polyData = diskSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
