/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// created by Shaun David Ramsey and Kristin Potter copyright (c) 2003
// email ramsey()cs.utah.edu with any questions
/*=========================================================================
  This copyright notice is available at:
http://www.opensource.org/licenses/mit-license.php

Copyright (c) 2003 Shaun David Ramsey, Kristin Potter, Charles Hansen

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sel copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
=========================================================================*/
#include "vtkBilinearQuadIntersection.h"

#include "vtkMath.h"

#define RAY_EPSILON 1e-12 // some small epsilon for flt pt

namespace
{

double GetBestDenominator(
  double v, double M1, double M2, double J1, double J2, double K1, double K2, double R1, double R2)
{
  double denom = (v * (M1 - M2) + J1 - J2);
  double d2 = (v * M1 + J1);
  if (fabs(denom) > fabs(d2)) // which denominator is bigger
  {
    return (v * (K2 - K1) + R2 - R1) / denom;
  }
  return -(v * K1 + R1) / d2;
}

double ComputeIntersectionFactor(
  const vtkVector3d& dir, const vtkVector3d& orig, const vtkVector3d& srfpos)
{
  // if x is bigger than y and z
  if (fabs(dir.GetX()) >= fabs(dir.GetY()) && fabs(dir.GetX()) >= fabs(dir.GetZ()))
  {
    return (srfpos.GetX() - orig.GetX()) / dir.GetX();
  }
  // if y is bigger than x and z
  else if (fabs(dir.GetY()) >= fabs(dir.GetZ())) // && fabs(dir.GetY()) >= fabs(dir.GetX()))
  {
    return (srfpos.GetY() - orig.GetY()) / dir.GetY();
  }
  // otherwise x isn't bigger than both and y isn't bigger than both
  else // if(fabs(dir.GetZ()) >= fabs(dir.GetX()) && fabs(dir.GetZ()) >= fabs(dir.GetY()))
  {
    return (srfpos.GetZ() - orig.GetZ()) / dir.GetZ();
  }
}
}

//----------------------------------------------------------------------------
vtkBilinearQuadIntersection::vtkBilinearQuadIntersection(const vtkVector3d& pt00,
  const vtkVector3d& pt01, const vtkVector3d& pt10, const vtkVector3d& pt11)
  : Point00(pt00.GetData())
  , Point01(pt01.GetData())
  , Point10(pt10.GetData())
  , Point11(pt11.GetData())
{
}

//----------------------------------------------------------------------------
double* vtkBilinearQuadIntersection::GetP00Data()
{
  return this->Point00.GetData();
}

//----------------------------------------------------------------------------
double* vtkBilinearQuadIntersection::GetP01Data()
{
  return this->Point01.GetData();
}

//----------------------------------------------------------------------------
double* vtkBilinearQuadIntersection::GetP10Data()
{
  return this->Point10.GetData();
}

//----------------------------------------------------------------------------
double* vtkBilinearQuadIntersection::GetP11Data()
{
  return this->Point11.GetData();
}

//----------------------------------------------------------------------------
vtkVector3d vtkBilinearQuadIntersection::ComputeCartesianCoordinates(double u, double v)
{
  vtkVector3d respt;
  respt.SetX(((1.0 - u) * (1.0 - v) * this->Point00.GetX() + (1.0 - u) * v * this->Point01.GetX() +
    u * (1.0 - v) * this->Point10.GetX() + u * v * this->Point11.GetX()));
  respt.SetY(((1.0 - u) * (1.0 - v) * this->Point00.GetY() + (1.0 - u) * v * this->Point01.GetY() +
    u * (1.0 - v) * this->Point10.GetY() + u * v * this->Point11.GetY()));
  respt.SetZ(((1.0 - u) * (1.0 - v) * this->Point00.GetZ() + (1.0 - u) * v * this->Point01.GetZ() +
    u * (1.0 - v) * this->Point10.GetZ() + u * v * this->Point11.GetZ()));

  int nbOfSwap = this->AxesSwapping;
  while (nbOfSwap != 0)
  {
    double tmp = respt.GetZ();
    respt.SetZ(respt.GetY());
    respt.SetY(respt.GetX());
    respt.SetX(tmp);
    nbOfSwap--;
  }
  return respt;
}

//----------------------------------------------------------------------------
bool vtkBilinearQuadIntersection::RayIntersection(
  const vtkVector3d& r, const vtkVector3d& q, vtkVector3d& uv)
{
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Equation of the ray intersection:
  // P(u, v) = (1-u)(1-v)this->Point00.+ (1-u)vthis->Point01.+
  //   u(1-v)this->Point10.+ uvthis->Point11
  // Equation of the ray:
  // R(t) = r + tq
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  vtkVector3d pos1, pos2; // vtkVector3d pos = ro + t * rd;
  int num_sol;            // number of solutions to the quadratic
  double vsol[2];         // the two roots from quadraticroot
  double t2, u;           // the t values of the two roots

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Variables for substitition
  // a = this->Point11.- this->Point10.- this->Point01.+ this->Point00
  // b = this->Point10.- this->Point00
  // c = this->Point01.- this->Point00
  // d = this->Point00. (d is shown below in the #ifdef ray area)
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Retrieve the xyz of the q part of ray
  double qx = q.GetX();
  double qy = q.GetY();
  double qz = q.GetZ();

  double rx = r.GetX();
  double ry = r.GetY();
  double rz = r.GetZ();

  this->AxesSwapping = 0;
  while (qz == 0.0 && this->AxesSwapping < 3)
  {
    this->AxesSwapping++;
    double tmp;

    tmp = qx;
    qx = qy;
    qy = qz;
    qz = tmp;

    tmp = rx;
    rx = ry;
    ry = rz;
    rz = tmp;

    tmp = this->Point00.GetX();
    this->Point00.SetX(this->Point00.GetY());
    this->Point00.SetY(this->Point00.GetZ());
    this->Point00.SetZ(tmp);

    tmp = this->Point01.GetX();
    this->Point01.SetX(this->Point01.GetY());
    this->Point01.SetY(this->Point01.GetZ());
    this->Point01.SetZ(tmp);

    tmp = this->Point10.GetX();
    this->Point10.SetX(this->Point10.GetY());
    this->Point10.SetY(this->Point10.GetZ());
    this->Point10.SetZ(tmp);

    tmp = this->Point11.GetX();
    this->Point11.SetX(this->Point11.GetY());
    this->Point11.SetY(this->Point11.GetZ());
    this->Point11.SetZ(tmp);
  }

  // Find a w.r.t. x, y, z
  double ax =
    this->Point11.GetX() - this->Point10.GetX() - this->Point01.GetX() + this->Point00.GetX();
  double ay =
    this->Point11.GetY() - this->Point10.GetY() - this->Point01.GetY() + this->Point00.GetY();
  double az =
    this->Point11.GetZ() - this->Point10.GetZ() - this->Point01.GetZ() + this->Point00.GetZ();

  // Find b w.r.t. x, y, z
  double bx = this->Point10.GetX() - this->Point00.GetX();
  double by = this->Point10.GetY() - this->Point00.GetY();
  double bz = this->Point10.GetZ() - this->Point00.GetZ();

  // Find c w.r.t. x, y, z
  double cx = this->Point01.GetX() - this->Point00.GetX();
  double cy = this->Point01.GetY() - this->Point00.GetY();
  double cz = this->Point01.GetZ() - this->Point00.GetZ();

  // Find d w.r.t. x, y, z - subtracting r just after
  double dx = this->Point00.GetX() - rx;
  double dy = this->Point00.GetY() - ry;
  double dz = this->Point00.GetZ() - rz;

  // Find A1 and A2
  double A1 = ax * qz - az * qx;
  double A2 = ay * qz - az * qy;

  // Find B1 and B2
  double B1 = bx * qz - bz * qx;
  double B2 = by * qz - bz * qy;

  // Find C1 and C2
  double C1 = cx * qz - cz * qx;
  double C2 = cy * qz - cz * qy;

  // Find D1 and D2
  double D1 = dx * qz - dz * qx;
  double D2 = dy * qz - dz * qy;

  double A = A2 * C1 - A1 * C2;
  double B = A2 * D1 - A1 * D2 + B2 * C1 - B1 * C2;
  double C = B2 * D1 - B1 * D2;

  uv.SetX(-2);
  uv.SetY(-2);
  uv.SetZ(-2);
  num_sol = vtkMath::QuadraticRoot(A, B, C, -RAY_EPSILON, 1 + RAY_EPSILON, vsol);
  switch (num_sol)
  {
    case 0:
      return false; // no solutions found
    case 1:
      uv.SetY(vsol[0]);
      uv.SetX(::GetBestDenominator(uv.GetY(), A2, A1, B2, B1, C2, C1, D2, D1));
      pos1 = this->ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
      uv.SetZ(::ComputeIntersectionFactor(q, r, pos1));

      return (uv.GetX() < 1 + RAY_EPSILON && uv.GetX() > -RAY_EPSILON && uv.GetZ() > 0);
    case 2: // two solutions found
      uv.SetY(vsol[0]);
      uv.SetX(::GetBestDenominator(uv.GetY(), A2, A1, B2, B1, C2, C1, D2, D1));
      pos1 = this->ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
      uv.SetZ(::ComputeIntersectionFactor(q, r, pos1));

      if (uv.GetX() < 1 + RAY_EPSILON && uv.GetX() > -RAY_EPSILON && uv.GetZ() > 0)
      {
        u = ::GetBestDenominator(vsol[1], A2, A1, B2, B1, C2, C1, D2, D1);
        if (u < 1 + RAY_EPSILON && u > RAY_EPSILON)
        {
          pos2 = this->ComputeCartesianCoordinates(u, vsol[1]);
          t2 = ::ComputeIntersectionFactor(q, r, pos2);
          if (t2 < 0 || uv.GetZ() < t2) // t2 is bad or t1 is better
          {
            return true;
          }
          // other wise both t2 > 0 and t2 < t1
          uv.SetY(vsol[1]);
          uv.SetX(u);
          uv.SetZ(t2);
          return true;
        }
        return true; // u2 is bad but u1 vars are still okay
      }
      else // doesn't fit in the root - try other one
      {
        uv.SetY(vsol[1]);
        uv.SetX(::GetBestDenominator(vsol[1], A2, A1, B2, B1, C2, C1, D2, D1));
        pos1 = this->ComputeCartesianCoordinates(uv.GetX(), uv.GetY());
        uv.SetZ(::ComputeIntersectionFactor(q, r, pos1));
        return (uv.GetX() < 1 + RAY_EPSILON && uv.GetX() > -RAY_EPSILON && uv.GetZ() > 0);
      }
    default:
      return false;
  }
}
