// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGCellSourceResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridElevationQuery.h"
#include "vtkDGInterpolateCalculator.h"
#include "vtkDGVert.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"
#include "vtkVector.h"

#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGCellSourceResponder);

void vtkDGCellSourceResponder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGCellSourceResponder::Query(
  vtkCellGridCellSource::Query* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;

  vtkStringToken cellTypeToken = cellType->GetClassName();
  vtkStringToken requestedType = request->GetCellType();
  if (requestedType != cellTypeToken)
  {
    // Do not create any cells of this type if not asked.
    return true;
  }

  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!dgCell)
  {
    return false;
  }

  auto* grid = dgCell->GetCellGrid();
  if (!grid)
  {
    return false;
  }

  auto npts = dgCell->GetNumberOfCorners();

  vtkNew<vtkDoubleArray> coords;
  coords->SetName("coords");
  coords->SetNumberOfComponents(3);
  coords->SetNumberOfTuples(npts);
  for (int ii = 0; ii < npts; ++ii)
  {
    coords->SetTuple(ii, dgCell->GetCornerParameter(ii).data());
  }

  vtkNew<vtkIdTypeArray> conn;
  conn->SetName("connectivity");
  conn->SetNumberOfComponents(npts);
  conn->SetNumberOfTuples(1);
  std::vector<vtkTypeUInt64> connTuple(npts);
  for (int ii = 0; ii < npts; ++ii)
  {
    connTuple[ii] = ii;
  }
  conn->SetUnsignedTuple(0, connTuple.data());

  auto* pointArrayGroup = grid->GetAttributes(cellTypeToken);
  pointArrayGroup->SetScalars(conn);
  dgCell->GetCellSpec().Connectivity = conn;

  auto cellArrayGroup = grid->GetAttributes("points"_token);
  cellArrayGroup->SetScalars(coords);

  vtkNew<vtkCellAttribute> shape;
  shape->Initialize("shape"_token, "ℝ³", 3);
  vtkCellAttribute::CellTypeInfo shapeInfo;
  shapeInfo.DOFSharing = "points"_token;
  if (cellTypeToken == "vtkDGVert"_token)
  {
    shapeInfo.FunctionSpace = "constant"_token;
    shapeInfo.Order = 0;
  }
  else
  {
    shapeInfo.FunctionSpace = "HGRAD"_token;
    shapeInfo.Order = 1;
  }
  shapeInfo.Basis = "C"_token;
  shapeInfo.ArraysByRole["connectivity"_token] = conn;
  shapeInfo.ArraysByRole["values"_token] = coords;
  shape->SetCellTypeInfo(cellTypeToken, shapeInfo);
  grid->SetShapeAttribute(shape);

  // clang-format off
  if (cellTypeToken == "vtkDGVert"_token)
  {
    this->CreateCellAttribute(
      dgCell, cellTypeToken,
      "constant", "ℝ³", 3,
      "constant"_token, "C"_token, 0,
      3, 1, "points"_token);
  }
  else
  {
    this->CreateCellAttribute(
      dgCell, cellTypeToken,
      "hgrad", "ℝ³", 3,
      "HGRAD"_token, "C"_token, 1,
      dgCell->GetNumberOfSidesOfDimension(0) * 3, 1, "points"_token);
  }
  // clang-format on
  if (dgCell->IsA("vtkDeRhamCell"))
  {
    // clang-format off
    this->CreateCellAttribute(
      dgCell, cellTypeToken,
      "hcurl", "ℝ³", 3,
      "HCURL"_token, "I"_token, 1,
      dgCell->GetNumberOfSidesOfDimension(1), 3);

    this->CreateCellAttribute(
      dgCell, cellTypeToken,
      "hdiv", "ℝ³", 3,
      "HDIV"_token, "I"_token, 1,
      dgCell->GetNumberOfSidesOfDimension(2), 3);
    // clang-format on
  }

  return true;
}

void vtkDGCellSourceResponder::CreateCellAttribute(vtkDGCell* dgCell, vtkStringToken cellTypeToken,
  const std::string& fieldName, vtkStringToken space, int numberOfComponents,
  vtkStringToken functionSpace, vtkStringToken basis, int order, vtkIdType numberOfValues,
  int basisSize, vtkStringToken dofSharing)
{
  vtkNew<vtkCellAttribute> attrib;
  vtkNew<vtkDoubleArray> attribVals;
  attribVals->SetName(fieldName.c_str());
  attribVals->SetNumberOfComponents(
    dofSharing.IsValid() ? numberOfComponents / basisSize : numberOfValues);
  attribVals->SetNumberOfTuples(
    dofSharing.IsValid() ? numberOfValues * basisSize / numberOfComponents : 1);
  vtkSMPTools::For(0, attribVals->GetNumberOfValues(),
    [&attribVals](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        attribVals->SetValue(ii, ii == 0 ? 1. : 0.);
      }
    });
  vtkDataArray* conn = nullptr;
  if (dofSharing.IsValid())
  {
    auto* cellArrayGroup = dgCell->GetCellGrid()->GetAttributes(dofSharing);
    cellArrayGroup->AddArray(attribVals);
    conn = dgCell->GetCellSpec().Connectivity;
  }
  else
  {
    auto* pointArrayGroup = dgCell->GetCellGrid()->GetAttributes(cellTypeToken);
    pointArrayGroup->AddArray(attribVals);
  }
  attrib->Initialize(fieldName, space, numberOfComponents);
  vtkCellAttribute::CellTypeInfo curlInfo;
  curlInfo.DOFSharing = dofSharing;
  curlInfo.FunctionSpace = functionSpace;
  curlInfo.Basis = basis;
  curlInfo.Order = order;
  if (conn)
  {
    curlInfo.ArraysByRole["connectivity"_token] = conn;
  }
  curlInfo.ArraysByRole["values"_token] = attribVals;
  attrib->SetCellTypeInfo(cellTypeToken, curlInfo);
  dgCell->GetCellGrid()->AddCellAttribute(attrib);
}

VTK_ABI_NAMESPACE_END
