// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGLagrangePoints.h"

#include "vtkDGHGradOperators.h"
#include "vtkObjectFactory.h"

using namespace vtk::literals;

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkDGLagrangePoints);

void vtkDGLagrangePoints::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Shape: " << vtkDGCell::GetShapeName(this->Shape).Data() << "\n";
  os << indent << "FunctionSpace: " << this->FunctionSpace.Data() << "\n";
  os << indent << "Basis: " << this->Basis.Data() << "\n";
  os << indent << "Order: [";
  for (const auto& axisOrder : this->Order)
  {
    os << " " << axisOrder;
  }
  os << " ]\n";
}

vtkSmartPointer<vtkCellAttributeCalculator> vtkDGLagrangePoints::PrepareForGrid(
  vtkCellMetadata* cell, vtkCellAttribute* attribute)
{
  auto* dgCell = vtkDGCell::SafeDownCast(cell);
  if (!dgCell || !attribute)
  {
    vtkErrorMacro("Null cell or attribute.");
    return nullptr;
  }

  auto cellTypeInfo = attribute->GetCellTypeInfo(dgCell->GetClassName());
  auto result = vtkSmartPointer<vtkDGLagrangePoints>::New();
  result->Shape = dgCell->GetShape();
  result->FunctionSpace = cellTypeInfo.FunctionSpace;
  result->Basis = cellTypeInfo.Basis;
  result->Order = dgCell->GetBasisOrder(cellTypeInfo);
  return result;
}

bool vtkDGLagrangePoints::GetParameters(std::vector<std::vector<double>>& params) const
{
  if (this->Shape == vtkDGCell::Shape::None || !this->Basis.IsValid())
  {
    return false;
  }

  params.clear();

  // A vertex has no parametric axes, so it has a single degree of freedom whose
  // "point" carries no coordinates.
  if (this->Order.empty())
  {
    params.emplace_back();
    return true;
  }

  // The tabulated points describe the hand-written, fixed-order HGrad "C"
  // bases, which number their degrees of freedom corner-first. The
  // arbitrary-order "A" bases - both HGrad's and Bezier's - number theirs
  // lexicographically instead, so the table must only be consulted for a
  // fixed-order "C" basis. Getting this wrong is not obvious: the points
  // would still be the right set and would still serve to sample a basis,
  // but they would be paired with the wrong degrees of freedom.
  bool isLagrange = this->FunctionSpace == "HGRAD"_token || this->FunctionSpace == "lagrange"_token;
  // Within HGrad, mirror the way vtkDGCell::GetOperatorEntry() chooses an
  // operator: a basis registered for this exact order wins, and only when there
  // is none does the arbitrary-order basis apply.
  bool isotropic = true;
  for (const auto& axisOrder : this->Order)
  {
    isotropic &= axisOrder == this->Order[0];
  }
  if (isLagrange && isotropic &&
    vtk::basis::hgrad::FixedOrderLagrangePoints(this->Shape, this->Basis, this->Order[0], params))
  {
    return true;
  }

  // The "G" bases are nodal on Gauss points rather than on the uniform lattice
  // the table above describes, so they have their own point set. It reports no
  // points for the tetrahedron and pyramid, whose "G" bases are modal.
  if (isLagrange && isotropic && this->Basis == "G"_token)
  {
    return vtk::basis::hgrad::FixedOrderGaussPoints(this->Shape, this->Order[0], params);
  }

  // Only the arbitrary-order basis is defined at an arbitrary order. A fixed
  // basis that reaches here has no table entry above and so no nodal points
  // to report - the pyramid's 18-node "C2" basis and the tetrahedron's and
  // pyramid's modal "G" bases are the examples, whose functions do not
  // interpolate their degrees of freedom anywhere - so return false rather
  // than treating a legitimate case as an error.
  if (this->Basis != "A"_token)
  {
    return false;
  }

  // The points below are uniformly spaced and enumerated with the r-axis
  // varying fastest, matching the arbitrary-order bases in Basis/HGrad.
  const auto& order(this->Order);
  switch (this->Shape)
  {
    case vtkDGCell::Shape::Edge:
      for (int ii = 0; ii <= order[0]; ++ii)
      {
        params.push_back({ order[0] > 0 ? -1. + 2. * ii / order[0] : 0. });
      }
      break;

    case vtkDGCell::Shape::Quadrilateral:
      for (int jj = 0; jj <= order[1]; ++jj)
      {
        double ss = order[1] > 0 ? -1. + 2. * jj / order[1] : 0.;
        for (int ii = 0; ii <= order[0]; ++ii)
        {
          params.push_back({ order[0] > 0 ? -1. + 2. * ii / order[0] : 0., ss });
        }
      }
      break;

    case vtkDGCell::Shape::Hexahedron:
      for (int kk = 0; kk <= order[2]; ++kk)
      {
        double tt = order[2] > 0 ? -1. + 2. * kk / order[2] : 0.;
        for (int jj = 0; jj <= order[1]; ++jj)
        {
          double ss = order[1] > 0 ? -1. + 2. * jj / order[1] : 0.;
          for (int ii = 0; ii <= order[0]; ++ii)
          {
            params.push_back({ order[0] > 0 ? -1. + 2. * ii / order[0] : 0., ss, tt });
          }
        }
      }
      break;

    case vtkDGCell::Shape::Triangle:
    {
      // The principal lattice: every multi-index of non-negative integers
      // summing to the order. A simplex has one total degree, so order[0]
      // governs both axes.
      double scale = order[0] > 0 ? 1. / order[0] : 0.;
      for (int jj = 0; jj <= order[0]; ++jj)
      {
        for (int ii = 0; ii <= order[0] - jj; ++ii)
        {
          params.push_back({ ii * scale, jj * scale });
        }
      }
    }
    break;

    case vtkDGCell::Shape::Tetrahedron:
    {
      double scale = order[0] > 0 ? 1. / order[0] : 0.;
      for (int kk = 0; kk <= order[0]; ++kk)
      {
        for (int jj = 0; jj <= order[0] - kk; ++jj)
        {
          for (int ii = 0; ii <= order[0] - jj - kk; ++ii)
          {
            params.push_back({ ii * scale, jj * scale, kk * scale });
          }
        }
      }
    }
    break;

    case vtkDGCell::Shape::Wedge:
    {
      // A triangle's principal lattice repeated once per point along t, whose
      // order is independent of the triangular cross-section's.
      double scale = order[0] > 0 ? 1. / order[0] : 0.;
      for (int kk = 0; kk <= order[2]; ++kk)
      {
        double tt = order[2] > 0 ? -1. + 2. * kk / order[2] : 0.;
        for (int jj = 0; jj <= order[0]; ++jj)
        {
          for (int ii = 0; ii <= order[0] - jj; ++ii)
          {
            params.push_back({ ii * scale, jj * scale, tt });
          }
        }
      }
    }
    break;

    case vtkDGCell::Shape::Pyramid:
      // We do not support arbitrary-order pyramids.
      return false;

    default:
    case vtkDGCell::Shape::None:
      break;
  }

  if (params.empty())
  {
    vtkErrorMacro("Unsupported shape/order combination.");
    return false;
  }
  return true;
}

VTK_ABI_NAMESPACE_END
