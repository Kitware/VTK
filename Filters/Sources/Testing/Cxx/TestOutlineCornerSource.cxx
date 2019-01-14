/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOutlineCornerSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkMinimalStandardRandomSequence.h>
#include <vtkOutlineCornerSource.h>
#include <vtkSmartPointer.h>

int TestOutlineCornerSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkOutlineCornerSource> outlineCornerSource
    = vtkSmartPointer<vtkOutlineCornerSource>::New();
  outlineCornerSource->SetBoxTypeToAxisAligned();
  outlineCornerSource->GenerateFacesOff();

  outlineCornerSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  double bounds[6];
  for(unsigned int i = 0; i < 6; ++i)
  {
    randomSequence->Next();
    bounds[i] = randomSequence->GetValue();
  }
  if(bounds[0] > bounds[3])
  {
    std::swap(bounds[0], bounds[3]);
  }
  if(bounds[1] > bounds[4])
  {
    std::swap(bounds[1], bounds[4]);
  }
  if(bounds[2] > bounds[5])
  {
    std::swap(bounds[2], bounds[5]);
  }
  outlineCornerSource->SetBounds(bounds);

  randomSequence->Next();
  double cornerFactor = randomSequence->GetValue();
  outlineCornerSource->SetCornerFactor(cornerFactor);

  outlineCornerSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = outlineCornerSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  outlineCornerSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  for(unsigned int i = 0; i < 6; ++i)
  {
    randomSequence->Next();
    bounds[i] = randomSequence->GetValue();
  }
  if(bounds[0] > bounds[3])
  {
    std::swap(bounds[0], bounds[3]);
  }
  if(bounds[1] > bounds[4])
  {
    std::swap(bounds[1], bounds[4]);
  }
  if(bounds[2] > bounds[5])
  {
    std::swap(bounds[2], bounds[5]);
  }
  outlineCornerSource->SetBounds(bounds);

  randomSequence->Next();
  cornerFactor = randomSequence->GetValue();
  outlineCornerSource->SetCornerFactor(cornerFactor);

  outlineCornerSource->Update();

  polyData = outlineCornerSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
