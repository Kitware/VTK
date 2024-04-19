/*=========================================================================

  Module:    V_HexMetric.hpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * V_HexMetric.hpp contains declarations of hex related shape quantities
 *
 * This file is part of VERDICT
 *
 */

#ifndef VERDICT_HEX_METRIC_HPP
#define VERDICT_HEX_METRIC_HPP

#include "verdict.h"

#include <cassert>
#include <cmath>

namespace VERDICT_NAMESPACE
{
//
//  Compute jacobian at each of the eight hex corner nodes
//
template <typename T>
void hex_nodal_jacobians(const T coords[24], T Jdet8x[8])
{
  T x0 = coords[0];
  T y0 = coords[1];
  T z0 = coords[2];
  T x1 = coords[3];
  T y1 = coords[4];
  T z1 = coords[5];
  T x2 = coords[6];
  T y2 = coords[7];
  T z2 = coords[8];
  T x3 = coords[9];
  T y3 = coords[10];
  T z3 = coords[11];
  T x4 = coords[12];
  T y4 = coords[13];
  T z4 = coords[14];
  T x5 = coords[15];
  T y5 = coords[16];
  T z5 = coords[17];
  T x6 = coords[18];
  T y6 = coords[19];
  T z6 = coords[20];
  T x7 = coords[21];
  T y7 = coords[22];
  T z7 = coords[23];

  //
  //  Compute the jacobian at each node location
  //
  T x0y1 = x0 * y1 - x1 * y0;
  T x0y2 = x0 * y2 - x2 * y0;
  T x0y3 = x0 * y3 - x3 * y0;
  T x0y4 = x0 * y4 - x4 * y0;
  T x0y5 = x0 * y5 - x5 * y0;
  T x0y7 = x0 * y7 - x7 * y0;

  T x1y2 = x1 * y2 - x2 * y1;
  T x1y3 = x1 * y3 - x3 * y1;
  T x1y4 = x1 * y4 - x4 * y1;
  T x1y5 = x1 * y5 - x5 * y1;
  T x1y6 = x1 * y6 - x6 * y1;

  T x2y3 = x2 * y3 - x3 * y2;
  T x2y5 = x2 * y5 - x5 * y2;
  T x2y6 = x2 * y6 - x6 * y2;
  T x2y7 = x2 * y7 - x7 * y2;

  T x3y4 = x3 * y4 - x4 * y3;
  T x3y6 = x3 * y6 - x6 * y3;
  T x3y7 = x3 * y7 - x7 * y3;

  T x4y5 = x4 * y5 - x5 * y4;
  T x4y6 = x4 * y6 - x6 * y4;
  T x4y7 = x4 * y7 - x7 * y4;

  T x5y6 = x5 * y6 - x6 * y5;
  T x5y7 = x5 * y7 - x7 * y5;

  T x6y7 = x6 * y7 - x7 * y6;

  Jdet8x[0] = (-x1y3 + x1y4 - x3y4) * z0 + (x0y3 - x0y4 + x3y4) * z1 + (-x0y1 + x0y4 - x1y4) * z3 +
    (x0y1 - x0y3 + x1y3) * z4;
  Jdet8x[1] = (-x1y2 + x1y5 - x2y5) * z0 + (x0y2 - x0y5 + x2y5) * z1 + (-x0y1 + x0y5 - x1y5) * z2 +
    (x0y1 - x0y2 + x1y2) * z5;
  Jdet8x[2] = (-x2y3 + x2y6 - x3y6) * z1 + (x1y3 - x1y6 + x3y6) * z2 + (-x1y2 + x1y6 - x2y6) * z3 +
    (x1y2 - x1y3 + x2y3) * z6;
  Jdet8x[3] = (-x2y3 + x2y7 - x3y7) * z0 + (x0y3 - x0y7 + x3y7) * z2 + (-x0y2 + x0y7 - x2y7) * z3 +
    (x0y2 - x0y3 + x2y3) * z7;
  Jdet8x[4] = (-x4y5 + x4y7 - x5y7) * z0 + (x0y5 - x0y7 + x5y7) * z4 + (-x0y4 + x0y7 - x4y7) * z5 +
    (x0y4 - x0y5 + x4y5) * z7;
  Jdet8x[5] = (-x4y5 + x4y6 - x5y6) * z1 + (x1y5 - x1y6 + x5y6) * z4 + (-x1y4 + x1y6 - x4y6) * z5 +
    (x1y4 - x1y5 + x4y5) * z6;
  Jdet8x[6] = (-x5y6 + x5y7 - x6y7) * z2 + (x2y6 - x2y7 + x6y7) * z5 + (-x2y5 + x2y7 - x5y7) * z6 +
    (x2y5 - x2y6 + x5y6) * z7;
  Jdet8x[7] = (-x4y6 + x4y7 - x6y7) * z3 + (x3y6 - x3y7 + x6y7) * z4 + (-x3y4 + x3y7 - x4y7) * z6 +
    (x3y4 - x3y6 + x4y6) * z7;
}
}

#endif
