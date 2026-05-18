// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkAppendPolyData.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkPolyLine.h>
#include <vtkPolygon.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataWriter.h>

#include <iostream>

namespace
{
int TestAppendPolyDataBase()
{
  vtkSmartPointer<vtkPoints> pointsArray0 = vtkSmartPointer<vtkPoints>::New();
  pointsArray0->InsertNextPoint(0.0, 0.0, 0.0);
  pointsArray0->InsertNextPoint(1.0, 1.0, 1.0);

  vtkSmartPointer<vtkPoints> pointsArray1 = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
  vtkIdType pointIds[1];
  pointIds[0] = pointsArray1->InsertNextPoint(5.0, 5.0, 5.0);
  vertices->InsertNextCell(1, pointIds);
  pointIds[0] = pointsArray1->InsertNextPoint(6.0, 6.0, 6.0);
  vertices->InsertNextCell(1, pointIds);

  vtkSmartPointer<vtkPolyData> inputPolyData0 = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points0 = vtkSmartPointer<vtkPoints>::New();
  points0->SetDataType(VTK_FLOAT);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  vtkSmartPointer<vtkXMLPolyDataWriter> inputWriter0 = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  inputWriter0->SetFileName("inputpolydata0.vtp");
  inputWriter0->SetInputData(inputPolyData0);
  inputWriter0->Write();

  vtkSmartPointer<vtkPolyData> inputPolyData1 = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points1 = vtkSmartPointer<vtkPoints>::New();
  points1->SetDataType(VTK_FLOAT);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);
  inputPolyData1->SetVerts(vertices);

  vtkSmartPointer<vtkXMLPolyDataWriter> inputWriter1 = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  inputWriter1->SetFileName("inputpolydata1.vtp");
  inputWriter1->SetInputData(inputPolyData1);
  inputWriter1->Write();

  vtkSmartPointer<vtkAppendPolyData> appendPolyData = vtkSmartPointer<vtkAppendPolyData>::New();
  appendPolyData->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);

  appendPolyData->AddInputData(inputPolyData0);
  appendPolyData->AddInputData(inputPolyData1);

  appendPolyData->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = appendPolyData->GetOutput();
  vtkSmartPointer<vtkXMLPolyDataWriter> outputWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  outputWriter->SetFileName("outputpolydata.vtp");
  outputWriter->SetInputData(outputPolyData);
  outputWriter->Write();

  vtkSmartPointer<vtkAppendPolyData> appendPolyDataWithNoCells =
    vtkSmartPointer<vtkAppendPolyData>::New();
  appendPolyDataWithNoCells->SetOutputPointsPrecision(vtkAlgorithm::DEFAULT_PRECISION);

  appendPolyDataWithNoCells->AddInputData(inputPolyData0);
  appendPolyDataWithNoCells->AddInputData(inputPolyData0);

  appendPolyDataWithNoCells->Update();

  vtkSmartPointer<vtkPolyData> outputPolyDataWithNoCells = appendPolyDataWithNoCells->GetOutput();
  vtkSmartPointer<vtkXMLPolyDataWriter> outputWriterWithNoCells =
    vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  outputWriterWithNoCells->SetFileName("outputpolydataWithNoCells.vtp");
  outputWriterWithNoCells->SetInputData(outputPolyDataWithNoCells);
  outputWriterWithNoCells->Write();

  if (outputPolyData->GetNumberOfPoints() !=
    inputPolyData0->GetNumberOfPoints() + inputPolyData1->GetNumberOfPoints())
  {
    std::cerr << "ERROR: The output number of points should be "
              << inputPolyData0->GetNumberOfPoints() + inputPolyData1->GetNumberOfPoints()
              << " but is " << outputPolyData->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyData->GetNumberOfCells() !=
    inputPolyData0->GetNumberOfCells() + inputPolyData1->GetNumberOfCells())
  {
    std::cerr << "ERROR: The output number of cells should be "
              << inputPolyData0->GetNumberOfCells() + inputPolyData1->GetNumberOfCells()
              << " but is " << outputPolyData->GetNumberOfCells() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyData->GetPoints()->GetDataType() != VTK_FLOAT)
  {
    std::cerr << "ERROR: The output points data should be " << VTK_FLOAT << " but is "
              << outputPolyData->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyDataWithNoCells->GetNumberOfPoints() != inputPolyData0->GetNumberOfPoints() * 2)
  {
    std::cerr << "ERROR: The output number of points should be "
              << inputPolyData0->GetNumberOfPoints() * 2 << " but is "
              << outputPolyDataWithNoCells->GetNumberOfPoints() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyDataWithNoCells->GetNumberOfCells() != 0)
  {
    std::cerr << "ERROR The output number of cells should be 0 but is "
              << " but is " << outputPolyDataWithNoCells->GetNumberOfCells() << std::endl;
    return EXIT_FAILURE;
  }

  if (outputPolyDataWithNoCells->GetPoints()->GetDataType() != VTK_FLOAT)
  {
    std::cerr << "ERROR: The output points data type should be " << VTK_FLOAT << " but is "
              << outputPolyDataWithNoCells->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points0->SetDataType(VTK_DOUBLE);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  appendPolyData->Update();

  outputPolyData = appendPolyData->GetOutput();

  if (outputPolyData->GetPoints()->GetDataType() != VTK_DOUBLE)
  {
    std::cerr << "ERROR: The output points data type should be " << VTK_DOUBLE << " but is "
              << outputPolyData->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points1->SetDataType(VTK_DOUBLE);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != VTK_DOUBLE)
  {
    std::cerr << "ERROR: The output points data type should be " << VTK_DOUBLE << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  appendPolyData->SetOutputPointsPrecision(vtkAlgorithm::SINGLE_PRECISION);

  points0->SetDataType(VTK_FLOAT);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  points1->SetDataType(VTK_FLOAT);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != VTK_FLOAT)
  {
    std::cerr << "ERROR: The output points data type should be " << VTK_FLOAT << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points0->SetDataType(VTK_DOUBLE);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != VTK_FLOAT)
  {
    std::cerr << "The output points data type should be " << VTK_FLOAT << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points1->SetDataType(VTK_DOUBLE);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != VTK_FLOAT)
  {
    std::cerr << "The output points data type should be " << VTK_FLOAT << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  appendPolyData->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);

  points0->SetDataType(VTK_FLOAT);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  points1->SetDataType(VTK_FLOAT);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != VTK_DOUBLE)
  {
    std::cerr << "The output points data type should be " << VTK_DOUBLE << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points0->SetDataType(VTK_DOUBLE);
  points0->DeepCopy(pointsArray0);
  inputPolyData0->SetPoints(points0);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != VTK_DOUBLE)
  {
    std::cerr << "The output points data type should be " << VTK_DOUBLE << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  points1->SetDataType(VTK_DOUBLE);
  points1->DeepCopy(pointsArray1);
  inputPolyData1->SetPoints(points1);

  appendPolyData->Update();

  if (appendPolyData->GetOutput()->GetPoints()->GetDataType() != VTK_DOUBLE)
  {
    std::cerr << "The output points data type should be " << VTK_DOUBLE << " but is "
              << appendPolyData->GetOutput()->GetPoints()->GetDataType() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestImplicitAppendPolyData()
{
  // Create a poly data with different cell types to check if the order of the cell
  // types is respected with implicit arrays
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 0.0, 0.0);
  points->InsertNextPoint(1.0, 1.0, 0.0);
  points->InsertNextPoint(0.0, 1.0, 0.0);

  // Create the polygon
  vtkNew<vtkPolygon> polygon;
  polygon->GetPointIds()->SetNumberOfIds(4); // make a quad
  polygon->GetPointIds()->SetId(0, 0);
  polygon->GetPointIds()->SetId(1, 1);
  polygon->GetPointIds()->SetId(2, 2);
  polygon->GetPointIds()->SetId(3, 3);

  // Add the polygon to a list of polygons
  vtkNew<vtkCellArray> polygons;
  polygons->InsertNextCell(polygon);

  vtkNew<vtkPolyLine> polyLine;
  polyLine->GetPointIds()->SetNumberOfIds(4);
  for (unsigned int i = 0; i < 4; i++)
  {
    polyLine->GetPointIds()->SetId(i, i);
  }

  // Create a cell array to store the lines in and add the lines to it.
  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(polyLine);

  // Create a polydata to store everything in.
  vtkNew<vtkPolyData> polyData;

  // Add the points to the dataset.
  polyData->SetPoints(points);

  // Add the lines to the dataset.
  polyData->SetLines(cells);

  // Add the polys to the dataset.
  polyData->SetPolys(polygons);

  vtkNew<vtkDoubleArray> cellArray;
  cellArray->SetName("cellArray");
  cellArray->SetNumberOfTuples(2);
  cellArray->SetTuple1(0, 0);
  cellArray->SetTuple1(1, 1);

  polyData->GetCellData()->AddArray(cellArray);

  vtkNew<vtkPolyData> polyDataCopied;
  polyDataCopied->DeepCopy(polyData);

  vtkNew<vtkAppendPolyData> appendPolyDataImplicit;

  appendPolyDataImplicit->AddInputData(polyData);
  appendPolyDataImplicit->AddInputData(polyDataCopied);
  appendPolyDataImplicit->SetUseImplicitArray(true);

  appendPolyDataImplicit->Update();

  vtkDataArray* outputArray =
    appendPolyDataImplicit->GetOutput()->GetCellData()->GetArray("cellArray");
  if (outputArray->GetTuple1(0) != 0 || outputArray->GetTuple1(1) != 0 ||
    outputArray->GetTuple1(2) != 1 || outputArray->GetTuple1(3) != 1)
  {
    std::cerr << "The output cell data array is incorrect" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
};

int TestAppendPolyData(int, char*[])
{
  if (::TestAppendPolyDataBase() == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  if (::TestImplicitAppendPolyData())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
