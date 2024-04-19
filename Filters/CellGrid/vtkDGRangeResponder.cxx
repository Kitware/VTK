// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGRangeResponder.h"

#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridRangeQuery.h"
#include "vtkDGHex.h"
#include "vtkDGTet.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkObjectFactory.h"
#include "vtkStringToken.h"
#include "vtkTypeInt64Array.h"

#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGRangeResponder);

bool vtkDGRangeResponder::Query(
  vtkCellGridRangeQuery* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)cellType;
  (void)caches;

  std::string cellTypeName = cellType->GetClassName();

  auto* attribute = request->GetCellAttribute();
  if (!attribute)
  {
    return false;
  }

  vtkStringToken cellTypeToken(cellTypeName);
  auto shapeArrays = attribute->GetArraysForCellType(cellTypeToken);
  auto* vals = vtkDataArray::SafeDownCast(shapeArrays["values"_token]);
  auto* conn = vtkTypeInt64Array::SafeDownCast(shapeArrays["connectivity"_token]);
  if (!vals || !conn)
  {
    vtkErrorMacro("Shape for \"" << cellTypeName << "\" missing points or connectivity.");
    return false;
  }

  auto attType = attribute->GetAttributeType().Data();
  if (attType.find("discontinuous Lagrange") != std::string::npos ||
    attType.find("DG constant") != std::string::npos ||
    attType.find("DG H") != std::string::npos) // Accept "DG HGRAD","DG HCURL", or "DG HDIV".
  {
    // We can ignore the connectivity and just iterate over components in the per-cell values array.
    return request->GetFiniteRange() ? this->DiscontinuousLagrange<true>(attribute, vals, request)
                                     : this->DiscontinuousLagrange<false>(attribute, vals, request);
  }
  else if (attType.find("continuous Lagrange") != std::string::npos ||
    attType.find("CG HGRAD") != std::string::npos)
  {
    // Use the connectivity to index into the per-corner values array.
    return request->GetFiniteRange()
      ? this->ContinuousLagrange<true>(attribute, conn, vals, request)
      : this->ContinuousLagrange<false>(attribute, conn, vals, request);
  }
  else if (attType.find("CG HCURL I1") != std::string::npos)
  {
    // TODO: FIXME: Properly handle.
    return request->GetFiniteRange()
      ? this->ContinuousLagrange<true>(attribute, conn, vals, request)
      : this->ContinuousLagrange<false>(attribute, conn, vals, request);
  }
  else
  {
    vtkWarningMacro("Unsupported attribute type \"" << attType << "\".");
    return false;
  }
}

template <bool FiniteRange>
bool vtkDGRangeResponder::DiscontinuousLagrange(
  vtkCellAttribute* attribute, vtkDataArray* values, vtkCellGridRangeQuery* request)
{
  int componentIndex = request->GetComponent();
  int cc = attribute->GetNumberOfComponents();
  vtkIdType mm = values->GetNumberOfComponents();
  vtkIdType nn = values->GetNumberOfTuples();
  if (nn <= 0)
  {
    return true;
  }

  std::array<double, 2> range{ vtkMath::Inf(), -vtkMath::Inf() };
  std::vector<double> tuple;
  tuple.resize(mm);
  switch (componentIndex)
  {
    case -2: // L₂ norm
      // Loop over all cells:
      for (vtkIdType ii = 0; ii < nn; ++ii)
      {
        // Loop over all collocation points in cell ii:
        values->GetTuple(ii, &tuple[0]);
        for (int jj = 0; jj < mm; jj += cc)
        {
          // Loop over all components of collocation point jj:
          double magnitudeSquared = 0.;
          for (int kk = 0; kk < cc; ++kk)
          {
            magnitudeSquared += tuple[jj + kk] * tuple[jj + kk];
          }
          double magnitude = std::sqrt(magnitudeSquared);
          if (FiniteRange && (vtkMath::IsNan(magnitude) || vtkMath::IsInf(magnitude)))
          {
            continue;
          }
          else if (!FiniteRange && vtkMath::IsNan(magnitude))
          {
            // If we find a NaN, the entire range is undefined. Return immediately
            range[0] = magnitude;
            range[1] = magnitude;
            request->AddRange(range);
            return true;
          }
          if (range[0] > magnitude)
          {
            range[0] = magnitude;
          }
          if (range[1] < magnitude)
          {
            range[1] = magnitude;
          }
        }
      }
      break;
    case -1: // L₁ norm
      // Loop over all cells:
      for (vtkIdType ii = 0; ii < nn; ++ii)
      {
        // Loop over all components of all collocation points in cell ii:
        values->GetTuple(ii, &tuple[0]);
        for (int jj = 0; jj < mm; ++jj)
        {
          double value = tuple[jj];
          if (FiniteRange && (vtkMath::IsNan(value) || vtkMath::IsInf(value)))
          {
            continue;
          }
          else if (!FiniteRange && vtkMath::IsNan(value))
          {
            // If we find a NaN, the entire range is undefined. Return immediately
            range[0] = value;
            range[1] = value;
            request->AddRange(range);
            return true;
          }
          if (range[0] > value)
          {
            range[0] = value;
          }
          if (range[1] < value)
          {
            range[1] = value;
          }
        }
      }
      break;
    default:
      for (vtkIdType ii = 0; ii < nn; ++ii)
      {
        values->GetTuple(ii, &tuple[0]);
        for (int jj = componentIndex; jj < mm; jj += cc)
        {
          double value = tuple[jj];
          if (FiniteRange && (vtkMath::IsNan(value) || vtkMath::IsInf(value)))
          {
            continue;
          }
          else if (!FiniteRange && vtkMath::IsNan(value))
          {
            // If we find a NaN, the entire range is undefined. Return immediately
            range[0] = value;
            range[1] = value;
            request->AddRange(range);
            return true;
          }
          if (range[0] > value)
          {
            range[0] = value;
          }
          if (range[1] < value)
          {
            range[1] = value;
          }
        }
      }
      break;
  }
  request->AddRange(range);
  return true;
}

template <bool FiniteRange>
bool vtkDGRangeResponder::ContinuousLagrange(vtkCellAttribute* attribute, vtkTypeInt64Array* conn,
  vtkDataArray* values, vtkCellGridRangeQuery* request)
{
  (void)attribute;
  int componentIndex = request->GetComponent();
  // This will not be true for H(Curl)/H(Div) function spaces:
  // assert(attribute->GetNumberOfComponents() == values->GetNumberOfComponents());
  vtkIdType mm = values->GetNumberOfComponents();
  vtkIdType nn = conn->GetNumberOfTuples();
  if (nn <= 0)
  {
    return true;
  } // Trivially empty range.
  std::array<double, 2> range{ vtkMath::Inf(), -vtkMath::Inf() };
  std::vector<double> tuple;
  std::vector<vtkTypeInt64> cellConn;
  tuple.resize(mm);
  cellConn.resize(conn->GetNumberOfComponents());
  switch (componentIndex)
  {
    case -2: // L₂ norm
      // Loop over all cells:
      for (vtkIdType ii = 0; ii < nn; ++ii)
      {
        // Loop over all (shared) collocation points in cell ii:
        conn->GetTypedTuple(ii, &cellConn[0]);
        for (const auto& pointId : cellConn)
        {
          // Loop over all components of each collocation point:
          values->GetTuple(pointId, &tuple[0]);
          double magnitudeSquared = 0.;
          for (const auto& value : tuple)
          {
            magnitudeSquared += value * value;
          }
          double magnitude = std::sqrt(magnitudeSquared);
          if (FiniteRange && (vtkMath::IsNan(magnitude) || vtkMath::IsInf(magnitude)))
          {
            continue;
          }
          else if (!FiniteRange && vtkMath::IsNan(magnitude))
          {
            // If we find a NaN, the entire range is undefined. Return immediately
            range[0] = magnitude;
            range[1] = magnitude;
            request->AddRange(range);
            return true;
          }
          if (range[0] > magnitude)
          {
            range[0] = magnitude;
          }
          if (range[1] < magnitude)
          {
            range[1] = magnitude;
          }
        }
      }
      break;
    case -1: // L₁ norm
      // Loop over all cells:
      for (vtkIdType ii = 0; ii < nn; ++ii)
      {
        // Loop over all (shared) collocation points in cell ii:
        conn->GetTypedTuple(ii, &cellConn[0]);
        for (const auto& pointId : cellConn)
        {
          // Loop over all components of each collocation point:
          values->GetTuple(pointId, &tuple[0]);
          for (const auto& value : tuple)
          {
            if (FiniteRange && (vtkMath::IsNan(value) || vtkMath::IsInf(value)))
            {
              continue;
            }
            else if (!FiniteRange && vtkMath::IsNan(value))
            {
              // If we find a NaN, the entire range is undefined. Return immediately
              range[0] = value;
              range[1] = value;
              request->AddRange(range);
              return true;
            }
            if (range[0] > value)
            {
              range[0] = value;
            }
            if (range[1] < value)
            {
              range[1] = value;
            }
          }
        }
      }
      break;
    default:
      for (vtkIdType ii = 0; ii < nn; ++ii)
      {
        // Loop over all (shared) collocation points in cell ii:
        conn->GetTypedTuple(ii, &cellConn[0]);
        for (const auto& pointId : cellConn)
        {
          // Loop over all components of each collocation point:
          values->GetTuple(pointId, &tuple[0]);
          double value = tuple[componentIndex];
          if (FiniteRange && (vtkMath::IsNan(value) || vtkMath::IsInf(value)))
          {
            continue;
          }
          else if (!FiniteRange && vtkMath::IsNan(value))
          {
            // If we find a NaN, the entire range is undefined. Return immediately
            range[0] = value;
            range[1] = value;
            request->AddRange(range);
            return true;
          }
          if (range[0] > value)
          {
            range[0] = value;
          }
          if (range[1] < value)
          {
            range[1] = value;
          }
        }
      }
      break;
  }
  request->AddRange(range);
  return true;
}

VTK_ABI_NAMESPACE_END
