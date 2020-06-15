/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeTriangle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeTriangle.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLagrangeCurve.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"

#define ENABLE_CACHING
#define SEVEN_POINT_TRIANGLE

vtkStandardNewMacro(vtkLagrangeTriangle);
//----------------------------------------------------------------------------
vtkLagrangeTriangle::vtkLagrangeTriangle()
  : vtkHigherOrderTriangle()
{
}

//----------------------------------------------------------------------------
vtkLagrangeTriangle::~vtkLagrangeTriangle() = default;

void vtkLagrangeTriangle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkLagrangeTriangle::GetEdge(int edgeId)
{
  vtkLagrangeCurve* result = EdgeCell;
  const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
    result->Points->SetNumberOfPoints(npts);
    result->PointIds->SetNumberOfIds(npts);
  };
  const auto set_ids_and_points = [&](const vtkIdType& edge_id, const vtkIdType& face_id) -> void {
    result->Points->SetPoint(edge_id, this->Points->GetPoint(face_id));
    result->PointIds->SetId(edge_id, this->PointIds->GetId(face_id));
  };

  this->SetEdgeIdsAndPoints(edgeId, set_number_of_ids_and_points, set_ids_and_points);
  return result;
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::InterpolateFunctions(const double pcoords[3], double* weights)
{
  // Adapted from P. Silvester, "High-Order Polynomial Triangular Finite
  // Elements for Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861.
  // Pergamon Press, 1969. The generic method is valid for all orders, but we
  // unroll the first two orders to reduce computational cost.

  double tau[3] = { pcoords[0], pcoords[1], 1. - pcoords[0] - pcoords[1] };

  vtkIdType n = this->GetOrder();

  if (n == 1)
  {
    // for the linear case, we simply return the parametric coordinates, rotated
    // into the parametric frame (e.g. barycentric tau_2 = parametric x).
    weights[0] = tau[2];
    weights[1] = tau[0];
    weights[2] = tau[1];
  }
  else if (n == 2)
  {
#ifdef SEVEN_POINT_TRIANGLE
    if (this->GetPoints()->GetNumberOfPoints() == 7)
    {
      double rs = tau[0] * tau[1];
      double rt = tau[0] * tau[2];
      double st = tau[1] * tau[2];
      double rst = rs * tau[2];
      weights[0] = tau[2] + 3.0 * rst - 2.0 * rt - 2.0 * st;
      weights[1] = tau[0] + 3.0 * rst - 2.0 * rt - 2.0 * rs;
      weights[2] = tau[1] + 3.0 * rst - 2.0 * rs - 2.0 * st;
      weights[3] = 4.0 * rt - 12.0 * rst;
      weights[4] = 4.0 * rs - 12.0 * rst;
      weights[5] = 4.0 * st - 12.0 * rst;
      weights[6] = 27.0 * rst;
      return;
    }
#endif
    weights[0] = tau[2] * (2.0 * tau[2] - 1.0);
    weights[1] = tau[0] * (2.0 * tau[0] - 1.0);
    weights[2] = tau[1] * (2.0 * tau[1] - 1.0);
    weights[3] = 4.0 * tau[0] * tau[2];
    weights[4] = 4.0 * tau[0] * tau[1];
    weights[5] = 4.0 * tau[1] * tau[2];
  }
  else
  {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

    for (vtkIdType idx = 0; idx < nPoints; idx++)
    {
      weights[idx] = 1.;
      vtkIdType lambda[3];
      this->ToBarycentricIndex(idx, lambda);

      for (vtkIdType dim = 0; dim < 3; dim++)
      {
        weights[idx] *= eta(n, lambda[dim], tau[dim]);
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkLagrangeTriangle::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  // Analytic differentiation of the triangle shape functions, as defined in
  // P. Silvester, "High-Order Polynomial Triangular Finite Elements for
  // Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861. Pergamon
  // Press, 1969. The generic method is valid for all orders, but we unroll the
  // first two orders to reduce computational cost.

  double tau[3] = { pcoords[0], pcoords[1], 1. - pcoords[0] - pcoords[1] };

  vtkIdType n = this->GetOrder();

  if (n == 1)
  {
    derivs[0] = -1;
    derivs[1] = 1;
    derivs[2] = 0;
    derivs[3] = -1;
    derivs[4] = 0;
    derivs[5] = 1;
  }
  else if (n == 2)
  {
#ifdef SEVEN_POINT_TRIANGLE
    if (this->GetPoints()->GetNumberOfPoints() == 7)
    {
      double tmr = tau[2] - tau[0];
      double tms = tau[2] - tau[1];
      derivs[0] = -1.0 + 3.0 * tau[1] * tmr - 2.0 * tmr + 2.0 * tau[1];
      derivs[1] = 1.0 + 3.0 * tau[1] * tmr - 2.0 * tmr - 2.0 * tau[1];
      derivs[2] = 3.0 * tau[1] * tmr;
      derivs[3] = 4.0 * tmr - 12.0 * tau[1] * tmr;
      derivs[4] = 4.0 * tau[1] - 12.0 * tau[1] * tmr;
      derivs[5] = -4.0 * tau[1] - 12.0 * tau[1] * tmr;
      derivs[6] = 27.0 * tau[1] * tmr;
      derivs[7] = -1.0 + 3.0 * tau[0] * tms - 2.0 * tms + 2.0 * tau[0];
      derivs[8] = 3.0 * tau[0] * tms;
      derivs[9] = 1.0 + 3.0 * tau[0] * tms - 2.0 * tms - 2.0 * tau[0];
      derivs[10] = -4.0 * tau[0] - 12.0 * tau[0] * tms;
      derivs[11] = 4.0 * tau[0] - 12.0 * tau[0] * tms;
      derivs[12] = 4.0 * tms - 12.0 * tau[0] * tms;
      derivs[13] = 27.0 * tau[0] * tms;
      return;
    }
#endif
    derivs[0] = 1.0 - 4.0 * tau[2];
    derivs[1] = 4.0 * tau[0] - 1.0;
    derivs[2] = 0.0;
    derivs[3] = 4.0 * (tau[2] - tau[0]);
    derivs[4] = 4.0 * tau[1];
    derivs[5] = -4.0 * tau[1];
    derivs[6] = 1.0 - 4.0 * tau[2];
    derivs[7] = 0.0;
    derivs[8] = 4.0 * tau[1] - 1.0;
    derivs[9] = -4.0 * tau[0];
    derivs[10] = 4.0 * tau[0];
    derivs[11] = 4.0 * (tau[2] - tau[1]);
  }
  else
  {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

    for (vtkIdType idx = 0; idx < nPoints; idx++)
    {
      vtkIdType lambda[3];
      this->ToBarycentricIndex(idx, lambda);

      double eta_alpha = eta(n, lambda[0], tau[0]);
      double eta_beta = eta(n, lambda[1], tau[1]);
      double eta_gamma = eta(n, lambda[2], tau[2]);

      double d_eta_alpha = d_eta(n, lambda[0], tau[0]);
      double d_eta_beta = d_eta(n, lambda[1], tau[1]);
      double d_eta_gamma = d_eta(n, lambda[2], tau[2]);

      double d_f_d_tau1 = (d_eta_alpha * eta_beta * eta_gamma - eta_alpha * eta_beta * d_eta_gamma);
      double d_f_d_tau2 = (eta_alpha * d_eta_beta * eta_gamma - eta_alpha * eta_beta * d_eta_gamma);

      derivs[idx] = d_f_d_tau1;
      derivs[nPoints + idx] = d_f_d_tau2;
    }
  }
}

vtkHigherOrderCurve* vtkLagrangeTriangle::getEdgeCell()
{
  return EdgeCell;
}
