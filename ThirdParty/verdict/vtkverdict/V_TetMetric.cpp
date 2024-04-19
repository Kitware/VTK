/*=========================================================================

  Module:    V_TetMetric.cpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * TetMetric.cpp contains quality calculations for Tets
 *
 * This file is part of VERDICT
 *
 */

#include "V_GaussIntegration.hpp"
#include "VerdictVector.hpp"
#include "verdict.h"
#include "verdict_defines.hpp"

#include <algorithm>
#include <cmath> // for std::isnan

namespace VERDICT_NAMESPACE
{
static const double one_third = 1.0 / 3.0;
static const double two_thirds = 2.0 / 3.0;
static const double one_fourth = 1.0 / 4.0;
static const double four_ninths = 4.0 / 9.0;
static const double sqrt2 = std::sqrt(2.0);
static const double sqrt3 = std::sqrt(3.0);
static const double sqrt6 = std::sqrt(6.0);
static const double three_times_1plussqrt3 = 3.0 * (1 + sqrt3);
static const double normal_coeff = 180. * .3183098861837906715377675267450287;
static const double aspect_ratio_normal_coeff = sqrt6 / 12.;
double tet10_characteristic_length(const double coordinates[][3]);

static const int tet10_subtet_conn[12][4] = { { 0, 4, 6, 7 }, { 1, 5, 4, 8 }, { 2, 6, 5, 9 },
  { 3, 8, 7, 9 }, { 4, 8, 5, 10 }, { 5, 8, 9, 10 }, { 9, 8, 7, 10 }, { 7, 8, 4, 10 },
  { 4, 5, 6, 10 }, { 5, 9, 6, 10 }, { 9, 7, 6, 10 }, { 7, 4, 6, 10 } };

static double fix_range(double v)
{
  if (std::isnan(v))
  {
    return VERDICT_DBL_MAX;
  }
  if (v >= VERDICT_DBL_MAX)
  {
    return VERDICT_DBL_MAX;
  }
  if (v <= -VERDICT_DBL_MAX)
  {
    return -VERDICT_DBL_MAX;
  }
  return v;
}

double tet_equiangle_skew(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector ab, ac, bc, bd, ad, cd;

  ab.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  ab.normalize();

  ac.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);
  ac.normalize();

  ad.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);
  ad.normalize();

  bc.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  bc.normalize();

  bd.set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);
  bd.normalize();

  cd.set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);
  cd.normalize();

  VerdictVector abc = bc * ab;
  abc.normalize();
  VerdictVector abd = ab * ad;
  abd.normalize();
  VerdictVector acd = cd * ad;
  acd.normalize();
  VerdictVector bcd = bc * cd;
  bcd.normalize();

  double alpha = std::acos(-(abc % abd));
  double beta = std::acos(-(abc % acd));
  double gamma = std::acos(-(abc % bcd));
  double delta = std::acos(-(abd % acd));
  double epsilon = std::acos(-(abd % bcd));
  double zeta = std::acos(-(acd % bcd));

  double min_angle = alpha;
  min_angle = min_angle < beta ? min_angle : beta;
  min_angle = min_angle < gamma ? min_angle : gamma;
  min_angle = min_angle < delta ? min_angle : delta;
  min_angle = min_angle < epsilon ? min_angle : epsilon;
  min_angle = min_angle < zeta ? min_angle : zeta;
  min_angle *= normal_coeff;

  double max_angle = alpha;
  max_angle = max_angle > beta ? max_angle : beta;
  max_angle = max_angle > gamma ? max_angle : gamma;
  max_angle = max_angle > delta ? max_angle : delta;
  max_angle = max_angle > epsilon ? max_angle : epsilon;
  max_angle = max_angle > zeta ? max_angle : zeta;
  max_angle *= normal_coeff;

  double theta = std::acos(1 / 3.0) * normal_coeff; // 70.528779365509308630754000660038;

  double dihedral_skew_max = (max_angle - theta) / (180 - theta);
  double dihedral_skew_min = (theta - min_angle) / theta;

  min_angle = 360.0;
  max_angle = 0.0;

  double angles[12];
  angles[0] = std::acos(-(ab % bc));
  angles[1] = std::acos((bc % ac));
  angles[2] = std::acos((ac % ab));
  angles[3] = std::acos(-(ab % bd));
  angles[4] = std::acos((bd % ad));
  angles[5] = std::acos((ad % ab));
  angles[6] = std::acos(-(bc % cd));
  angles[7] = std::acos((cd % bd));
  angles[8] = std::acos((bd % bc));
  angles[9] = std::acos((ad % cd));
  angles[10] = std::acos(-(cd % ac));
  angles[11] = std::acos((ac % ad));

  for (int a = 0; a < 12; a++)
  {
    if (angles[a] < min_angle)
    {
      min_angle = angles[a];
    }
    if (angles[a] > max_angle)
    {
      max_angle = angles[a];
    }
  }
  max_angle *= normal_coeff;
  min_angle *= normal_coeff;
  double skew_max = (max_angle - 60.0) / 120.0;
  double skew_min = (60.0 - min_angle) / 60.0;

  double max_skew = dihedral_skew_min;
  max_skew = max_skew > dihedral_skew_max ? max_skew : dihedral_skew_max;
  max_skew = max_skew > skew_min ? max_skew : skew_min;
  max_skew = max_skew > skew_max ? max_skew : skew_max;

  return max_skew;
}

/*!
  get the weights based on the average size
  of a tet
 */
static int tet_get_weight(
  VerdictVector& w1, VerdictVector& w2, VerdictVector& w3, double average_tet_volume)
{
  w1.set(1, 0, 0);
  w2.set(0.5, 0.5 * sqrt3, 0);
  w3.set(0.5, sqrt3 / 6.0, sqrt2 / sqrt3);

  double scale = std::pow(6. * average_tet_volume / determinant(w1, w2, w3), 0.3333333333333);

  w1 *= scale;
  w2 *= scale;
  w3 *= scale;

  return 1;
}

/*!
   the edge ratio of a tet

   NB (P. Pebay 01/22/07):
     Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
     minimum edge lengths
 */
double tet_edge_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector a, b, c, d, e, f;

  a.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  b.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  c.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  d.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  e.set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);

  f.set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  double a2 = a.length_squared();
  double b2 = b.length_squared();
  double c2 = c.length_squared();
  double d2 = d.length_squared();
  double e2 = e.length_squared();
  double f2 = f.length_squared();

  double m2, M2, mab, mcd, mef, Mab, Mcd, Mef;

  if (a2 < b2)
  {
    mab = a2;
    Mab = b2;
  }
  else // b2 <= a2
  {
    mab = b2;
    Mab = a2;
  }
  if (c2 < d2)
  {
    mcd = c2;
    Mcd = d2;
  }
  else // d2 <= c2
  {
    mcd = d2;
    Mcd = c2;
  }
  if (e2 < f2)
  {
    mef = e2;
    Mef = f2;
  }
  else // f2 <= e2
  {
    mef = f2;
    Mef = e2;
  }

  m2 = mab < mcd ? mab : mcd;
  m2 = m2 < mef ? m2 : mef;

  if (m2 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  M2 = Mab > Mcd ? Mab : Mcd;
  M2 = M2 > Mef ? M2 : Mef;

  const double edge_ratio = std::sqrt(M2 / m2);

  return fix_range(edge_ratio);
}

/*!
  the scaled jacobian of a tet

  minimum of the jacobian divided by the lengths of 3 edge vectors

 */
double tet_scaled_jacobian(int /*num_nodes*/, const double coordinates[][3])
{
  const VerdictVector side0(coordinates[1][0] - coordinates[0][0],
    coordinates[1][1] - coordinates[0][1], coordinates[1][2] - coordinates[0][2]);
  const VerdictVector side1(coordinates[2][0] - coordinates[1][0],
    coordinates[2][1] - coordinates[1][1], coordinates[2][2] - coordinates[1][2]);
  const VerdictVector side2(coordinates[0][0] - coordinates[2][0],
    coordinates[0][1] - coordinates[2][1], coordinates[0][2] - coordinates[2][2]);
  const VerdictVector side3(coordinates[3][0] - coordinates[0][0],
    coordinates[3][1] - coordinates[0][1], coordinates[3][2] - coordinates[0][2]);
  const VerdictVector side4(coordinates[3][0] - coordinates[1][0],
    coordinates[3][1] - coordinates[1][1], coordinates[3][2] - coordinates[1][2]);
  const VerdictVector side5(coordinates[3][0] - coordinates[2][0],
    coordinates[3][1] - coordinates[2][1], coordinates[3][2] - coordinates[2][2]);

  const double jacobi = side3 % (side2 * side0);

  // products of lengths squared of each edge attached to a node.
  const double side0_length_squared = side0.length_squared();
  const double side1_length_squared = side1.length_squared();
  const double side2_length_squared = side2.length_squared();
  const double side3_length_squared = side3.length_squared();
  const double side4_length_squared = side4.length_squared();
  const double side5_length_squared = side5.length_squared();

  const double length_squared[4] = { side0_length_squared * side2_length_squared *
      side3_length_squared,
    side0_length_squared * side1_length_squared * side4_length_squared,
    side1_length_squared * side2_length_squared * side5_length_squared,
    side3_length_squared * side4_length_squared * side5_length_squared };
  int which_node = 0;
  if (length_squared[1] > length_squared[which_node])
  {
    which_node = 1;
  }
  if (length_squared[2] > length_squared[which_node])
  {
    which_node = 2;
  }
  if (length_squared[3] > length_squared[which_node])
  {
    which_node = 3;
  }

  double length_product = std::sqrt(length_squared[which_node]);
  if (length_product < std::abs(jacobi))
  {
    length_product = std::abs(jacobi);
  }

  if (length_product < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  return (double)(sqrt2 * jacobi / length_product);
}

/*!
  The radius ratio of a tet

  NB (P. Pebay 04/16/07):
    CR / (3.0 * IR) where CR is the circumsphere radius and IR is the inscribed
    sphere radius.
    Note that this function is similar to the aspect beta of a tet, except that
    it does not return VERDICT_DBL_MAX if the element has negative orientation.
 */
double tet_radius_ratio(int /*num_nodes*/, const double coordinates[][3])
{

  // Determine side vectors
  VerdictVector side[6];

  side[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  side[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  side[2].set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  side[3].set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  side[4].set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);

  side[5].set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  VerdictVector numerator = side[3].length_squared() * (side[2] * side[0]) +
    side[2].length_squared() * (side[3] * side[0]) + side[0].length_squared() * (side[3] * side[2]);

  double area_sum;
  area_sum = ((side[2] * side[0]).length() + (side[3] * side[0]).length() +
               (side[4] * side[1]).length() + (side[3] * side[2]).length()) *
    0.5;

  double volume = tet_volume(4, coordinates);

  if (std::abs(volume) < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }
  else
  {
    const double radius_ratio = numerator.length() * area_sum / (108 * volume * volume);
    return fix_range(radius_ratio);
  }
}

/*!
  The aspect ratio of a tet

  NB (P. Pebay 01/22/07):
    Hmax / (2 sqrt(6) r) where Hmax and r respectively denote the greatest edge
    length and the inradius of the tetrahedron
  NB (J. Pouderoux 01/27/15)
    This will return VERDICT_DBL_MAX when the volume of the tetrahedron is ill-
    conditioned. Previously, this would only happen when the volume was small
    and positive, but now ill-conditioned inverted tetrahedra are also included.
 */
double tet_aspect_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  // Determine side vectors
  VerdictVector ab, bc, ac, ad, bd, cd;

  ab.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  ac.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  ad.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  double detTet = ab % (ac * ad);

  if (std::abs(detTet) < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  bc.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  bd.set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);

  cd.set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  double ab2 = ab.length_squared();
  double bc2 = bc.length_squared();
  double ac2 = ac.length_squared();
  double ad2 = ad.length_squared();
  double bd2 = bd.length_squared();
  double cd2 = cd.length_squared();

  double A = ab2 > bc2 ? ab2 : bc2;
  double B = ac2 > ad2 ? ac2 : ad2;
  double C = bd2 > cd2 ? bd2 : cd2;
  double D = A > B ? A : B;
  double hm = D > C ? std::sqrt(D) : std::sqrt(C);

  bd = ab * bc;
  A = bd.length();
  bd = ab * ad;
  B = bd.length();
  bd = ac * ad;
  C = bd.length();
  bd = bc * cd;
  D = bd.length();

  const double aspect_ratio = aspect_ratio_normal_coeff * hm * (A + B + C + D) / std::abs(detTet);

  return fix_range(aspect_ratio);
}

/*!
  the aspect gamma of a tet

  srms^3 / (8.48528137423857*V) where srms = sqrt(sum(Si^2)/6), where Si is the edge length
 */
double tet_aspect_gamma(int /*num_nodes*/, const double coordinates[][3])
{
  // Determine side vectors
  VerdictVector side0, side1, side2, side3, side4, side5;

  side0.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  side1.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  side2.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  side3.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  side4.set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);

  side5.set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  double volume = std::abs(tet_volume(4, coordinates));

  if (volume < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }
  else
  {
    double srms =
      std::sqrt((side0.length_squared() + side1.length_squared() + side2.length_squared() +
                  side3.length_squared() + side4.length_squared() + side5.length_squared()) /
        6.0);

    double aspect_ratio_gamma = std::pow(srms, 3) / (8.48528137423857 * volume);
    return (double)aspect_ratio_gamma;
  }
}

/*!
  The aspect frobenius of a tet

  NB (P. Pebay 01/22/07):
    Frobenius condition number when the reference element is regular
 */
double tet_aspect_frobenius(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector ab, ac, ad;

  ab.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  ac.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  ad.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  double denominator = ab % (ac * ad);
  denominator *= denominator;
  denominator *= 2.;
  denominator = 3. * std::pow(denominator, one_third);

  if (denominator < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double u[3];
  ab.get_xyz(u);
  double v[3];
  ac.get_xyz(v);
  double w[3];
  ad.get_xyz(w);

  double numerator = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
  numerator += v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
  numerator += w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
  numerator *= 1.5;
  numerator -= v[0] * u[0] + v[1] * u[1] + v[2] * u[2];
  numerator -= w[0] * u[0] + w[1] * u[1] + w[2] * u[2];
  numerator -= w[0] * v[0] + w[1] * v[1] + w[2] * v[2];

  double aspect_frobenius = numerator / denominator;

  return fix_range(aspect_frobenius);
}

/*!
  The minimum angle of a tet

  NB (P. Pebay 01/22/07):
    minimum nonoriented dihedral angle
 */
double tet_minimum_angle(int /*num_nodes*/, const double coordinates[][3])
{
  // Determine side vectors
  VerdictVector ab, bc, ad, cd;

  ab.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  ad.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  bc.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  cd.set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  VerdictVector abc = ab * bc;
  double nabc = abc.length();
  VerdictVector abd = ab * ad;
  double nabd = abd.length();
  VerdictVector acd = ad * cd;
  double nacd = acd.length();
  VerdictVector bcd = bc * cd;
  double nbcd = bcd.length();

  double alpha = std::acos((abc % abd) / (nabc * nabd));
  double beta = std::acos((abc % acd) / (nabc * nacd));
  double gamma = std::acos((abc % bcd) / (nabc * nbcd));
  double delta = std::acos((abd % acd) / (nabd * nacd));
  double epsilon = std::acos((abd % bcd) / (nabd * nbcd));
  double zeta = std::acos((acd % bcd) / (nacd * nbcd));

  alpha = alpha < beta ? alpha : beta;
  alpha = alpha < gamma ? alpha : gamma;
  alpha = alpha < delta ? alpha : delta;
  alpha = alpha < epsilon ? alpha : epsilon;
  alpha = alpha < zeta ? alpha : zeta;
  alpha *= normal_coeff;

  return fix_range(alpha);
}

/*!
  The collapse ratio of a tet

  NB (J. Pouderoux 01/27/15)
    This will return VERDICT_DBL_MAX when the volume of the tetrahedron is ill-
    conditioned. Previously, this would only happen when the volume was small
    and positive, but now ill-conditioned inverted tetrahedra are also included.
 */
double tet_collapse_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  // Determine side vectors
  VerdictVector e01, e02, e03, e12, e13, e23;

  e01.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  e02.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  e03.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  e12.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  e13.set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);

  e23.set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  double l[6];
  l[0] = e01.length();
  l[1] = e02.length();
  l[2] = e03.length();
  l[3] = e12.length();
  l[4] = e13.length();
  l[5] = e23.length();

  // Find longest edge for each bounding triangle of tetrahedron
  double l012 = l[4] > l[0] ? l[4] : l[0];
  l012 = l[1] > l012 ? l[1] : l012;
  double l031 = l[0] > l[2] ? l[0] : l[2];
  l031 = l[3] > l031 ? l[3] : l031;
  double l023 = l[2] > l[1] ? l[2] : l[1];
  l023 = l[5] > l023 ? l[5] : l023;
  double l132 = l[4] > l[3] ? l[4] : l[3];
  l132 = l[5] > l132 ? l[5] : l132;

  // Compute collapse ratio for each vertex/triangle pair
  VerdictVector N;
  double h, magN;
  double cr;
  double crMin;

  N = e01 * e02;
  magN = N.length();
  h = (e03 % N) / magN; // height of vertex 3 above 0-1-2
  crMin = h / l012;     // ratio of height to longest edge of 0-1-2

  N = e03 * e01;
  magN = N.length();
  h = (e02 % N) / magN; // height of vertex 2 above 0-3-1
  cr = h / l031;        // ratio of height to longest edge of 0-3-1
  if (cr < crMin)
  {
    crMin = cr;
  }

  N = e02 * e03;
  magN = N.length();
  h = (e01 % N) / magN; // height of vertex 1 above 0-2-3
  cr = h / l023;        // ratio of height to longest edge of 0-2-3
  if (cr < crMin)
  {
    crMin = cr;
  }

  N = e12 * e13;
  magN = N.length();
  h = (e01 % N) / magN; // height of vertex 0 above 1-3-2
  cr = h / l132;        // ratio of height to longest edge of 1-3-2
  if (cr < crMin)
  {
    crMin = cr;
  }

  return fix_range(crMin);
}

double tet_equivolume_skew(int num_nodes, const double coordinates[][3])
{
  //- Find the vectors from the origin to each of the nodes on the tet.
  VerdictVector vectA(coordinates[0][0], coordinates[0][1], coordinates[0][2]);
  VerdictVector vectB(coordinates[1][0], coordinates[1][1], coordinates[1][2]);
  VerdictVector vectC(coordinates[2][0], coordinates[2][1], coordinates[2][2]);
  VerdictVector vectD(coordinates[3][0], coordinates[3][1], coordinates[3][2]);

  VerdictVector vectAB = vectB - vectA;
  VerdictVector vectAC = vectC - vectA;
  VerdictVector vectAD = vectD - vectA;

  double sq_lengthAB = vectAB.length_squared();
  double sq_lengthAC = vectAC.length_squared();
  double sq_lengthAD = vectAD.length_squared();

  VerdictVector cpBC = vectAB * vectAC;
  VerdictVector cpDB = vectAD * vectAB;
  VerdictVector cpCD = vectAC * vectAD;

  VerdictVector num = sq_lengthAD * cpBC + sq_lengthAC * cpDB + sq_lengthAB * cpCD;
  double den = 2 * vectAB % cpCD;

  double circumradius = num.length() / den;

  double volume = tet_volume(num_nodes, coordinates);
  double optimal_length = circumradius / std::sqrt(double(3.0) / 8.0);
  double optimal_volume = (1.0 / 12.0) * std::sqrt(double(2.0)) * std::pow(optimal_length, 3);

  const double eq_v_skew = (optimal_volume - volume) / optimal_volume;
  return fix_range(eq_v_skew);
}

double tet_squish_index(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector vectA(coordinates[0][0], coordinates[0][1], coordinates[0][2]);
  VerdictVector vectB(coordinates[1][0], coordinates[1][1], coordinates[1][2]);
  VerdictVector vectC(coordinates[2][0], coordinates[2][1], coordinates[2][2]);
  VerdictVector vectD(coordinates[3][0], coordinates[3][1], coordinates[3][2]);

  VerdictVector tetCenter = vectA + vectB + vectC + vectD;
  tetCenter /= 4.0;

  /*                  top view

                          C
                         /|\
                        / 5 \
                     2 /  D  \ 1
                      / 3/ \4 \
                     /_/     \_\
                    A-----------B
                          0
  */

  VerdictVector side[6];

  side[0].set(vectA, vectB);
  side[1].set(vectB, vectC);
  side[2].set(vectC, vectA);
  side[3].set(vectA, vectD);
  side[4].set(vectB, vectD);
  side[5].set(vectC, vectD);

  double maxSquishIndex = 0;
  double squishIndex = 0;
  VerdictVector faceCenter;
  VerdictVector centerCenterVector;
  VerdictVector faceAreaVector;

  // face 1
  faceCenter = (vectA + vectB + vectD) / 3.0;
  centerCenterVector = faceCenter - tetCenter;
  faceAreaVector = 0.5 * (side[0] * side[4]);

  squishIndex = 1 -
    (faceAreaVector % centerCenterVector) / (faceAreaVector.length() * centerCenterVector.length());
  if (squishIndex > maxSquishIndex)
  {
    maxSquishIndex = squishIndex;
  }

  // face 2
  faceCenter = (vectB + vectC + vectD) / 3.0;
  centerCenterVector = faceCenter - tetCenter;
  faceAreaVector = 0.5 * (side[1] * side[5]);

  squishIndex = 1 -
    (faceAreaVector % centerCenterVector) / (faceAreaVector.length() * centerCenterVector.length());
  if (squishIndex > maxSquishIndex)
  {
    maxSquishIndex = squishIndex;
  }

  // face 3
  faceCenter = (vectA + vectC + vectD) / 3.0;
  centerCenterVector = faceCenter - tetCenter;
  faceAreaVector = 0.5 * (side[2] * side[3]);

  squishIndex = 1 -
    (faceAreaVector % centerCenterVector) / (faceAreaVector.length() * centerCenterVector.length());
  if (squishIndex > maxSquishIndex)
  {
    maxSquishIndex = squishIndex;
  }

  // face 4
  faceCenter = (vectA + vectB + vectC) / 3.0;
  centerCenterVector = faceCenter - tetCenter;
  faceAreaVector = 0.5 * (side[1] * side[0]);

  squishIndex = 1 -
    (faceAreaVector % centerCenterVector) / (faceAreaVector.length() * centerCenterVector.length());
  if (squishIndex > maxSquishIndex)
  {
    maxSquishIndex = squishIndex;
  }
  return maxSquishIndex;
}

static const double TET15_node_local_coord[15][3] = { { 0, 0, 0 }, { 1.0, 0, 0 }, { 0, 1.0, 0 },
  { 0, 0, 1.0 }, { .5, 0, 0 }, { .5, .5, 0 }, { 0, .5, 0 }, { 0, 0, .5 }, { .5, 0, .5 },
  { 0, .5, .5 }, { one_third, one_third, 0 }, { one_third, 0, one_third },
  { one_third, one_third, one_third }, { 0, one_third, one_third },
  { one_fourth, one_fourth, one_fourth } };

static void TET15_gradients_of_the_shape_functions_for_R_S_T(
  const double rst[3], double dhdr[15], double dhds[15], double dhdt[15])
{
  // dh/dr;
  dhdr[0] = -1.0;
  dhdr[1] = 1.0;
  dhdr[2] = 0.0;
  dhdr[3] = 0.0;
  dhdr[4] = 4.0 * (1.0 - 2.0 * rst[0] - rst[1] - rst[2]);
  dhdr[5] = 4.0 * rst[1];
  dhdr[6] = -4.0 * rst[1];
  dhdr[7] = -4.0 * rst[2];
  dhdr[8] = 4.0 * rst[2];
  dhdr[9] = 0.0;
  dhdr[11] = 27.0 * (rst[1] - 2.0 * rst[0] * rst[1] - rst[1] * rst[1] - rst[1] * rst[2]);
  dhdr[14] = 27.0 * (rst[2] - 2.0 * rst[0] * rst[2] - rst[1] * rst[2] - rst[2] * rst[2]);
  dhdr[12] = 27.0 * rst[1] * rst[2];
  dhdr[13] = -27.0 * rst[1] * rst[2];
  dhdr[10] = 256.0 *
    (rst[1] * rst[2] - 2.0 * rst[0] * rst[1] * rst[2] - rst[1] * rst[1] * rst[2] -
      rst[1] * rst[2] * rst[2]);

  // dh/ds;
  dhds[0] = -1.0;
  dhds[1] = 0.0;
  dhds[2] = 1.0;
  dhds[3] = 0.0;
  dhds[4] = -4.0 * rst[0];
  dhds[5] = 4.0 * rst[0];
  dhds[6] = 4.0 * (1.0 - rst[0] - 2.0 * rst[1] - rst[2]);
  dhds[7] = -4.0 * rst[2];
  dhds[8] = 0.0;
  dhds[9] = 4.0 * rst[2];
  dhds[11] = 27.0 * (rst[0] - rst[0] * rst[0] - 2.0 * rst[0] * rst[1] - rst[0] * rst[2]);
  dhds[14] = -27.0 * rst[0] * rst[2];
  dhds[12] = 27.0 * rst[0] * rst[2];
  dhds[13] = 27.0 * (rst[2] - rst[0] * rst[2] - 2.0 * rst[1] * rst[2] - rst[2] * rst[2]);
  dhds[10] = 256.0 *
    (rst[0] * rst[2] - rst[0] * rst[0] * rst[2] - 2.0 * rst[0] * rst[1] * rst[2] -
      rst[0] * rst[2] * rst[2]);

  // dh/dt;
  dhdt[0] = -1.0;
  dhdt[1] = 0.0;
  dhdt[2] = 0.0;
  dhdt[3] = 1.0;
  dhdt[4] = -4.0 * rst[0];
  dhdt[5] = 0.0;
  dhdt[6] = -4.0 * rst[1];
  dhdt[7] = 4.0 * (1.0 - rst[0] - rst[1] - 2.0 * rst[2]);
  dhdt[8] = 4.0 * rst[0];
  dhdt[9] = 4.0 * rst[1];
  dhdt[11] = -27.0 * rst[0] * rst[1];
  dhdt[14] = 27.0 * (rst[0] - rst[0] * rst[0] - rst[0] * rst[1] - 2.0 * rst[0] * rst[2]);
  dhdt[12] = 27.0 * rst[0] * rst[1];
  dhdt[13] = 27.0 * (rst[1] - rst[0] * rst[1] - rst[1] * rst[1] - 2.0 * rst[1] * rst[2]);
  dhdt[10] = 256.0 *
    (rst[0] * rst[1] - rst[0] * rst[0] * rst[1] - rst[0] * rst[1] * rst[1] -
      2.0 * rst[0] * rst[1] * rst[2]);

  // ----------------------------------------------;
  // ADD CONTRIBUTIONS OF NODES 5-15 TO NODES 1-14;
  // ----------------------------------------------;

  // dh/dr;
  dhdr[11] = dhdr[11] - 108.0 * dhdr[10] / 256.0;
  dhdr[14] = dhdr[14] - 108.0 * dhdr[10] / 256.0;
  dhdr[12] = dhdr[12] - 108.0 * dhdr[10] / 256.0;
  dhdr[13] = dhdr[13] - 108.0 * dhdr[10] / 256.0;
  dhdr[4] = dhdr[4] - four_ninths * (dhdr[11] + dhdr[14]) - .25 * dhdr[10];
  dhdr[5] = dhdr[5] - four_ninths * (dhdr[11] + dhdr[12]) - .25 * dhdr[10];
  dhdr[6] = dhdr[6] - four_ninths * (dhdr[11] + dhdr[13]) - .25 * dhdr[10];
  dhdr[7] = dhdr[7] - four_ninths * (dhdr[14] + dhdr[13]) - .25 * dhdr[10];
  dhdr[8] = dhdr[8] - four_ninths * (dhdr[14] + dhdr[12]) - .25 * dhdr[10];
  dhdr[9] = dhdr[9] - four_ninths * (dhdr[12] + dhdr[13]) - .25 * dhdr[10];
  dhdr[0] = dhdr[0] - .5 * (dhdr[4] + dhdr[6] + dhdr[7]) -
    one_third * (dhdr[11] + dhdr[14] + dhdr[13]) - .25 * dhdr[10];
  dhdr[1] = dhdr[1] - .5 * (dhdr[4] + dhdr[5] + dhdr[8]) -
    one_third * (dhdr[11] + dhdr[14] + dhdr[12]) - .25 * dhdr[10];
  dhdr[2] = dhdr[2] - .5 * (dhdr[5] + dhdr[6] + dhdr[9]) -
    one_third * (dhdr[11] + dhdr[12] + dhdr[13]) - .25 * dhdr[10];
  dhdr[3] = dhdr[3] - .5 * (dhdr[7] + dhdr[8] + dhdr[9]) -
    one_third * (dhdr[14] + dhdr[12] + dhdr[13]) - .25 * dhdr[10];

  // dh/ds;
  dhds[11] = dhds[11] - 108.0 * dhds[10] / 256.0;
  dhds[14] = dhds[14] - 108.0 * dhds[10] / 256.0;
  dhds[12] = dhds[12] - 108.0 * dhds[10] / 256.0;
  dhds[13] = dhds[13] - 108.0 * dhds[10] / 256.0;
  dhds[4] = dhds[4] - four_ninths * (dhds[11] + dhds[14]) - .25 * dhds[10];
  dhds[5] = dhds[5] - four_ninths * (dhds[11] + dhds[12]) - .25 * dhds[10];
  dhds[6] = dhds[6] - four_ninths * (dhds[11] + dhds[13]) - .25 * dhds[10];
  dhds[7] = dhds[7] - four_ninths * (dhds[14] + dhds[13]) - .25 * dhds[10];
  dhds[8] = dhds[8] - four_ninths * (dhds[14] + dhds[12]) - .25 * dhds[10];
  dhds[9] = dhds[9] - four_ninths * (dhds[12] + dhds[13]) - .25 * dhds[10];
  dhds[0] = dhds[0] - .5 * (dhds[4] + dhds[6] + dhds[7]) -
    one_third * (dhds[11] + dhds[14] + dhds[13]) - .25 * dhds[10];
  dhds[1] = dhds[1] - .5 * (dhds[4] + dhds[5] + dhds[8]) -
    one_third * (dhds[11] + dhds[14] + dhds[12]) - .25 * dhds[10];
  dhds[2] = dhds[2] - .5 * (dhds[5] + dhds[6] + dhds[9]) -
    one_third * (dhds[11] + dhds[12] + dhds[13]) - .25 * dhds[10];
  dhds[3] = dhds[3] - .5 * (dhds[7] + dhds[8] + dhds[9]) -
    one_third * (dhds[14] + dhds[12] + dhds[13]) - .25 * dhds[10];

  // dh/dt;
  dhdt[11] = dhdt[11] - 108.0 * dhdt[10] / 256.0;
  dhdt[14] = dhdt[14] - 108.0 * dhdt[10] / 256.0;
  dhdt[12] = dhdt[12] - 108.0 * dhdt[10] / 256.0;
  dhdt[13] = dhdt[13] - 108.0 * dhdt[10] / 256.0;
  dhdt[4] = dhdt[4] - four_ninths * (dhdt[11] + dhdt[14]) - .25 * dhdt[10];
  dhdt[5] = dhdt[5] - four_ninths * (dhdt[11] + dhdt[12]) - .25 * dhdt[10];
  dhdt[6] = dhdt[6] - four_ninths * (dhdt[11] + dhdt[13]) - .25 * dhdt[10];
  dhdt[7] = dhdt[7] - four_ninths * (dhdt[14] + dhdt[13]) - .25 * dhdt[10];
  dhdt[8] = dhdt[8] - four_ninths * (dhdt[14] + dhdt[12]) - .25 * dhdt[10];
  dhdt[9] = dhdt[9] - four_ninths * (dhdt[12] + dhdt[13]) - .25 * dhdt[10];
  dhdt[0] = dhdt[0] - .5 * (dhdt[4] + dhdt[6] + dhdt[7]) -
    one_third * (dhdt[11] + dhdt[14] + dhdt[13]) - .25 * dhdt[10];
  dhdt[1] = dhdt[1] - .5 * (dhdt[4] + dhdt[5] + dhdt[8]) -
    one_third * (dhdt[11] + dhdt[14] + dhdt[12]) - .25 * dhdt[10];
  dhdt[2] = dhdt[2] - .5 * (dhdt[5] + dhdt[6] + dhdt[9]) -
    one_third * (dhdt[11] + dhdt[12] + dhdt[13]) - .25 * dhdt[10];
  dhdt[3] = dhdt[3] - .5 * (dhdt[7] + dhdt[8] + dhdt[9]) -
    one_third * (dhdt[14] + dhdt[12] + dhdt[13]) - .25 * dhdt[10];
}

double calculate_tet_volume_using_sides(
  const VerdictVector& side0, const VerdictVector& side2, const VerdictVector& side3)
{
  return (double)((side3 % (side2 * side0)) / 6.0);
}

/*!
  the volume of a tet

  1/6 * jacobian at a corner node
 */
double tet_volume(int num_nodes, const double coordinates[][3])
{
  // Determine side vectors
  VerdictVector side0, side2, side3;
  if (4 == num_nodes)
  {
    side2.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]);

    side0.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
      coordinates[2][2] - coordinates[0][2]);

    side3.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
      coordinates[3][2] - coordinates[0][2]);
    return calculate_tet_volume_using_sides(side0, side2, side3);
  }
  else
  {
    VerdictVector tet_pts[15];

    // create a vector for each point
    for (int k = 0; k < num_nodes; k++)
    {
      tet_pts[k].set(coordinates[k][0], coordinates[k][1], coordinates[k][2]);
    }

    // determine center point of the higher-order nodes
    VerdictVector centroid(0, 0, 0);
    for (int k = 4; k < num_nodes; k++)
    {
      centroid += VerdictVector(coordinates[k][0], coordinates[k][1], coordinates[k][2]);
    }
    centroid /= (num_nodes - 4);

    if (8 == num_nodes)
    {
      double tet_volume = 0;

      int tet_face_conn[4][4] = { { 0, 2, 1, 4 }, { 0, 1, 3, 7 }, { 1, 2, 3, 5 }, { 0, 3, 2, 6 } };

      for (int i = 0; i < 4; i++)
      {
        VerdictVector& node0 = tet_pts[tet_face_conn[i][0]];
        VerdictVector& node1 = tet_pts[tet_face_conn[i][1]];
        VerdictVector& node2 = tet_pts[tet_face_conn[i][2]];
        VerdictVector& node3 = tet_pts[tet_face_conn[i][3]];

        // 012
        side2 = node3 - node0;
        side0 = node1 - node0;
        side3 = centroid - node0;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 123
        side2 = node3 - node1;
        side0 = node2 - node1;
        side3 = centroid - node1;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 032
        side2 = node2 - node0;
        side0 = node3 - node0;
        side3 = centroid - node0;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);
      }

      return tet_volume;
    }
    if (10 == num_nodes)
    {
      double tet_volume = 0;

      int tet_face_conn[4][6] = { { 0, 2, 1, 6, 5, 4 }, { 0, 1, 3, 4, 8, 7 }, { 1, 2, 3, 5, 9, 8 },
        { 0, 3, 2, 7, 9, 6 } };

      for (int i = 0; i < 4; i++)
      {
        VerdictVector& node0 = tet_pts[tet_face_conn[i][0]];
        VerdictVector& node1 = tet_pts[tet_face_conn[i][1]];
        VerdictVector& node2 = tet_pts[tet_face_conn[i][2]];
        VerdictVector& node3 = tet_pts[tet_face_conn[i][3]];
        VerdictVector& node4 = tet_pts[tet_face_conn[i][4]];
        VerdictVector& node5 = tet_pts[tet_face_conn[i][5]];

        // 053
        side2 = node5 - node0;
        side0 = node3 - node0;
        side3 = centroid - node0;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 134
        side2 = node3 - node1;
        side0 = node4 - node1;
        side3 = centroid - node1;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 254
        side2 = node4 - node2;
        side0 = node5 - node2;
        side3 = centroid - node2;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 345
        side2 = node5 - node3;
        side0 = node4 - node3;
        side3 = centroid - node3;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);
      }

      return tet_volume;
    }
    if (num_nodes >= 14)
    {
      double tet_volume = 0;

      int tet_face_conn[4][7] = { { 0, 3, 2, 7, 9, 6, 12 }, { 0, 2, 1, 6, 5, 4, 10 },
        { 0, 1, 3, 4, 8, 7, 13 }, { 1, 2, 3, 5, 9, 8, 11 } };

      if (num_nodes == 15)
      {
        tet_face_conn[0][6]++;
        tet_face_conn[1][6]++;
        tet_face_conn[2][6]++;
        tet_face_conn[3][6]++;
      }

      for (int i = 0; i < 4; i++)
      {
        VerdictVector& node0 = tet_pts[tet_face_conn[i][0]];
        VerdictVector& node1 = tet_pts[tet_face_conn[i][1]];
        VerdictVector& node2 = tet_pts[tet_face_conn[i][2]];
        VerdictVector& node3 = tet_pts[tet_face_conn[i][3]];
        VerdictVector& node4 = tet_pts[tet_face_conn[i][4]];
        VerdictVector& node5 = tet_pts[tet_face_conn[i][5]];
        VerdictVector& node6 = tet_pts[tet_face_conn[i][6]];

        // 056
        side2 = node5 - node0;
        side0 = node6 - node0;
        side3 = centroid - node0;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 063
        side2 = node6 - node0;
        side0 = node3 - node0;
        side3 = centroid - node0;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 136
        side2 = node3 - node1;
        side0 = node6 - node1;
        side3 = centroid - node1;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 164
        side2 = node6 - node1;
        side0 = node4 - node1;
        side3 = centroid - node1;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 246
        side2 = node4 - node2;
        side0 = node6 - node2;
        side3 = centroid - node2;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);

        // 265
        side2 = node6 - node2;
        side0 = node5 - node2;
        side3 = centroid - node2;
        tet_volume += calculate_tet_volume_using_sides(side0, side2, side3);
      }

      return tet_volume;
    }
  }
  return 0;
}

/*!
  the condition of a tet

  condition number of the jacobian matrix at any corner

  NB (J. Pouderoux 01/27/15)
    This will return VERDICT_DBL_MAX when the volume of the tetrahedron is ill-
    conditioned. Previously, this would only happen when the volume was small
    and positive, but now ill-conditioned inverted tetrahedra are also included.
 */
double tet_condition(int /*num_nodes*/, const double coordinates[][3])
{
  double condition, term1, term2, det;

  VerdictVector side0, side2, side3;

  side0.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  side2.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  side3.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  VerdictVector c_1, c_2, c_3;

  c_1 = side0;
  c_2 = (-2 * side2 - side0) / sqrt3;
  c_3 = (3 * side3 + side2 - side0) / sqrt6;

  term1 = c_1 % c_1 + c_2 % c_2 + c_3 % c_3;
  term2 = (c_1 * c_2) % (c_1 * c_2) + (c_2 * c_3) % (c_2 * c_3) + (c_1 * c_3) % (c_1 * c_3);
  det = c_1 % (c_2 * c_3);

  if (std::abs(det) <= VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }
  else
  {
    condition = std::sqrt(term1 * term2) / (3.0 * det);
  }

  return (double)condition;
}

/*!
  the jacobian of a tet

  TODO
 */
double tet_jacobian(int num_nodes, const double coordinates[][3])
{
  if (num_nodes == 15)
  {
    double dhdr[15];
    double dhds[15];
    double dhdt[15];
    double min_determinant = VERDICT_DBL_MAX;

    for (int i = 0; i < 15; i++)
    {
      TET15_gradients_of_the_shape_functions_for_R_S_T(TET15_node_local_coord[i], dhdr, dhds, dhdt);
      double jacobian[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };

      for (int j = 0; j < 15; j++)
      {
        jacobian[0][0] += coordinates[j][0] * dhdr[j];
        jacobian[0][1] += coordinates[j][0] * dhds[j];
        jacobian[0][2] += coordinates[j][0] * dhdt[j];
        jacobian[1][0] += coordinates[j][1] * dhdr[j];
        jacobian[1][1] += coordinates[j][1] * dhds[j];
        jacobian[1][2] += coordinates[j][1] * dhdt[j];
        jacobian[2][0] += coordinates[j][2] * dhdr[j];
        jacobian[2][1] += coordinates[j][2] * dhds[j];
        jacobian[2][2] += coordinates[j][2] * dhdt[j];
      }
      double det =
        (VerdictVector(jacobian[0]) * VerdictVector(jacobian[1])) % VerdictVector(jacobian[2]);
      min_determinant = std::min(det, min_determinant);
    }
    return min_determinant;
  }
  else
  {
    VerdictVector side0, side2, side3;

    side0.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]);

    side2.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
      coordinates[0][2] - coordinates[2][2]);

    side3.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
      coordinates[3][2] - coordinates[0][2]);

    return (double)(side3 % (side2 * side0));
  }
}

/*!
  the shape of a tet

  3/ condition number of weighted jacobian matrix
 */
double tet_shape(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector edge0, edge2, edge3;

  edge0.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  edge2.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  edge3.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  double jacobian = edge3 % (edge2 * edge0);
  if (jacobian < VERDICT_DBL_MIN)
  {
    return (double)0.0;
  }
  double num = 3 * std::pow(sqrt2 * jacobian, two_thirds);
  double den = 1.5 * (edge0 % edge0 + edge2 % edge2 + edge3 % edge3) -
    (edge0 % -edge2 + -edge2 % edge3 + edge3 % edge0);

  if (den < VERDICT_DBL_MIN)
  {
    return (double)0.0;
  }

  double shape = num / den;
  if (shape < 0)
  {
    shape = 0;
  }
  return fix_range(shape);
}

/*!
  the relative size of a tet

  Min(J,1/J), where J is the determinant of the weighted Jacobian matrix
 */
double tet_relative_size_squared(
  int /*num_nodes*/, const double coordinates[][3], double average_tet_volume)
{
  double size;
  VerdictVector w1, w2, w3;
  tet_get_weight(w1, w2, w3, average_tet_volume);
  double avg_volume = (w1 % (w2 * w3)) / 6.0;

  double volume = tet_volume(4, coordinates);

  if (avg_volume < VERDICT_DBL_MIN)
  {
    return 0.0;
  }
  else
  {
    size = volume / avg_volume;
    if (size <= VERDICT_DBL_MIN)
    {
      return 0.0;
    }
    if (size > 1)
    {
      size = (double)(1) / size;
    }
  }
  return (double)(size * size);
}

/*!
  the shape and size of a tet

  Product of the shape and relative size
 */
double tet_shape_and_size(int num_nodes, const double coordinates[][3], double average_tet_volume)
{
  double shape, size;
  shape = tet_shape(num_nodes, coordinates);
  size = tet_relative_size_squared(num_nodes, coordinates, average_tet_volume);

  return (double)(shape * size);
}

/*!
  the distortion of a tet
 */
double tet_distortion(int num_nodes, const double coordinates[][3])
{
  double distortion = VERDICT_DBL_MAX;
  int number_of_gauss_points = 0;
  if (num_nodes < 10)
  {
    // for linear tet, the distortion is always 1 because
    // straight edge tets are the target shape for tet
    return 1.0;
  }
  else if (num_nodes >= 10)
  {
    // use four integration points for quadratic tet
    number_of_gauss_points = 4;
  }
  num_nodes = 10;

  int number_dims = 3;
  int total_number_of_gauss_points = number_of_gauss_points;
  // use is_tri=1 to indicate this is for tet in 3D
  int is_tri = 1;

  double shape_function[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy1[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy2[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy3[maxTotalNumberGaussPoints][maxNumberNodes];
  double weight[maxTotalNumberGaussPoints];

  // create an object of GaussIntegration for tet
  GaussIntegration gint{};
  gint.initialize(number_of_gauss_points, num_nodes, number_dims, is_tri);
  gint.calculate_shape_function_3d_tet();
  gint.get_shape_func(shape_function[0], dndy1[0], dndy2[0], dndy3[0], weight);

  // vector xxi is the derivative vector of coordinates w.r.t local xi coordinate in the
  // computation space
  // vector xet is the derivative vector of coordinates w.r.t local et coordinate in the
  // computation space
  // vector xze is the derivative vector of coordinates w.r.t local ze coordinate in the
  // computation space
  VerdictVector xxi, xet, xze, xin;

  double jacobian, minimum_jacobian;
  double element_volume = 0.0;
  minimum_jacobian = VERDICT_DBL_MAX;

  // calculate element volume
  int ife, ja;
  for (ife = 0; ife < total_number_of_gauss_points; ife++)
  {
    xxi.set(0.0, 0.0, 0.0);
    xet.set(0.0, 0.0, 0.0);
    xze.set(0.0, 0.0, 0.0);

    for (ja = 0; ja < num_nodes; ja++)
    {
      xin.set(coordinates[ja][0], coordinates[ja][1], coordinates[ja][2]);
      xxi += dndy1[ife][ja] * xin;
      xet += dndy2[ife][ja] * xin;
      xze += dndy3[ife][ja] * xin;
    }

    // determinant
    jacobian = xxi % (xet * xze);
    if (minimum_jacobian > jacobian)
    {
      minimum_jacobian = jacobian;
    }

    element_volume += weight[ife] * jacobian;
  } // element_volume is 6 times the actual volume

  // loop through all nodes
  double dndy1_at_node[maxNumberNodes][maxNumberNodes];
  double dndy2_at_node[maxNumberNodes][maxNumberNodes];
  double dndy3_at_node[maxNumberNodes][maxNumberNodes];

  gint.calculate_derivative_at_nodes_3d_tet(dndy1_at_node, dndy2_at_node, dndy3_at_node);
  int node_id;
  for (node_id = 0; node_id < num_nodes; node_id++)
  {
    xxi.set(0.0, 0.0, 0.0);
    xet.set(0.0, 0.0, 0.0);
    xze.set(0.0, 0.0, 0.0);

    for (ja = 0; ja < num_nodes; ja++)
    {
      xin.set(coordinates[ja][0], coordinates[ja][1], coordinates[ja][2]);
      xxi += dndy1_at_node[node_id][ja] * xin;
      xet += dndy2_at_node[node_id][ja] * xin;
      xze += dndy3_at_node[node_id][ja] * xin;
    }

    jacobian = xxi % (xet * xze);
    if (minimum_jacobian > jacobian)
    {
      minimum_jacobian = jacobian;
    }
  }
  if (std::abs(element_volume) > 0.0)
  {
    distortion = minimum_jacobian / element_volume;
  }

  return fix_range(distortion);
}

double tet_inradius(int num_nodes, const double coordinates[][3])
{
  // avoid access beyond the end of the array
  if (num_nodes < 4)
  {
    return 0.;
  }

  if (num_nodes == 10)
  {
    return tet10_characteristic_length(coordinates);
  }

  // area1 (0,1,2)
  double a1 = verdict::tri_area(3, coordinates);

  // area2 (0,3,1)
  double tmp_coords[3][3];
  tmp_coords[0][0] = coordinates[0][0];
  tmp_coords[0][1] = coordinates[0][1];
  tmp_coords[0][2] = coordinates[0][2];

  tmp_coords[1][0] = coordinates[3][0];
  tmp_coords[1][1] = coordinates[3][1];
  tmp_coords[1][2] = coordinates[3][2];

  tmp_coords[2][0] = coordinates[1][0];
  tmp_coords[2][1] = coordinates[1][1];
  tmp_coords[2][2] = coordinates[1][2];

  double a2 = verdict::tri_area(3, tmp_coords);

  // area3 (0,2,3)
  tmp_coords[1][0] = coordinates[2][0];
  tmp_coords[1][1] = coordinates[2][1];
  tmp_coords[1][2] = coordinates[2][2];

  tmp_coords[2][0] = coordinates[3][0];
  tmp_coords[2][1] = coordinates[3][1];
  tmp_coords[2][2] = coordinates[3][2];

  double a3 = verdict::tri_area(3, tmp_coords);

  // area4 (1,3,2)
  tmp_coords[0][0] = coordinates[1][0];
  tmp_coords[0][1] = coordinates[1][1];
  tmp_coords[0][2] = coordinates[1][2];

  tmp_coords[1][0] = coordinates[3][0];
  tmp_coords[1][1] = coordinates[3][1];
  tmp_coords[1][2] = coordinates[3][2];

  tmp_coords[2][0] = coordinates[2][0];
  tmp_coords[2][1] = coordinates[2][1];
  tmp_coords[2][2] = coordinates[2][2];

  double a4 = verdict::tri_area(3, tmp_coords);

  double tet_volume = verdict::tet_volume(4, coordinates);

  double inradius = 3 * tet_volume / (a1 + a2 + a3 + a4);

  return inradius;
}

double tet_timestep(int num_nodes, const double coordinates[][3], double density,
  double poissons_ratio, double youngs_modulus)
{
  double char_length = 0;
  if (10 == num_nodes)
  {
    char_length = 2 * tet10_characteristic_length(coordinates);
  }
  else
  {
    char_length = 2 * tet_inradius(num_nodes, coordinates);
  }

  double M =
    youngs_modulus * (1 - poissons_ratio) / ((1 - 2 * poissons_ratio) * (1 + poissons_ratio));
  double denominator = std::sqrt(M / density);

  return char_length / denominator;
}

VerdictVector tet10_auxillary_node_coordinate(const double coordinates[][3])
{
  VerdictVector aux_node(0, 0, 0);
  for (int i = 4; i < 10; i++)
  {
    VerdictVector tmp_vec(coordinates[i][0], coordinates[i][1], coordinates[i][2]);
    aux_node += tmp_vec;
  }
  aux_node /= 6;

  return aux_node;
}

double tet10_min_inradius(const double coordinates[][3], int begin_index, int end_index)
{
  double min_tetinradius = VERDICT_DBL_MAX;

  VerdictVector auxillary_node = tet10_auxillary_node_coordinate(coordinates);

  for (int i = begin_index; i <= end_index; i++)
  {
    int subtet_conn[4];
    subtet_conn[0] = tet10_subtet_conn[i][0];
    subtet_conn[1] = tet10_subtet_conn[i][1];
    subtet_conn[2] = tet10_subtet_conn[i][2];
    subtet_conn[3] = tet10_subtet_conn[i][3];

    // get the coordinates of the nodes
    double subtet_coords[4][3];
    for (int k = 0; k < 4; k++)
    {
      int node_index = subtet_conn[k];

      if (10 == node_index)
      {
        subtet_coords[k][0] = auxillary_node.x();
        subtet_coords[k][1] = auxillary_node.y();
        subtet_coords[k][2] = auxillary_node.z();
      }
      else
      {
        subtet_coords[k][0] = coordinates[node_index][0];
        subtet_coords[k][1] = coordinates[node_index][1];
        subtet_coords[k][2] = coordinates[node_index][2];
      }
    }

    double tmp_inradius = tet_inradius(4, subtet_coords);

    if (tmp_inradius < min_tetinradius)
    {
      min_tetinradius = tmp_inradius;
    }
  }
  return min_tetinradius;
}

double tet10_characteristic_length(const double coordinates[][3])
{
  // compute auxillary node coordinate
  double min_tetinradius = tet10_min_inradius(coordinates, 0, 11);
  min_tetinradius *= 2.3;

  return min_tetinradius;
}
double calculate_tet4_outer_radius(const double coordinates[][3])
{
  verdict::VerdictVector nE[4];
  for (int i{ 0 }; i < 4; i++)
  {
    nE[i].set(coordinates[i]);
  }

  double aC = (nE[1] - nE[0]).length();
  double bC = (nE[2] - nE[0]).length();
  double cC = (nE[3] - nE[0]).length();
  double AC = (nE[3] - nE[2]).length();
  double BC = (nE[3] - nE[1]).length();
  double CC = (nE[2] - nE[1]).length();
  double VP = ((nE[1] - nE[0]) * (nE[2] - nE[0])) % (nE[3] - nE[0]) / 6;
  double outer_radius = std::sqrt((aC * AC + bC * BC + cC * CC) * (aC * AC + bC * BC - cC * CC) *
                          (aC * AC - bC * BC + cC * CC) * (-aC * AC + bC * BC + cC * CC)) /
    24 / VP;

  return outer_radius;
}

double tet10_normalized_inradius(const double coordinates[][3])
{
  double min_inradius_for_subtet_with_parent_node = tet10_min_inradius(coordinates, 0, 3);
  double min_inradius_for_subtet_with_no_parent_node = tet10_min_inradius(coordinates, 4, 11);

  double outer_radius = calculate_tet4_outer_radius(coordinates);

  double normalized_inradius_for_subtet_with_parent_node =
    6.0 * min_inradius_for_subtet_with_parent_node / outer_radius;
  double normalized_inradius_for_subtet_with_no_parent_node =
    three_times_1plussqrt3 * min_inradius_for_subtet_with_no_parent_node / outer_radius;

  double norm_inrad = std::min(normalized_inradius_for_subtet_with_parent_node,
    normalized_inradius_for_subtet_with_no_parent_node);
  return fix_range(norm_inrad);
}

double tet4_normalized_inradius(const double coordinates[][3])
{
  double tet10_coords[10][3];
  for (int i = 0; i < 4; i++)
  {
    tet10_coords[i][0] = coordinates[i][0];
    tet10_coords[i][1] = coordinates[i][1];
    tet10_coords[i][2] = coordinates[i][2];
  }

  static int eidx[6][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 }, { 0, 3 }, { 1, 3 }, { 2, 3 } };
  for (int i = 4; i < 10; i++)
  {
    int i0 = eidx[i - 4][0];
    int i1 = eidx[i - 4][1];
    tet10_coords[i][0] = (coordinates[i0][0] + coordinates[i1][0]) * 0.5;
    tet10_coords[i][1] = (coordinates[i0][1] + coordinates[i1][1]) * 0.5;
    tet10_coords[i][2] = (coordinates[i0][2] + coordinates[i1][2]) * 0.5;
  }
  return tet10_normalized_inradius(tet10_coords);
}

double tet_normalized_inradius(int num_nodes, const double coordinates[][3])
{
  if (num_nodes == 4)
  {
    return tet4_normalized_inradius(coordinates);
  }
  else if (num_nodes >= 10)
  {
    return tet10_normalized_inradius(coordinates);
  }
  return 0.0;
}

double tet_mean_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  const VerdictVector side0(coordinates[1][0] - coordinates[0][0],
    coordinates[1][1] - coordinates[0][1], coordinates[1][2] - coordinates[0][2]);

  const VerdictVector side2(coordinates[0][0] - coordinates[2][0],
    coordinates[0][1] - coordinates[2][1], coordinates[0][2] - coordinates[2][2]);

  const VerdictVector side3(coordinates[3][0] - coordinates[0][0],
    coordinates[3][1] - coordinates[0][1], coordinates[3][2] - coordinates[0][2]);

  const double tetVolume = calculate_tet_volume_using_sides(side0, side2, side3);
  if (std::abs(tetVolume) < VERDICT_DBL_MIN)
  {
    return 0.0;
  }

  const VerdictVector side1(coordinates[2][0] - coordinates[1][0],
    coordinates[2][1] - coordinates[1][1], coordinates[2][2] - coordinates[1][2]);

  const VerdictVector side4(coordinates[3][0] - coordinates[1][0],
    coordinates[3][1] - coordinates[1][1], coordinates[3][2] - coordinates[1][2]);

  const VerdictVector side5(coordinates[3][0] - coordinates[2][0],
    coordinates[3][1] - coordinates[2][1], coordinates[3][2] - coordinates[2][2]);

  const double side0_length_squared = side0.length_squared();
  const double side1_length_squared = side1.length_squared();
  const double side2_length_squared = side2.length_squared();
  const double side3_length_squared = side3.length_squared();
  const double side4_length_squared = side4.length_squared();
  const double side5_length_squared = side5.length_squared();

  const int sign = tetVolume < 0. ? -1 : 1;
  return sign * 12. * std::pow(3. * std::abs(tetVolume), 2. / 3.) /
    (side0_length_squared + side1_length_squared + side2_length_squared + side3_length_squared +
      side4_length_squared + side5_length_squared);
}
} // namespace verdict
