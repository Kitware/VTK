// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGSidesResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellGrid.h"
#include "vtkCellGridBoundsQuery.h"
#include "vtkDGCell.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"

#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGSidesResponder);

bool vtkDGSidesResponder::Query(
  vtkCellGridSidesQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;

  auto* grid = cellType->GetCellGrid();
  std::string cellTypeName = cellType->GetClassName();
  vtkStringToken cellAttrName(cellTypeName.substr(3));
  auto* conn = vtkTypeInt64Array::SafeDownCast(grid->GetAttributes(cellAttrName)->GetArray("conn"));
  auto* dgCellType = dynamic_cast<vtkDGCell*>(cellType);
  if (!conn || !dgCellType)
  {
    return false;
  }
  std::unordered_set<std::int64_t> pointIDs;
  int nc = conn->GetNumberOfComponents();
  int minSideDim = dgCellType->GetDimension() - 1; // We only care about sides of dimension d-1.
  int numSideTypes = dgCellType->GetNumberOfSideTypes();
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
      auto range = dgCellType->GetSideRangeForType(sideType);
      auto shape = dgCellType->GetSideShape(range.first);
      // Only hash sides of dimension d-1
      if (vtkDGCell::GetShapeDimension(shape) < minSideDim)
      {
        break;
      }
      auto shapeName = vtkDGCell::GetShapeName(shape);
      // Loop over sides of the given type:
      for (int sideIdx = range.first; sideIdx < range.second; ++sideIdx)
      {
        const auto& sideConn = dgCellType->GetSideConnectivity(sideIdx);
        side.resize(sideConn.size());
        int jj = 0;
        for (const auto& sidePointIndex : sideConn)
        {
          side[jj++] = entry[sidePointIndex];
        }
        // Hash the sideIdx'th side of element ii and add it to the query's storage.
        query->AddSide(cellAttrName, ii, sideIdx, shapeName, side);
      }
    }
  }
  return true;
}

VTK_ABI_NAMESPACE_END
