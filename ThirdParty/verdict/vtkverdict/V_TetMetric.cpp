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

#include <math.h>

namespace VERDICT_NAMESPACE
{
static constexpr double three_times_1plussqrt3 = 3.0 * (1 + sqrt3);
static constexpr double normal_coeff = 180. * .3183098861837906715377675267450287;
static constexpr double aspect_ratio_normal_coeff = sqrt6 / 12.;
VERDICT_HOST_DEVICE double tet10_characteristic_length(const double coordinates[][3]);

VERDICT_HOST_DEVICE static const int* tet10_subtet_conn(int i)
{
  static constexpr int stet10_subtet_conn[12][4] = { { 0, 4, 6, 7 }, { 1, 5, 4, 8 }, { 2, 6, 5, 9 },
    { 3, 8, 7, 9 }, { 4, 8, 5, 10 }, { 5, 8, 9, 10 }, { 9, 8, 7, 10 }, { 7, 8, 4, 10 },
    { 4, 5, 6, 10 }, { 5, 9, 6, 10 }, { 9, 7, 6, 10 }, { 7, 4, 6, 10 } };
  return stet10_subtet_conn[i];
}

VERDICT_HOST_DEVICE static double fix_range(double v)
{
  if (isnan(v))
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

VERDICT_HOST_DEVICE double tet_equiangle_skew(int /*num_nodes*/, const double coordinates[][3])
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

  double alpha = acos(-(abc % abd));
  double beta = acos(-(abc % acd));
  double gamma = acos(-(abc % bcd));
  double delta = acos(-(abd % acd));
  double epsilon = acos(-(abd % bcd));
  double zeta = acos(-(acd % bcd));

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

  double theta = acos(1 / 3.0) * normal_coeff; // 70.528779365509308630754000660038;

  double dihedral_skew_max = (max_angle - theta) / (180 - theta);
  double dihedral_skew_min = (theta - min_angle) / theta;

  min_angle = 360.0;
  max_angle = 0.0;

  double angles[12];
  angles[0] = acos(-(ab % bc));
  angles[1] = acos((bc % ac));
  angles[2] = acos((ac % ab));
  angles[3] = acos(-(ab % bd));
  angles[4] = acos((bd % ad));
  angles[5] = acos((ad % ab));
  angles[6] = acos(-(bc % cd));
  angles[7] = acos((cd % bd));
  angles[8] = acos((bd % bc));
  angles[9] = acos((ad % cd));
  angles[10] = acos(-(cd % ac));
  angles[11] = acos((ac % ad));

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
VERDICT_HOST_DEVICE static int tet_get_weight(
  VerdictVector& w1, VerdictVector& w2, VerdictVector& w3, double average_tet_volume)
{
  w1.set(1, 0, 0);
  w2.set(0.5, 0.5 * sqrt3, 0);
  w3.set(0.5, sqrt3 / 6.0, sqrt2 / sqrt3);

  double scale = pow(6. * average_tet_volume / determinant(w1, w2, w3), one_third);

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
VERDICT_HOST_DEVICE double tet_edge_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector v[6];

  v[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  v[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  v[2].set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  v[3].set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  v[4].set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);

  v[5].set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  apply_elem_scaling_on_edges(4, coordinates, 6, v);

  double a2 = v[0].length_squared();
  double b2 = v[1].length_squared();
  double c2 = v[2].length_squared();
  double d2 = v[3].length_squared();
  double e2 = v[4].length_squared();
  double f2 = v[5].length_squared();

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

  const double edge_ratio = sqrt(M2 / m2);

  return fix_range(edge_ratio);
}

VERDICT_HOST_DEVICE static const double* TET15_node_local_coord(int i)
{
  static const double sTET15_node_local_coord[15][3] = { { 0, 0, 0 }, { 1.0, 0, 0 }, { 0, 1.0, 0 },
    { 0, 0, 1.0 }, { .5, 0, 0 }, { .5, .5, 0 }, { 0, .5, 0 }, { 0, 0, .5 }, { .5, 0, .5 },
    { 0, .5, .5 }, { one_third, one_third, 0 }, { one_third, 0, one_third },
    { one_third, one_third, one_third }, { 0, one_third, one_third },
    { one_fourth, one_fourth, one_fourth } };
  return sTET15_node_local_coord[i];
};

VERDICT_HOST_DEVICE static void TET15_gradients_of_the_shape_functions_for_R_S_T(
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

VERDICT_HOST_DEVICE static const double* TET10_node_local_coord(int i)
{
  static const double node_local_coord[10][3] = { { 0, 0, 0 }, { 1.0, 0, 0 }, { 0, 1.0, 0 },
    { 0, 0, 1.0 }, { .5, 0, 0 }, { .5, .5, 0 }, { 0, .5, 0 }, { 0, 0, .5 }, { .5, 0, .5 },
    { 0, .5, .5 } };
  return node_local_coord[i];
};

VERDICT_HOST_DEVICE static void TET10_gradients_of_the_shape_functions_for_R_S_T(
  const double rst[3], double dhdr[10], double dhds[10], double dhdt[10])
{
  double r = rst[0];
  double s = rst[1];
  double t = rst[2];

  // dh/dr;
  dhdr[0] = 4.0 * (r + s + t) - 3.0;
  dhdr[1] = 4.0 * r - 1.0;
  dhdr[2] = 0.0;
  dhdr[3] = 0.0;
  dhdr[4] = 4.0 - 8.0 * r - 4.0 * s - 4.0 * t;
  dhdr[5] = 4.0 * s;
  dhdr[6] = -4.0 * s;
  dhdr[7] = -4.0 * t;
  dhdr[8] = 4.0 * t;
  dhdr[9] = 0.0;

  // dh/ds;
  dhds[0] = 4.0 * (r + s + t) - 3.0;
  dhds[1] = 0.0;
  dhds[2] = 4.0 * s - 1.0;
  dhds[3] = 0.0;
  dhds[4] = -4.0 * r;
  dhds[5] = 4.0 * r;
  dhds[6] = 4.0 - 4.0 * r - 8.0 * s - 4.0 * t;
  dhds[7] = -4.0 * t;
  dhds[8] = 0.0;
  dhds[9] = 4.0 * t;

  // dh/dt;
  dhdt[0] = 4.0 * (r + s + t) - 3.0;
  dhdt[1] = 0.0;
  dhdt[2] = 0.0;
  dhdt[3] = 4.0 * t - 1.0;
  dhdt[4] = -4.0 * r;
  dhdt[5] = 0.0;
  dhdt[6] = -4.0 * s;
  dhdt[7] = 4.0 - 4.0 * r - 4.0 * s - 8.0 * t;
  dhdt[8] = 4.0 * r;
  dhdt[9] = 4.0 * s;
}

/*!
  the jacobian of a tet

  TODO
 */

template <typename ContainerType>
VERDICT_HOST_DEVICE static double tet_jacobian_impl(int num_nodes, ContainerType coordinates)
{
  if (num_nodes == 15)
  {
    double dhdr[15];
    double dhds[15];
    double dhdt[15];
    double min_determinant = VERDICT_DBL_MAX;

    for (int i = 0; i < 15; i++)
    {
      TET15_gradients_of_the_shape_functions_for_R_S_T(TET15_node_local_coord(i), dhdr, dhds, dhdt);
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
      min_determinant = fmin(det, min_determinant);
    }
    return min_determinant;
  }
  else if (num_nodes == 10)
  {
    double dhdr[10];
    double dhds[10];
    double dhdt[10];
    double min_determinant = VERDICT_DBL_MAX;

    for (int i = 0; i < 10; i++)
    {
      TET10_gradients_of_the_shape_functions_for_R_S_T(TET10_node_local_coord(i), dhdr, dhds, dhdt);
      double jacobian[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };

      for (int j = 0; j < 10; j++)
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
      min_determinant = fmin(det, min_determinant);
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
  the scaled jacobian of a tet

  minimum of the jacobian divided by the lengths of 3 edge vectors

 */

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet_scaled_jacobian_impl(CoordsContainerType coordinates)
{
  VerdictVector side0{coordinates[0], coordinates[1]};
  VerdictVector side1{coordinates[1], coordinates[2]};
  VerdictVector side2{coordinates[2], coordinates[0]};
  VerdictVector side3{coordinates[0], coordinates[3]};
  VerdictVector side4{coordinates[1], coordinates[3]};
  VerdictVector side5{coordinates[2], coordinates[3]};

  double char_size = elem_scaling(4, coordinates).scale;
  side0 /= char_size;
  side1 /= char_size;
  side2 /= char_size;
  side3 /= char_size;
  side4 /= char_size;
  side5 /= char_size;

  const double jacobi = side3 % (side2 * side0);

  // products of lengths squared of each edge attached to a node.
  const double side0_length_squared = side0.length_squared();
  const double side1_length_squared = side1.length_squared();
  const double side2_length_squared = side2.length_squared();
  const double side3_length_squared = side3.length_squared();
  const double side4_length_squared = side4.length_squared();
  const double side5_length_squared = side5.length_squared();

  const double length_squared[4] = {
    side0_length_squared * side2_length_squared * side3_length_squared,
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

  double length_product = sqrt(length_squared[which_node]);
  if (length_product < fabs(jacobi))
  {
    length_product = fabs(jacobi);
  }

  if (length_product < VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }

  return sqrt2 * jacobi / length_product;
}


template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet10_scaled_jacobian_impl(CoordsContainerType coordinates)
{
  VerdictVector node_pos[10] =
  {
    {coordinates[0]},
    {coordinates[1]},
    {coordinates[2]},
    {coordinates[3]},
    {coordinates[4]},
    {coordinates[5]},
    {coordinates[6]},
    {coordinates[7]},
    {coordinates[8]},
    {coordinates[9]}
  };

  apply_elem_scaling_on_points(10, coordinates, 10, node_pos);

  double jacobi = tet_jacobian_impl(10, node_pos);

  // products of lengths squared of each edge attached to a node.
  const double side0_length = VerdictVector(node_pos[0], node_pos[4]).length() + VerdictVector(node_pos[4], node_pos[1]).length();
  const double side1_length = VerdictVector(node_pos[1], node_pos[5]).length() + VerdictVector(node_pos[5], node_pos[2]).length();
  const double side2_length = VerdictVector(node_pos[2], node_pos[6]).length() + VerdictVector(node_pos[6], node_pos[0]).length();
  const double side3_length = VerdictVector(node_pos[0], node_pos[7]).length() + VerdictVector(node_pos[7], node_pos[3]).length();
  const double side4_length = VerdictVector(node_pos[1], node_pos[8]).length() + VerdictVector(node_pos[8], node_pos[3]).length();
  const double side5_length = VerdictVector(node_pos[2], node_pos[9]).length() + VerdictVector(node_pos[9], node_pos[3]).length();

  const double length[4] = {
    side0_length * side2_length * side3_length,
    side0_length * side1_length * side4_length,
    side1_length * side2_length * side5_length,
    side3_length * side4_length * side5_length };
  int which_node = 0;
  if (length[1] > length[which_node])
  {
    which_node = 1;
  }
  if (length[2] > length[which_node])
  {
    which_node = 2;
  }
  if (length[3] > length[which_node])
  {
    which_node = 3;
  }

  double length_product = length[which_node];
  if (length_product < fabs(jacobi))
  {
    length_product = fabs(jacobi);
  }

  if (length_product < VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }

  return sqrt2 * jacobi / length_product;
}

VERDICT_HOST_DEVICE double tet_scaled_jacobian(int num_nodes, const double coordinates[][3])
{
  if(num_nodes == 10)
    return tet10_scaled_jacobian_impl(coordinates);
  return tet_scaled_jacobian_impl(coordinates);
}

VERDICT_HOST_DEVICE double tet_scaled_jacobian_from_loc_ptrs(int num_nodes, const double * const * coordinates)
{
  if (num_nodes == 10)
    return tet10_scaled_jacobian_impl(coordinates);
  return tet_scaled_jacobian_impl(coordinates);
}
/*!
  The radius ratio of a tet

  NB (P. Pebay 04/16/07):
    CR / (3.0 * IR) where CR is the circumsphere radius and IR is the inscribed
    sphere radius.
    Note that this function is similar to the aspect beta of a tet, except that
    it does not return VERDICT_DBL_MAX if the element has negative orientation.
 */
VERDICT_HOST_DEVICE double tet_radius_ratio(int /*num_nodes*/, const double coordinates[][3])
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

  double char_size = apply_elem_scaling_on_edges(4, coordinates, 6, side);

  VerdictVector numerator = side[3].length_squared() * (side[2] * side[0]) +
    side[2].length_squared() * (side[3] * side[0]) + side[0].length_squared() * (side[3] * side[2]);

  double area_sum;
  area_sum = ((side[2] * side[0]).length() + (side[3] * side[0]).length() +
               (side[4] * side[1]).length() + (side[3] * side[2]).length()) *
    0.5;

  double volume = tet_volume(4, coordinates);
  volume /= (char_size*char_size*char_size);

  if (fabs(volume) < VERDICT_DBL_MIN)
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
template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet_aspect_ratio_impl(int /*num_nodes*/, const CoordsContainerType coordinates)
{
  // Determine side vectors
  VerdictVector ab{coordinates[0], coordinates[1]};
  VerdictVector ac{coordinates[0], coordinates[2]};
  VerdictVector ad{coordinates[0], coordinates[3]};

  double char_size = elem_scaling(4, coordinates).scale;
  ab /= char_size;
  ac /= char_size;
  ad /= char_size;

  double detTet = ab % (ac * ad);

  if (fabs(detTet) < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  VerdictVector bc{coordinates[1], coordinates[2]};
  VerdictVector bd{coordinates[1], coordinates[3]};
  VerdictVector cd{coordinates[2], coordinates[3]};
  bc /= char_size;
  bd /= char_size;
  cd /= char_size;

  const double ab2 = ab.length_squared();
  const double bc2 = bc.length_squared();
  const double ac2 = ac.length_squared();
  const double ad2 = ad.length_squared();
  const double bd2 = bd.length_squared();
  const double cd2 = cd.length_squared();

  double A = ab2 > bc2 ? ab2 : bc2;
  double B = ac2 > ad2 ? ac2 : ad2;
  double C = bd2 > cd2 ? bd2 : cd2;
  double D = A > B ? A : B;
  const double hm = D > C ? sqrt(D) : sqrt(C);

  bd = ab * bc;
  A = bd.length();
  bd = ab * ad;
  B = bd.length();
  bd = ac * ad;
  C = bd.length();
  bd = bc * cd;
  D = bd.length();

  const double aspect_ratio = aspect_ratio_normal_coeff * hm * (A + B + C + D) / fabs(detTet);

  return fix_range(aspect_ratio);
}

VERDICT_HOST_DEVICE double tet_aspect_ratio(int num_nodes, const double coordinates[][3])
{
    return tet_aspect_ratio_impl(num_nodes, coordinates);
}
VERDICT_HOST_DEVICE double tet_aspect_ratio_from_loc_ptrs(int num_nodes, const double * const *coordinates)
{
    return tet_aspect_ratio_impl(num_nodes, coordinates);
}

/*!
  the aspect gamma of a tet

  srms^3 / (8.48528137423857*V) where srms = sqrt(sum(Si^2)/6), where Si is the edge length
 */
VERDICT_HOST_DEVICE double tet_aspect_gamma(int /*num_nodes*/, const double coordinates[][3])
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

  double char_size = apply_elem_scaling_on_edges(4, coordinates, 6, side);

  double volume = fabs(tet_volume(4, coordinates));
  volume /= (char_size*char_size*char_size);

  if (volume < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }
  else
  {
    double srms =
      sqrt((side[0].length_squared() + side[1].length_squared() + side[2].length_squared() +
                  side[3].length_squared() + side[4].length_squared() + side[5].length_squared()) /
        6.0);

    double aspect_ratio_gamma = (srms * srms * srms) / (8.48528137423857 * volume);
    return (double)aspect_ratio_gamma;
  }
}

/*!
  The aspect frobenius of a tet

  NB (P. Pebay 01/22/07):
    Frobenius condition number when the reference element is regular
 */
VERDICT_HOST_DEVICE double tet_aspect_frobenius(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector side[3];

  side[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  side[1].set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  side[2].set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  double char_size = elem_scaling(4, coordinates, 3).scale;
  side[0] /= char_size;
  side[1] /= char_size;
  side[2] /= char_size;

  double denominator = side[0] % (side[1] * side[2]);
  denominator *= denominator;
  denominator *= 2.;
  denominator = 3. * pow(denominator, one_third);

  if (denominator < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double u[3];
  side[0].get_xyz(u);
  double v[3];
  side[1].get_xyz(v);
  double w[3];
  side[2].get_xyz(w);

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
VERDICT_HOST_DEVICE double tet_minimum_angle(int /*num_nodes*/, const double coordinates[][3])
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

  double alpha = acos((abc % abd) / (nabc * nabd));
  double beta = acos((abc % acd) / (nabc * nacd));
  double gamma = acos((abc % bcd) / (nabc * nbcd));
  double delta = acos((abd % acd) / (nabd * nacd));
  double epsilon = acos((abd % bcd) / (nabd * nbcd));
  double zeta = acos((acd % bcd) / (nacd * nbcd));

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
VERDICT_HOST_DEVICE double tet_collapse_ratio(int /*num_nodes*/, const double coordinates[][3])
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

VERDICT_HOST_DEVICE double tet_equivolume_skew(int num_nodes, const double coordinates[][3])
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
  double optimal_length = circumradius / sqrt(double(3.0) / 8.0);
  double optimal_volume = (1.0 / 12.0) * sqrt(double(2.0)) * (optimal_length * optimal_length * optimal_length);

  const double eq_v_skew = (optimal_volume - volume) / optimal_volume;
  return fix_range(eq_v_skew);
}

VERDICT_HOST_DEVICE double tet_squish_index(int /*num_nodes*/, const double coordinates[][3])
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

VERDICT_HOST_DEVICE static double calculate_tet_volume_using_sides(
  const VerdictVector& side0, const VerdictVector& side2, const VerdictVector& side3)
{
  return (double)((side3 % (side2 * side0)) / 6.0);
}

/*!
  the volume of a tet

  1/6 * jacobian at a corner node
 */
template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet_volume_impl(int num_nodes, const CoordsContainerType coordinates)
{
  // Determine side vectors
  if (4 == num_nodes)
  {
    const VerdictVector side2{coordinates[0], coordinates[1]};
    const VerdictVector side0{coordinates[0], coordinates[2]};
    const VerdictVector side3{coordinates[0], coordinates[3]};
    return calculate_tet_volume_using_sides(side0, side2, side3);
  }
  else
  {
    VerdictVector tet_pts[15];
    VerdictVector side0, side2, side3;

    // create a vector for each point
    for (int k = 0; k < num_nodes; k++)
    {
      tet_pts[k].set(coordinates[k][0], coordinates[k][1], coordinates[k][2]);
    }

    // determine center point of the higher-order nodes
    VerdictVector centroid(0.0, 0.0, 0.0);
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

VERDICT_HOST_DEVICE double tet_volume(int num_nodes, const double coordinates[][3])
{
    return tet_volume_impl(num_nodes, coordinates);
}

VERDICT_HOST_DEVICE double tet_volume_from_loc_ptrs(int num_nodes, const double * const *coordinates)
{
    return tet_volume_impl(num_nodes, coordinates);
}

/*!
  the condition of a tet

  condition number of the jacobian matrix at any corner

  NB (J. Pouderoux 01/27/15)
    This will return VERDICT_DBL_MAX when the volume of the tetrahedron is ill-
    conditioned. Previously, this would only happen when the volume was small
    and positive, but now ill-conditioned inverted tetrahedra are also included.
 */
template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet_condition_impl(int /*num_nodes*/, const CoordsContainerType coordinates)
{
  VerdictVector side0{coordinates[0], coordinates[1]};
  VerdictVector side2{coordinates[2], coordinates[0]};
  VerdictVector side3{coordinates[0], coordinates[3]};

  double char_size = elem_scaling(4, coordinates).scale;
  side0 /= char_size;
  side2 /= char_size;
  side3 /= char_size;

  const VerdictVector c_1 = side0;
  const VerdictVector c_2 = (-2. * side2 - side0) / sqrt3;
  const VerdictVector c_3 = (3. * side3 + side2 - side0) / sqrt6;

  const double term1 = (c_1 % c_1) + (c_2 % c_2) + (c_3 % c_3);
  const double term2 = (c_1 * c_2) % (c_1 * c_2) + (c_2 * c_3) % (c_2 * c_3) + (c_1 * c_3) % (c_1 * c_3);
  const double det = c_1 % (c_2 * c_3);

  if (fabs(det) <= VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }
  else
  {
    return sqrt(term1) * sqrt(term2) / (3.0 * det);
  }
}

VERDICT_HOST_DEVICE double tet_condition(int num_nodes, const double coordinates[][3])
{
    return tet_condition_impl(num_nodes, coordinates);
}

VERDICT_HOST_DEVICE double tet_condition_from_loc_ptrs(int num_nodes, const double * const *coordinates)
{
    return tet_condition_impl(num_nodes, coordinates);
}

VERDICT_HOST_DEVICE double tet_jacobian(int num_nodes, const double coordinates[][3])
{
  return tet_jacobian_impl(num_nodes, coordinates);
}

/*!
  the shape of a tet

  3/ condition number of weighted jacobian matrix
 */
VERDICT_HOST_DEVICE double tet_shape(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector edge[3];

  edge[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  edge[1].set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  edge[2].set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  apply_elem_scaling_on_edges(4, coordinates, 3, edge);

  double jacobian = edge[2] % (edge[1] * edge[0]);
  if (jacobian < VERDICT_DBL_MIN)
  {
    return (double)0.0;
  }
  double num = 3 * pow(sqrt2 * jacobian, two_thirds);
  double den = 1.5 * (edge[0] % edge[0] + edge[1] % edge[1] + edge[2] % edge[2]) -
    (edge[0] % -edge[1] + -edge[1] % edge[2] + edge[2] % edge[0]);

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
VERDICT_HOST_DEVICE double tet_relative_size_squared(
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
VERDICT_HOST_DEVICE double tet_shape_and_size(int num_nodes, const double coordinates[][3], double average_tet_volume)
{
  double shape, size;
  shape = tet_shape(num_nodes, coordinates);
  size = tet_relative_size_squared(num_nodes, coordinates, average_tet_volume);

  return (double)(shape * size);
}

/*!
  the distortion of a tet
 */
VERDICT_HOST_DEVICE double tet_distortion(int num_nodes, const double coordinates[][3])
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
  if (fabs(element_volume) > 0.0)
  {
    distortion = minimum_jacobian / element_volume;
  }

  return fix_range(distortion);
}

VERDICT_HOST_DEVICE double tet_inradius(int num_nodes, const double coordinates[][3])
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

VERDICT_HOST_DEVICE double tet_timestep(int num_nodes, const double coordinates[][3], double density,
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
  double denominator = sqrt(M / density);

  return char_length / denominator;
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE VerdictVector tet10_auxillary_node_coordinate(const CoordsContainerType coordinates)
{
  VerdictVector aux_node(0.0, 0.0, 0.0);
  for (int i = 4; i < 10; i++)
  {
    VerdictVector tmp_vec(coordinates[i][0], coordinates[i][1], coordinates[i][2]);
    aux_node += tmp_vec;
  }
  aux_node /= 6;

  return aux_node;
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet10_min_inradius(const CoordsContainerType coordinates, int begin_index, int end_index)
{
  double min_tetinradius = VERDICT_DBL_MAX;

  VerdictVector auxillary_node = tet10_auxillary_node_coordinate(coordinates);

  for (int i = begin_index; i <= end_index; i++)
  {
    const int* subtet_conn = tet10_subtet_conn(i);
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

VERDICT_HOST_DEVICE double tet10_characteristic_length(const double coordinates[][3])
{
  // compute auxillary node coordinate
  double min_tetinradius = tet10_min_inradius(coordinates, 0, 11);
  min_tetinradius *= 2.3;

  return min_tetinradius;
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double calculate_tet4_outer_radius(const CoordsContainerType coordinates)
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
  double VP = fabs(((nE[1] - nE[0]) * (nE[2] - nE[0])) % (nE[3] - nE[0]) / 6);
  double outer_radius = sqrt((aC * AC + bC * BC + cC * CC) * (aC * AC + bC * BC - cC * CC) *
                          (aC * AC - bC * BC + cC * CC) * (-aC * AC + bC * BC + cC * CC)) /
    24 / VP;

  return outer_radius;
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet10_normalized_inradius(const CoordsContainerType coordinates)
{
  double min_inradius_for_subtet_with_parent_node = tet10_min_inradius(coordinates, 0, 3);
  double min_inradius_for_subtet_with_no_parent_node = tet10_min_inradius(coordinates, 4, 11);

  double outer_radius = calculate_tet4_outer_radius(coordinates);

  double normalized_inradius_for_subtet_with_parent_node =
    6.0 * min_inradius_for_subtet_with_parent_node / outer_radius;
  double normalized_inradius_for_subtet_with_no_parent_node =
    three_times_1plussqrt3 * min_inradius_for_subtet_with_no_parent_node / outer_radius;

  double norm_inrad = fmin(normalized_inradius_for_subtet_with_parent_node,
    normalized_inradius_for_subtet_with_no_parent_node);
  return fix_range(norm_inrad);
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet4_normalized_inradius(const CoordsContainerType coordinates)
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

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet_normalized_inradius_impl(int num_nodes, const CoordsContainerType coordinates)
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

VERDICT_HOST_DEVICE double tet_normalized_inradius(int num_nodes, const double coordinates[][3])
{
    return tet_normalized_inradius_impl(num_nodes, coordinates);
}

VERDICT_HOST_DEVICE double tet_normalized_inradius_from_loc_ptrs(int num_nodes, const double * const *coordinates)
{
    return tet_normalized_inradius_impl(num_nodes, coordinates);
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet4_mean_ratio(const CoordsContainerType coordinates)
{
  VerdictVector side0{coordinates[0], coordinates[1]};
  VerdictVector side2{coordinates[2], coordinates[0]};
  VerdictVector side3{coordinates[0], coordinates[3]};

  double char_size = elem_scaling(4, coordinates).scale;
  side0 /= char_size;
  side2 /= char_size;
  side3 /= char_size;

  const double tetVolume = calculate_tet_volume_using_sides(side0, side2, side3);
  if (fabs( tetVolume ) < VERDICT_DBL_MIN)
  {
    return 0.0;
  }

  VerdictVector side1{coordinates[1], coordinates[2]};
  VerdictVector side4{coordinates[1], coordinates[3]};
  VerdictVector side5{coordinates[2], coordinates[3]};

  side1 /= char_size;
  side4 /= char_size;
  side5 /= char_size;

  const double side0_length_squared = side0.length_squared();
  const double side1_length_squared = side1.length_squared();
  const double side2_length_squared = side2.length_squared();
  const double side3_length_squared = side3.length_squared();
  const double side4_length_squared = side4.length_squared();
  const double side5_length_squared = side5.length_squared();

  //const int sign = tetVolume < 0. ? -1 : 1;
  //return sign * 12. * std::pow(3.*fabs(tetVolume), 2./3.) / (side0_length_squared + side1_length_squared + side2_length_squared + side3_length_squared + side4_length_squared + side5_length_squared);
  double sum = (side0_length_squared + side1_length_squared + side2_length_squared +
    side3_length_squared + side4_length_squared + side5_length_squared) / 6;
  return 6 * sqrt2 * tetVolume / pow(sum, 3. / 2.);    
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet10_mean_ratio(const CoordsContainerType coordinates)
{
  double min_tet_mean_ratio = VERDICT_DBL_MAX;

  VerdictVector auxillary_node = tet10_auxillary_node_coordinate(coordinates);

  double aux_node_scale = 3.0 * sqrt3 * 0.25;

  for (int i = 0; i <= 11; i++)
  {
    const int* subtet_conn = tet10_subtet_conn(i);

    //get the coordinates of the nodes
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

    double tmp_mean_ratio = tet4_mean_ratio(subtet_coords);

    if (i > 3)
    {
      tmp_mean_ratio *= aux_node_scale;
    }

    if (tmp_mean_ratio < min_tet_mean_ratio)
    {
      min_tet_mean_ratio = tmp_mean_ratio;
    }
  }
  return min_tet_mean_ratio;
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE static double tet_mean_ratio_impl(int num_nodes, const CoordsContainerType coordinates)
{
  if (num_nodes == 4)
  {
    return tet4_mean_ratio(coordinates);
  }
  else if (num_nodes >= 10)
  {
    return tet10_mean_ratio(coordinates);
  }
  return 0.0;
}

VERDICT_HOST_DEVICE double tet_mean_ratio(int num_nodes, const double coordinates[][3])
{
   return tet_mean_ratio_impl(num_nodes, coordinates);
}

VERDICT_HOST_DEVICE double tet_mean_ratio_from_loc_ptrs(int num_nodes, const double * const * coordinates)
{
   return tet_mean_ratio_impl(num_nodes, coordinates);
}
} // namespace verdict
