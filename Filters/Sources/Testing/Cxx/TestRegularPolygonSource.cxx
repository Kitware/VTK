/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRegularPolygonSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkMinimalStandardRandomSequence.h>
#include <vtkRegularPolygonSource.h>
#include <vtkSmartPointer.h>

int TestRegularPolygonSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkRegularPolygonSource> regularPolygonSource
    = vtkSmartPointer<vtkRegularPolygonSource>::New();
  regularPolygonSource->SetNumberOfSides(8);
  regularPolygonSource->GeneratePolygonOn();
  regularPolygonSource->GeneratePolylineOn();

  regularPolygonSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  randomSequence->Next();
  double radius = randomSequence->GetValue();
  regularPolygonSource->SetRadius(radius);

  double center[3];
  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  regularPolygonSource->SetCenter(center);

  regularPolygonSource->Update();

  double normal[3];
  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    normal[i] = randomSequence->GetValue();
    }
  regularPolygonSource->SetNormal(normal);

  regularPolygonSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = regularPolygonSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  regularPolygonSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  radius = randomSequence->GetValue();
  regularPolygonSource->SetRadius(radius);

  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  regularPolygonSource->SetCenter(center);

  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    normal[i] = randomSequence->GetValue();
    }
  regularPolygonSource->SetNormal(normal);

  regularPolygonSource->Update();

  polyData = regularPolygonSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
