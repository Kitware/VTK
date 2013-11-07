/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPlatonicSolidSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkPlatonicSolidSource.h>
#include <vtkSmartPointer.h>

int TestPlatonicSolidSource(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  vtkSmartPointer<vtkPlatonicSolidSource> platonicSolidSource
    = vtkSmartPointer<vtkPlatonicSolidSource>::New();

  platonicSolidSource->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  platonicSolidSource->SetSolidTypeToCube();
  platonicSolidSource->Update();

  vtkSmartPointer<vtkPolyData> polyData = platonicSolidSource->GetOutput();
  vtkSmartPointer<vtkPoints> points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetSolidTypeToDodecahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetSolidTypeToIcosahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetSolidTypeToOctahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetSolidTypeToTetrahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_FLOAT)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  platonicSolidSource->SetSolidTypeToCube();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetSolidTypeToDodecahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetSolidTypeToIcosahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetSolidTypeToOctahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  platonicSolidSource->SetSolidTypeToTetrahedron();
  platonicSolidSource->Update();

  polyData = platonicSolidSource->GetOutput();
  points = polyData->GetPoints();

  if(points->GetDataType() != VTK_DOUBLE)
    {
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
