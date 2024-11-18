// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPCANormalEstimation.h"
#include "vtkSmartPointer.h"

#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

int TestPCANormalEstimation1Point();
int TestPCANormalEstimationKNN();
int TestPCANormalEstimationRadius();
int TestPCANormalEstimationKNNAndRadius();
int TestPCANormalEstimationGenerationMode();

int TestOutput(vtkDataArray* normals, const double output[3]);

int TestPCANormalEstimationModes(int, char*[])
{
  if (TestPCANormalEstimation1Point() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  if (TestPCANormalEstimationKNN() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  if (TestPCANormalEstimationRadius() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  if (TestPCANormalEstimationKNNAndRadius() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  if (TestPCANormalEstimationGenerationMode() != EXIT_SUCCESS)
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int TestPCANormalEstimation1Point()
{
  const double pt1[3] = { 0, 0, 0 };

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->InsertNextPoint(pt1);

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);

  vtkNew<vtkPCANormalEstimation> normalEstimation;
  normalEstimation->SetInputData(polyData);
  normalEstimation->SetSearchMode(0);
  normalEstimation->SetSampleSize(3);
  normalEstimation->SetNormalOrientation(3);
  normalEstimation->Update();

  vtkPointSet* output = vtkPointSet::SafeDownCast(normalEstimation->GetOutputDataObject(0));

  // Get output normals
  vtkDataArray* normals = output->GetPointData()->GetNormals();
  const double expectedOutput[3] = { 0, 1, 0 }; // 1 point default normal
  return TestOutput(normals, expectedOutput);
}

int TestPCANormalEstimationKNN()
{
  const double pt1[3] = { 0, 0, 0 };
  const double pt2[3] = { 1, 0, 0 };
  const double pt3[3] = { 0, 1, 0 };
  const double pt4[3] = { 2, 1, 0 };

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->InsertNextPoint(pt1);
  points->InsertNextPoint(pt2);
  points->InsertNextPoint(pt3);
  points->InsertNextPoint(pt4);

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);

  vtkNew<vtkPCANormalEstimation> normalEstimation;
  normalEstimation->SetInputData(polyData);
  normalEstimation->SetSearchMode(0);
  normalEstimation->SetSampleSize(3);
  normalEstimation->SetNormalOrientation(3);
  normalEstimation->Update();

  vtkPointSet* output = vtkPointSet::SafeDownCast(normalEstimation->GetOutputDataObject(0));

  // Get output normals
  vtkDataArray* normals = output->GetPointData()->GetNormals();
  const double expectedOutput[3] = { 0, 0, 1 };
  return TestOutput(normals, expectedOutput);
}

int TestPCANormalEstimationRadius()
{
  const double pt1[3] = { 0, 0, 0 };
  const double pt2[3] = { 0, 0, 1 };
  const double pt3[3] = { 0, 1, 0 };
  const double pt4[3] = { 0, 1, 2 };

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->InsertNextPoint(pt1);
  points->InsertNextPoint(pt2);
  points->InsertNextPoint(pt3);
  points->InsertNextPoint(pt4);

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);

  vtkNew<vtkPCANormalEstimation> normalEstimation;
  normalEstimation->SetInputData(polyData);
  normalEstimation->SetSearchMode(1);
  normalEstimation->SetRadius(2.5);
  normalEstimation->SetNormalOrientation(3);
  normalEstimation->Update();

  vtkPointSet* output = vtkPointSet::SafeDownCast(normalEstimation->GetOutputDataObject(0));

  // Get output normals
  vtkDataArray* normals = output->GetPointData()->GetNormals();
  const double expectedOutput[3] = { 1, 0, 0 };
  return TestOutput(normals, expectedOutput);
}

int TestPCANormalEstimationKNNAndRadius()
{
  const double pt1[3] = { 0, 0, 0 };
  const double pt2[3] = { 0, 0, 1 };
  const double pt3[3] = { 0, 1, 0 };
  const double pt4[3] = { 0, 0, 2 };
  const double pt5[3] = { 0, 1, 2 };

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->InsertNextPoint(pt1);
  points->InsertNextPoint(pt2);
  points->InsertNextPoint(pt3);
  points->InsertNextPoint(pt4);
  points->InsertNextPoint(pt5);

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);

  // Both modes should return the same result
  const double expectedOutput[3] = { 1, 0, 0 };

  // Test KNN mode with radius
  vtkNew<vtkPCANormalEstimation> normalEstimation;
  normalEstimation->SetInputData(polyData);
  normalEstimation->SetSearchMode(0);
  normalEstimation->SetRadius(1.5);
  normalEstimation->SetSampleSize(3);
  normalEstimation->SetNormalOrientation(3);
  normalEstimation->Update();

  vtkPointSet* output = vtkPointSet::SafeDownCast(normalEstimation->GetOutputDataObject(0));
  int result = TestOutput(output->GetPointData()->GetNormals(), expectedOutput);

  normalEstimation->SetSearchMode(1);
  normalEstimation->Update();

  output = vtkPointSet::SafeDownCast(normalEstimation->GetOutputDataObject(0));
  result |= TestOutput(output->GetPointData()->GetNormals(), expectedOutput);

  return result;
}

int TestOutput(vtkDataArray* normals, const double output[3])
{
  for (vtkIdType i = 0; i < normals->GetNumberOfTuples(); ++i)
  {
    double* normal = normals->GetTuple(i);
    if (normal[0] != output[0] || normal[1] != output[1] || normal[2] != output[2])
    {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

int TestPCANormalEstimationGenerationMode()
{
  const double pt1[3] = { 0, 0, 0 };
  const double pt2[3] = { 0, 0, 1 };
  const double pt3[3] = { 0, 1, 0 };
  const double pt4[3] = { 0, 0, 2 };
  const double pt5[3] = { 0, 1, 2 };

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataTypeToDouble();
  points->InsertNextPoint(pt1);
  points->InsertNextPoint(pt2);
  points->InsertNextPoint(pt3);
  points->InsertNextPoint(pt4);
  points->InsertNextPoint(pt5);

  // Create input polydata
  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints(points);

  // Create normals estimator
  vtkNew<vtkPCANormalEstimation> normalEstimation;
  normalEstimation->SetInputData(polyData);

  // Test number of cells with default CellGenerationMode
  normalEstimation->Update();
  vtkPolyData* output = vtkPolyData::SafeDownCast(normalEstimation->GetOutputDataObject(0));
  vtkIdType numCellsDefaultMode = output->GetNumberOfCells();

  // Test number of cells with CellGenerationMode set to vtkConvertToPointCloud::NO_CELLS
  normalEstimation->SetCellGenerationMode(vtkConvertToPointCloud::NO_CELLS);
  normalEstimation->Update();
  output = vtkPolyData::SafeDownCast(normalEstimation->GetOutputDataObject(0));
  vtkIdType numCellsNoCellsMode = output->GetNumberOfCells();

  // Test number of cells with CellGenerationMode set to vtkConvertToPointCloud::POLYVERTEX_CELL
  normalEstimation->SetCellGenerationMode(vtkConvertToPointCloud::POLYVERTEX_CELL);
  normalEstimation->Update();
  output = vtkPolyData::SafeDownCast(normalEstimation->GetOutputDataObject(0));
  vtkIdType numCellsPolyMode = output->GetNumberOfCells();

  // Test number of cells with CellGenerationMode set to vtkConvertToPointCloud::VERTEX_CELLS
  normalEstimation->SetCellGenerationMode(vtkConvertToPointCloud::VERTEX_CELLS);
  normalEstimation->Update();
  output = vtkPolyData::SafeDownCast(normalEstimation->GetOutputDataObject(0));
  vtkIdType numCellsVertexCellMode = output->GetNumberOfCells();

  if ((numCellsDefaultMode == 0) && (numCellsNoCellsMode == 0) && (numCellsPolyMode == 1) &&
    (numCellsVertexCellMode == polyData->GetPoints()->GetNumberOfPoints()))
  {
    return EXIT_SUCCESS;
  }
  else
  {
    return EXIT_FAILURE;
  }
}
