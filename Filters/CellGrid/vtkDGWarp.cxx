// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGWarp.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDGCell.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"
#include "vtkVector.h"

#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGWarp);

void vtkDGWarp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGWarp::Query(
  vtkCellGridWarp::Query* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
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
    vtkErrorMacro("No shape attribute.");
    return false;
  }

  auto* warpAtt = request->GetDeformationAttribute();
  if (!warpAtt)
  {
    vtkErrorMacro("No deformation attribute.");
    return false;
  }

  if (warpAtt->GetNumberOfComponents() != shapeAtt->GetNumberOfComponents())
  {
    vtkErrorMacro("Shape (" << shapeAtt->GetNumberOfComponents() << ") and "
                            << "\"" << warpAtt->GetName().Data() << "\" ("
                            << warpAtt->GetNumberOfComponents() << ")"
                            << " must have the same number of components but do not.");
    return false;
  }

  std::string cellTypeName = cellType->GetClassName();
  vtkStringToken cellTypeToken(cellTypeName);

  auto shapeInfo = shapeAtt->GetCellTypeInfo(cellTypeToken);
  auto& shapeArrays = shapeInfo.ArraysByRole;
  auto warpInfo = warpAtt->GetCellTypeInfo(cellTypeToken);
  auto& warpArrays = warpInfo.ArraysByRole;

  // NB: We could also test that DOFSharing is the same in both shapeInfo and warpInfo,
  //     but some files put points in a separate vtkDataSetAttributes instance than
  //     point-data. We should at least check that both vtkDataSetAttributes instances
  //     have the same number of tuples.
  if (!warpInfo.DOFSharing.IsValid() || !shapeInfo.DOFSharing.IsValid())
  {
    vtkErrorMacro("Shape (" << shapeInfo.DOFSharing.Data() << ") and "
                            << "\"" << warpAtt->GetName().Data() << "\" ("
                            << warpInfo.DOFSharing.Data() << ")"
                            << " must both have shared DOF, but do not.");
    return false;
  }

  if (warpInfo.FunctionSpace != "HGRAD"_token || shapeInfo.FunctionSpace != "HGRAD"_token)
  {
    vtkErrorMacro("Shape (" << shapeInfo.FunctionSpace.Data() << ") and "
                            << "\"" << warpAtt->GetName().Data() << "\" ("
                            << warpInfo.FunctionSpace.Data() << ")"
                            << " must both be in the HGRAD function space, but are not.");
    return false;
  }

  if (warpInfo.Basis != shapeInfo.Basis)
  {
    vtkErrorMacro("Shape (" << shapeInfo.Basis.Data() << ") and "
                            << "\"" << warpAtt->GetName().Data() << "\" (" << warpInfo.Basis.Data()
                            << ")"
                            << " must have the same basis, but do not.");
    return false;
  }

  if (warpInfo.Order != shapeInfo.Order)
  {
    vtkErrorMacro("Shape (" << shapeInfo.Order << ") and "
                            << "\"" << warpAtt->GetName().Data() << "\" (" << warpInfo.Order << ")"
                            << " must have the same order, but do not.");
    return false;
  }

  // Fetch corner points of cells.
  auto* pts = vtkDataArray::SafeDownCast(shapeArrays["values"_token]);
  // Fetch deflection vectors.
  auto* defl = vtkDataArray::SafeDownCast(warpArrays["values"_token]);

  if (!pts || !defl)
  {
    vtkErrorMacro("Shape (" << pts << ") or \"" << warpAtt->GetName().Data() << "\" (" << defl
                            << ") missing value array.");
    return false;
  }

  auto* outPts = pts->NewInstance();
  outPts->DeepCopy(pts);
  vtkSMPTools::For(0, outPts->GetNumberOfTuples(),
    [&](vtkIdType begin, vtkIdType end)
    {
      std::array<double, 3> xx;
      std::array<double, 3> dd;
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        pts->GetTuple(ii, xx.data());
        defl->GetTuple(ii, dd.data());
        for (int jj = 0; jj < 3; ++jj)
        {
          xx[jj] += request->GetScaleFactor() * dd[jj];
        }
        outPts->SetTuple(ii, xx.data());
      }
    });
  grid->GetAttributes(shapeInfo.DOFSharing)->RemoveArray(pts->GetName());
  grid->GetAttributes(shapeInfo.DOFSharing)->AddArray(outPts);
  shapeArrays["values"_token] = outPts;
  shapeAtt->SetCellTypeInfo(cellTypeToken, shapeInfo);
  outPts->FastDelete();

  return true;
}

VTK_ABI_NAMESPACE_END
