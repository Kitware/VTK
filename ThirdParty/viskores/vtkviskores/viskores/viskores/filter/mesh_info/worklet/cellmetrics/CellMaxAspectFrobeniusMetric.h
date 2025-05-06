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
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef viskores_worklet_cellmetrics_CellMaxAspectFrobeniusMetric_h
#define viskores_worklet_cellmetrics_CellMaxAspectFrobeniusMetric_h

/*
* Mesh quality metric functions that compute the maximum aspect frobenius of certain mesh cells,
* each of which are composed of two or more triangles or tetrahedrons. The output aspect metric
* value is the maximum among all triangles or tetrahedrons.
* The aspect frobenius metric generally measures the degree of regularity of a cell, with
* a value of 1 representing a regular cell.
** These metric computations are adapted from the VTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** The maximum aspect frobenious computations for pyramid and wedge cell types are not defined in the
* VTK implementation, but are provided here.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
*/

#include "CellAspectFrobeniusMetric.h"
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

//This approximates the aspect frobenius of a tetrahedron, except for slight
//mathematical differences. In the standard aspect frobenius metric, a tetrahedron
//is compared to a reference right equilateral tetrahedron. However, in the max
//aspect frobenius metric of hexahedrons, the component tetrahedrons are compared
//to reference right isoceles tetrahedrons. Thus, some of the calculations differ
//to account for the change in reference tetrahedron. This condition computation
//is not to be confused with the separate CellConditionMetric metric, but is similar
//in computation.
template <typename OutType, typename VecType>
VISKORES_EXEC OutType ComputeTetCondition(const VecType edges[])
{
  //Compute the determinant/volume of the reference tet.
  //(right isosceles tet for max aspect frobenius of hexs, pyramids, and wedges)
  OutType det = (OutType)viskores::Dot(edges[0], viskores::Cross(edges[1], edges[2]));

  if (det <= viskores::NegativeInfinity<OutType>())
    return viskores::Infinity<OutType>();

  OutType term1 =
    static_cast<OutType>(viskores::Dot(edges[0], edges[0]) + viskores::Dot(edges[1], edges[1]) +
                         viskores::Dot(edges[2], edges[2]));

  VecType crosses[3] = { viskores::Cross(edges[0], edges[1]),
                         viskores::Cross(edges[1], edges[2]),
                         viskores::Cross(edges[2], edges[0]) };

  OutType term2 = static_cast<OutType>(viskores::Dot(crosses[0], crosses[0]) +
                                       viskores::Dot(crosses[1], crosses[1]) +
                                       viskores::Dot(crosses[2], crosses[2]));

  return viskores::Sqrt(term1 * term2) / det;
}

// ========================= Unsupported cells ==================================

// By default, cells have undefined aspect frobenius unless the shape type template is specialized below.
template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent&,
                                                   const PointCoordVecType&,
                                                   CellShapeType,
                                                   viskores::ErrorCode& ec)
{
  ec = viskores::ErrorCode::InvalidCellMetric;
  return OutType(0.0);
}

//If the polygon has 3 vertices or 4 vertices, then just call
//the functions for Triangle and Quad cell types. Otherwise,
//this metric is not supported for (n>4)-vertex polygons, such
//as pentagons or hexagons, or (n<3)-vertex polygons, such as lines or points.
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent& numPts,
                                                   const PointCoordVecType& pts,
                                                   viskores::CellShapeTagPolygon,
                                                   viskores::ErrorCode& ec)
{
  if (numPts == 3)
    return viskores::worklet::cellmetrics::CellAspectFrobeniusMetric<OutType>(
      numPts, pts, viskores::CellShapeTagTriangle(), ec);
  if (numPts == 4)
    return CellMaxAspectFrobeniusMetric<OutType>(numPts, pts, viskores::CellShapeTagQuad(), ec);
  else
  {
    ec = viskores::ErrorCode::InvalidCellMetric;
    return OutType(0.0);
  }
}

//The max aspect frobenius metric is not supported for lines/edges.
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent&,
                                                   const PointCoordVecType&,
                                                   viskores::CellShapeTagLine,
                                                   viskores::ErrorCode& ec)
{
  ec = viskores::ErrorCode::InvalidCellMetric;
  return OutType(0.0);
}

//The max aspect frobenius metric is not uniquely defined for triangles,
//since the standard aspect frobenius metric is used for triangles.
//Thus, this implementation simply calls the aspect frobenius metric.
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent& numPts,
                                                   const PointCoordVecType& pts,
                                                   viskores::CellShapeTagTriangle,
                                                   viskores::ErrorCode& ec)
{
  if (numPts != 3)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }
  return viskores::worklet::cellmetrics::CellAspectFrobeniusMetric<OutType>(
    numPts, pts, viskores::CellShapeTagTriangle(), ec);
}

//The max aspect frobenius metric is not supported for pyramids.
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent&,
                                                   const PointCoordVecType&,
                                                   viskores::CellShapeTagPyramid,
                                                   viskores::ErrorCode& ec)
{
  ec = viskores::ErrorCode::InvalidCellMetric;
  return OutType(0.0);
}

// ========================= 2D cells ==================================

// Computes the max aspect frobenius of a quad.
// Formula: The maximum aspect frobenius metric among the four triangles formed
// at the four corner points of the quad. Given a corner point, two other points are
// chosen in a uniform, counter-clockwise manner to form a triangle. The aspect frobenius metric
// is computed on this triangle. The maximum among this four computed triangle metrics
// is returned as output.
// Equals 1 for a unit square.
// Acceptable range: [1,1.3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent& numPts,
                                                   const PointCoordVecType& pts,
                                                   viskores::CellShapeTagQuad,
                                                   viskores::ErrorCode& ec)
{
  if (numPts != 4)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }
  //The 4 edges of a quad
  using Edge = typename PointCoordVecType::ComponentType;
  const Edge QuadEdges[4] = { pts[1] - pts[0], pts[2] - pts[1], pts[3] - pts[2], pts[0] - pts[3] };

  FloatType a2 = viskores::MagnitudeSquared(QuadEdges[0]);
  FloatType b2 = viskores::MagnitudeSquared(QuadEdges[1]);
  FloatType c2 = viskores::MagnitudeSquared(QuadEdges[2]);
  FloatType d2 = viskores::MagnitudeSquared(QuadEdges[3]);

  //Compute the length of the cross product for each of the 4 reference triangles.
  //The result is twice the area of the triangle.
  FloatType ab = viskores::Magnitude(viskores::Cross(QuadEdges[0], QuadEdges[1]));
  FloatType bc = viskores::Magnitude(viskores::Cross(QuadEdges[1], QuadEdges[2]));
  FloatType cd = viskores::Magnitude(viskores::Cross(QuadEdges[2], QuadEdges[3]));
  FloatType da = viskores::Magnitude(viskores::Cross(QuadEdges[3], QuadEdges[0]));

  if (ab < viskores::NegativeInfinity<OutType>() || bc < viskores::NegativeInfinity<OutType>() ||
      cd < viskores::NegativeInfinity<OutType>() || da < viskores::NegativeInfinity<OutType>())
    return viskores::Infinity<OutType>();

  FloatType qmax = (a2 + b2) / ab; //Initial max aspect frobenius (triangle 0)

  FloatType qcur = (b2 + c2) / bc; //Keep checking/updating max (triangles 1 - 3)
  qmax = qmax > qcur ? qmax : qcur;

  qcur = (c2 + d2) / cd;
  qmax = qmax > qcur ? qmax : qcur;

  qcur = (d2 + a2) / da;
  qmax = qmax > qcur ? qmax : qcur;

  OutType max_aspect_frobenius = 0.5f * (OutType)qmax;

  if (max_aspect_frobenius > 0)
    viskores::Min(max_aspect_frobenius, viskores::Infinity<OutType>());

  return viskores::Max(max_aspect_frobenius, viskores::NegativeInfinity<OutType>());
}

// ============================= 3D Volume cells ==================================i

// Computes the aspect frobenius of a tetrahedron.
// Formula: Sum of lengths of 3 edges, divided by a multiple of the triangle area.
// Equals 1 for a right regular tetrahedron (4 equilateral triangles).
// Acceptable range: [1,1.3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent& numPts,
                                                   const PointCoordVecType& pts,
                                                   viskores::CellShapeTagTetra,
                                                   viskores::ErrorCode& ec)
{
  if (numPts != 4)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  return viskores::worklet::cellmetrics::CellAspectFrobeniusMetric<OutType, PointCoordVecType>(
    numPts, pts, viskores::CellShapeTagTetra(), ec);
}

// Computes the maximum aspect frobenius of a hexahedron.
// Formula: The maximum aspect frobenius metric among the eight tetrahedrons formed
// at the eight corner points of the hex. Given a corner point, three other points are
// chosen in a uniform, counter-clockwise manner to form a tetrahedron. The aspect frobenius metric
// is computed on this tet, with respect to a reference right isosceles tet. The maximum among
// these eight computed tet metrics is returned as output.
// Equals 1 for a unit cube (right isosceles tet formed at all 8 corner points).
// Acceptable range: [1,1.3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent& numPts,
                                                   const PointCoordVecType& pts,
                                                   viskores::CellShapeTagHexahedron,
                                                   viskores::ErrorCode& ec)
{
  if (numPts != 8)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  using Edge = typename PointCoordVecType::ComponentType;

  //8 tets: one constructed at each different corner of the hex
  //For each tet: Two base edges and one vertical edge, used to compute the tet volume
  const Edge TetEdges[8][3] = {
    {
      pts[1] - pts[0], //Base edge 1
      pts[3] - pts[0], //Base edge 2
      pts[4] - pts[0]  //Vertical edge 3
    },                 //tet 0

    { pts[2] - pts[1], pts[0] - pts[1], pts[5] - pts[1] }, //tet 1

    { pts[3] - pts[2], pts[1] - pts[2], pts[6] - pts[2] }, //tet 2

    { pts[0] - pts[3], pts[2] - pts[3], pts[7] - pts[3] }, //tet 3

    { pts[7] - pts[4], pts[5] - pts[4], pts[0] - pts[4] }, //tet 4

    { pts[4] - pts[5], pts[6] - pts[5], pts[1] - pts[5] }, //tet 5

    { pts[5] - pts[6], pts[7] - pts[6], pts[2] - pts[6] }, //tet 6

    { pts[6] - pts[7], pts[4] - pts[7], pts[3] - pts[7] } //tet 7
  };

  //For each tet, compute the condition metric, which approximates the deviation of the
  //tet's volume to that of a right isoceles tetrahedron. Return the maximum metric value
  //among all 8 tets as the maximum aspect frobenius.
  OutType max_aspect_frobenius = ComputeTetCondition<OutType, Edge>(TetEdges[0]);
  OutType curr;
  for (viskores::Id i = 1; i < 8; i++)
  {
    curr = ComputeTetCondition<OutType, Edge>(TetEdges[i]);
    if (curr <= 0.0)
      return viskores::Infinity<OutType>();
    if (curr > max_aspect_frobenius)
      max_aspect_frobenius = curr;
  }

  max_aspect_frobenius *= (OutType)0.3333333;

  if (max_aspect_frobenius > 0)
    return viskores::Min(max_aspect_frobenius, viskores::Infinity<OutType>());

  return viskores::Max(max_aspect_frobenius, OutType(0.0));
}

// Computes the maximum aspect frobenius of a wedge.
// Formula: The maximum aspect frobenius metric among the six tetrahedrons formed
// from the six corner points of the two triangular faces. Given a corner point, three other points are
// chosen in a uniform, counter-clockwise manner to form a tetrahedron. The aspect frobenius metric
// is computed on this tet, with respect to an equilateral tet. The maximum among
// these six computed tet metrics is returned as output.
// Equals 1 for a unit wedge (two equilateral triangles of unit edge length and 3 unit squares).
// Acceptable range: [1,1.3]
// Full range: [1,FLOAT_MAX]
template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellMaxAspectFrobeniusMetric(const viskores::IdComponent& numPts,
                                                   const PointCoordVecType& pts,
                                                   viskores::CellShapeTagWedge,
                                                   viskores::ErrorCode& ec)
{
  if (numPts != 6)
  {
    ec = viskores::ErrorCode::InvalidNumberOfPoints;
    return OutType(0.0);
  }

  using Vector = typename PointCoordVecType::ComponentType;

  //Four positively-oriented points of each tet
  const viskores::Vec<Vector, 4> tetras[6] = {
    viskores::Vec<Vector, 4>(pts[0], pts[1], pts[2], pts[3]), //tet 0
    viskores::Vec<Vector, 4>(pts[1], pts[2], pts[0], pts[4]), //tet 1
    viskores::Vec<Vector, 4>(pts[2], pts[0], pts[1], pts[5]), //tet 2
    viskores::Vec<Vector, 4>(pts[3], pts[5], pts[4], pts[0]), //tet 3
    viskores::Vec<Vector, 4>(pts[4], pts[3], pts[5], pts[1]), //tet 4
    viskores::Vec<Vector, 4>(pts[5], pts[4], pts[3], pts[2])  //tet 5
  };

  //For each tet, call the aspect frobenius metric.
  //Return the maximum metric value among all 6 tets as the maximum aspect frobenius.
  const viskores::IdComponent tetPts = 4;
  OutType max_aspect_frobenius = viskores::worklet::cellmetrics::CellAspectFrobeniusMetric<OutType>(
    tetPts, tetras[0], viskores::CellShapeTagTetra(), ec);
  OutType curr;
  for (viskores::Id i = 1; i < 6; i++)
  {
    curr = viskores::worklet::cellmetrics::CellAspectFrobeniusMetric<OutType>(
      tetPts, tetras[i], viskores::CellShapeTagTetra(), ec);
    if (curr > max_aspect_frobenius)
      max_aspect_frobenius = curr;
  }

  //Divide by metric value of unit wedge (normalization)
  max_aspect_frobenius /= 1.16477;

  if (max_aspect_frobenius > 0)
  {
    return viskores::Min(max_aspect_frobenius, viskores::Infinity<OutType>());
  }
  else
  {
    return viskores::Max(max_aspect_frobenius, viskores::NegativeInfinity<OutType>());
  }
}
} // namespace cellmetrics
} // namespace worklet
} // namespace viskores
#endif // viskores_worklet_cellmetrics_CellMaxAspectFrobeniusMetric_h
