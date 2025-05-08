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
#ifndef viskores_worklet_cellmetrics_CellStretchMetric_h
#define viskores_worklet_cellmetrics_CellStretchMetric_h

/*
* Mesh quality metric functions that compute the aspect frobenius of certain mesh cells.
* The aspect frobenius metric generally measures the degree of regularity of a cell, with
* a value of 1 representing a regular cell..
** These metric computations are adapted from the VTK implementation of the Verdict library,
* which provides a set of mesh/cell metrics for evaluating the geometric qualities of regions
* of mesh spaces.
** See: The Verdict Library Reference Manual (for per-cell-type metric formulae)
* See: vtk/ThirdParty/verdict/vtkverdict (for VTK code implementation of this metric)
*/

#include "TypeOfCellHexahedral.h"
#include "TypeOfCellQuadrilateral.h"
#include "TypeOfCellTetrahedral.h"
#include "TypeOfCellTriangle.h"
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

template <typename OutType, typename PointCoordVecType, typename CellShapeType>
VISKORES_EXEC OutType CellStretchMetric(const viskores::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        CellShapeType shape,
                                        viskores::ErrorCode& ec)
{
  UNUSED(numPts);
  UNUSED(pts);
  UNUSED(shape);
  UNUSED(ec);
  return OutType(-1.0);
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellStretchMetric(const viskores::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        viskores::CellShapeTagQuad,
                                        viskores::ErrorCode& ec)
{
  UNUSED(numPts);
  UNUSED(ec);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar root2 = viskores::Sqrt(Scalar(2.0));
  const Scalar LMin = GetQuadLMin<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar DMax = GetQuadDMax<Scalar, Vector, CollectionOfPoints>(pts);

  if (DMax <= Scalar(0.0))
  {
    return viskores::Infinity<Scalar>();
  }

  const Scalar q = root2 * (LMin / DMax);

  return q;
}

template <typename OutType, typename PointCoordVecType>
VISKORES_EXEC OutType CellStretchMetric(const viskores::IdComponent& numPts,
                                        const PointCoordVecType& pts,
                                        viskores::CellShapeTagHexahedron,
                                        viskores::ErrorCode& ec)
{
  UNUSED(numPts);
  UNUSED(ec);
  using Scalar = OutType;
  using CollectionOfPoints = PointCoordVecType;
  using Vector = typename PointCoordVecType::ComponentType;

  const Scalar root3 = viskores::Sqrt(Scalar(3.0));
  const Scalar LMin = GetHexLMin<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar DMax = GetHexDMax<Scalar, Vector, CollectionOfPoints>(pts);

  if (DMax <= Scalar(0.0))
  {
    return viskores::Infinity<Scalar>();
  }

  const Scalar q = root3 * (LMin / DMax);

  return q;
}
}
}
}
#endif // viskores_worklet_cellmetrics_CellStretchMetric_h
