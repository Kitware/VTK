// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTableToRectilinearGrid.h"

#include "vtkArrayDispatch.h"
#include "vtkConstantArray.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIndexedArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSetGet.h"
#include "vtkTable.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkTableToRectilinearGrid);

namespace
{
constexpr vtkIdType INVALID_ROW_ID = -1;

/**
 * Create an indexed array with the same ValueType as the inputArray.
 * Add it in the given PointData.
 */
struct AddIndexedArrayWorker
{
  template <typename ArrayType>
  void operator()(ArrayType* inputArray, vtkIdTypeArray* indexArray, vtkPointData* outputFD) const
  {
    using ValueType = vtk::GetAPIType<ArrayType>;

    vtkNew<vtkIndexedArray<ValueType>> indexedArray;
    indexedArray->SetName(inputArray->GetName());
    indexedArray->ConstructBackend(indexArray, inputArray, true);
    indexedArray->SetNumberOfComponents(inputArray->GetNumberOfComponents());
    indexedArray->SetNumberOfTuples(indexArray->GetNumberOfTuples());

    outputFD->AddArray(indexedArray);
  }
};

}

// ----------------------------------------------------------------------------
vtkTableToRectilinearGrid::vtkTableToRectilinearGrid()
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::ROW, "");
  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::ROW, "");
  this->SetInputArrayToProcess(2, 0, 0, vtkDataObject::ROW, "");
}

// ----------------------------------------------------------------------------
int vtkTableToRectilinearGrid::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto inputTable = vtkTable::GetData(inputVector[0]);
  auto outputGrid = vtkRectilinearGrid::GetData(outputVector);

  this->SetCoordinateArrays(inputVector, outputGrid);

  auto pointToRow = this->ComputePointToRowMap(inputVector, inputTable, outputGrid);
  this->MarkBlanks(outputGrid, pointToRow);

  this->ColumnsToPointData(inputTable, outputGrid, pointToRow);

  return 1;
}

// ----------------------------------------------------------------------------
void vtkTableToRectilinearGrid::SetXYZColumns(
  const std::string& xarray, const std::string& yarray, const std::string& zarray)
{
  this->SetXColumn(xarray);
  this->SetYColumn(yarray);
  this->SetZColumn(zarray);
}

// ----------------------------------------------------------------------------
int vtkTableToRectilinearGrid::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}

// ----------------------------------------------------------------------------
void vtkTableToRectilinearGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkTableToRectilinearGrid::SetXColumn(const std::string& name)
{
  vtkDebugMacro(" setting XColumn to " << name);
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::ROW, name.c_str());
}

// ----------------------------------------------------------------------------
std::string vtkTableToRectilinearGrid::GetXColumn()
{
  vtkInformation* arrayInfo = this->GetInputArrayInformation(0);
  std::string name = arrayInfo->Get(vtkDataObject::FIELD_NAME());
  return name;
}

// ----------------------------------------------------------------------------
void vtkTableToRectilinearGrid::SetYColumn(const std::string& name)
{
  vtkDebugMacro(" setting YColumn to " << name);
  this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::ROW, name.c_str());
}

// ----------------------------------------------------------------------------
std::string vtkTableToRectilinearGrid::GetYColumn()
{
  vtkInformation* arrayInfo = this->GetInputArrayInformation(1);
  std::string name = arrayInfo->Get(vtkDataObject::FIELD_NAME());
  return name;
}

// ----------------------------------------------------------------------------
void vtkTableToRectilinearGrid::SetZColumn(const std::string& name)
{
  vtkDebugMacro(" setting ZColumn to " << name);
  this->SetInputArrayToProcess(2, 0, 0, vtkDataObject::ROW, name.c_str());
}

// ----------------------------------------------------------------------------
std::string vtkTableToRectilinearGrid::GetZColumn()
{
  vtkInformation* arrayInfo = this->GetInputArrayInformation(2);
  std::string name = arrayInfo->Get(vtkDataObject::FIELD_NAME());
  return name;
}

// ----------------------------------------------------------------------------
void vtkTableToRectilinearGrid::SetCoordinateArrays(
  vtkInformationVector** inputInfo, vtkRectilinearGrid* outputGrid)
{
  int dimensions[3] = { 1, 1, 1 };

  for (int axisIdx = 0; axisIdx < 3; axisIdx++)
  {
    vtkDataArray* column = this->GetInputArrayToProcess(axisIdx, inputInfo);

    if (!column)
    {
      vtkInformation* arrayInfo = this->GetInputArrayInformation(axisIdx);
      std::string name = arrayInfo->Get(vtkDataObject::FIELD_NAME());

      // Missing column is valid but leads to 2D or 1D grid.
      // But if name was specified, this is probably not intended.
      if (!name.empty())
      {
        vtkWarningMacro(
          "Cannot retrieve column with name " << name << ". Resulting grid may be unexpected.");
      }
    }

    vtkNew<vtkDoubleArray> coordArray;
    this->ComputeCoordinateArray(column, coordArray);
    dimensions[axisIdx] = coordArray->GetNumberOfTuples();
    if (axisIdx == 0)
    {
      coordArray->SetName("Xcoords");
      outputGrid->SetXCoordinates(coordArray);
    }
    else if (axisIdx == 1)
    {
      coordArray->SetName("Ycoords");
      outputGrid->SetYCoordinates(coordArray);
    }
    else
    {
      coordArray->SetName("Zcoords");
      outputGrid->SetZCoordinates(coordArray);
    }
  }

  outputGrid->SetDimensions(dimensions);
}

// ----------------------------------------------------------------------------
bool vtkTableToRectilinearGrid::ComputeCoordinateArray(vtkDataArray* column, vtkDoubleArray* coords)
{
  if (!column)
  {
    coords->InsertNextTuple1(0.);
    return false;
  }
  auto inputIter = vtk::DataArrayTupleRange(column);
  for (const auto inputCoord : inputIter)
  {
    auto outIter = vtk::DataArrayValueRange(coords);
    auto pos = std::find(outIter.begin(), outIter.end(), inputCoord[0]);
    if (pos == outIter.end())
    {
      coords->InsertNextValue(inputCoord[0]);
    }
  }

  auto outIter = vtk::DataArrayValueRange(coords);
  std::sort(outIter.begin(), outIter.end());

  return true;
}

// ----------------------------------------------------------------------------
vtkSmartPointer<vtkIdTypeArray> vtkTableToRectilinearGrid::ComputePointToRowMap(
  vtkInformationVector** inputInfo, vtkTable* table, vtkRectilinearGrid* grid)
{
  vtkNew<vtkIdTypeArray> pointToRowMap;
  pointToRowMap->SetNumberOfTuples(grid->GetNumberOfPoints());
  pointToRowMap->Fill(::INVALID_ROW_ID);

  vtkDataArray* xColumn = this->GetInputArrayToProcess(0, inputInfo);
  vtkDataArray* yColumn = this->GetInputArrayToProcess(1, inputInfo);
  vtkDataArray* zColumn = this->GetInputArrayToProcess(2, inputInfo);

  vtkNew<vtkConstantArray<double>> emptyDim;
  emptyDim->SetNumberOfTuples(table->GetNumberOfRows());
  emptyDim->ConstructBackend(0.);
  if (!xColumn)
  {
    xColumn = emptyDim;
  }
  if (!yColumn)
  {
    yColumn = emptyDim;
  }
  if (!zColumn)
  {
    zColumn = emptyDim;
  }

  // For each row, retrieve the i,j,k indices for the given point in the output grid
  // to get the matching point id.
  for (vtkIdType rowId = 0; rowId < table->GetNumberOfRows(); rowId++)
  {
    double* x = xColumn->GetTuple(rowId);
    double* y = yColumn->GetTuple(rowId);
    double* z = zColumn->GetTuple(rowId);
    const double coord[3] = { x[0], y[0], z[0] };
    int structuredCoord[3];
    double param[3];
    grid->ComputeStructuredCoordinates(coord, structuredCoord, param);
    int pointStructuredCoord[3] = { structuredCoord[0] + static_cast<int>(param[0]),
      structuredCoord[1] + static_cast<int>(param[1]),
      structuredCoord[2] + static_cast<int>(param[2]) };
    vtkIdType gridId = grid->ComputePointId(pointStructuredCoord);

    pointToRowMap->SetValue(gridId, rowId);
  }

  return pointToRowMap;
}

// ----------------------------------------------------------------------------
void vtkTableToRectilinearGrid::MarkBlanks(vtkRectilinearGrid* grid, vtkIdTypeArray* pointToRowMap)
{
  vtkNew<vtkIdList> cellIds;
  for (vtkIdType ptId = 0; ptId < grid->GetNumberOfPoints(); ptId++)
  {
    if (pointToRowMap->GetValue(ptId) == ::INVALID_ROW_ID)
    {
      grid->BlankPoint(ptId);
      grid->GetPointCells(ptId, cellIds);
      for (auto ptCellId = 0; ptCellId < cellIds->GetNumberOfIds(); ptCellId++)
      {
        grid->BlankCell(cellIds->GetId(ptCellId));
      }
      pointToRowMap->SetValue(ptId, 0);
    }
  }
}

// ----------------------------------------------------------------------------
void vtkTableToRectilinearGrid::ColumnsToPointData(
  vtkTable* table, vtkRectilinearGrid* grid, vtkIdTypeArray* pointToRow)
{
  for (vtkIdType colIdx = 0; colIdx < table->GetNumberOfColumns(); colIdx++)
  {
    auto column = vtkDataArray::SafeDownCast(table->GetColumn(colIdx));
    if (!column)
    {
      continue;
    }

    AddIndexedArrayWorker worker;
    using Dispatcher = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
    if (!Dispatcher::Execute(column, worker, pointToRow, grid->GetPointData()))
    {
      worker(column, pointToRow, grid->GetPointData());
    }
  }
}

VTK_ABI_NAMESPACE_END
