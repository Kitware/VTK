// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridIsocontourPoints.h"

#include "vtkCellArray.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDoubleArray.h"
#include "vtkFiltersCellGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTypeUInt32Array.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCellGridIsocontourPoints);
vtkStandardNewMacro(vtkCellGridIsocontourPoints::Query);
vtkCxxSetObjectMacro(vtkCellGridIsocontourPoints::Query, CellAttribute, vtkCellAttribute);

vtkCellGridIsocontourPoints::Query::~Query()
{
  this->SetCellAttribute(nullptr);
}

bool vtkCellGridIsocontourPoints::Query::Initialize()
{
  if (!this->Superclass::Initialize())
  {
    return false;
  }
  this->IsocontourPoints.clear();
  this->CellIndexBase.clear();
  this->CellOffsets = nullptr;
  this->Allocated = false;
  return true;
}

void vtkCellGridIsocontourPoints::Query::SetIsocontourPoints(vtkStringToken cellTypeName,
  std::vector<std::vector<vtk::basis::bezier::CriticalPoint>> isocontourPointsByCell)
{
  this->IsocontourPoints[cellTypeName] = std::move(isocontourPointsByCell);
}

const std::vector<std::vector<vtk::basis::bezier::CriticalPoint>>&
vtkCellGridIsocontourPoints::Query::GetIsocontourPoints(vtkStringToken cellTypeName) const
{
  static const std::vector<std::vector<vtk::basis::bezier::CriticalPoint>> empty;
  auto it = this->IsocontourPoints.find(cellTypeName);
  return it == this->IsocontourPoints.end() ? empty : it->second;
}

void vtkCellGridIsocontourPoints::Query::Allocate()
{
  if (this->Allocated)
  {
    return;
  }
  this->Allocated = true;

  // Each cell type's own base index into the dense (type, local cell ID)
  // numbering everything below uses.
  vtkIdType totalCells = 0; // Every cell, of every type, empty or not.
  for (const auto& perType : this->IsocontourPoints)
  {
    this->CellIndexBase[perType.first] = totalCells;
    totalCells += static_cast<vtkIdType>(perType.second.size());
  }

  // CellOffsets starts out holding each cell's own isocontour-point count;
  // cellHasIsocontourPoints is the same, compacted to one 0/1 byte per cell.
  this->CellOffsets = vtkSmartPointer<vtkIdTypeArray>::New();
  this->CellOffsets->SetNumberOfTuples(totalCells + 1);
  vtkNew<vtkUnsignedCharArray> cellHasIsocontourPoints;
  cellHasIsocontourPoints->SetNumberOfTuples(totalCells + 1);
  cellHasIsocontourPoints->SetValue(totalCells, 0); // sentinel
  for (const auto& perType : this->IsocontourPoints)
  {
    const vtkIdType base = this->CellIndexBase[perType.first];
    const auto& isocontourPointsByCell = perType.second;
    vtkSMPTools::For(0, static_cast<vtkIdType>(isocontourPointsByCell.size()),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          const vtkIdType count = static_cast<vtkIdType>(isocontourPointsByCell[cellId].size());
          this->CellOffsets->SetValue(base + cellId, count);
          cellHasIsocontourPoints->SetValue(base + cellId, count > 0 ? 1 : 0);
        }
      });
  }

  // Widened (unsigned char would overflow past 255 cells-with-points) and
  // scanned in place: a flagged cell's entry becomes its own poly-vertex
  // cell index, below - its rank among only the cells with isocontour points.
  vtkNew<vtkIdTypeArray> cellVertexIndex;
  cellVertexIndex->DeepCopy(cellHasIsocontourPoints);

  // Both totals come for free as each scan's return value.
  vtkIdType numIsocontourPoints = 0;
  vtkIdType numCellsWithIsocontourPoints = 0;
  if (totalCells > 0)
  {
    numIsocontourPoints = vtkSMPTools::ExclusiveScan(this->CellOffsets->GetPointer(0),
      this->CellOffsets->GetPointer(totalCells), static_cast<vtkIdType>(0));
    numCellsWithIsocontourPoints = vtkSMPTools::ExclusiveScan(cellVertexIndex->GetPointer(0),
      cellVertexIndex->GetPointer(totalCells), static_cast<vtkIdType>(0));
  }
  this->CellOffsets->SetValue(totalCells, numIsocontourPoints);

  // Every cell's points are already numbered contiguously, so the
  // connectivity array is simply 0, 1, ..., numIsocontourPoints - 1.
  vtkNew<vtkIdTypeArray> vertConnectivity;
  vertConnectivity->SetNumberOfTuples(numIsocontourPoints);
  vtkSMPTools::For(0, numIsocontourPoints, [&](vtkIdType begin, vtkIdType end)
    { std::iota(vertConnectivity->GetPointer(begin), vertConnectivity->GetPointer(end), begin); });

  // One poly-vertex output cell per source cell with an isocontour point
  // (cells with none are skipped, not emitted as degenerate zero-point
  // cells), filled in parallel now that each cell's compacted position is
  // known.
  vtkNew<vtkIdTypeArray> vertOffsets;
  vertOffsets->SetNumberOfTuples(numCellsWithIsocontourPoints + 1);
  vtkSMPTools::For(0, totalCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        if (cellHasIsocontourPoints->GetValue(cellId))
        {
          vertOffsets->SetValue(
            cellVertexIndex->GetValue(cellId), this->CellOffsets->GetValue(cellId));
        }
      }
    });
  vertOffsets->SetValue(numCellsWithIsocontourPoints, numIsocontourPoints);

  vtkNew<vtkCellArray> verts;
  verts->SetData(vertOffsets, vertConnectivity);
  this->Output->SetVerts(verts);

  this->Output->GetPoints()->SetNumberOfPoints(numIsocontourPoints);
  // Every point-data array: the ones above and an array for each attribute.
  auto* pointData = this->Output->GetPointData();
  for (int array = 0; array < pointData->GetNumberOfArrays(); ++array)
  {
    pointData->GetArray(array)->SetNumberOfTuples(numIsocontourPoints);
  }
}

vtkIdType vtkCellGridIsocontourPoints::Query::GetCellTypeOffset(vtkStringToken cellTypeName) const
{
  auto it = this->CellIndexBase.find(cellTypeName);
  return it == this->CellIndexBase.end() ? 0 : it->second;
}

vtkIdType vtkCellGridIsocontourPoints::Query::GetCellOffset(
  vtkIdType cellTypeOffset, vtkIdType cellId) const
{
  return this->CellOffsets->GetValue(cellTypeOffset + cellId);
}

void vtkCellGridIsocontourPoints::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CellAttribute: " << this->CellAttribute << "\n";
  os << indent << "Isovalue: " << this->Isovalue << "\n";
  os << indent << "Include1DCells: " << (this->Include1DCells ? "On" : "Off") << "\n";
  os << indent << "Include2DCells: " << (this->Include2DCells ? "On" : "Off") << "\n";
  os << indent << "Include3DCells: " << (this->Include3DCells ? "On" : "Off") << "\n";
  os << indent << "ParticleRelaxationIterations: " << this->ParticleRelaxationIterations << "\n";
  os << indent << "InterpolateAttributes: " << (this->InterpolateAttributes ? "On" : "Off") << "\n";
  os << indent << "ComputeGradients: " << (this->ComputeGradients ? "On" : "Off") << "\n";
}

bool vtkCellGridIsocontourPoints::Query::IncludesDimension(int dimension) const
{
  switch (dimension)
  {
    case 1:
      return this->Include1DCells;
    case 2:
      return this->Include2DCells;
    case 3:
      return this->Include3DCells;
    default:
      return false;
  }
}

vtkMTimeType vtkCellGridIsocontourPoints::Query::GetMTime()
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

vtkCellGridIsocontourPoints::vtkCellGridIsocontourPoints()
{
  vtkFiltersCellGrid::RegisterCellsAndResponders();
}

void vtkCellGridIsocontourPoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Request:\n";
  this->Request->PrintSelf(os, indent.GetNextIndent());
}

vtkMTimeType vtkCellGridIsocontourPoints::GetMTime()
{
  return std::max(this->Superclass::GetMTime(), this->Request->GetMTime());
}

void vtkCellGridIsocontourPoints::SetCellAttribute(vtkCellAttribute* attribute)
{
  this->Request->SetCellAttribute(attribute);
}

void vtkCellGridIsocontourPoints::SetIsovalue(double isovalue)
{
  this->Request->SetIsovalue(isovalue);
}

void vtkCellGridIsocontourPoints::SetInclude1DCells(bool include)
{
  this->Request->SetInclude1DCells(include);
}

void vtkCellGridIsocontourPoints::SetInclude2DCells(bool include)
{
  this->Request->SetInclude2DCells(include);
}

void vtkCellGridIsocontourPoints::SetInclude3DCells(bool include)
{
  this->Request->SetInclude3DCells(include);
}

void vtkCellGridIsocontourPoints::SetParticleRelaxationIterations(int iterations)
{
  this->Request->SetParticleRelaxationIterations(iterations);
}

void vtkCellGridIsocontourPoints::SetInterpolateAttributes(bool interpolate)
{
  this->Request->SetInterpolateAttributes(interpolate);
}

void vtkCellGridIsocontourPoints::SetComputeGradients(bool compute)
{
  this->Request->SetComputeGradients(compute);
}

int vtkCellGridIsocontourPoints::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
    return 1;
  }
  return this->Superclass::FillInputPortInformation(port, info);
}

int vtkCellGridIsocontourPoints::RequestData(
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
  vtkNew<vtkDoubleArray> gradients;
  gradients->SetNumberOfComponents(3);
  gradients->SetName("Gradient");
  output->GetPointData()->AddArray(cellTypes);
  output->GetPointData()->AddArray(cellIds);
  output->GetPointData()->AddArray(rst);
  output->GetPointData()->SetScalars(values);
  if (this->Request->GetComputeGradients())
  {
    output->GetPointData()->AddArray(gradients);
  }

  // An array for every attribute of the input but its shape: the query evaluates
  // them at the points. (An attribute named like one of the arrays above is
  // skipped.)
  if (this->Request->GetInterpolateAttributes())
  {
    for (const auto& inputAttribute : input->GetCellAttributeList())
    {
      const char* name = inputAttribute->GetName().Data().c_str();
      if (inputAttribute == input->GetShapeAttribute() || output->GetPointData()->GetArray(name))
      {
        continue;
      }
      vtkNew<vtkDoubleArray> attributeValues;
      attributeValues->SetNumberOfComponents(inputAttribute->GetNumberOfComponents());
      attributeValues->SetName(name);
      output->GetPointData()->AddArray(attributeValues);
    }
  }

  this->Request->Output = output;
  const bool ok = input->Query(this->Request);
  this->Request->Output = nullptr;
  if (!ok)
  {
    vtkErrorMacro("Input failed to respond to query.");
    output->Initialize();
    return 0;
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
