// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGSidesResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridBoundsQuery.h"
#include "vtkDGCell.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"

#include <sstream>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGSidesResponder);

bool vtkDGSidesResponder::Query(
  vtkCellGridSidesQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;
  switch (query->GetPass())
  {
    case vtkCellGridSidesQuery::PassWork::HashSides:
      return this->HashSides(query, dynamic_cast<vtkDGCell*>(cellType));
    case vtkCellGridSidesQuery::PassWork::GenerateSideSets:
      return this->GenerateSideSets(query, dynamic_cast<vtkDGCell*>(cellType));
  }
  return false;
}

bool vtkDGSidesResponder::HashSides(vtkCellGridSidesQuery* query, vtkDGCell* cellType)
{
  auto* grid = cellType ? cellType->GetCellGrid() : nullptr;
  if (!grid)
  {
    vtkErrorMacro("Cells of type \"" << cellType->GetClassName() << "\" have no parent grid.");
    return false;
  }
  std::string cellTypeName = cellType->GetClassName();
  vtkStringToken cellTypeToken(cellTypeName);

  auto& cellSpec = cellType->GetCellSpec();
  auto* conn = vtkTypeInt64Array::SafeDownCast(cellSpec.Connectivity);
  if (!conn)
  {
    vtkErrorMacro("No connectivity or bad cell type.");
    return false;
  }

  std::unordered_set<std::int64_t> pointIDs;
  int nc = conn->GetNumberOfComponents();
  int minSideDim = cellType->GetDimension() - 1; // We only care about sides of dimension d-1.
  int numSideTypes = cellType->GetNumberOfSideTypes();
  std::vector<vtkTypeInt64> entry;
  std::vector<vtkIdType> side;
  entry.resize(nc);
  // Loop over elements, one per tuple of conn:
  for (vtkIdType ii = 0; ii < conn->GetNumberOfTuples(); ++ii)
  {
    conn->GetTypedTuple(ii, entry.data());
    // Loop over types of side (one entry per shape) of the element:
    for (int sideType = 0; sideType < numSideTypes; ++sideType)
    {
      auto range = cellType->GetSideRangeForType(sideType);
      auto shape = cellType->GetSideShape(range.first);
      // Only hash sides of dimension d-1
      if (vtkDGCell::GetShapeDimension(shape) < minSideDim)
      {
        break;
      }
      auto shapeName = vtkDGCell::GetShapeName(shape);
      // Loop over sides of the given type:
      for (int sideIdx = range.first; sideIdx < range.second; ++sideIdx)
      {
        const auto& sideConn = cellType->GetSideConnectivity(sideIdx);
        side.resize(sideConn.size());
        int jj = 0;
        for (const auto& sidePointIndex : sideConn)
        {
          side[jj++] = entry[sidePointIndex];
        }
        // Hash the sideIdx'th side of element ii and add it to the query's storage.
        query->AddSide(cellTypeToken, ii, sideIdx, shapeName, side);
      }
    }
  }
  return true;
}

bool vtkDGSidesResponder::GenerateSideSets(vtkCellGridSidesQuery* query, vtkDGCell* cellType)
{
  auto* grid = cellType ? cellType->GetCellGrid() : nullptr;
  if (!grid)
  {
    vtkErrorMacro("Cells of type \"" << cellType->GetClassName() << "\" have no parent grid.");
    return false;
  }

  vtkIdType offset = 0;
  // If we generated any side-sets, then turn off the output grid's cells
  // unless (a) they are of dimension 2 or less and (b) the query is configured
  // to leave them on. The query does this for triangles/quads (resp. edges) so
  // that they are rendered along with edges (resp. vertices) that bound them.
  bool shouldBlankCells = cellType->GetDimension() > 2 || !query->GetPreserveRenderableCells();
  auto sideSetArrays = query->GetSideSetArrays(cellType->GetClassName());
  if (!sideSetArrays.empty() && shouldBlankCells)
  {
    cellType->GetCellSpec().Blanked = true;
  }
  else
  {
    offset += cellType->GetCellSpec().Connectivity->GetNumberOfTuples();
  }

  // Now add the side sets.
  auto& sideSpecs = cellType->GetSideSpecs();
  offset = sideSpecs.empty()
    ? offset
    : sideSpecs.back().Offset + sideSpecs.back().Connectivity->GetNumberOfTuples();
  for (const auto& sideSetArray : sideSetArrays)
  {
    std::ostringstream groupName;
    groupName << sideSetArray.SideShape.Data() << " sides of " << sideSetArray.CellType.Data();
    vtkStringToken groupToken(groupName.str());
    auto* sideGroup = grid->GetAttributes(groupToken.GetId());
    auto sideShape = vtkDGCell::GetShapeEnum(sideSetArray.SideShape);
    int sideType = cellType->GetSideTypeForShape(sideShape);
    sideGroup->AddArray(sideSetArray.Sides);
    sideGroup->SetScalars(sideSetArray.Sides);
    sideSpecs.emplace_back(sideSetArray.Sides, offset, /*blank*/ false, sideShape, sideType);
    offset += sideSetArray.Sides->GetNumberOfTuples();
  }

  return true;
}

VTK_ABI_NAMESPACE_END
