/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParametricFunctionSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkMinimalStandardRandomSequence.h>
#include <vtkParametricEllipsoid.h>
#include <vtkParametricFunctionSource.h>
#include <vtkSmartPointer.h>

int TestParametricFunctionSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence
    = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkParametricFunctionSource> parametricFunctionSource
    = vtkSmartPointer<vtkParametricFunctionSource>::New();
  parametricFunctionSource->SetUResolution(64);
  parametricFunctionSource->SetVResolution(64);
  parametricFunctionSource->SetWResolution(64);
  parametricFunctionSource->SetScalarModeToNone();
  parametricFunctionSource->GenerateTextureCoordinatesOff();

  parametricFunctionSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  vtkSmartPointer<vtkParametricEllipsoid> parametricEllipsoid
    = vtkSmartPointer<vtkParametricEllipsoid>::New();

  randomSequence->Next();
  double xRadius = randomSequence->GetValue();
  parametricEllipsoid->SetXRadius(xRadius);

  randomSequence->Next();
  double yRadius = randomSequence->GetValue();
  parametricEllipsoid->SetYRadius(yRadius);

  randomSequence->Next();
  double zRadius = randomSequence->GetValue();
  parametricEllipsoid->SetZRadius(zRadius);

  parametricFunctionSource->SetParametricFunction(parametricEllipsoid);

  parametricFunctionSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = parametricFunctionSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  parametricFunctionSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  randomSequence->Next();
  xRadius = randomSequence->GetValue();
  parametricEllipsoid->SetXRadius(xRadius);

  randomSequence->Next();
  yRadius = randomSequence->GetValue();
  parametricEllipsoid->SetYRadius(yRadius);

  randomSequence->Next();
  zRadius = randomSequence->GetValue();
  parametricEllipsoid->SetZRadius(zRadius);

  parametricFunctionSource->SetParametricFunction(parametricEllipsoid);

  parametricFunctionSource->Update();

  polyData = parametricFunctionSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
