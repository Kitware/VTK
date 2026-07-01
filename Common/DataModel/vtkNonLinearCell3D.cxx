// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkNonLinearCell3D.h"

#include "vtkDataArrayRange.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkPolygon.h"
#include "vtkVector.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
void vtkNonLinearCell3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
// The orientation of a cell is determined by the sign of the Jacobian
// determinant of the parametric-to-world mapping:
//
//   J[i][j] = d(x_i) / d(xi_j) = sum_k  x_k[i] * dN_k/d(xi_j)
//
// where x_k are the world-space control-point coordinates, N_k are the
// shape functions, and xi_j are the parametric coordinates.
//
// det(J) > 0: orientation-preserving mapping  => cell is right-handed
// det(J) < 0: orientation-reversing mapping   => cell is inside-out
//
// The determinant is evaluated at the parametric center, where J is
// well-defined for any non-degenerate cell.
bool vtkNonLinearCell3D::IsInsideOut()
{
  const vtkIdType nPts = this->GetNumberOfPoints();

  double center[3];
  this->GetParametricCenter(center);

  std::vector<double> derivs(3 * nPts, 0.0);
  this->InterpolateDerivs(center, derivs.data());

  // InterpolateDerivs lays out derivatives as:
  //   derivs[0..nPts-1]         = dN_k/d(xi_0)
  //   derivs[nPts..2*nPts-1]    = dN_k/d(xi_1)
  //   derivs[2*nPts..3*nPts-1]  = dN_k/d(xi_2)
  double J[3][3] = {};
  for (vtkIdType k = 0; k < nPts; ++k)
  {
    double pt[3];
    this->Points->GetPoint(k, pt);
    for (int i = 0; i < 3; ++i)
    {
      J[i][0] += pt[i] * derivs[k];
      J[i][1] += pt[i] * derivs[nPts + k];
      J[i][2] += pt[i] * derivs[2 * nPts + k];
    }
  }

  // Determinant3x3(J[0], J[1], J[2]) treats J[0..2] as columns, computing
  // det(J^T) = det(J) — same sign regardless of row/column interpretation.
  return vtkMath::Determinant3x3(J[0], J[1], J[2]) < 0.0;
}

//----------------------------------------------------------------------------
// For each control point p_i, the outward displacement direction is computed
// by summing the parametric-space face normals n_{i,j} over all incident faces
// and transforming the result to world space via D_i (Nanson's formula).
//
// D_i = derivs[3*m+j] = d(xi_m)/d(x_j) = (J^{-1})_{m,j}, obtained by calling
// Derivatives with the cell's parametric coordinates as the values array.
// Applying D_i to nSum_i gives delta[j] = (J^{-T} nSum_i)_j, the correct
// world-space normal direction for all non-orthogonal Jacobians.
//
// Each face normal is area-weighted: the unit outward normal is scaled by the
// face area so that larger faces contribute proportionally more to the
// displacement direction. This is equivalent to summing Newell normals.
// By linearity (David Thompson's simplification), the face normals are
// accumulated first and D_i is applied once: D_i sum_j n_{i,j} = sum_j D_i n_{i,j}.
int vtkNonLinearCell3D::Inflate(double dist)
{
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return 0;
  }

  const vtkIdType nPts = this->GetNumberOfPoints();
  auto pointRange = vtk::DataArrayTupleRange<3>(pointsArray);
  const double* pcoords = this->GetParametricCoords();
  if (!pcoords)
  {
    vtkErrorMacro(<< "GetParametricCoords returned null");
    return 0;
  }

  dist *= this->IsInsideOut() ? -1.0 : 1.0;

  // Buffer to accumulate new positions before write-back, so that later
  // iterations still read the original (pre-inflation) coordinates.
  std::vector<double> buf(3 * nPts);
  std::copy_n(pointsArray->GetPointer(0), 3 * nPts, buf.data());

  // Pre-allocated buffer for shape-function derivatives.
  std::vector<double> sd(3 * nPts);

  for (vtkIdType ptId = 0; ptId < nPts; ++ptId)
  {
    const vtkIdType* incidentFaceIds;
    const vtkIdType nFaces = this->GetPointToIncidentFaces(ptId, incidentFaceIds);

    if (nFaces == 0)
    {
      continue; // CenterPoint: no face participation => no displacement
    }

    const double* ri = pcoords + 3 * ptId;

    // Steps 1–3 together implement what Derivatives(0, ri, pcoords, 3, derivs) does,
    // but split into pieces so that a degenerate Jacobian (e.g. at the quadratic-pyramid apex)
    // is detected and skipped cleanly before any undefined output is produced.

    // Step 1: evaluate shape-function derivatives and build the Jacobian J at ri.
    this->InterpolateDerivs(ri, sd.data());
    double J[3][3] = {};
    for (vtkIdType k = 0; k < nPts; ++k)
    {
      const auto xpt = pointRange[k];
      for (int i = 0; i < 3; ++i)
      {
        J[i][0] += xpt[i] * sd[k];
        J[i][1] += xpt[i] * sd[nPts + k];
        J[i][2] += xpt[i] * sd[2 * nPts + k];
      }
    }
    if (std::abs(vtkMath::Determinant3x3(J[0], J[1], J[2])) < 1e-12)
    {
      continue; // singular Jacobian: displacement direction undefined, leave point in place
    }

    // Step 2: invert J.
    double jInv[3][3] = {};
    vtkMath::Invert3x3(J, jInv); // guaranteed to succeed: det != 0

    // Step 3: compute derivs — mirrors what Derivatives() does after it has
    // jI = J^{-T}: for each output dimension k, accumulate sum[m] over nodes,
    // then project through jInv to get derivs[3*k+j] = d(xi_k)/d(x_j).
    double derivs[9] = {};
    for (int k = 0; k < 3; ++k)
    {
      double sum[3] = {};
      for (vtkIdType i = 0; i < nPts; ++i)
      {
        const double val = pcoords[3 * i + k];
        sum[0] += sd[i] * val;
        sum[1] += sd[nPts + i] * val;
        sum[2] += sd[2 * nPts + i] * val;
      }
      for (int j = 0; j < 3; ++j)
      {
        derivs[3 * k + j] = sum[0] * jInv[0][j] + sum[1] * jInv[1][j] + sum[2] * jInv[2][j];
      }
    }

    // Step 4: accumulate area-weighted face normals over all incident faces.
    // For each face: unit outward normal × face area = area-weighted normal.
    // This is equivalent to Newell's formula and ensures larger faces
    // contribute proportionally more to the displacement direction.
    vtkVector3d nSum(0.0, 0.0, 0.0);
    for (vtkIdType fIdx = 0; fIdx < nFaces; ++fIdx)
    {
      const vtkIdType* facePts;
      const vtkIdType faceSize = this->GetFacePoints(incidentFaceIds[fIdx], facePts);

      if (faceSize < 3)
      {
        continue;
      }

      // Copy the parametric coordinates of the face points into a flat array.
      // Max face size is 9 (quad + face-center node) × 3 coords.
      double faceCoords[27];
      for (vtkIdType fp = 0; fp < faceSize; ++fp)
      {
        const double* p = pcoords + 3 * facePts[fp];
        faceCoords[3 * fp + 0] = p[0];
        faceCoords[3 * fp + 1] = p[1];
        faceCoords[3 * fp + 2] = p[2];
      }

      vtkVector3d n;
      const double area =
        vtkPolygon::ComputeArea(static_cast<int>(faceSize), faceCoords, n.GetData());
      nSum += n * area;
    }

    if (nSum.Norm() < 1e-12)
    {
      continue; // degenerate: all face normals cancel
    }

    // Step 5: compute delta = D_i * nSum = J^{-T} nSum (David's simplification).
    // delta[j] = sum_m derivs[3*m+j] * nSum[m] = (J^{-T} nSum)_j
    vtkVector3d delta(0.0, 0.0, 0.0);
    for (int m = 0; m < 3; ++m)
    {
      for (int j = 0; j < 3; ++j)
      {
        delta[j] += derivs[3 * m + j] * nSum[m];
      }
    }

    // Step 6: normalize and displace.
    const double deltaLen = delta.Norm();
    if (deltaLen < 1e-12)
    {
      continue; // degenerate Jacobian at this point
    }

    delta.Normalize();

    const auto pt = pointRange[ptId];
    buf[3 * ptId + 0] = pt[0] + dist * delta[0];
    buf[3 * ptId + 1] = pt[1] + dist * delta[1];
    buf[3 * ptId + 2] = pt[2] + dist * delta[2];
  }

  std::copy_n(buf.data(), 3 * nPts, pointsArray->GetPointer(0));
  return 1;
}

VTK_ABI_NAMESPACE_END
