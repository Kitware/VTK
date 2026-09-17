// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridFindCriticalPoints.h"

#include "vtkCellArray.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDoubleArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTypeUInt32Array.h"
#include "vtkUnsignedCharArray.h"

#include <numeric>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellGridFindCriticalPoints);
vtkStandardNewMacro(vtkCellGridFindCriticalPoints::Query);
vtkCxxSetObjectMacro(vtkCellGridFindCriticalPoints::Query, CellAttribute, vtkCellAttribute);

vtkCellGridFindCriticalPoints::Query::~Query()
{
  this->SetCellAttribute(nullptr);
}

bool vtkCellGridFindCriticalPoints::Query::Initialize()
{
  if (!this->Superclass::Initialize())
  {
    return false;
  }
  this->CriticalPoints.clear();
  this->Types.clear();
  this->CellIndexBase.clear();
  this->CellOffsets = nullptr;
  this->Allocated = false;
  return true;
}

void vtkCellGridFindCriticalPoints::Query::SetCriticalPoints(vtkStringToken cellTypeName,
  std::vector<std::vector<vtk::basis::bezier::CriticalPoint>> criticalPointsByCell)
{
  this->CriticalPoints[cellTypeName] = std::move(criticalPointsByCell);
}

const std::vector<std::vector<vtk::basis::bezier::CriticalPoint>>&
vtkCellGridFindCriticalPoints::Query::GetCriticalPoints(vtkStringToken cellTypeName) const
{
  static const std::vector<std::vector<vtk::basis::bezier::CriticalPoint>> empty;
  auto it = this->CriticalPoints.find(cellTypeName);
  return it == this->CriticalPoints.end() ? empty : it->second;
}

void vtkCellGridFindCriticalPoints::Query::SetCriticalPointTypes(
  vtkStringToken cellTypeName, std::vector<std::vector<int>> typesByCell)
{
  this->Types[cellTypeName] = std::move(typesByCell);
}

const std::vector<std::vector<int>>& vtkCellGridFindCriticalPoints::Query::GetCriticalPointTypes(
  vtkStringToken cellTypeName) const
{
  static const std::vector<std::vector<int>> empty;
  auto it = this->Types.find(cellTypeName);
  return it == this->Types.end() ? empty : it->second;
}

void vtkCellGridFindCriticalPoints::Query::Allocate()
{
  if (this->Allocated)
  {
    return;
  }
  this->Allocated = true;

  // Each cell type's own base index into the dense (type, local cell ID)
  // numbering everything below uses.
  vtkIdType totalCells = 0; // Every cell, of every type, empty or not.
  for (const auto& perType : this->CriticalPoints)
  {
    this->CellIndexBase[perType.first] = totalCells;
    totalCells += static_cast<vtkIdType>(perType.second.size());
  }

  // CellOffsets starts out holding each cell's own critical-point count;
  // cellHasCriticalPoints is the same, compacted to one 0/1 byte per cell.
  this->CellOffsets = vtkSmartPointer<vtkIdTypeArray>::New();
  this->CellOffsets->SetNumberOfTuples(totalCells + 1);
  vtkNew<vtkUnsignedCharArray> cellHasCriticalPoints;
  cellHasCriticalPoints->SetNumberOfTuples(totalCells + 1);
  cellHasCriticalPoints->SetValue(totalCells, 0); // sentinel
  for (const auto& perType : this->CriticalPoints)
  {
    const vtkIdType base = this->CellIndexBase[perType.first];
    const auto& criticalPointsByCell = perType.second;
    vtkSMPTools::For(0, static_cast<vtkIdType>(criticalPointsByCell.size()),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          const vtkIdType count = static_cast<vtkIdType>(criticalPointsByCell[cellId].size());
          this->CellOffsets->SetValue(base + cellId, count);
          cellHasCriticalPoints->SetValue(base + cellId, count > 0 ? 1 : 0);
        }
      });
  }

  // Widened (unsigned char would overflow past 255 cells-with-points) and
  // scanned in place: a flagged cell's entry becomes its own poly-vertex
  // cell index, below - its rank among only the cells with critical points.
  vtkNew<vtkIdTypeArray> cellVertexIndex;
  cellVertexIndex->DeepCopy(cellHasCriticalPoints);

  // Both totals come for free as each scan's return value.
  vtkIdType numCriticalPoints = 0;
  vtkIdType numCellsWithCriticalPoints = 0;
  if (totalCells > 0)
  {
    numCriticalPoints = vtkSMPTools::ExclusiveScan(this->CellOffsets->GetPointer(0),
      this->CellOffsets->GetPointer(totalCells), static_cast<vtkIdType>(0));
    numCellsWithCriticalPoints = vtkSMPTools::ExclusiveScan(cellVertexIndex->GetPointer(0),
      cellVertexIndex->GetPointer(totalCells), static_cast<vtkIdType>(0));
  }
  this->CellOffsets->SetValue(totalCells, numCriticalPoints);

  // Every cell's points are already numbered contiguously, so the
  // connectivity array is simply 0, 1, ..., numCriticalPoints - 1.
  vtkNew<vtkIdTypeArray> vertConnectivity;
  vertConnectivity->SetNumberOfTuples(numCriticalPoints);
  vtkSMPTools::For(0, numCriticalPoints, [&](vtkIdType begin, vtkIdType end)
    { std::iota(vertConnectivity->GetPointer(begin), vertConnectivity->GetPointer(end), begin); });

  // One poly-vertex output cell per source cell with a critical point
  // (cells with none are skipped, not emitted as degenerate zero-point
  // cells), filled in parallel now that each cell's compacted position is
  // known.
  vtkNew<vtkIdTypeArray> vertOffsets;
  vertOffsets->SetNumberOfTuples(numCellsWithCriticalPoints + 1);
  vtkSMPTools::For(0, totalCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        if (cellHasCriticalPoints->GetValue(cellId))
        {
          vertOffsets->SetValue(
            cellVertexIndex->GetValue(cellId), this->CellOffsets->GetValue(cellId));
        }
      }
    });
  vertOffsets->SetValue(numCellsWithCriticalPoints, numCriticalPoints);

  vtkNew<vtkCellArray> verts;
  verts->SetData(vertOffsets, vertConnectivity);
  this->Output->SetVerts(verts);

  this->Output->GetPoints()->SetNumberOfPoints(numCriticalPoints);
  this->Output->GetPointData()->GetArray("CellType")->SetNumberOfTuples(numCriticalPoints);
  this->Output->GetPointData()->GetArray("CellId")->SetNumberOfTuples(numCriticalPoints);
  this->Output->GetPointData()->GetArray("RST")->SetNumberOfTuples(numCriticalPoints);
  this->Output->GetPointData()->GetArray("Value")->SetNumberOfTuples(numCriticalPoints);
  this->Output->GetPointData()->GetArray("Type")->SetNumberOfTuples(numCriticalPoints);
}

vtkIdType vtkCellGridFindCriticalPoints::Query::GetCellTypeOffset(vtkStringToken cellTypeName) const
{
  auto it = this->CellIndexBase.find(cellTypeName);
  return it == this->CellIndexBase.end() ? 0 : it->second;
}

vtkIdType vtkCellGridFindCriticalPoints::Query::GetCellOffset(
  vtkIdType cellTypeOffset, vtkIdType cellId) const
{
  return this->CellOffsets->GetValue(cellTypeOffset + cellId);
}

void vtkCellGridFindCriticalPoints::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CellAttribute: " << this->CellAttribute << "\n";
}

vtkMTimeType vtkCellGridFindCriticalPoints::Query::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  if (this->CellAttribute)
  {
    vtkMTimeType attribMTime = this->CellAttribute->GetMTime();
    mTime = (attribMTime > mTime ? attribMTime : mTime);
  }
  return mTime;
}

// ------------

vtkCellGridFindCriticalPoints::vtkCellGridFindCriticalPoints()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
}

void vtkCellGridFindCriticalPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Request:\n";
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

vtkMTimeType vtkCellGridFindCriticalPoints::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->Request->GetMTime());
}

void vtkCellGridFindCriticalPoints::SetCellAttribute(vtkCellAttribute* attribute)
{
  this->Request->SetCellAttribute(attribute);
}

int vtkCellGridFindCriticalPoints::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
    return 1;
  }
  return this->Superclass::FillInputPortInformation(port, info);
}

int vtkCellGridFindCriticalPoints::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  auto* input = vtkCellGrid::GetData(inInfo[0]);
  auto* output = vtkPolyData::GetData(outInfo);
  if (!input)
  {
    vtkWarningMacro("Empty input.");
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }

  output->Initialize();
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  output->SetPoints(points);

  vtkNew<vtkTypeUInt32Array> cellTypes;
  cellTypes->SetName("CellType");
  vtkNew<vtkIdTypeArray> cellIds;
  cellIds->SetName("CellId");
  vtkNew<vtkDoubleArray> rst;
  rst->SetNumberOfComponents(3);
  rst->SetName("RST");
  vtkNew<vtkDoubleArray> values;
  values->SetName("Value");
  vtkNew<vtkIntArray> types;
  types->SetName("Type");
  output->GetPointData()->AddArray(cellTypes);
  output->GetPointData()->AddArray(cellIds);
  output->GetPointData()->AddArray(rst);
  output->GetPointData()->SetScalars(values);
  output->GetPointData()->AddArray(types);

  this->Request->Input = input;
  this->Request->Output = output;
  if (!input->Query(this->Request))
  {
    vtkErrorMacro("Input failed to respond to query.");
    output->Initialize();
    return 0;
  }
  this->Request->Input = nullptr;
  this->Request->Output = nullptr;

  return 1;
}

VTK_ABI_NAMESPACE_END
