/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestMergeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkMergeFilter.h"

#include "vtkCommand.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkDataObjectGenerator.h"
#include "vtkTestErrorObserver.h"
#include "vtkMathUtilities.h"

#include <cstdio>
#include <sstream>

static vtkSmartPointer<vtkPointData> MakePointData(unsigned int numberOfPoints);
static vtkSmartPointer<vtkCellData> MakeCellData(unsigned int numberOfCells);

int UnitTestMergeFilter (int, char*[])
{
  int status = 0;

  vtkSmartPointer<vtkPolyData> polyData0 =
    vtkSmartPointer<vtkPolyData>::New();

  // Test empty input
  std::cout << "Testing empty input...";
  std::ostringstream print0;
  vtkSmartPointer<vtkMergeFilter> merge0 =
    vtkSmartPointer<vtkMergeFilter>::New();
  merge0->Print(print0);

  // Check for null inputs
  int status0 = 0;
  if (merge0->GetGeometry() != NULL)
  {
    std::cout << std::endl << "  GetGeometry() expected NULL" << std::endl;
    status0++;
  }
  if (merge0->GetGeometry() != NULL)
  {
    status++;
    std::cout << std::endl << "  GetGeometry() expected NULL" << std::endl;
  }
  status0 = 0;

  if (merge0->GetScalars() != NULL)
  {
    status++;
    std::cout << std::endl << "  GetScalars() expected NULL" << std::endl;
  }
  status0 = 0;

  if (merge0->GetVectors() != NULL)
  {
    status++;
    std::cout << std::endl << "  GetVectors( ) expected NULL" << std::endl;
  }
  status0 = 0;

  if (merge0->GetNormals() != NULL)
  {
    status++;
    std::cout << std::endl << "  GetNormals() expected NULL" << std::endl;
  }
  status0 = 0;

  if (merge0->GetTCoords() != NULL)
  {
    status++;
    std::cout << std::endl << "  GetTCoords() expected NULL" << std::endl;
  }
  status0 = 0;

  if (merge0->GetTensors() != NULL)
  {
    status++;
    std::cout << std::endl << "  GetTensorsd() expected NULL" << std::endl;
  }
  status0 = 0;

  merge0->SetGeometryInputData(polyData0);
  vtkSmartPointer<vtkTest::ErrorObserver>  warningObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();
  merge0->AddObserver(vtkCommand::WarningEvent, warningObserver);
  merge0->Update();
  status0 += warningObserver->CheckWarningMessage("Nothing to merge!");
  if (status0)
  {
    status++;
    std::cout << "FAILED." << std::endl;
  }
  else
  {
    std::cout << "PASSED." << std::endl;
  }

  // Create a PolyData
  vtkSmartPointer<vtkDataObjectGenerator> dog1 =
    vtkSmartPointer<vtkDataObjectGenerator>::New();
  dog1->SetProgram("PD1");
  dog1->Update();
  vtkSmartPointer<vtkPolyData> polyData =
    vtkPolyData::SafeDownCast(dog1->GetOutput());

  vtkSmartPointer<vtkPolyData> polyData2 =
    vtkPolyData::SafeDownCast(dog1->GetOutput());

  vtkSmartPointer<vtkPointData> pointData =
    MakePointData(polyData2->GetNumberOfPoints());
  vtkSmartPointer<vtkCellData> cellData =
    MakeCellData(polyData2->GetNumberOfCells());
  polyData2->GetPointData()->SetScalars(pointData->GetScalars());
  polyData2->GetPointData()->SetNormals(pointData->GetNormals());
  polyData2->GetPointData()->SetVectors(pointData->GetVectors());
  polyData2->GetPointData()->SetTCoords(pointData->GetTCoords());
  polyData2->GetPointData()->SetTensors(pointData->GetTensors());
  polyData2->GetCellData()->SetScalars(cellData->GetScalars());
  polyData2->GetCellData()->SetNormals(cellData->GetNormals());
  polyData2->GetCellData()->SetVectors(cellData->GetVectors());
  polyData2->GetCellData()->SetTCoords(cellData->GetTCoords());
  polyData2->GetCellData()->SetTensors(cellData->GetTensors());

  vtkSmartPointer<vtkMergeFilter> merge1 =
    vtkSmartPointer<vtkMergeFilter>::New();
  merge1->SetGeometryInputData(polyData);
  merge1->SetScalarsData(polyData2);
  merge1->SetNormalsData(polyData2);
  merge1->SetVectorsData(polyData2);
  merge1->SetTCoordsData(polyData2);
  merge1->SetTensorsData(polyData2);
  merge1->AddField("Point X", polyData2);
  merge1->AddField("Point Y", polyData2);
  merge1->AddField("Point Z", polyData2);
  merge1->AddField("Cell Ids", polyData2);
  // empty name for coverage
  merge1->AddField("", polyData2);

  merge1->Update();

  // Now verify that the arrays have been merged
  if (merge1->GetGeometry() != vtkDataSet::SafeDownCast(polyData))
  {
    std::cout << "ERROR: Input geometry does not match" << std::endl;
    status++;
  }
  if (merge1->GetScalars()->GetPointData()->GetScalars() !=
      polyData2->GetPointData()->GetScalars())
  {
    std::cout << "ERROR: Scalars not merged" << std::endl;
    status++;
  }
  if (merge1->GetVectors()->GetPointData()->GetVectors() !=
      polyData2->GetPointData()->GetVectors())
  {
    std::cout << "ERROR: Vectors not merged" << std::endl;
    status++;
  }
  if (merge1->GetNormals()->GetPointData()->GetNormals() !=
      polyData2->GetPointData()->GetNormals())
  {
    std::cout << "ERROR: Normals not merged" << std::endl;
    status++;
  }
  if (merge1->GetTCoords()->GetPointData()->GetTCoords() !=
      polyData2->GetPointData()->GetTCoords())
  {
    std::cout << "ERROR: TCoords not merged" << std::endl;
    status++;
  }
  if (merge1->GetTensors()->GetPointData()->GetTensors() !=
      polyData2->GetPointData()->GetTensors())
  {
    std::cout << "ERROR: Tensors not merged" << std::endl;
    status++;
  }
  merge1->AddObserver(vtkCommand::WarningEvent, warningObserver);
  vtkSmartPointer<vtkPointData> pointData2 =
    MakePointData(100);
  vtkSmartPointer<vtkCellData> cellData2 =
    MakeCellData(100);
  polyData2->GetPointData()->SetScalars(pointData2->GetScalars());
  polyData2->GetPointData()->SetNormals(pointData2->GetNormals());
  polyData2->GetPointData()->SetVectors(pointData2->GetVectors());
  polyData2->GetPointData()->SetTCoords(pointData2->GetTCoords());
  polyData2->GetPointData()->SetTensors(pointData2->GetTensors());
  polyData2->GetCellData()->SetScalars(cellData2->GetScalars());
  polyData2->GetCellData()->SetNormals(cellData2->GetNormals());
  polyData2->GetCellData()->SetVectors(cellData2->GetVectors());
  polyData2->GetCellData()->SetTCoords(cellData2->GetTCoords());
  polyData2->GetCellData()->SetTensors(cellData2->GetTensors());

  merge1->SetGeometryInputData(polyData);
  merge1->SetScalarsData(polyData2);
  merge1->SetNormalsData(polyData2);
  merge1->SetVectorsData(polyData2);
  merge1->SetTCoordsData(polyData2);
  merge1->SetTensorsData(polyData2);
  merge1->AddField("Point X", polyData2);
  merge1->AddField("Point Y", polyData2);
  merge1->AddField("Point Z", polyData2);
  merge1->AddField("Cell Ids", polyData2);
  // empty name for coverage
  merge1->AddField("", polyData2);

  merge1->Update();
  status += warningObserver->CheckWarningMessage("cannot be merged");
  if (status)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}

vtkSmartPointer<vtkPointData> MakePointData(unsigned int numberOfPoints)
{
  vtkSmartPointer<vtkPointData> pointData =
    vtkSmartPointer<vtkPointData>::New();
  vtkSmartPointer<vtkFloatArray> scalarsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  scalarsArray->SetNumberOfTuples(numberOfPoints);
  scalarsArray->SetNumberOfComponents(1);
  scalarsArray->SetName("Scalars:floatArray");
  vtkSmartPointer<vtkFloatArray> normalsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  normalsArray->SetNumberOfTuples(numberOfPoints);
  normalsArray->SetNumberOfComponents(3);
  normalsArray->SetName("Normals:floatArray");
  vtkSmartPointer<vtkFloatArray> vectorsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  vectorsArray->SetNumberOfTuples(numberOfPoints);
  vectorsArray->SetNumberOfComponents(3);
  vectorsArray->SetName("Vectors:floatArray");
  vtkSmartPointer<vtkFloatArray> tcoordsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  tcoordsArray->SetNumberOfTuples(numberOfPoints);
  tcoordsArray->SetNumberOfComponents(2);
  tcoordsArray->SetName("Tcoords:floatArray");
  vtkSmartPointer<vtkFloatArray> tensorsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  tensorsArray->SetNumberOfTuples(numberOfPoints);
  tensorsArray->SetNumberOfComponents(9);
  tensorsArray->SetName("Tensors:floatArray");
  for (unsigned int i = 0; i < numberOfPoints; ++i)
  {
    scalarsArray->InsertTuple1(i, i);
    normalsArray->InsertTuple3(i, i, i+1, i+2);
    vectorsArray->InsertTuple3(i, i+1, i+2, i+3);
    tcoordsArray->InsertTuple2(i, i*2, i*3);
    tensorsArray->InsertTuple9(i, i, i+1, i+2, i+3, i+4, i+5,i+6, i+7,i+8);
  }
  pointData->SetScalars(scalarsArray);
  pointData->SetNormals(normalsArray);
  pointData->SetVectors(vectorsArray);
  pointData->SetTCoords(tcoordsArray);
  pointData->SetTensors(tensorsArray);
  return pointData;
}

vtkSmartPointer<vtkCellData> MakeCellData(unsigned int numberOfCells)
{
  vtkSmartPointer<vtkCellData> cellData =
    vtkSmartPointer<vtkCellData>::New();
  vtkSmartPointer<vtkFloatArray> scalarsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  scalarsArray->SetNumberOfTuples(numberOfCells);
  scalarsArray->SetNumberOfComponents(1);
  scalarsArray->SetName("Scalars:floatArray");
  vtkSmartPointer<vtkFloatArray> normalsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  normalsArray->SetNumberOfTuples(numberOfCells);
  normalsArray->SetNumberOfComponents(3);
  normalsArray->SetName("Normals:floatArray");
  vtkSmartPointer<vtkFloatArray> vectorsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  vectorsArray->SetNumberOfTuples(numberOfCells);
  vectorsArray->SetNumberOfComponents(3);
  vectorsArray->SetName("Vectors:floatArray");
  vtkSmartPointer<vtkFloatArray> tcoordsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  tcoordsArray->SetNumberOfTuples(numberOfCells);
  tcoordsArray->SetNumberOfComponents(2);
  tcoordsArray->SetName("Tcoords:floatArray");
  vtkSmartPointer<vtkFloatArray> tensorsArray =
    vtkSmartPointer<vtkFloatArray>::New();
  tensorsArray->SetNumberOfTuples(numberOfCells);
  tensorsArray->SetNumberOfComponents(9);
  tensorsArray->SetName("Tensors:floatArray");
  for (unsigned int i = 0; i < numberOfCells; ++i)
  {
    scalarsArray->InsertTuple1(i, i);
    normalsArray->InsertTuple3(i, i, i+1, i+2);
    vectorsArray->InsertTuple3(i, i+1, i+2, i+3);
    tcoordsArray->InsertTuple2(i, i*2, i*3);
    tensorsArray->InsertTuple9(i, i, i+1, i+2, i+3, i+4, i+5,i+6, i+7,i+8);
  }
  cellData->SetScalars(scalarsArray);
  cellData->SetNormals(normalsArray);
  cellData->SetVectors(vectorsArray);
  cellData->SetTCoords(tcoordsArray);
  cellData->SetTensors(tensorsArray);
  return cellData;
}
