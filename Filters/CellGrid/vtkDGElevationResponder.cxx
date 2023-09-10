// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGElevationResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridElevationQuery.h"
#include "vtkDGHex.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"
#include "vtkVectorOperators.h"

#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGElevationResponder);

void vtkDGElevationResponder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGElevationResponder::Query(
  vtkCellGridElevationQuery* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)request;
  (void)cellType;
  (void)caches;

  if (!cellType)
  {
    return false;
  }

  auto* grid = cellType->GetCellGrid();
  if (!grid)
  {
    return false;
  }

  // Fetch arrays that define element shapes.
  auto shapeAtt = grid->GetShapeAttribute();
  if (!shapeAtt)
  {
    return false;
  }
  std::string cellTypeName = cellType->GetClassName();
  auto shapeArrays = shapeAtt->GetArraysForCellType(cellTypeName);

  // Fetch corner points of cells.
  auto* pts = vtkDataArray::SafeDownCast(shapeArrays["values"_token]);
  // Fetch corner connectivity of cells.
  auto* conn = vtkTypeInt64Array::SafeDownCast(shapeArrays["connectivity"_token]);

  if (!pts || !conn)
  {
    return false;
  }

  // Allocate a container to hold 1 cell's connectivity
  int nc = conn->GetNumberOfComponents();
  std::vector<vtkTypeInt64> entry;
  entry.resize(nc);

  // Allocate a container to hold 1 point's coordinates
  // plus one to accumulate the cell center.
  vtkVector3d pcoord;
  vtkVector3d center;
  int dim = pts->GetNumberOfComponents();
  if (dim != 3)
  {
    vtkErrorMacro("Unsupported point dimension " << dim << ". Expected 3.");
    return false;
  }

  std::function<double(const vtkVector3d&)> evaluateElevation;
  vtkVector3d origin(request->Origin.data());
  vtkVector3d axis(request->Axis.data());
  switch (request->NumberOfAxes)
  {
    case 1:
      evaluateElevation = [&](const vtkVector3d& pt) { return (pt - origin).Dot(axis); };
      break;
    case 2:
      evaluateElevation = [&](const vtkVector3d& pt) {
        double dist = (pt - origin).Dot(axis);
        return (pt - (origin + dist * axis)).Norm();
      };
      break;
    case 3:
      evaluateElevation = [&](const vtkVector3d& pt) { return (pt - origin).Norm(); };
      break;
    default:
    {
      vtkErrorMacro("Unsupported number of axes " << request->NumberOfAxes);
    }
      return false;
  }

  vtkNew<vtkFloatArray> elevation;
  elevation->SetName(request->Name.c_str());
  elevation->SetNumberOfComponents(nc); // Elevation will be same order as shape.
  elevation->SetNumberOfTuples(conn->GetNumberOfTuples());
  for (vtkIdType ii = 0; ii < conn->GetNumberOfTuples(); ++ii)
  {
    conn->GetTypedTuple(ii, &entry[0]);
    for (int jj = 0; jj < nc; ++jj)
    {
      pts->GetTuple(entry[jj], pcoord.GetData());
      center += pcoord;
      elevation->SetTypedComponent(ii, jj, evaluateElevation(pcoord));
    }
    // Now add shock if non-zero.
    if (request->Shock != 0.)
    {
      for (int jj = 0; jj < nc; ++jj)
      {
        pts->GetTuple(entry[jj], pcoord.GetData());
        double shock = (center - pcoord).Norm() * request->Shock;
        elevation->SetTypedComponent(ii, jj, shock + elevation->GetTypedComponent(ii, jj));
      }
    }
  }
  // Add the elevation data to the grid
  grid->GetAttributes(cellTypeName)->AddArray(elevation);
  vtkCellAttribute::ArraysForCellType arrays;
  arrays["values"] = elevation;
  request->Elevation->SetArraysForCellType(cellTypeName, arrays);
  return true;
}

VTK_ABI_NAMESPACE_END
