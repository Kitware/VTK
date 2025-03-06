// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGTransformResponder.h"

#include "vtkAbstractTransform.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridTransform.h"
#include "vtkDGHex.h"
#include "vtkDGInterpolateCalculator.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDGTet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"

#include <algorithm>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGTransformResponder);

bool vtkDGTransformResponder::Query(
  vtkCellGridTransform::Query* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)cellType;
  (void)caches;

  std::string cellTypeName = cellType->GetClassName();
  vtkStringToken cellTypeToken(cellTypeName);
  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!dgCell)
  {
    vtkErrorMacro("Unsupported cell type \"" << cellTypeName << "\".");
    return false;
  }

  auto* attribute = request->GetCellAttribute();
  if (!attribute)
  {
    attribute = cellType->GetCellGrid()->GetShapeAttribute();
  }
  auto cellTypeInfo = attribute->GetCellTypeInfo(cellTypeToken);

  auto it = cellTypeInfo.ArraysByRole.find("values");
  if (it == cellTypeInfo.ArraysByRole.end() || !vtkDataArray::SafeDownCast(it->second))
  {
    vtkErrorMacro("No array in \"values\" role or the array was not a vtkDataArray.");
    return false;
  }
  auto* values = vtkDataArray::SafeDownCast(it->second);
  int nc = values->GetNumberOfComponents();
  auto basisOp = dgCell->GetOperatorEntry("Basis"_token, cellTypeInfo);
  bool dofSharing = cellTypeInfo.DOFSharing.IsValid();

  // ncpb = number of components per basis function:
  // nvpt = number of 3-d vectors per tuple:
  int ncpb = (dofSharing ? nc : nc / basisOp.NumberOfFunctions);
  int nvpt = (dofSharing ? nc / 3 : nc / 3);
  // TODO: Handle nvpt == 9 as well to transform matrices.
  if (ncpb != 3)
  {
    vtkErrorMacro("Values to be transformed must be 3-d vectors.");
    return false;
  }
  bool ok = true;

  auto xfm = request->GetTransform();
  auto* transformedValues = request->CreateNewDataArray(values);
  vtkIdType nt = values->GetNumberOfTuples();
  transformedValues->SetName(values->GetName());
  transformedValues->SetNumberOfComponents(values->GetNumberOfComponents());
  transformedValues->SetNumberOfTuples(values->GetNumberOfTuples());
  // Note that we keep values and transformedValues around at the same
  // time in order to always apply transforms in the highest possible
  // precision before writing to the transformedValues array.
  // If you are reading this, DO NOT try to speed things up in a way that
  // loses precision; always use TransformPoint(double*, double*).
  if (dofSharing)
  {
    vtkSMPTools::For(0, nt,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType ii = begin; ii < end; ++ii)
        {
          std::array<double, 3> tuple;
          values->GetTuple(ii, tuple.data());
          xfm->TransformPoint(tuple.data(), tuple.data());
          transformedValues->SetTuple(ii, tuple.data());
        }
      });
    auto* arrayGroup = cellType->GetCellGrid()->GetAttributes(cellTypeInfo.DOFSharing);
    arrayGroup->RemoveArray(values->GetName());
    arrayGroup->AddArray(transformedValues);
    transformedValues->FastDelete();
  }
  else
  {
    if (cellTypeInfo.FunctionSpace == "HGRAD"_token ||
      cellTypeInfo.FunctionSpace == "constant"_token)
    {
      // DG HGRAD data repeats a vector value once for each basis
      vtkSMPTools::For(0, nt,
        [&](vtkIdType begin, vtkIdType end)
        {
          std::vector<double> tuple;
          tuple.resize(values->GetNumberOfComponents());
          for (vtkIdType ii = begin; ii < end; ++ii)
          {
            values->GetTuple(ii, tuple.data());
            for (int jj = 0; jj < nvpt; ++jj)
            {
              xfm->TransformPoint(tuple.data() + 3 * jj, tuple.data() + 3 * jj);
            }
            transformedValues->SetTuple(ii, tuple.data());
          }
        });
      auto* arrayGroup = cellType->GetCellGrid()->GetAttributes(cellTypeToken);
      arrayGroup->RemoveArray(values->GetName());
      arrayGroup->AddArray(transformedValues);
      transformedValues->FastDelete();
    }
    else
    {
      transformedValues->Delete();
      vtkErrorMacro("Invalid function space \"" << cellTypeInfo.FunctionSpace.Data() << "\".");
      ok = false;
    }
  }
  if (ok)
  {
    cellTypeInfo.ArraysByRole["values"_token] = transformedValues;
    attribute->SetCellTypeInfo(cellTypeToken, cellTypeInfo);
  }

  return ok;
}

VTK_ABI_NAMESPACE_END
