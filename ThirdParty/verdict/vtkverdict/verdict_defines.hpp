/*=========================================================================

  Module:    verdict_defines.hpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 * verdict_defines.cpp contains common definitions
 *
 * This file is part of VERDICT
 *
 */

#ifndef VERDICT_DEFINES
#define VERDICT_DEFINES

#include "VerdictVector.hpp"

#include <cmath>

namespace VERDICT_NAMESPACE
{

enum VerdictBoolean
{
  VERDICT_FALSE = 0,
  VERDICT_TRUE = 1
};

inline double determinant(double a, double b, double c, double d)
{
  return ((a) * (d) - (b) * (c));
}

inline double determinant(VerdictVector v1, VerdictVector v2, VerdictVector v3)
{
  return VerdictVector::Dot(v1, (v2 * v3));
}

static const double sqrt_2 = std::sqrt(2.0);

inline double normalize_jacobian(
  double jacobi, VerdictVector& v1, VerdictVector& v2, VerdictVector& v3, int tet_flag = 0)
{
  double return_value = 0.0;

  if (jacobi != 0.0)
  {
    double l1, l2, l3, length_product;
    // Note: there may be numerical problems if one is a lot shorter
    // than the others this way. But scaling each vector before the
    // triple product would involve 3 square roots instead of just
    // one.
    l1 = v1.length_squared();
    l2 = v2.length_squared();
    l3 = v3.length_squared();
    length_product = std::sqrt(l1 * l2 * l3);

    // if some numerical scaling problem, or just plain roundoff,
    // then push back into range [-1,1].
    if (length_product < std::abs(jacobi))
    {
      length_product = std::abs(jacobi);
    }

    if (tet_flag == 1)
    {
      return_value = sqrt_2 * jacobi / length_product;
    }
    else
    {
      return_value = jacobi / length_product;
    }
  }
  return return_value;
}

inline double norm_squared(double m11, double m21, double m12, double m22)
{
  return m11 * m11 + m21 * m21 + m12 * m12 + m22 * m22;
}

inline int skew_matrix(double gm11, double gm12, double gm22, double det, double& qm11,
  double& qm21, double& qm12, double& qm22)
{
  double tmp = std::sqrt(gm11 * gm22);
  if (tmp == 0)
  {
    return false;
  }

  qm11 = 1;
  qm21 = 0;
  qm12 = gm12 / tmp;
  qm22 = det / tmp;
  return true;
}

inline void inverse(VerdictVector x1, VerdictVector x2, VerdictVector x3, VerdictVector& u1,
  VerdictVector& u2, VerdictVector& u3)
{
  double detx = determinant(x1, x2, x3);
  VerdictVector rx1, rx2, rx3;

  rx1.set(x1.x(), x2.x(), x3.x());
  rx2.set(x1.y(), x2.y(), x3.y());
  rx3.set(x1.z(), x2.z(), x3.z());

  u1 = rx2 * rx3;
  u2 = rx3 * rx1;
  u3 = rx1 * rx2;

  u1 /= detx;
  u2 /= detx;
  u3 /= detx;
}

inline void form_Q(const VerdictVector& v1, const VerdictVector& v2, const VerdictVector& v3,
  VerdictVector& q1, VerdictVector& q2, VerdictVector& q3)
{
  double g11, g12, g13, g22, g23, g33;

  g11 = VerdictVector::Dot(v1, v1);
  g12 = VerdictVector::Dot(v1, v2);
  g13 = VerdictVector::Dot(v1, v3);
  g22 = VerdictVector::Dot(v2, v2);
  g23 = VerdictVector::Dot(v2, v3);
  g33 = VerdictVector::Dot(v3, v3);

  double rtg11 = std::sqrt(g11);
  double rtg22 = std::sqrt(g22);
  double rtg33 = std::sqrt(g33);
  VerdictVector temp1;

  temp1 = v1 * v2;

  double cross = std::sqrt(VerdictVector::Dot(temp1, temp1));

  double q11, q21, q31;
  double q12, q22, q32;
  double q13, q23, q33;

  q11 = 1;
  q21 = 0;
  q31 = 0;

  q12 = g12 / rtg11 / rtg22;
  q22 = cross / rtg11 / rtg22;
  q32 = 0;

  q13 = g13 / rtg11 / rtg33;
  q23 = (g11 * g23 - g12 * g13) / rtg11 / rtg33 / cross;
  temp1 = v2 * v3;
  q33 = VerdictVector::Dot(v1, temp1) / rtg33 / cross;

  q1.set(q11, q21, q31);
  q2.set(q12, q22, q32);
  q3.set(q13, q23, q33);
}

inline void product(VerdictVector& a1, VerdictVector& a2, VerdictVector& a3, VerdictVector& b1,
  VerdictVector& b2, VerdictVector& b3, VerdictVector& c1, VerdictVector& c2, VerdictVector& c3)
{
  VerdictVector x1, x2, x3;

  x1.set(a1.x(), a2.x(), a3.x());
  x2.set(a1.y(), a2.y(), a3.y());
  x3.set(a1.z(), a2.z(), a3.z());

  c1.set(VerdictVector::Dot(x1, b1), VerdictVector::Dot(x2, b1), VerdictVector::Dot(x3, b1));
  c2.set(VerdictVector::Dot(x1, b2), VerdictVector::Dot(x2, b2), VerdictVector::Dot(x3, b2));
  c3.set(VerdictVector::Dot(x1, b3), VerdictVector::Dot(x2, b3), VerdictVector::Dot(x3, b3));
}

inline double norm_squared(VerdictVector& x1, VerdictVector& x2, VerdictVector& x3)

{
  return VerdictVector::Dot(x1, x1) + VerdictVector::Dot(x2, x2) + VerdictVector::Dot(x3, x3);
}

inline double skew_x(VerdictVector& q1, VerdictVector& q2, VerdictVector& q3, VerdictVector& qw1,
  VerdictVector& qw2, VerdictVector& qw3)
{
  double normsq1, normsq2, kappa;
  VerdictVector u1, u2, u3;
  VerdictVector x1, x2, x3;

  inverse(qw1, qw2, qw3, u1, u2, u3);
  product(q1, q2, q3, u1, u2, u3, x1, x2, x3);
  inverse(x1, x2, x3, u1, u2, u3);
  normsq1 = norm_squared(x1, x2, x3);
  normsq2 = norm_squared(u1, u2, u3);
  kappa = std::sqrt(normsq1 * normsq2);

  double skew = 0;
  if (kappa > VERDICT_DBL_MIN)
  {
    skew = 3 / kappa;
  }

  return skew;
}
} // namespace verdict

#endif
