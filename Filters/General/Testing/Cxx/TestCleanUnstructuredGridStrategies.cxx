// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCleanUnstructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMathUtilities.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>
#include <numeric>

namespace
{

bool TestFirstPointStrategy();
bool TestAveragingStrategy();
bool TestSpatialDensityStrategy();

}

int TestCleanUnstructuredGridStrategies(int, char*[])
{
  bool res = true;
  res &= ::TestFirstPointStrategy();
  res &= ::TestAveragingStrategy();
  res &= ::TestSpatialDensityStrategy();
  return (res ? EXIT_SUCCESS : EXIT_FAILURE);
}

namespace
{

constexpr double TOL = 1e-6;
constexpr unsigned int NUM_POINTS = 8;
constexpr unsigned int NUM_CELLS = 2;

const std::vector<double> TetraPoints = {
  // tetra 0
  0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0,
  // tetra 1
  0.0, 0.0, 1.0, 2.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 2.0
};

const std::vector<double> FirstPointOutput = { 0.0, 1.0, 2.0, 3.0, 5.0, 6.0, 7.0 };
const std::vector<double> AveragingOutput = { 0.0, 1.0, 2.0, 3.5, 5.0, 6.0, 7.0 };
const std::vector<double> SpatialDensityOutput = { 0.0, 1.0, 2.0, 3.66666666667, 5.0, 6.0, 7.0 };

vtkSmartPointer<vtkUnstructuredGrid> MakeTestGrid()
{
  // points
  vtkNew<vtkDoubleArray> pointArray;
  pointArray->SetNumberOfComponents(3);
  pointArray->SetNumberOfTuples(NUM_POINTS);
  auto pRange = vtk::DataArrayValueRange<3>(pointArray);
  std::copy(TetraPoints.begin(), TetraPoints.end(), pRange.begin());
  vtkNew<vtkPoints> points;
  points->SetData(pointArray);

  // connectivity
  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfComponents(1);
  offsets->SetNumberOfTuples(NUM_CELLS + 1);
  for (vtkIdType iC = 0; iC < NUM_CELLS + 1; ++iC)
  {
    offsets->SetValue(iC, iC * 4);
  }
  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfComponents(1);
  connectivity->SetNumberOfTuples(NUM_CELLS * 4);
  auto cRange = vtk::DataArrayValueRange<1>(connectivity);
  std::iota(cRange.begin(), cRange.end(), 0);
  vtkNew<vtkCellArray> cellArr;
  cellArr->SetData(offsets, connectivity);

  // cell types
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->SetNumberOfComponents(1);
  cellTypes->SetNumberOfTuples(NUM_CELLS);
  for (vtkIdType iC = 0; iC < NUM_CELLS; ++iC)
  {
    cellTypes->SetValue(iC, VTK_TETRA);
  }

  // grid
  vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  grid->SetPoints(points);
  grid->SetCells(cellTypes, cellArr);

  // point data
  vtkNew<vtkDoubleArray> dArr;
  dArr->SetName("IotaScalar");
  dArr->SetNumberOfComponents(1);
  dArr->SetNumberOfTuples(NUM_POINTS);
  auto dRange = vtk::DataArrayValueRange<1>(dArr);
  std::iota(dRange.begin(), dRange.end(), 0.0);
  grid->GetPointData()->AddArray(dArr);
  return grid;
}

bool TestGenericStrategy(int strat, const std::vector<double>& vals)
{
  vtkSmartPointer<vtkUnstructuredGrid> input = MakeTestGrid();

  vtkNew<vtkCleanUnstructuredGrid> cleaner;
  cleaner->SetPointDataWeighingStrategy(strat);
  cleaner->SetInputData(input);
  cleaner->Update();

  vtkUnstructuredGrid* output = cleaner->GetOutput();
  if (!output)
  {
    std::cerr << "Output unstructured grid is nullptr" << std::endl;
    return false;
  }

  vtkDoubleArray* scalar =
    vtkArrayDownCast<vtkDoubleArray>(output->GetPointData()->GetArray("IotaScalar"));
  if (scalar->GetNumberOfValues() != NUM_POINTS - 1)
  {
    std::cerr << "Output scalar field does not have correct number of values: "
              << scalar->GetNumberOfValues() << " != " << NUM_POINTS - 1 << std::endl;
    return false;
  }
  for (vtkIdType iP = 0; iP < scalar->GetNumberOfValues(); ++iP)
  {
    if (!vtkMathUtilities::FuzzyCompare(scalar->GetValue(iP), vals[iP], TOL))
    {
      std::cerr << "Output scalar field did not get weighted correctly at index " << iP << ": "
                << scalar->GetValue(3) << " != " << vals[iP] << std::endl;
      return false;
    }
  }
  return true;
}

bool TestFirstPointStrategy()
{
  return TestGenericStrategy(vtkCleanUnstructuredGrid::FIRST_POINT, FirstPointOutput);
}

bool TestAveragingStrategy()
{
  return TestGenericStrategy(vtkCleanUnstructuredGrid::AVERAGING, AveragingOutput);
}

bool TestSpatialDensityStrategy()
{
  return TestGenericStrategy(vtkCleanUnstructuredGrid::SPATIAL_DENSITY, SpatialDensityOutput);
}

}
