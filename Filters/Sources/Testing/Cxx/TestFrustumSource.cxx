/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFrustumSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCamera.h>
#include <vtkFrustumSource.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPlanes.h>
#include <vtkSmartPointer.h>

int TestFrustumSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkFrustumSource> frustumSource
    = vtkSmartPointer<vtkFrustumSource>::New();
  frustumSource->ShowLinesOn();

  frustumSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  randomSequence->Next();
  double linesLength = randomSequence->GetValue();
  frustumSource->SetLinesLength(linesLength);

  vtkSmartPointer<vtkCamera> camera = vtkSmartPointer<vtkCamera>::New();

  double position[3];
  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    position[i] = randomSequence->GetValue();
  }
  camera->SetPosition(position);
  double focalPoint[3];
  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    focalPoint[i] = randomSequence->GetValue();
  }
  camera->SetFocalPoint(focalPoint);
  double planeCoefficients[24];
  camera->GetFrustumPlanes(1.0, planeCoefficients);

  vtkSmartPointer<vtkPlanes> planes = vtkSmartPointer<vtkPlanes>::New();
  planes->SetFrustumPlanes(planeCoefficients);
  frustumSource->SetPlanes(planes);

  frustumSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = frustumSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  frustumSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  linesLength = randomSequence->GetValue();
  frustumSource->SetLinesLength(linesLength);

  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    position[i] = randomSequence->GetValue();
  }
  camera->SetPosition(position);
  for(unsigned int i = 0; i < 3; ++i)
  {
    randomSequence->Next();
    focalPoint[i] = randomSequence->GetValue();
  }
  camera->SetFocalPoint(focalPoint);
  camera->GetFrustumPlanes(1.0, planeCoefficients);

  planes->SetFrustumPlanes(planeCoefficients);
  frustumSource->SetPlanes(planes);

  frustumSource->Update();

  polyData = frustumSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
