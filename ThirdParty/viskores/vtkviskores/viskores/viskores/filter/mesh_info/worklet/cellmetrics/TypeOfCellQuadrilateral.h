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
#ifndef viskores_worklet_cellmetrics_TypeOfCellQuadrilateral
#define viskores_worklet_cellmetrics_TypeOfCellQuadrilateral
/**
 * The Verdict manual defines a set of commonly
 * used components of a quadrilateral (quad). For example,
 * area, side lengths, and so forth.
 *
 * These definitions can be found starting on
 * page 32 of the Verdict manual.
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
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadL0(const CollectionOfPoints& pts)
{
  const Vector L0(pts[1] - pts[0]);
  return L0;
}

/**
 * Returns the L1 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadL1(const CollectionOfPoints& pts)
{
  const Vector L1(pts[2] - pts[1]);
  return L1;
}

/**
 * Returns the L2 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadL2(const CollectionOfPoints& pts)
{
  const Vector L2(pts[3] - pts[2]);
  return L2;
}

/**
 * Returns the L3 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadL3(const CollectionOfPoints& pts)
{
  const Vector L3(pts[0] - pts[3]);
  return L3;
}

/**
 * Returns the L0 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadL0Magnitude(const CollectionOfPoints& pts)
{
  const Scalar l0 = static_cast<Scalar>(
    viskores::Sqrt(viskores::MagnitudeSquared(GetQuadL0<Scalar, Vector, CollectionOfPoints>(pts))));
  return l0;
}

/**
 * Returns the L1 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadL1Magnitude(const CollectionOfPoints& pts)
{
  const Scalar l1 = static_cast<Scalar>(
    viskores::Sqrt(viskores::MagnitudeSquared(GetQuadL1<Scalar, Vector, CollectionOfPoints>(pts))));
  return l1;
}

/**
 * Returns the L2 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadL2Magnitude(const CollectionOfPoints& pts)
{
  const Scalar l2 = static_cast<Scalar>(
    viskores::Sqrt(viskores::MagnitudeSquared(GetQuadL2<Scalar, Vector, CollectionOfPoints>(pts))));
  return l2;
}

/**
 * Returns the L3 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadL3Magnitude(const CollectionOfPoints& pts)
{
  const Scalar l3 = static_cast<Scalar>(
    viskores::Sqrt(viskores::MagnitudeSquared(GetQuadL3<Scalar, Vector, CollectionOfPoints>(pts))));
  return l3;
}

/**
 * Returns the Max of the magnitude of each vector which makes up the sides of the Quad.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadLMax(const CollectionOfPoints& pts)
{
  const Scalar l0 = GetQuadL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetQuadL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetQuadL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l3 = GetQuadL3Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar lmax = viskores::Max(l0, viskores::Max(l1, viskores::Max(l2, l3)));
  return lmax;
}

/**
 * Returns the Min of the magnitude of each vector which makes up the sides of the Quad.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadLMin(const CollectionOfPoints& pts)
{
  const Scalar l0 = GetQuadL0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l1 = GetQuadL1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l2 = GetQuadL2Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar l3 = GetQuadL3Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar lmin = viskores::Min(l0, viskores::Min(l1, viskores::Min(l2, l3)));
  return lmin;
}

/**
 * Returns the D0 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadD0(const CollectionOfPoints& pts)
{
  const Vector D0(pts[2] - pts[0]);
  return D0;
}

/**
 * Returns the D1 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadD1(const CollectionOfPoints& pts)
{
  const Vector D1(pts[3] - pts[1]);
  return D1;
}

/**
 * Returns the D0 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadD0Magnitude(const CollectionOfPoints& pts)
{
  const Scalar d0 = static_cast<Scalar>(
    viskores::Sqrt(viskores::MagnitudeSquared(GetQuadD0<Scalar, Vector, CollectionOfPoints>(pts))));
  return d0;
}

/**
 * Returns the D0 vector's magnitude, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadD1Magnitude(const CollectionOfPoints& pts)
{
  const Scalar d1 = static_cast<Scalar>(
    viskores::Sqrt(viskores::MagnitudeSquared(GetQuadD1<Scalar, Vector, CollectionOfPoints>(pts))));
  return d1;
}

/**
 * Returns the Max of the magnitude of each vector which makes up the diagonals of the Quad.
 *
 *  \param [in] pts The four points which define the Quad.
 *  \return Returns the magnitude of the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadDMax(const CollectionOfPoints& pts)
{
  const Scalar d0 = GetQuadD0Magnitude<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar d1 = GetQuadD1Magnitude<Scalar, Vector, CollectionOfPoints>(pts);

  const Scalar dmax = viskores::Max(d0, d1);
  return dmax;
}

/**
 * Returns the X0 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadX0(const CollectionOfPoints& pts)
{
  const Vector X0((pts[1] - pts[0]) + (pts[2] - pts[3]));
  return X0;
}

/**
 * Returns the X1 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadX1(const CollectionOfPoints& pts)
{
  const Vector X1((pts[2] - pts[1]) + (pts[3] - pts[0]));
  return X1;
}

/**
 * Returns the N0 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadN0(const CollectionOfPoints& pts)
{
  const Vector A = GetQuadL3<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector B = GetQuadL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N0 = viskores::Cross(A, B);
  return N0;
}

/**
 * Returns the N1 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadN1(const CollectionOfPoints& pts)
{
  const Vector A = GetQuadL0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector B = GetQuadL1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N1 = viskores::Cross(A, B);
  return N1;
}

/**
 * Returns the N2 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadN2(const CollectionOfPoints& pts)
{
  const Vector A = GetQuadL1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector B = GetQuadL2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N2 = viskores::Cross(A, B);
  return N2;
}

/**
 * Returns the N3 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadN3(const CollectionOfPoints& pts)
{
  const Vector A = GetQuadL2<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector B = GetQuadL3<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N3 = viskores::Cross(A, B);
  return N3;
}

/**
 * Returns the normal center vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadNc(const CollectionOfPoints& pts)
{
  const Vector A = GetQuadX0<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector B = GetQuadX1<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector Nc = viskores::Cross(A, B);
  return Nc;
}

/**
 * Returns the normalized N0 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadN0Normalized(const CollectionOfPoints& pts)
{
  return viskores::Normal(GetQuadN0<Scalar, Vector, CollectionOfPoints>(pts));
}

/**
 * Returns the normalized N1 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadN1Normalized(const CollectionOfPoints& pts)
{
  return viskores::Normal(GetQuadN1<Scalar, Vector, CollectionOfPoints>(pts));
}

/**
 * Returns the normalized N2 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadN2Normalized(const CollectionOfPoints& pts)
{
  return viskores::Normal(GetQuadN2<Scalar, Vector, CollectionOfPoints>(pts));
}

/**
 * Returns the normalized N3 vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadN3Normalized(const CollectionOfPoints& pts)
{
  return viskores::Normal(GetQuadN3<Scalar, Vector, CollectionOfPoints>(pts));
}

/**
 * Returns the normalized Nc vector, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the vector.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Vector GetQuadNcNormalized(const CollectionOfPoints& pts)
{
  return viskores::Normal(GetQuadNc<Scalar, Vector, CollectionOfPoints>(pts));
}

/**
 * Returns the alpha0 scalar, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the scalar.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadAlpha0(const CollectionOfPoints& pts)
{
  const Vector normalizedCenterNormal =
    GetQuadNcNormalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N0 = GetQuadN0<Scalar, Vector, CollectionOfPoints>(pts);
  return static_cast<Scalar>(viskores::Dot(normalizedCenterNormal, N0));
}

/**
 * Returns the alpha1 scalar, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the scalar.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadAlpha1(const CollectionOfPoints& pts)
{
  const Vector normalizedCenterNormal =
    GetQuadNcNormalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N1 = GetQuadN1<Scalar, Vector, CollectionOfPoints>(pts);
  return static_cast<Scalar>(viskores::Dot(normalizedCenterNormal, N1));
}

/**
 * Returns the alpha2 scalar, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the scalar.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadAlpha2(const CollectionOfPoints& pts)
{
  const Vector normalizedCenterNormal =
    GetQuadNcNormalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N2 = GetQuadN2<Scalar, Vector, CollectionOfPoints>(pts);
  return static_cast<Scalar>(viskores::Dot(normalizedCenterNormal, N2));
}


/**
 * Returns the alpha3 scalar, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the scalar.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadAlpha3(const CollectionOfPoints& pts)
{
  const Vector normalizedCenterNormal =
    GetQuadNcNormalized<Scalar, Vector, CollectionOfPoints>(pts);
  const Vector N3 = GetQuadN3<Scalar, Vector, CollectionOfPoints>(pts);
  return static_cast<Scalar>(viskores::Dot(normalizedCenterNormal, N3));
}


/**
 * Returns the area of the quad, as defined by the verdict manual.
 *
 *  \param [in] pts The four points which define the quadrilateral.
 *  \return Returns the area.
 */
template <typename Scalar, typename Vector, typename CollectionOfPoints>
VISKORES_EXEC Scalar GetQuadArea(const CollectionOfPoints& pts)
{
  const Scalar quarter(0.25);
  const Scalar a0 = GetQuadAlpha0<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar a1 = GetQuadAlpha1<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar a2 = GetQuadAlpha2<Scalar, Vector, CollectionOfPoints>(pts);
  const Scalar a3 = GetQuadAlpha3<Scalar, Vector, CollectionOfPoints>(pts);
  return quarter * (a0 + a1 + a2 + a3);
}

#endif
