/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGlyphSource2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkGlyphSource2D.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkSmartPointer.h>

int TestGlyphSource2D(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkGlyphSource2D> glyphSource
    = vtkSmartPointer<vtkGlyphSource2D>::New();
  glyphSource->SetColor(1.0, 1.0, 1.0);
  glyphSource->CrossOff();
  glyphSource->DashOff();
  glyphSource->FilledOn();
  glyphSource->SetGlyphTypeToVertex();

  glyphSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double center[3];
  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  glyphSource->SetCenter(center);

  randomSequence->Next();
  double rotationAngle = randomSequence->GetValue();
  glyphSource->SetRotationAngle(rotationAngle);

  randomSequence->Next();
  double scale = randomSequence->GetValue();
  glyphSource->SetScale(scale);

  glyphSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = glyphSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  glyphSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for(unsigned int i = 0; i < 3; ++i)
    {
    randomSequence->Next();
    center[i] = randomSequence->GetValue();
    }
  glyphSource->SetCenter(center);

  randomSequence->Next();
  rotationAngle = randomSequence->GetValue();
  glyphSource->SetRotationAngle(rotationAngle);

  randomSequence->Next();
  scale = randomSequence->GetValue();
  glyphSource->SetScale(scale);

  glyphSource->Update();

  polyData = glyphSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
