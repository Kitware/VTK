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
#ifndef viskores_worklet_cellmetrics_TypeOfCellTriangle
#define viskores_worklet_cellmetrics_TypeOfCellTriangle
/**
 * The Verdict manual defines a set of commonly
 * used components of a triangle. For example,
 * area, side lengths, and so forth.
 *
 * These definitions can be found starting on
 * page 17 of the Verdict manual.
 *
 * This file contains a set of functions which
 * implement return the values of those commonly
 * used components for subsequent use in metrics.
 */

#include <viskores/Math.h>
#include <viskores/VectorAnalysis.h>


/**
 * Returns the L0 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The three points which define the triangle.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetTriangleL0(const CollectionOfPoints& pts)
{
  const Vector L0(pts[2] - pts[1]);
  return L0;
}

/**
 * Returns the L1 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The three points which define the triangle.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetTriangleL1(const CollectionOfPoints& pts)
{
  const Vector L1(pts[0] - pts[2]);
  return L1;
}

/**
 * Returns the L2 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The three points which define the triangle.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetTriangleL2(const CollectionOfPoints& pts)
{
  const Vector L2(pts[1] - pts[0]);
  return L2;
}

/**
 * Returns the L0 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The three points which define the triangle.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetTriangleL0Magnitude(const CollectionOfPoints& pts)
{
  const Scalar l0 = static_cast<Scalar>(viskores::Sqrt(
    viskores::MagnitudeSquared(GetTriangleL0<Scalar, Vector, CollectionOfPoints>(pts))));
  return l0;
}

/**
 * Returns the L1 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The three points which define the triangle.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetTriangleL1Magnitude(const CollectionOfPoints& pts)
{
  const Scalar l1 = static_cast<Scalar>(viskores::Sqrt(
    viskores::MagnitudeSquared(GetTriangleL1<Scalar, Vector, CollectionOfPoints>(pts))));
  return l1;
}

/**
 * Returns the L2 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The three points which define the triangle.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetTriangleL2Magnitude(const CollectionOfPoints& pts)
{
  const Scalar l2 = static_cast<Scalar>(viskores::Sqrt(
    viskores::MagnitudeSquared(GetTriangleL2<Scalar, Vector, CollectionOfPoints>(pts))));
  return l2;
}

/**
 *  Returns the Max of the magnitude of each vector which makes up the sides of the triangle.
 *
 *  That is to say, the length of the longest side.
 *
 *  \param [in] pts The three points which define the verticies of the triangle.
 *
 *  \return Returns the max of the triangle side lengths.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetTriangleLMax(const CollectionOfPoints& pts)
{
  const Scalar l0 = GetTriangleL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetTriangleL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetTriangleL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar lmax = viskores::Max(l0, viskores::Max(l1, l2));
  return lmax;
}

/**
 *  Returns the Min of the magnitude of each vector which makes up the sides of the triangle.
 *
 *  That is to say, the length of the shortest side.
 *
 *  \param [in] pts The three points which define the verticies of the triangle.
 *
 *  \return Returns the max of the triangle side lengths.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetTriangleLMin(const CollectionOfPoints& pts)
{
  const Scalar l0 = GetTriangleL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetTriangleL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetTriangleL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar lmin = viskores::Min(l0, viskores::Min(l1, l2));
  return lmin;
}

/**
 *  Returns the area of the triangle.
 *
 *  \param [in] pts The three points which define the verticies of the triangle.
 *
 *  \return Returns the are of the triangle..
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetTriangleArea(const CollectionOfPoints& pts)
{
  const Vector L0 = GetTriangleL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector L1 = GetTriangleL1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar hhalf(0.5);
  const Scalar crossProductMagnitude =
    static_cast<Scalar>(viskores::Sqrt(viskores::MagnitudeSquared(viskores::Cross(L0, L1))));
  const Scalar area = hhalf * crossProductMagnitude;
  return area;
}

/**
 *  Returns the radius of a circle inscribed within the given triangle. This is commonly denoted as 'r'.
 *
 *  \param [in] pts The three points which define the verticies of the triangle.
 *
 *  \return Returns the inradius.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetTriangleInradius(const CollectionOfPoints& pts)
{
  const Scalar two(2.0);
  const Scalar area = GetTriangleArea<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l0 = GetTriangleL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetTriangleL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetTriangleL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar inradius = (two * area) / (l0 + l1 + l2);
  return inradius;
}

/**
 *  Returns the radius of a circle circumscribed around the given triangle. This is commonly denoted as 'R'.
 *
 *  \param [in] pts The three points which define the verticies of the triangle.
 *
 *  \return Returns the circumradius.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetTriangleCircumradius(const CollectionOfPoints& pts)
{
  const Scalar four(4.0);
  const Scalar area = GetTriangleArea<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l0 = GetTriangleL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetTriangleL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetTriangleL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar circumradius = (l0 * l1 * l2) / (four * area);
  return circumradius;
}

#endif
