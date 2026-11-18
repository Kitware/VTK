// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGIsocontourResponder.h"

#include "vtkCellAttribute.h"
#include "vtkDGCell.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGIsocontourResponder);

bool vtkDGIsocontourResponder::Query(vtkCellGridIsocontour::Query* query, vtkCellMetadata* cellType,
  vtkCellGridResponders* vtkNotUsed(caches))
{
  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  auto* attribute = query->GetCellAttribute();
  if (!dgCell || !attribute)
  {
    return false;
  }
  vtkStringToken cellTypeName(dgCell->GetClassName());
  auto info = attribute->GetCellTypeInfo(cellTypeName);
  // Only Bezier control values bound the field; a cell type in another basis is
  // left out.
  if (info.FunctionSpace != "Bezier"_token)
  {
    return true;
  }
  auto basis = dgCell->GetOperatorEntry("Basis"_token, info);
  auto* values = info.GetArrayForRoleAs<vtkDataArray>("values"_token);
  auto* connectivity = info.GetArrayForRoleAs<vtkDataArray>("connectivity"_token);
  if (!basis || !values)
  {
    vtkErrorMacro("The attribute has no Bezier basis or values for " << cellTypeName.Data() << ".");
    return false;
  }
  const int numFunctions = basis.NumberOfFunctions;
  // The control values are shared by the cells that use them (and indexed
  // through the connectivity), or each cell has its own.
  const bool shared = info.DOFSharing.IsValid() && connectivity;
  const vtkIdType numCells =
    shared ? connectivity->GetNumberOfTuples() : values->GetNumberOfTuples();
  if (numCells == 0)
  {
    // There is nothing to scan (the scan of an empty range is undefined).
    return true;
  }
  const double isovalue = query->GetIsovalue();

  // Flag each intersected cell with a one, turn the flags into the offsets where
  // the IDs of those cells are written (an exclusive scan, so the offset of an
  // intersected cell is the number of intersected ones before it), and write
  // them there.
  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfValues(numCells + 1);
  offsets->Fill(0);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      std::vector<vtkTypeUInt64> dofIds(shared ? numFunctions : 0);
      std::vector<double> coefficients(numFunctions);
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        if (!shared)
        {
          values->GetTuple(cellId, coefficients.data());
        }
        else
        {
          connectivity->GetUnsignedTuple(cellId, dofIds.data());
          for (int ff = 0; ff < numFunctions; ++ff)
          {
            coefficients[ff] = values->GetComponent(static_cast<vtkIdType>(dofIds[ff]), 0);
          }
        }
        // The field lies within the convex hull of its Bezier coefficients,
        // so a cell they do not bracket cannot hold the isocontour.
        const auto hull = std::minmax_element(coefficients.begin(), coefficients.end());
        const bool isIntersected = *hull.first <= isovalue && isovalue <= *hull.second;
        offsets->SetValue(cellId, isIntersected ? 1 : 0);
      }
    });
  const vtkIdType numIntersected = vtkSMPTools::ExclusiveScan(
    offsets->GetPointer(0), offsets->GetPointer(numCells), static_cast<vtkIdType>(0));
  offsets->SetValue(numCells, numIntersected);
  if (numIntersected == 0)
  {
    return true;
  }

  vtkNew<vtkIdTypeArray> cellIds;
  cellIds->SetNumberOfValues(numIntersected);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        // A cell is intersected when its offset is followed by a different one.
        if (offsets->GetValue(cellId + 1) != offsets->GetValue(cellId))
        {
          cellIds->SetValue(offsets->GetValue(cellId), cellId);
        }
      }
    });
  int order = 0;
  for (int axisOrder : dgCell->GetBasisOrder(info))
  {
    order = std::max(order, axisOrder);
  }
  query->AddCandidateCells(cellTypeName, cellIds, order);
  return true;
}

VTK_ABI_NAMESPACE_END
