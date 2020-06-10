/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangeTetra.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLagrangeTetra.h"

#include "vtkDoubleArray.h"
#include "vtkLagrangeCurve.h"
#include "vtkLagrangeTriangle.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkTetra.h"

vtkStandardNewMacro(vtkLagrangeTetra);
//----------------------------------------------------------------------------
vtkLagrangeTetra::vtkLagrangeTetra()
  : vtkHigherOrderTetra()
{
}

//----------------------------------------------------------------------------
vtkLagrangeTetra::~vtkLagrangeTetra() = default;

void vtkLagrangeTetra::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCell* vtkLagrangeTetra::GetEdge(int edgeId)
{
  vtkLagrangeCurve* result = EdgeCell;
  const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
    result->Points->SetNumberOfPoints(npts);
    result->PointIds->SetNumberOfIds(npts);
  };
  const auto set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void {
    result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
    result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
  };

  this->SetEdgeIdsAndPoints(edgeId, set_number_of_ids_and_points, set_ids_and_points);
  return result;
}

vtkCell* vtkLagrangeTetra::GetFace(int faceId)
{
  vtkLagrangeTriangle* result = FaceCell;
  const auto set_number_of_ids_and_points = [&](const vtkIdType& npts) -> void {
    result->Points->SetNumberOfPoints(npts);
    result->PointIds->SetNumberOfIds(npts);
  };
  const auto set_ids_and_points = [&](const vtkIdType& face_id, const vtkIdType& vol_id) -> void {
    result->Points->SetPoint(face_id, this->Points->GetPoint(vol_id));
    result->PointIds->SetId(face_id, this->PointIds->GetId(vol_id));
  };

  this->SetFaceIdsAndPoints(result, faceId, set_number_of_ids_and_points, set_ids_and_points);
  return result;
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::InterpolateFunctions(const double pcoords[3], double* weights)
{
  // Adapted from P. Silvester, "High-Order Polynomial Triangular Finite
  // Elements for Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861.
  // Pergamon Press, 1969. The generic method is valid for all orders, but we
  // unroll the first two orders to reduce computational cost.

  double tau[4] = { pcoords[0], pcoords[1], pcoords[2], 1. - pcoords[0] - pcoords[1] - pcoords[2] };

  vtkIdType n = this->GetOrder();

  if (n == 1)
  {
    // for the linear case, we simply return the parametric coordinates, rotated
    // into the parametric frame (e.g. barycentric tau_3 = parametric x).
    weights[0] = tau[3];
    weights[1] = tau[0];
    weights[2] = tau[1];
    weights[3] = tau[2];
  }
  else if (n == 2)
  {
    if (this->GetPoints()->GetNumberOfPoints() == 15)
    {
      double u = tau[3], r = tau[0], s = tau[1], t = tau[2];
      double ur = u * r, us = u * s, ut = u * t, rs = r * s, rt = r * t, st = s * t;
      double urs = ur * s, urt = ur * t, ust = us * t, rst = rs * t;
      double urst = urs * t;

      weights[0] = u - 2.0 * (ur + us + ut) + 3.0 * (urs + urt + ust) - 4.0 * urst;
      weights[1] = r - 2.0 * (ur + rs + rt) + 3.0 * (urs + urt + rst) - 4.0 * urst;
      weights[2] = s - 2.0 * (rs + us + st) + 3.0 * (urs + rst + ust) - 4.0 * urst;
      weights[3] = t - 2.0 * (ut + rt + st) + 3.0 * (urt + ust + rst) - 4.0 * urst;
      weights[4] = 4.0 * ur - 12.0 * (urs + urt) + 32.0 * urst;
      weights[5] = 4.0 * rs - 12.0 * (urs + rst) + 32.0 * urst;
      weights[6] = 4.0 * us - 12.0 * (urs + ust) + 32.0 * urst;
      weights[7] = 4.0 * ut - 12.0 * (urt + ust) + 32.0 * urst;
      weights[8] = 4.0 * rt - 12.0 * (urt + rst) + 32.0 * urst;
      weights[9] = 4.0 * st - 12.0 * (rst + ust) + 32.0 * urst;
      weights[10] = 27.0 * urs - 108.0 * urst;
      weights[11] = 27.0 * urt - 108.0 * urst;
      weights[12] = 27.0 * rst - 108.0 * urst;
      weights[13] = 27.0 * ust - 108.0 * urst;
      weights[14] = 256.0 * urst;
      return;
    }

    weights[0] = tau[3] * (2.0 * tau[3] - 1.0);
    weights[1] = tau[0] * (2.0 * tau[0] - 1.0);
    weights[2] = tau[1] * (2.0 * tau[1] - 1.0);
    weights[3] = tau[2] * (2.0 * tau[2] - 1.0);
    weights[4] = 4.0 * tau[3] * tau[0];
    weights[5] = 4.0 * tau[0] * tau[1];
    weights[6] = 4.0 * tau[1] * tau[3];
    weights[7] = 4.0 * tau[2] * tau[3];
    weights[8] = 4.0 * tau[0] * tau[2];
    weights[9] = 4.0 * tau[1] * tau[2];
  }
  else
  {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

    for (vtkIdType idx = 0; idx < nPoints; idx++)
    {
      vtkIdType lambda[4];
      this->ToBarycentricIndex(idx, lambda);

      weights[idx] = (vtkLagrangeTriangle::eta(n, lambda[0], tau[0]) *
        vtkLagrangeTriangle::eta(n, lambda[1], tau[1]) *
        vtkLagrangeTriangle::eta(n, lambda[2], tau[2]) *
        vtkLagrangeTriangle::eta(n, lambda[3], tau[3]));
    }
  }
}

//----------------------------------------------------------------------------
void vtkLagrangeTetra::InterpolateDerivs(const double pcoords[3], double* derivs)
{
  // Analytic differentiation of the tetra shape functions, as adapted from
  // P. Silvester, "High-Order Polynomial Triangular Finite Elements for
  // Potential Problems". Int. J. Engng Sci. Vol. 7, pp. 849-861. Pergamon
  // Press, 1969. The generic method is valid for all orders, but we unroll the
  // first two orders to reduce computational cost.

  double tau[4] = { pcoords[0], pcoords[1], pcoords[2], 1. - pcoords[0] - pcoords[1] - pcoords[2] };

  vtkIdType n = this->GetOrder();

  if (n == 1)
  {
    derivs[0] = -1.0;
    derivs[1] = 1.0;
    derivs[2] = 0.0;
    derivs[3] = 0.0;
    derivs[4] = -1.0;
    derivs[5] = 0.0;
    derivs[6] = 1.0;
    derivs[7] = 0.0;
    derivs[8] = -1.0;
    derivs[9] = 0.0;
    derivs[10] = 0.0;
    derivs[11] = 1.0;
  }
  else if (n == 2)
  {
    if (this->GetPoints()->GetNumberOfPoints() == 15)
    {
      double r = tau[0], s = tau[1], t = tau[2], u = tau[3];
      double rs = r * s, rt = r * t, st = s * t;
      double umr = u - r, ums = u - s, umt = u - t;
      double *dWdr = &derivs[0], *dWds = &derivs[15], *dWdt = &derivs[30];

      dWdr[0] = 1.0 - 4.0 * u + 3.0 * ((s + t) * umr - st) - 4.0 * st * umr;
      dWdr[1] = 1.0 - 2.0 * (umr + s + t) + 3.0 * ((s + t) * umr + st) - 4.0 * st * umr;
      dWdr[2] = 3.0 * s * umr - 4.0 * st * umr;
      dWdr[3] = 3.0 * t * umr - 4.0 * st * umr;
      dWdr[4] = 4.0 * umr - 12.0 * umr * (s + t) + 32.0 * st * umr;
      dWdr[5] = 4.0 * s - 12.0 * s * (umr + t) + 32.0 * st * umr;
      dWdr[6] = -4.0 * s - 12.0 * s * (umr - t) + 32.0 * st * umr;
      dWdr[7] = -4.0 * t - 12.0 * t * (umr - s) + 32.0 * st * umr;
      dWdr[8] = 4.0 * t - 12.0 * t * (umr + s) + 32.0 * st * umr;
      dWdr[9] = 32.0 * st * umr;
      dWdr[10] = 27.0 * s * umr - 108.0 * st * umr;
      dWdr[11] = 27.0 * t * umr - 108.0 * st * umr;
      dWdr[12] = 27.0 * st - 108.0 * st * umr;
      dWdr[13] = -27.0 * st - 108.0 * st * umr;
      dWdr[14] = 256.0 * st * umr;

      dWds[0] = 1.0 - 4.0 * u + 3.0 * ((r + t) * ums - rt) - 4.0 * rt * ums;
      dWds[1] = 3.0 * r * ums - 4.0 * rt * ums;
      dWds[2] = 1 - 2.0 * (r + ums + t) + 3.0 * ((r + t) * ums + rt) - 4.0 * rt * ums;
      dWds[3] = 3.0 * t * ums - 4.0 * rt * ums;
      dWds[4] = -4.0 * r - 12.0 * r * (ums - t) + 32.0 * rt * ums;
      dWds[5] = 4.0 * r - 12.0 * r * (ums + t) + 32.0 * rt * ums;
      dWds[6] = 4.0 * ums - 12.0 * ums * (r + t) + 32.0 * rt * ums;
      dWds[7] = -4.0 * t - 12.0 * t * (ums - r) + 32.0 * rt * ums;
      dWds[8] = 32.0 * rt * ums;
      dWds[9] = 4.0 * t - 12.0 * t * (r + ums) + 32.0 * rt * ums;
      dWds[10] = 27.0 * r * ums - 108.0 * rt * ums;
      dWds[11] = -27.0 * rt - 108.0 * rt * ums;
      dWds[12] = 27.0 * rt - 108.0 * rt * ums;
      dWds[13] = 27.0 * t * ums - 108.0 * rt * ums;
      dWds[14] = 256.0 * rt * ums;

      dWdt[0] = 1.0 - 4.0 * u + 3.0 * ((r + s) * umt - rs) - 4.0 * rs * umt;
      dWdt[1] = 3.0 * r * umt - 4.0 * rs * umt;
      dWdt[2] = 3.0 * s * umt - 4.0 * rs * umt;
      dWdt[3] = 1 - 2.0 * (umt + r + s) + 3.0 * ((r + s) * umt + rs) - 4.0 * rs * umt;
      dWdt[4] = -4.0 * r - 12.0 * r * (umt - s) + 32.0 * rs * umt;
      dWdt[5] = 32.0 * rs * umt;
      dWdt[6] = -4.0 * s - 12.0 * s * (umt - r) + 32.0 * rs * umt;
      dWdt[7] = 4.0 * umt - 12.0 * umt * (r + s) + 32.0 * rs * umt;
      dWdt[8] = 4.0 * r - 12.0 * r * (umt + s) + 32.0 * rs * umt;
      dWdt[9] = 4.0 * s - 12.0 * s * (r + umt) + 32.0 * rs * umt;
      dWdt[10] = -27.0 * rs - 108.0 * rs * umt;
      dWdt[11] = 27.0 * r * umt - 108.0 * rs * umt;
      dWdt[12] = 27.0 * rs - 108.0 * rs * umt;
      dWdt[13] = 27.0 * s * umt - 108.0 * rs * umt;
      dWdt[14] = 256.0 * rs * umt;

      return;
    }
    derivs[0] = 1.0 - 4.0 * tau[3];
    derivs[1] = 4.0 * tau[0] - 1.0;
    derivs[2] = 0.0;
    derivs[3] = 0.0;
    derivs[4] = 4.0 * (tau[3] - tau[0]);
    derivs[5] = 4.0 * tau[1];
    derivs[6] = -4.0 * tau[1];
    derivs[7] = -4.0 * tau[2];
    derivs[8] = 4.0 * tau[2];
    derivs[9] = 0.0;
    derivs[10] = 1.0 - 4.0 * tau[3];
    derivs[11] = 0.0;
    derivs[12] = 4.0 * tau[1] - 1.0;
    derivs[13] = 0.0;
    derivs[14] = -4.0 * tau[0];
    derivs[15] = 4.0 * tau[0];
    derivs[16] = 4.0 * (tau[3] - tau[1]);
    derivs[17] = -4.0 * tau[2];
    derivs[18] = 0.0;
    derivs[19] = 4.0 * tau[2];
    derivs[20] = 1.0 - 4.0 * tau[3];
    derivs[21] = 0.0;
    derivs[22] = 0.0;
    derivs[23] = 4.0 * tau[2] - 1.0;
    derivs[24] = -4.0 * tau[0];
    derivs[25] = 0.0;
    derivs[26] = -4.0 * tau[1];
    derivs[27] = 4.0 * (tau[3] - tau[2]);
    derivs[28] = 4.0 * tau[0];
    derivs[29] = 4.0 * tau[1];
  }
  else
  {
    vtkIdType nPoints = this->GetPoints()->GetNumberOfPoints();

    for (vtkIdType idx = 0; idx < nPoints; idx++)
    {
      vtkIdType lambda[4];
      this->ToBarycentricIndex(idx, lambda);

      double eta_alpha = vtkLagrangeTriangle::eta(n, lambda[0], tau[0]);
      double eta_beta = vtkLagrangeTriangle::eta(n, lambda[1], tau[1]);
      double eta_gamma = vtkLagrangeTriangle::eta(n, lambda[2], tau[2]);
      double eta_delta = vtkLagrangeTriangle::eta(n, lambda[3], tau[3]);

      double d_eta_alpha = vtkLagrangeTriangle::d_eta(n, lambda[0], tau[0]);
      double d_eta_beta = vtkLagrangeTriangle::d_eta(n, lambda[1], tau[1]);
      double d_eta_gamma = vtkLagrangeTriangle::d_eta(n, lambda[2], tau[2]);
      double d_eta_delta = vtkLagrangeTriangle::d_eta(n, lambda[3], tau[3]);

      double d_f_d_tau1 = (d_eta_alpha * eta_beta * eta_gamma * eta_delta -
        eta_alpha * eta_beta * eta_gamma * d_eta_delta);
      double d_f_d_tau2 = (eta_alpha * d_eta_beta * eta_gamma * eta_delta -
        eta_alpha * eta_beta * eta_gamma * d_eta_delta);
      double d_f_d_tau3 = (eta_alpha * eta_beta * d_eta_gamma * eta_delta -
        eta_alpha * eta_beta * eta_gamma * d_eta_delta);
      // double d_f_d_tau4 = (eta_alpha*eta_beta*eta_gamma*d_eta_delta -
      //                      d_eta_alpha*eta_beta*eta_gamma*eta_delta -
      //                      eta_alpha*d_eta_beta*eta_gamma*eta_delta -
      //                      eta_alpha*eta_beta*d_eta_gamma*eta_delta);

      derivs[idx] = d_f_d_tau1;
      derivs[nPoints + idx] = d_f_d_tau2;
      derivs[2 * nPoints + idx] = d_f_d_tau3;
    }
  }
}
vtkHigherOrderCurve* vtkLagrangeTetra::getEdgeCell()
{
  return EdgeCell;
}
vtkHigherOrderTriangle* vtkLagrangeTetra::getFaceCell()
{
  return FaceCell;
}
