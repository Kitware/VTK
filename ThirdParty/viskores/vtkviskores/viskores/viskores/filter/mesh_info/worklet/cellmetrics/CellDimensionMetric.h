//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_worklet_cellmetrics_CellDimensionMetric_h
#define viskores_worklet_cellmetrics_CellDimensionMetric_h

/*
 * Mesh quality metric functions that compute the dimension of mesh cells.  The
 * Dimension metric is defined as the volume divided by two times the gradient
 * of the volume.
 *
 * This metric was designed in context of Sandia's Pronto code for stable time
 * step calculation.
 *
 * These metric computations are adapted from the VTK implementation of the
 * Verdict library, which provides a set of mesh/cell metrics for evaluating the
 * geometric qualities of regions of mesh spaces.
 *
 * See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
 * See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this
 * metric)
 */

#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>
#include <viskores/ErrorCode.h>
#include <viskores/VecTraits.h>
#include <viskores/VectorAnalysis.h>

#define UNUSED(expr) (void)(expr);

namespace viskores
{
namespace worklet
{
namespace cellmetrics
{

using FloatType = viskores::FloatDefault;

// ========================= Unsupported cells ==================================

// Dimension is only defined for Hexahedral cell typesk
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellDimensionMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          CellShapeType shape,
                                          viskores::ErrorCode&)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  return OutType(-1.);
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellDimensionMetric(const viskores::IdComponent& numPts,
                                          const PointCoordVecType& pts,
                                          viskores::CellShapeTagHexahedron,
                                          viskores::ErrorCode& ec)
{
  UNUSED(numPts);
  UNUSED(ec);

  OutType gradop[8][3];
  OutType x1 = static_cast<OutType>(pts[0][0]);
  OutType x2 = static_cast<OutType>(pts[1][0]);
  OutType x3 = static_cast<OutType>(pts[2][0]);
  OutType x4 = static_cast<OutType>(pts[3][0]);
  OutType x5 = static_cast<OutType>(pts[4][0]);
  OutType x6 = static_cast<OutType>(pts[5][0]);
  OutType x7 = static_cast<OutType>(pts[6][0]);
  OutType x8 = static_cast<OutType>(pts[7][0]);

  OutType y1 = static_cast<OutType>(pts[0][1]);
  OutType y2 = static_cast<OutType>(pts[1][1]);
  OutType y3 = static_cast<OutType>(pts[2][1]);
  OutType y4 = static_cast<OutType>(pts[3][1]);
  OutType y5 = static_cast<OutType>(pts[4][1]);
  OutType y6 = static_cast<OutType>(pts[5][1]);
  OutType y7 = static_cast<OutType>(pts[6][1]);
  OutType y8 = static_cast<OutType>(pts[7][1]);

  OutType z1 = static_cast<OutType>(pts[0][2]);
  OutType z2 = static_cast<OutType>(pts[1][2]);
  OutType z3 = static_cast<OutType>(pts[2][2]);
  OutType z4 = static_cast<OutType>(pts[3][2]);
  OutType z5 = static_cast<OutType>(pts[4][2]);
  OutType z6 = static_cast<OutType>(pts[5][2]);
  OutType z7 = static_cast<OutType>(pts[6][2]);
  OutType z8 = static_cast<OutType>(pts[7][2]);

  OutType z24 = z2 - z4;
  OutType z52 = z5 - z2;
  OutType z45 = z4 - z5;
  gradop[0][0] = (y2 * (z6 - z3 - z45) + y3 * z24 + y4 * (z3 - z8 - z52) + y5 * (z8 - z6 - z24) +
                  y6 * z52 + y8 * z45) /
    (OutType)12.0;

  OutType z31 = z3 - z1;
  OutType z63 = z6 - z3;
  OutType z16 = z1 - z6;
  gradop[1][0] = (y3 * (z7 - z4 - z16) + y4 * z31 + y1 * (z4 - z5 - z63) + y6 * (z5 - z7 - z31) +
                  y7 * z63 + y5 * z16) /
    (OutType)12.0;

  OutType z42 = z4 - z2;
  OutType z74 = z7 - z4;
  OutType z27 = z2 - z7;
  gradop[2][0] = (y4 * (z8 - z1 - z27) + y1 * z42 + y2 * (z1 - z6 - z74) + y7 * (z6 - z8 - z42) +
                  y8 * z74 + y6 * z27) /
    (OutType)12.0;

  OutType z13 = z1 - z3;
  OutType z81 = z8 - z1;
  OutType z38 = z3 - z8;
  gradop[3][0] = (y1 * (z5 - z2 - z38) + y2 * z13 + y3 * (z2 - z7 - z81) + y8 * (z7 - z5 - z13) +
                  y5 * z81 + y7 * z38) /
    (OutType)12.0;

  OutType z86 = z8 - z6;
  OutType z18 = z1 - z8;
  OutType z61 = z6 - z1;
  gradop[4][0] = (y8 * (z4 - z7 - z61) + y7 * z86 + y6 * (z7 - z2 - z18) + y1 * (z2 - z4 - z86) +
                  y4 * z18 + y2 * z61) /
    (OutType)12.0;

  OutType z57 = z5 - z7;
  OutType z25 = z2 - z5;
  OutType z72 = z7 - z2;
  gradop[5][0] = (y5 * (z1 - z8 - z72) + y8 * z57 + y7 * (z8 - z3 - z25) + y2 * (z3 - z1 - z57) +
                  y1 * z25 + y3 * z72) /
    (OutType)12.0;

  OutType z68 = z6 - z8;
  OutType z36 = z3 - z6;
  OutType z83 = z8 - z3;
  gradop[6][0] = (y6 * (z2 - z5 - z83) + y5 * z68 + y8 * (z5 - z4 - z36) + y3 * (z4 - z2 - z68) +
                  y2 * z36 + y4 * z83) /
    (OutType)12.0;

  OutType z75 = z7 - z5;
  OutType z47 = z4 - z7;
  OutType z54 = z5 - z4;
  gradop[7][0] = (y7 * (z3 - z6 - z54) + y6 * z75 + y5 * (z6 - z1 - z47) + y4 * (z1 - z3 - z75) +
                  y3 * z47 + y1 * z54) /
    (OutType)12.0;

  OutType x24 = x2 - x4;
  OutType x52 = x5 - x2;
  OutType x45 = x4 - x5;
  gradop[0][1] = (z2 * (x6 - x3 - x45) + z3 * x24 + z4 * (x3 - x8 - x52) + z5 * (x8 - x6 - x24) +
                  z6 * x52 + z8 * x45) /
    (OutType)12.0;

  OutType x31 = x3 - x1;
  OutType x63 = x6 - x3;
  OutType x16 = x1 - x6;
  gradop[1][1] = (z3 * (x7 - x4 - x16) + z4 * x31 + z1 * (x4 - x5 - x63) + z6 * (x5 - x7 - x31) +
                  z7 * x63 + z5 * x16) /
    (OutType)12.0;

  OutType x42 = x4 - x2;
  OutType x74 = x7 - x4;
  OutType x27 = x2 - x7;
  gradop[2][1] = (z4 * (x8 - x1 - x27) + z1 * x42 + z2 * (x1 - x6 - x74) + z7 * (x6 - x8 - x42) +
                  z8 * x74 + z6 * x27) /
    (OutType)12.0;

  OutType x13 = x1 - x3;
  OutType x81 = x8 - x1;
  OutType x38 = x3 - x8;
  gradop[3][1] = (z1 * (x5 - x2 - x38) + z2 * x13 + z3 * (x2 - x7 - x81) + z8 * (x7 - x5 - x13) +
                  z5 * x81 + z7 * x38) /
    (OutType)12.0;

  OutType x86 = x8 - x6;
  OutType x18 = x1 - x8;
  OutType x61 = x6 - x1;
  gradop[4][1] = (z8 * (x4 - x7 - x61) + z7 * x86 + z6 * (x7 - x2 - x18) + z1 * (x2 - x4 - x86) +
                  z4 * x18 + z2 * x61) /
    (OutType)12.0;

  OutType x57 = x5 - x7;
  OutType x25 = x2 - x5;
  OutType x72 = x7 - x2;
  gradop[5][1] = (z5 * (x1 - x8 - x72) + z8 * x57 + z7 * (x8 - x3 - x25) + z2 * (x3 - x1 - x57) +
                  z1 * x25 + z3 * x72) /
    (OutType)12.0;

  OutType x68 = x6 - x8;
  OutType x36 = x3 - x6;
  OutType x83 = x8 - x3;
  gradop[6][1] = (z6 * (x2 - x5 - x83) + z5 * x68 + z8 * (x5 - x4 - x36) + z3 * (x4 - x2 - x68) +
                  z2 * x36 + z4 * x83) /
    (OutType)12.0;

  OutType x75 = x7 - x5;
  OutType x47 = x4 - x7;
  OutType x54 = x5 - x4;
  gradop[7][1] = (z7 * (x3 - x6 - x54) + z6 * x75 + z5 * (x6 - x1 - x47) + z4 * (x1 - x3 - x75) +
                  z3 * x47 + z1 * x54) /
    (OutType)12.0;

  OutType y24 = y2 - y4;
  OutType y52 = y5 - y2;
  OutType y45 = y4 - y5;
  gradop[0][2] = (x2 * (y6 - y3 - y45) + x3 * y24 + x4 * (y3 - y8 - y52) + x5 * (y8 - y6 - y24) +
                  x6 * y52 + x8 * y45) /
    (OutType)12.0;

  OutType y31 = y3 - y1;
  OutType y63 = y6 - y3;
  OutType y16 = y1 - y6;
  gradop[1][2] = (x3 * (y7 - y4 - y16) + x4 * y31 + x1 * (y4 - y5 - y63) + x6 * (y5 - y7 - y31) +
                  x7 * y63 + x5 * y16) /
    (OutType)12.0;

  OutType y42 = y4 - y2;
  OutType y74 = y7 - y4;
  OutType y27 = y2 - y7;
  gradop[2][2] = (x4 * (y8 - y1 - y27) + x1 * y42 + x2 * (y1 - y6 - y74) + x7 * (y6 - y8 - y42) +
                  x8 * y74 + x6 * y27) /
    (OutType)12.0;

  OutType y13 = y1 - y3;
  OutType y81 = y8 - y1;
  OutType y38 = y3 - y8;
  gradop[3][2] = (x1 * (y5 - y2 - y38) + x2 * y13 + x3 * (y2 - y7 - y81) + x8 * (y7 - y5 - y13) +
                  x5 * y81 + x7 * y38) /
    (OutType)12.0;

  OutType y86 = y8 - y6;
  OutType y18 = y1 - y8;
  OutType y61 = y6 - y1;
  gradop[4][2] = (x8 * (y4 - y7 - y61) + x7 * y86 + x6 * (y7 - y2 - y18) + x1 * (y2 - y4 - y86) +
                  x4 * y18 + x2 * y61) /
    (OutType)12.0;

  OutType y57 = y5 - y7;
  OutType y25 = y2 - y5;
  OutType y72 = y7 - y2;
  gradop[5][2] = (x5 * (y1 - y8 - y72) + x8 * y57 + x7 * (y8 - y3 - y25) + x2 * (y3 - y1 - y57) +
                  x1 * y25 + x3 * y72) /
    (OutType)12.0;

  OutType y68 = y6 - y8;
  OutType y36 = y3 - y6;
  OutType y83 = y8 - y3;
  gradop[6][2] = (x6 * (y2 - y5 - y83) + x5 * y68 + x8 * (y5 - y4 - y36) + x3 * (y4 - y2 - y68) +
                  x2 * y36 + x4 * y83) /
    (OutType)12.0;

  OutType y75 = y7 - y5;
  OutType y47 = y4 - y7;
  OutType y54 = y5 - y4;
  gradop[7][2] = (x7 * (y3 - y6 - y54) + x6 * y75 + x5 * (y6 - y1 - y47) + x4 * (y1 - y3 - y75) +
                  x3 * y47 + x1 * y54) /
    (OutType)12.0;
  OutType volume = OutType(pts[0][0]) * gradop[0][0] + OutType(pts[1][0]) * gradop[1][0] +
    OutType(pts[2][0]) * gradop[2][0] + OutType(pts[3][0]) * gradop[3][0] +
    OutType(pts[4][0]) * gradop[4][0] + OutType(pts[5][0]) * gradop[5][0] +
    OutType(pts[6][0]) * gradop[6][0] + OutType(pts[7][0]) * gradop[7][0];
  OutType two = (OutType)2.;
  OutType aspect = (OutType).5 * viskores::Pow(volume, two) /
    (viskores::Pow(gradop[0][0], two) + viskores::Pow(gradop[1][0], two) +
     viskores::Pow(gradop[2][0], two) + viskores::Pow(gradop[3][0], two) +
     viskores::Pow(gradop[4][0], two) + viskores::Pow(gradop[5][0], two) +
     viskores::Pow(gradop[6][0], two) + viskores::Pow(gradop[7][0], two) +
     viskores::Pow(gradop[0][1], two) + viskores::Pow(gradop[1][1], two) +
     viskores::Pow(gradop[2][1], two) + viskores::Pow(gradop[3][1], two) +
     viskores::Pow(gradop[4][1], two) + viskores::Pow(gradop[5][1], two) +
     viskores::Pow(gradop[6][1], two) + viskores::Pow(gradop[7][1], two) +
     viskores::Pow(gradop[0][2], two) + viskores::Pow(gradop[1][2], two) +
     viskores::Pow(gradop[2][2], two) + viskores::Pow(gradop[3][2], two) +
     viskores::Pow(gradop[4][2], two) + viskores::Pow(gradop[5][2], two) +
     viskores::Pow(gradop[6][2], two) + viskores::Pow(gradop[7][2], two));

  return viskores::Sqrt(aspect);
}

} // cell metrics
} // worklet
} // viskores

#endif // viskores_worklet_cellmetrics_CellDimensionMetric_h
