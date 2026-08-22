// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCellArray.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkHexahedron.h>
#include <vtkMaskPoints.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTriangleFilter.h>
#include <vtkUnstructuredGrid.h>

#include <cmath>
#include <iostream>

namespace
{
void InitializePolyData(vtkPolyData* polyData, int dataType)
{
  vtkSmartPointer<vtkMinimalStandardRandomSequence> randomSequence =
    vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();
  randomSequence->SetSeed(1);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
  verts->InsertNextCell(4);

  if (dataType == VTK_DOUBLE)
  {
    points->SetDataType(VTK_DOUBLE);
    for (unsigned int i = 0; i < 4; ++i)
    {
      double point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = randomSequence->GetValue();
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }
  else
  {
    points->SetDataType(VTK_FLOAT);
    for (unsigned int i = 0; i < 4; ++i)
    {
      float point[3];
      for (unsigned int j = 0; j < 3; ++j)
      {
        randomSequence->Next();
        point[j] = static_cast<float>(randomSequence->GetValue());
      }
      verts->InsertCellPoint(points->InsertNextPoint(point));
    }
  }

  points->Squeeze();
  polyData->SetPoints(points);
  verts->Squeeze();
  polyData->SetVerts(verts);
}

int MaskPoints(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkMaskPoints> maskPoints = vtkSmartPointer<vtkMaskPoints>::New();
  maskPoints->SetOutputPointsPrecision(outputPointsPrecision);
  maskPoints->SetMaximumNumberOfPoints(2);
  maskPoints->SetRandomModeType(0);
  maskPoints->RandomModeOn();
  maskPoints->SetInputData(inputPolyData);

  maskPoints->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = maskPoints->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}

// Adds an array that stores the id of each point, so that a mismatch between the
// output points and the point data that is copied to them can be detected.
void AddPointIdArray(vtkDataSet* dataSet)
{
  vtkNew<vtkDoubleArray> pointIds;
  pointIds->SetName("PointId");
  pointIds->SetNumberOfComponents(1);
  pointIds->SetNumberOfTuples(dataSet->GetNumberOfPoints());
  for (vtkIdType pointId = 0; pointId < dataSet->GetNumberOfPoints(); ++pointId)
  {
    pointIds->SetValue(pointId, static_cast<double>(pointId));
  }
  dataSet->GetPointData()->AddArray(pointIds);
}

// Id of the point at the given coordinates in the grid that CreateHexahedralGrid builds.
vtkIdType PointIdOfCoordinates(int numberOfPointsPerAxis, int i, int j, int k)
{
  return i + numberOfPointsPerAxis * (j + numberOfPointsPerAxis * k);
}

// Grid of hexahedra: it has 3D cells, which the volume sampling mode requires.
vtkSmartPointer<vtkUnstructuredGrid> CreateHexahedralGrid(int numberOfPointsPerAxis)
{
  vtkNew<vtkPoints> points;
  for (int k = 0; k < numberOfPointsPerAxis; ++k)
  {
    for (int j = 0; j < numberOfPointsPerAxis; ++j)
    {
      for (int i = 0; i < numberOfPointsPerAxis; ++i)
      {
        points->InsertNextPoint(i, j, k);
      }
    }
  }

  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);
  for (int k = 0; k < numberOfPointsPerAxis - 1; ++k)
  {
    for (int j = 0; j < numberOfPointsPerAxis - 1; ++j)
    {
      for (int i = 0; i < numberOfPointsPerAxis - 1; ++i)
      {
        const int corners[8][3] = { { i, j, k }, { i + 1, j, k }, { i + 1, j + 1, k },
          { i, j + 1, k }, { i, j, k + 1 }, { i + 1, j, k + 1 }, { i + 1, j + 1, k + 1 },
          { i, j + 1, k + 1 } };
        vtkNew<vtkHexahedron> hexahedron;
        for (int corner = 0; corner < 8; ++corner)
        {
          hexahedron->GetPointIds()->SetId(corner,
            PointIdOfCoordinates(
              numberOfPointsPerAxis, corners[corner][0], corners[corner][1], corners[corner][2]));
        }
        grid->InsertNextCell(hexahedron->GetCellType(), hexahedron->GetPointIds());
      }
    }
  }
  AddPointIdArray(grid);
  return grid;
}

// Triangulated sphere: it has 2D cells, which the surface sampling mode requires.
vtkSmartPointer<vtkPolyData> CreateTriangulatedSphere()
{
  vtkNew<vtkSphereSource> sphereSource;
  sphereSource->SetThetaResolution(20);
  sphereSource->SetPhiResolution(20);
  vtkNew<vtkTriangleFilter> triangleFilter;
  triangleFilter->SetInputConnection(sphereSource->GetOutputPort());
  triangleFilter->Update();

  vtkNew<vtkPolyData> sphere;
  sphere->DeepCopy(triangleFilter->GetOutput());
  AddPointIdArray(sphere);
  return sphere;
}

// Masks the points of the data set and checks that each output point got the point
// data of the input point that it was sampled from. Returns the number of errors.
int CheckPointDataOfMaskedPoints(vtkDataSet* input, int randomModeType, const char* modeName)
{
  vtkNew<vtkMaskPoints> maskPoints;
  maskPoints->SetInputData(input);
  maskPoints->GenerateVerticesOff();
  maskPoints->RandomModeOn();
  maskPoints->SetRandomModeType(randomModeType);
  maskPoints->SetMaximumNumberOfPoints(100);
  maskPoints->Update();

  vtkPolyData* output = maskPoints->GetOutput();
  const vtkIdType numberOfOutputPoints = output ? output->GetNumberOfPoints() : 0;
  if (numberOfOutputPoints == 0)
  {
    std::cerr << modeName << ": there are no points in the output" << std::endl;
    return 1;
  }

  vtkDataArray* outputPointIds = output->GetPointData()->GetArray("PointId");
  if (!outputPointIds)
  {
    std::cerr << modeName << ": the point data array is missing from the output" << std::endl;
    return 1;
  }

  int errors = 0;
  for (vtkIdType outputPointIndex = 0; outputPointIndex < numberOfOutputPoints; ++outputPointIndex)
  {
    // Each point data value is the id of the input point that the value belongs to,
    // therefore the input point at that id must be where the output point is.
    const vtkIdType inputPointId =
      static_cast<vtkIdType>(outputPointIds->GetTuple1(outputPointIndex));
    if (inputPointId < 0 || inputPointId >= input->GetNumberOfPoints())
    {
      std::cerr << modeName << ": output point " << outputPointIndex << " has invalid point data "
                << inputPointId << std::endl;
      ++errors;
      continue;
    }
    double outputPosition[3] = { 0.0, 0.0, 0.0 };
    double inputPosition[3] = { 0.0, 0.0, 0.0 };
    output->GetPoint(outputPointIndex, outputPosition);
    input->GetPoint(inputPointId, inputPosition);
    const double tolerance = 1e-6;
    if (std::fabs(outputPosition[0] - inputPosition[0]) > tolerance ||
      std::fabs(outputPosition[1] - inputPosition[1]) > tolerance ||
      std::fabs(outputPosition[2] - inputPosition[2]) > tolerance)
    {
      if (errors < 5)
      {
        std::cerr << modeName << ": the point data of output point " << outputPointIndex
                  << " belongs to input point " << inputPointId << " at (" << inputPosition[0]
                  << ", " << inputPosition[1] << ", " << inputPosition[2]
                  << "), but the output point is at (" << outputPosition[0] << ", "
                  << outputPosition[1] << ", " << outputPosition[2] << ")" << std::endl;
      }
      ++errors;
    }
  }
  if (errors > 0)
  {
    std::cerr << modeName << ": the point data of " << errors << " of " << numberOfOutputPoints
              << " output points belongs to another point" << std::endl;
  }
  return errors;
}

// Checks that the point data of the output points is preserved in all random modes.
int MaskPointsPointData()
{
  int errors = 0;

  vtkSmartPointer<vtkUnstructuredGrid> grid = CreateHexahedralGrid(8);
  errors += CheckPointDataOfMaskedPoints(
    grid, vtkMaskPoints::RANDOMIZED_ID_STRIDES, "RANDOMIZED_ID_STRIDES");
  errors += CheckPointDataOfMaskedPoints(grid, vtkMaskPoints::RANDOM_SAMPLING, "RANDOM_SAMPLING");
  errors +=
    CheckPointDataOfMaskedPoints(grid, vtkMaskPoints::SPATIALLY_STRATIFIED, "SPATIALLY_STRATIFIED");
  errors += CheckPointDataOfMaskedPoints(
    grid, vtkMaskPoints::UNIFORM_SPATIAL_BOUNDS, "UNIFORM_SPATIAL_BOUNDS");
  // Volume sampling only uses 3D cells
  errors += CheckPointDataOfMaskedPoints(
    grid, vtkMaskPoints::UNIFORM_SPATIAL_VOLUME, "UNIFORM_SPATIAL_VOLUME");

  // Surface sampling only uses 2D cells
  vtkSmartPointer<vtkPolyData> sphere = CreateTriangulatedSphere();
  errors += CheckPointDataOfMaskedPoints(
    sphere, vtkMaskPoints::UNIFORM_SPATIAL_SURFACE, "UNIFORM_SPATIAL_SURFACE");

  return errors;
}
}

int TestMaskPoints(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int dataType = MaskPoints(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = MaskPoints(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = MaskPoints(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = MaskPoints(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = MaskPoints(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = MaskPoints(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  if (MaskPointsPointData() != 0)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
