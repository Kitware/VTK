/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDecimatePro.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkCellArray.h>
#include <vtkDecimatePro.h>
#include <vtkSmartPointer.h>

namespace
{
void InitializePolyData(vtkPolyData *polyData, int dataType)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(dataType);
  points->InsertNextPoint(-1.40481710, -0.03868163, -1.01241910);
  points->InsertNextPoint(-1.41186166, 0.29086590, 0.96023101);
  points->InsertNextPoint(-0.13218975, -1.22439861, 1.21793830);
  points->InsertNextPoint(-0.12514521, -1.55394614, -0.75471181);
  points->InsertNextPoint(0.13218975, 1.22439861, -1.21793830);
  points->InsertNextPoint(0.12514521, 1.55394614, 0.75471181);
  points->InsertNextPoint(1.40481710, 0.03868163, 1.01241910);
  points->InsertNextPoint(1.41186166, -0.29086590, -0.96023101);
  points->Squeeze();

  polyData->SetPoints(points);

  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
  verts->InsertNextCell(8);
  for(unsigned int i = 0; i < 8; ++i)
  {
    verts->InsertCellPoint(i);
  }
  verts->Squeeze();

  polyData->SetVerts(verts);

  vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
  vtkIdType pointIds[3];
  pointIds[0] = 0;
  pointIds[1] = 1;
  pointIds[2] = 2;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 2;
  pointIds[2] = 3;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 3;
  pointIds[2] = 7;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 4;
  pointIds[2] = 5;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 5;
  pointIds[2] = 1;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 7;
  pointIds[2] = 4;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 1;
  pointIds[1] = 2;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 1;
  pointIds[1] = 6;
  pointIds[2] = 5;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 2;
  pointIds[1] = 3;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 3;
  pointIds[1] = 7;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 4;
  pointIds[1] = 5;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 4;
  pointIds[1] = 6;
  pointIds[2] = 7;
  polys->InsertNextCell(3, pointIds);
  polys->Squeeze();

  polyData->SetPolys(polys);
}

int DecimatePro(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkPolyData> inputPolyData
    = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkDecimatePro> decimatePro
    = vtkSmartPointer<vtkDecimatePro>::New();
  decimatePro->SetOutputPointsPrecision(outputPointsPrecision);
  decimatePro->SetInputData(inputPolyData);

  decimatePro->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = decimatePro->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}
}

int TestDecimatePro(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  int dataType = DecimatePro(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if(dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = DecimatePro(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if(dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
