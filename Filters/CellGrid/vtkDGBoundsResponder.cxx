// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGBoundsResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridBoundsQuery.h"
#include "vtkDGHex.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"

#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGBoundsResponder);

bool vtkDGBoundsResponder::Query(
  vtkCellGridBoundsQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)query;
  (void)cellType;
  (void)caches;

  auto* grid = cellType->GetCellGrid();
  std::string cellTypeName = cellType->GetClassName();
  if (!grid->GetShapeAttribute())
  {
    vtkErrorMacro("Cells of type \"" << cellTypeName << "\" have no parent grid.");
    return false;
  }

  auto* shape = grid->GetShapeAttribute();
  if (!shape)
  {
    vtkErrorMacro("Cells of type \"" << cellTypeName << "\" have no shape.");
    return false;
  }

  vtkStringToken cellTypeToken(cellTypeName);
  auto shapeArrays = shape->GetCellTypeInfo(cellTypeToken).ArraysByRole;
  auto* pts = vtkDataArray::SafeDownCast(shapeArrays["values"_token]);
  auto* conn = vtkDataArray::SafeDownCast(shapeArrays["connectivity"_token]);
  if (!pts || !conn || !conn->IsIntegral())
  {
    vtkErrorMacro("Shape for \"" << cellTypeName << "\" missing points or connectivity.");
    return false;
  }

  std::unordered_set<std::int64_t> pointIDs;
  int nc = conn->GetNumberOfComponents();
  std::vector<vtkTypeInt64> entry;
  entry.resize(nc);
  for (vtkIdType ii = 0; ii < conn->GetNumberOfTuples(); ++ii)
  {
    conn->GetIntegerTuple(ii, entry.data());
    for (int jj = 0; jj < nc; ++jj)
    {
      pointIDs.insert(entry[jj]);
    }
  }
  if (pts->GetNumberOfTuples() > 0)
  {
    std::vector<double> pcoord;
    int dim = pts->GetNumberOfComponents();
    pcoord.resize(dim);

    // Initialize the bounds:
    vtkBoundingBox bbox;
    // TODO: Check isnan/isinf() on each component and iterate through points if true
    //       until we find a valid point to serve as the infinitesimal starting bounds.
    //       For now, just grab the first point:
    pts->GetTuple(pointIDs.empty() ? 0 : *pointIDs.begin(), pcoord.data());
    bbox.SetMinPoint(pcoord.data());
    bbox.SetMaxPoint(pcoord.data());

    for (const auto& pointID : pointIDs)
    {
      pts->GetTuple(pointID, pcoord.data());
      bbox.AddPoint(pcoord.data());
    }
    query->AddBounds(bbox);
  }
  return true;
}

VTK_ABI_NAMESPACE_END
