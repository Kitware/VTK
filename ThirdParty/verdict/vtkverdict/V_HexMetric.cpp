/*=========================================================================

  Module:    V_HexMetric.cpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * HexMetric.cpp contains quality calculations for hexes
 *
 * This file is part of VERDICT
 *
 */

#include "V_HexMetric.hpp"
#include "V_GaussIntegration.hpp"
#include "VerdictVector.hpp"
#include "verdict.h"

#include <algorithm>
#include <cmath> // for std::isnan

namespace VERDICT_NAMESPACE
{
extern void quad_minimum_maximum_angle(double min_max_angles[2], const double coordinates[][3]);

static const double one_third = 1.0 / 3.0;
static const double two_thirds = 2.0 / 3.0;
static const double sqrt3 = std::sqrt(3.0);

//! weights based on the average size of a hex
static int hex_get_weight(
  VerdictVector& v1, VerdictVector& v2, VerdictVector& v3, double average_size)
{
  if (average_size == 0)
  {
    return 0;
  }

  v1.set(1, 0, 0);
  v2.set(0, 1, 0);
  v3.set(0, 0, 1);

  double scale = std::pow(average_size / (VerdictVector::Dot(v1, (v2 * v3))), 0.33333333333);
  v1 *= scale;
  v2 *= scale;
  v3 *= scale;

  return 1;
}

static const double HEX27_node_local_coord[27][3] = { { -1, -1, -1 }, { 1, -1, -1 }, { 1, 1, -1 },
  { -1, 1, -1 }, { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 }, { 0, -1, -1 },
  { 1, 0, -1 }, { 0, 1, -1 }, { -1, 0, -1 }, { -1, -1, 0 }, { 1, -1, 0 }, { 1, 1, 0 }, { -1, 1, 0 },
  { 0, -1, 1 }, { 1, 0, 1 }, { 0, 1, 1 }, { -1, 0, 1 }, { 0, 0, 0 }, { 0, 0, -1 }, { 0, 0, 1 },
  { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 } };

static int hex20_subtet_conn[36][4] = { { 0, 12, 8, 20 }, { 4, 16, 12, 20 }, { 16, 5, 13, 20 },
  { 1, 8, 13, 20 }, { 8, 12, 16, 20 }, { 8, 16, 13, 20 },

  { 1, 13, 9, 20 }, { 5, 17, 13, 20 }, { 6, 14, 17, 20 }, { 2, 9, 14, 20 }, { 9, 17, 14, 20 },
  { 9, 13, 17, 20 },

  { 7, 15, 18, 20 }, { 3, 10, 15, 20 }, { 2, 14, 10, 20 }, { 6, 18, 14, 20 }, { 10, 18, 15, 20 },
  { 10, 14, 18, 20 },

  { 7, 19, 15, 20 }, { 4, 12, 19, 20 }, { 0, 11, 12, 20 }, { 3, 15, 11, 20 }, { 11, 19, 12, 20 },
  { 11, 15, 19, 20 },

  { 4, 19, 16, 20 }, { 5, 16, 17, 20 }, { 6, 17, 18, 20 }, { 7, 18, 19, 20 }, { 16, 18, 17, 20 },
  { 16, 19, 18, 20 },

  { 0, 8, 11, 20 }, { 8, 1, 9, 20 }, { 2, 10, 9, 20 }, { 3, 11, 10, 20 }, { 8, 9, 10, 20 },
  { 8, 10, 11, 20 } };

static int hex27_subtet_conn[48][4] = { { 0, 12, 8, 20 }, { 4, 16, 12, 20 }, { 16, 5, 13, 20 },
  { 1, 8, 13, 20 }, { 25, 8, 12, 20 }, { 25, 12, 16, 20 }, { 25, 16, 13, 20 }, { 25, 13, 8, 20 },

  { 1, 13, 9, 20 }, { 5, 17, 13, 20 }, { 6, 14, 17, 20 }, { 2, 9, 14, 20 }, { 24, 9, 13, 20 },
  { 24, 13, 17, 20 }, { 24, 17, 14, 20 }, { 24, 14, 9, 20 },

  { 7, 15, 18, 20 }, { 3, 10, 15, 20 }, { 2, 14, 10, 20 }, { 6, 18, 14, 20 }, { 26, 10, 14, 20 },
  { 26, 14, 18, 20 }, { 26, 18, 15, 20 }, { 26, 15, 10, 20 },

  { 7, 19, 15, 20 }, { 4, 12, 19, 20 }, { 0, 11, 12, 20 }, { 3, 15, 11, 20 }, { 23, 11, 15, 20 },
  { 23, 15, 19, 20 }, { 23, 19, 12, 20 }, { 23, 12, 11, 20 },

  { 4, 19, 16, 20 }, { 5, 16, 17, 20 }, { 6, 17, 18, 20 }, { 7, 18, 19, 20 }, { 22, 16, 19, 20 },
  { 22, 19, 18, 20 }, { 22, 18, 17, 20 }, { 22, 17, 16, 20 },

  { 0, 8, 11, 20 }, { 8, 1, 9, 20 }, { 2, 10, 9, 20 }, { 3, 11, 10, 20 }, { 21, 8, 9, 20 },
  { 21, 9, 10, 20 }, { 21, 10, 11, 20 }, { 21, 11, 8, 20 } };

static double compute_tet_volume(VerdictVector& v1, VerdictVector& v2, VerdictVector& v3)
{
  return (double)((v3 % (v1 * v2)) / 6.0);
}

// Compute interior node
VerdictVector hex20_auxillary_node_coordinate(const double coordinates[][3])
{
  VerdictVector aux_node(0, 0, 0);
  for (int i = 0; i < 8; i++)
  {
    VerdictVector tmp_vec(coordinates[i][0], coordinates[i][1], coordinates[i][2]);
    aux_node += tmp_vec;
  }
  aux_node /= 6;

  return aux_node;
}

static void HEX27_gradients_of_the_shape_functions_for_RST(
  const double rst[3], double dhdr[27], double dhds[27], double dhdt[27])
{
  double g1r = -0.5 * rst[0] * (1 - rst[0]);
  double g1s = -0.5 * rst[1] * (1 - rst[1]);
  double g1t = -0.5 * rst[2] * (1 - rst[2]);

  double g2r = (1 + rst[0]) * (1 - rst[0]);
  double g2s = (1 + rst[1]) * (1 - rst[1]);
  double g2t = (1 + rst[2]) * (1 - rst[2]);

  double g3r = 0.5 * rst[0] * (1 + rst[0]);
  double g3s = 0.5 * rst[1] * (1 + rst[1]);
  double g3t = 0.5 * rst[2] * (1 + rst[2]);

  double g1r_r = rst[0] - 0.5;
  double g1s_s = rst[1] - 0.5;
  double g1t_t = rst[2] - 0.5;

  double g2r_r = -2 * rst[0];
  double g2s_s = -2 * rst[1];
  double g2t_t = -2 * rst[2];

  double g3r_r = rst[0] + 0.5;
  double g3s_s = rst[1] + 0.5;
  double g3t_t = rst[2] + 0.5;

  // dh/dr;
  dhdr[0] = g1r_r * g1s * g1t;
  dhdr[1] = g3r_r * g1s * g1t;
  dhdr[2] = g3r_r * g3s * g1t;
  dhdr[3] = g1r_r * g3s * g1t;
  dhdr[4] = g1r_r * g1s * g3t;
  dhdr[5] = g3r_r * g1s * g3t;
  dhdr[6] = g3r_r * g3s * g3t;
  dhdr[7] = g1r_r * g3s * g3t;
  dhdr[8] = g2r_r * g1s * g1t;
  dhdr[9] = g3r_r * g2s * g1t;
  dhdr[10] = g2r_r * g3s * g1t;
  dhdr[11] = g1r_r * g2s * g1t;
  dhdr[16] = g2r_r * g1s * g3t;
  dhdr[17] = g3r_r * g2s * g3t;
  dhdr[18] = g2r_r * g3s * g3t;
  dhdr[19] = g1r_r * g2s * g3t;
  dhdr[12] = g1r_r * g1s * g2t;
  dhdr[13] = g3r_r * g1s * g2t;
  dhdr[14] = g3r_r * g3s * g2t;
  dhdr[15] = g1r_r * g3s * g2t;
  dhdr[23] = g1r_r * g2s * g2t;
  dhdr[24] = g3r_r * g2s * g2t;
  dhdr[25] = g2r_r * g1s * g2t;
  dhdr[26] = g2r_r * g3s * g2t;
  dhdr[21] = g2r_r * g2s * g1t;
  dhdr[22] = g2r_r * g2s * g3t;
  dhdr[20] = g2r_r * g2s * g2t;

  // dh/ds;
  dhds[0] = g1r * g1s_s * g1t;
  dhds[1] = g3r * g1s_s * g1t;
  dhds[2] = g3r * g3s_s * g1t;
  dhds[3] = g1r * g3s_s * g1t;
  dhds[4] = g1r * g1s_s * g3t;
  dhds[5] = g3r * g1s_s * g3t;
  dhds[6] = g3r * g3s_s * g3t;
  dhds[7] = g1r * g3s_s * g3t;
  dhds[8] = g2r * g1s_s * g1t;
  dhds[9] = g3r * g2s_s * g1t;
  dhds[10] = g2r * g3s_s * g1t;
  dhds[11] = g1r * g2s_s * g1t;
  dhds[16] = g2r * g1s_s * g3t;
  dhds[17] = g3r * g2s_s * g3t;
  dhds[18] = g2r * g3s_s * g3t;
  dhds[19] = g1r * g2s_s * g3t;
  dhds[12] = g1r * g1s_s * g2t;
  dhds[13] = g3r * g1s_s * g2t;
  dhds[14] = g3r * g3s_s * g2t;
  dhds[15] = g1r * g3s_s * g2t;
  dhds[23] = g1r * g2s_s * g2t;
  dhds[24] = g3r * g2s_s * g2t;
  dhds[25] = g2r * g1s_s * g2t;
  dhds[26] = g2r * g3s_s * g2t;
  dhds[21] = g2r * g2s_s * g1t;
  dhds[22] = g2r * g2s_s * g3t;
  dhds[20] = g2r * g2s_s * g2t;

  // dh/dt;
  dhdt[0] = g1r * g1s * g1t_t;
  dhdt[1] = g3r * g1s * g1t_t;
  dhdt[2] = g3r * g3s * g1t_t;
  dhdt[3] = g1r * g3s * g1t_t;
  dhdt[4] = g1r * g1s * g3t_t;
  dhdt[5] = g3r * g1s * g3t_t;
  dhdt[6] = g3r * g3s * g3t_t;
  dhdt[7] = g1r * g3s * g3t_t;
  dhdt[8] = g2r * g1s * g1t_t;
  dhdt[9] = g3r * g2s * g1t_t;
  dhdt[10] = g2r * g3s * g1t_t;
  dhdt[11] = g1r * g2s * g1t_t;
  dhdt[16] = g2r * g1s * g3t_t;
  dhdt[17] = g3r * g2s * g3t_t;
  dhdt[18] = g2r * g3s * g3t_t;
  dhdt[19] = g1r * g2s * g3t_t;
  dhdt[12] = g1r * g1s * g2t_t;
  dhdt[13] = g3r * g1s * g2t_t;
  dhdt[14] = g3r * g3s * g2t_t;
  dhdt[15] = g1r * g3s * g2t_t;
  dhdt[23] = g1r * g2s * g2t_t;
  dhdt[24] = g3r * g2s * g2t_t;
  dhdt[25] = g2r * g1s * g2t_t;
  dhdt[26] = g2r * g3s * g2t_t;
  dhdt[21] = g2r * g2s * g1t_t;
  dhdt[22] = g2r * g2s * g3t_t;
  dhdt[20] = g2r * g2s * g2t_t;

  for (int i = 0; i < 27; i++)
  {
    dhdr[i] *= 2;
    dhds[i] *= 2;
    dhdt[i] *= 2;
  }
}

#define make_hex_nodes(coord, pos)                                                                 \
  for (int mhcii = 0; mhcii < 8; mhcii++)                                                          \
  {                                                                                                \
    pos[mhcii].set(coord[mhcii][0], coord[mhcii][1], coord[mhcii][2]);                             \
  }

#define make_edge_length_squares(edges, lengths)                                                   \
  {                                                                                                \
    for (int melii = 0; melii < 12; melii++)                                                       \
      lengths[melii] = edges[melii].length_squared();                                              \
  }

//! make VerdictVectors from coordinates
static void make_hex_edges(const double coordinates[][3], VerdictVector edges[12])
{
  edges[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  edges[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  edges[2].set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);
  edges[3].set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);
  edges[4].set(coordinates[5][0] - coordinates[4][0], coordinates[5][1] - coordinates[4][1],
    coordinates[5][2] - coordinates[4][2]);
  edges[5].set(coordinates[6][0] - coordinates[5][0], coordinates[6][1] - coordinates[5][1],
    coordinates[6][2] - coordinates[5][2]);
  edges[6].set(coordinates[7][0] - coordinates[6][0], coordinates[7][1] - coordinates[6][1],
    coordinates[7][2] - coordinates[6][2]);
  edges[7].set(coordinates[4][0] - coordinates[7][0], coordinates[4][1] - coordinates[7][1],
    coordinates[4][2] - coordinates[7][2]);
  edges[8].set(coordinates[4][0] - coordinates[0][0], coordinates[4][1] - coordinates[0][1],
    coordinates[4][2] - coordinates[0][2]);
  edges[9].set(coordinates[5][0] - coordinates[1][0], coordinates[5][1] - coordinates[1][1],
    coordinates[5][2] - coordinates[1][2]);
  edges[10].set(coordinates[6][0] - coordinates[2][0], coordinates[6][1] - coordinates[2][1],
    coordinates[6][2] - coordinates[2][2]);
  edges[11].set(coordinates[7][0] - coordinates[3][0], coordinates[7][1] - coordinates[3][1],
    coordinates[7][2] - coordinates[3][2]);
}

#if 0  /* Not currently used and not exposed in verdict.h */
/*!
  localizes hex coordinates
 */
static void localize_hex_coordinates(const double coordinates[][3], VerdictVector position[8] )
{

  int ii;
  for ( ii = 0; ii < 8; ii++ )
  {
    position[ii].set( coordinates[ii][0],
        coordinates[ii][1],
        coordinates[ii][2] );
  }
  
  // ... Make centroid of element the center of coordinate system
  VerdictVector point_1256 = position[1];
  point_1256 += position[2];
  point_1256 += position[5];
  point_1256 += position[6];

  VerdictVector point_0374 = position[0];
  point_0374 += position[3];
  point_0374 += position[7];
  point_0374 += position[4];

  VerdictVector centroid = point_1256;
  centroid += point_0374;
  centroid /= 8.0;

  int i;
  for ( i = 0; i < 8; i++)
  {
  position[i] -= centroid;
  }

  // ... Rotate element such that center of side 1-2 and 0-3 define X axis
  double DX = point_1256.x() - point_0374.x();
  double DY = point_1256.y() - point_0374.y();
  double DZ = point_1256.z() - point_0374.z();

  double AMAGX = std::sqrt(DX*DX + DZ*DZ);
  double AMAGY = std::sqrt(DX*DX + DY*DY + DZ*DZ);
  double FMAGX = AMAGX == 0.0 ? 1.0 : 0.0;
  double FMAGY = AMAGY == 0.0 ? 1.0 : 0.0;

  double CZ = DX / (AMAGX + FMAGX) + FMAGX;
  double SZ = DZ / (AMAGX + FMAGX);
  double CY = AMAGX / (AMAGY + FMAGY) + FMAGY;
  double SY = DY / (AMAGY + FMAGY);

  double temp;
 
  for (i = 0; i < 8; i++) 
  {
    temp =  CY * CZ * position[i].x() + CY * SZ * position[i].z() +
      SY * position[i].y();
    position[i].y( -SY * CZ * position[i].x() - SY * SZ * position[i].z() +
        CY * position[i].y());
    position[i].z( -SZ * position[i].x() + CZ * position[i].z());
    position[i].x(temp);
  }

  // ... Now, rotate about Y
  VerdictVector delta = -position[0];
  delta -= position[1];
  delta += position[2];
  delta += position[3];
  delta -= position[4];
  delta -= position[5];
  delta += position[6];
  delta += position[7];

  DY = delta.y();
  DZ = delta.z();

  AMAGY = std::sqrt(DY*DY + DZ*DZ);
  FMAGY = AMAGY == 0.0 ? 1.0 : 0.0;

  double CX = DY / (AMAGY + FMAGY) + FMAGY;
  double SX = DZ / (AMAGY + FMAGY);
  
  for (i = 0; i < 8; i++) 
  {
    temp =  CX * position[i].y() + SX * position[i].z();
    position[i].z(-SX * position[i].y() + CX * position[i].z());
    position[i].y(temp);
  }
}

static double safe_ratio3( const double numerator, 
    const double denominator,
    const double max_ratio )
{
  // this filter is essential for good running time in practice
  double return_value;

  const double filter_n = max_ratio * 1.0e-16;
  const double filter_d = 1.0e-16;
  if ( std::abs( numerator ) <= filter_n && std::abs( denominator ) >= filter_d )
  {
    return_value = numerator / denominator;
  }
  else
  {
    return_value = std::abs(numerator) / max_ratio >= std::abs(denominator) ?
      ( (numerator >= 0.0 && denominator >= 0.0) ||
        (numerator < 0.0 && denominator < 0.0) ?
        max_ratio : -max_ratio )
      : numerator / denominator;
  }
  return return_value;
}
#endif /* Not currently used and not exposed in verdict.h */

static double safe_ratio(const double numerator, const double denominator)
{

  double return_value;
  const double filter_n = VERDICT_DBL_MAX;
  const double filter_d = VERDICT_DBL_MIN;
  if (std::abs(numerator) <= filter_n && std::abs(denominator) >= filter_d)
  {
    return_value = numerator / denominator;
  }
  else
  {
    return_value = VERDICT_DBL_MAX;
  }

  return return_value;
}

static double condition_comp(
  const VerdictVector& xxi, const VerdictVector& xet, const VerdictVector& xze)
{
  double det = VerdictVector::Dot(xxi, (xet * xze));

  if (det <= VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }

  double term1 =
    VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze);
  double term2 = VerdictVector::Dot((xxi * xet), (xxi * xet)) +
    VerdictVector::Dot((xet * xze), (xet * xze)) + VerdictVector::Dot((xze * xxi), (xze * xxi));

  return std::sqrt(term1 * term2) / det;
}

static double oddy_comp(
  const VerdictVector& xxi, const VerdictVector& xet, const VerdictVector& xze)
{
  double g11, g12, g13, g22, g23, g33, rt_g;

  g11 = VerdictVector::Dot(xxi, xxi);
  g12 = VerdictVector::Dot(xxi, xet);
  g13 = VerdictVector::Dot(xxi, xze);
  g22 = VerdictVector::Dot(xet, xet);
  g23 = VerdictVector::Dot(xet, xze);
  g33 = VerdictVector::Dot(xze, xze);
  rt_g = VerdictVector::Dot(xxi, (xet * xze));

  double oddy_metric;
  if (rt_g > VERDICT_DBL_MIN)
  {
    double norm_G_squared =
      g11 * g11 + 2.0 * g12 * g12 + 2.0 * g13 * g13 + g22 * g22 + 2.0 * g23 * g23 + g33 * g33;

    double norm_J_squared = g11 + g22 + g33;

    oddy_metric = (norm_G_squared - one_third * norm_J_squared * norm_J_squared) /
      std::pow(rt_g, 4. * one_third);
  }
  else
  {
    oddy_metric = VERDICT_DBL_MAX;
  }
  return oddy_metric;
}

//! calculates edge lengths of a hex
static double hex_edge_length(int max_min, const double coordinates[][3])
{
  double temp[3], edge[12];
  int i;

  // lengths^2 of edges
  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[1][i] - coordinates[0][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[0] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[2][i] - coordinates[1][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[1] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[3][i] - coordinates[2][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[2] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[0][i] - coordinates[3][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[3] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[5][i] - coordinates[4][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[4] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[6][i] - coordinates[5][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[5] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[7][i] - coordinates[6][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[6] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[4][i] - coordinates[7][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[7] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[4][i] - coordinates[0][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[8] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[5][i] - coordinates[1][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[9] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[6][i] - coordinates[2][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[10] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[7][i] - coordinates[3][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[11] = std::sqrt(temp[0] + temp[1] + temp[2]);

  double _edge = edge[0];

  if (max_min == 0)
  {
    for (i = 1; i < 12; i++)
    {
      _edge = std::min(_edge, edge[i]);
    }
    return _edge;
  }
  else
  {
    for (i = 1; i < 12; i++)
    {
      _edge = std::max(_edge, edge[i]);
    }
    return _edge;
  }
}

static double diag_length(int max_min, const double coordinates[][3])
{
  double temp[3], diag[4];
  int i;

  // lengths^2  f diag nals
  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[6][i] - coordinates[0][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[0] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[4][i] - coordinates[2][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[1] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[7][i] - coordinates[1][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[2] = std::sqrt(temp[0] + temp[1] + temp[2]);

  for (i = 0; i < 3; i++)
  {
    temp[i] = coordinates[5][i] - coordinates[3][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[3] = std::sqrt(temp[0] + temp[1] + temp[2]);

  double diagonal = diag[0];
  if (max_min == 0) // Return min diagonal
  {
    for (i = 1; i < 4; i++)
    {
      diagonal = std::min(diagonal, diag[i]);
    }
    return diagonal;
  }
  else // Return max diagonal
  {
    for (i = 1; i < 4; i++)
    {
      diagonal = std::max(diagonal, diag[i]);
    }
    return diagonal;
  }
}

//! calculates efg values
static VerdictVector calc_hex_efg(int efg_index, VerdictVector coordinates[8])
{

  VerdictVector efg;

  switch (efg_index)
  {
    case 1:
      efg = coordinates[1];
      efg += coordinates[2];
      efg += coordinates[5];
      efg += coordinates[6];
      efg -= coordinates[0];
      efg -= coordinates[3];
      efg -= coordinates[4];
      efg -= coordinates[7];
      break;

    case 2:
      efg = coordinates[2];
      efg += coordinates[3];
      efg += coordinates[6];
      efg += coordinates[7];
      efg -= coordinates[0];
      efg -= coordinates[1];
      efg -= coordinates[4];
      efg -= coordinates[5];
      break;

    case 3:
      efg = coordinates[4];
      efg += coordinates[5];
      efg += coordinates[6];
      efg += coordinates[7];
      efg -= coordinates[0];
      efg -= coordinates[1];
      efg -= coordinates[2];
      efg -= coordinates[3];
      break;

    case 12:
      efg = coordinates[0];
      efg += coordinates[2];
      efg += coordinates[4];
      efg += coordinates[6];
      efg -= coordinates[1];
      efg -= coordinates[3];
      efg -= coordinates[5];
      efg -= coordinates[7];
      break;

    case 13:
      efg = coordinates[0];
      efg += coordinates[3];
      efg += coordinates[5];
      efg += coordinates[6];
      efg -= coordinates[1];
      efg -= coordinates[2];
      efg -= coordinates[4];
      efg -= coordinates[7];
      break;

    case 23:
      efg = coordinates[0];
      efg += coordinates[1];
      efg += coordinates[6];
      efg += coordinates[7];
      efg -= coordinates[2];
      efg -= coordinates[3];
      efg -= coordinates[4];
      efg -= coordinates[5];
      break;

    case 123:
      efg = coordinates[0];
      efg += coordinates[2];
      efg += coordinates[5];
      efg += coordinates[7];
      efg -= coordinates[1];
      efg -= coordinates[5];
      efg -= coordinates[6];
      efg -= coordinates[2];
      break;

    default:
      efg.set(0, 0, 0);
  }
  return efg;
}

/*!
   the edge ratio of a hex

   NB (P. Pebay 01/23/07):
     Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
     minimum edge lengths
 */
double hex_edge_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector edges[12];
  make_hex_edges(coordinates, edges);

  double a2 = edges[0].length_squared();
  double b2 = edges[1].length_squared();
  double c2 = edges[2].length_squared();
  double d2 = edges[3].length_squared();
  double e2 = edges[4].length_squared();
  double f2 = edges[5].length_squared();
  double g2 = edges[6].length_squared();
  double h2 = edges[7].length_squared();
  double i2 = edges[8].length_squared();
  double j2 = edges[9].length_squared();
  double k2 = edges[10].length_squared();
  double l2 = edges[11].length_squared();

  double mab, mcd, mef, Mab, Mcd, Mef;
  double mgh, mij, mkl, Mgh, Mij, Mkl;

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
  if (g2 < h2)
  {
    mgh = g2;
    Mgh = h2;
  }
  else // h2 <= g2
  {
    mgh = h2;
    Mgh = g2;
  }
  if (i2 < j2)
  {
    mij = i2;
    Mij = j2;
  }
  else // j2 <= i2
  {
    mij = j2;
    Mij = i2;
  }
  if (k2 < l2)
  {
    mkl = k2;
    Mkl = l2;
  }
  else // l2 <= k2
  {
    mkl = l2;
    Mkl = k2;
  }

  double m2;
  m2 = mab < mcd ? mab : mcd;
  m2 = m2 < mef ? m2 : mef;
  m2 = m2 < mgh ? m2 : mgh;
  m2 = m2 < mij ? m2 : mij;
  m2 = m2 < mkl ? m2 : mkl;

  if (m2 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double M2;
  M2 = Mab > Mcd ? Mab : Mcd;
  M2 = M2 > Mef ? M2 : Mef;
  M2 = M2 > Mgh ? M2 : Mgh;
  M2 = M2 > Mij ? M2 : Mij;
  M2 = M2 > Mkl ? M2 : Mkl;
  m2 = m2 < mef ? m2 : mef;

  M2 = Mab > Mcd ? Mab : Mcd;
  M2 = M2 > Mef ? M2 : Mef;

  double edge_ratio = std::sqrt(M2 / m2);

  if (edge_ratio > 0)
  {
    return (double)std::min(edge_ratio, VERDICT_DBL_MAX);
  }
  return (double)std::max(edge_ratio, -VERDICT_DBL_MAX);
}

/*!
  max edge ratio of a hex

  Maximum edge length ratio at hex center
 */
double hex_max_edge_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  double aspect;
  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  double aspect_12, aspect_13, aspect_23;

  VerdictVector efg1 = calc_hex_efg(1, node_pos);
  VerdictVector efg2 = calc_hex_efg(2, node_pos);
  VerdictVector efg3 = calc_hex_efg(3, node_pos);

  double mag_efg1 = efg1.length();
  double mag_efg2 = efg2.length();
  double mag_efg3 = efg3.length();

  aspect_12 = safe_ratio(std::max(mag_efg1, mag_efg2), std::min(mag_efg1, mag_efg2));
  aspect_13 = safe_ratio(std::max(mag_efg1, mag_efg3), std::min(mag_efg1, mag_efg3));
  aspect_23 = safe_ratio(std::max(mag_efg2, mag_efg3), std::min(mag_efg2, mag_efg3));

  aspect = std::max({ aspect_12, aspect_13, aspect_23 });

  if (aspect > 0)
  {
    return (double)std::min(aspect, VERDICT_DBL_MAX);
  }
  return (double)std::max(aspect, -VERDICT_DBL_MAX);
}

double hex_equiangle_skew(int /*num_nodes*/, const double coordinates[][3])
{
  double quad[4][3];
  double min_angle = 360.0;
  double max_angle = 0.0;
  double min_max_angle[2];

  quad[0][0] = coordinates[0][0];
  quad[0][1] = coordinates[0][1];
  quad[0][2] = coordinates[0][2];
  quad[1][0] = coordinates[1][0];
  quad[1][1] = coordinates[1][1];
  quad[1][2] = coordinates[1][2];
  quad[2][0] = coordinates[5][0];
  quad[2][1] = coordinates[5][1];
  quad[2][2] = coordinates[5][2];
  quad[3][0] = coordinates[4][0];
  quad[3][1] = coordinates[4][1];
  quad[3][2] = coordinates[4][2];

  quad_minimum_maximum_angle(min_max_angle, quad);
  if (min_max_angle[0] < min_angle)
  {
    min_angle = min_max_angle[0];
  }
  if (min_max_angle[1] > max_angle)
  {
    max_angle = min_max_angle[1];
  }

  quad[0][0] = coordinates[1][0];
  quad[0][1] = coordinates[1][1];
  quad[0][2] = coordinates[1][2];
  quad[1][0] = coordinates[2][0];
  quad[1][1] = coordinates[2][1];
  quad[1][2] = coordinates[2][2];
  quad[2][0] = coordinates[6][0];
  quad[2][1] = coordinates[6][1];
  quad[2][2] = coordinates[6][2];
  quad[3][0] = coordinates[5][0];
  quad[3][1] = coordinates[5][1];
  quad[3][2] = coordinates[5][2];

  quad_minimum_maximum_angle(min_max_angle, quad);
  if (min_max_angle[0] < min_angle)
  {
    min_angle = min_max_angle[0];
  }
  if (min_max_angle[1] > max_angle)
  {
    max_angle = min_max_angle[1];
  }

  quad[0][0] = coordinates[2][0];
  quad[0][1] = coordinates[2][1];
  quad[0][2] = coordinates[2][2];
  quad[1][0] = coordinates[3][0];
  quad[1][1] = coordinates[3][1];
  quad[1][2] = coordinates[3][2];
  quad[2][0] = coordinates[7][0];
  quad[2][1] = coordinates[7][1];
  quad[2][2] = coordinates[7][2];
  quad[3][0] = coordinates[6][0];
  quad[3][1] = coordinates[6][1];
  quad[3][2] = coordinates[6][2];

  quad_minimum_maximum_angle(min_max_angle, quad);
  if (min_max_angle[0] < min_angle)
  {
    min_angle = min_max_angle[0];
  }
  if (min_max_angle[1] > max_angle)
  {
    max_angle = min_max_angle[1];
  }

  quad[0][0] = coordinates[3][0];
  quad[0][1] = coordinates[3][1];
  quad[0][2] = coordinates[3][2];
  quad[1][0] = coordinates[0][0];
  quad[1][1] = coordinates[0][1];
  quad[1][2] = coordinates[0][2];
  quad[2][0] = coordinates[4][0];
  quad[2][1] = coordinates[4][1];
  quad[2][2] = coordinates[4][2];
  quad[3][0] = coordinates[7][0];
  quad[3][1] = coordinates[7][1];
  quad[3][2] = coordinates[7][2];

  quad_minimum_maximum_angle(min_max_angle, quad);
  if (min_max_angle[0] < min_angle)
  {
    min_angle = min_max_angle[0];
  }
  if (min_max_angle[1] > max_angle)
  {
    max_angle = min_max_angle[1];
  }

  quad[0][0] = coordinates[4][0];
  quad[0][1] = coordinates[4][1];
  quad[0][2] = coordinates[4][2];
  quad[1][0] = coordinates[5][0];
  quad[1][1] = coordinates[5][1];
  quad[1][2] = coordinates[5][2];
  quad[2][0] = coordinates[6][0];
  quad[2][1] = coordinates[6][1];
  quad[2][2] = coordinates[6][2];
  quad[3][0] = coordinates[7][0];
  quad[3][1] = coordinates[7][1];
  quad[3][2] = coordinates[7][2];

  quad_minimum_maximum_angle(min_max_angle, quad);
  if (min_max_angle[0] < min_angle)
  {
    min_angle = min_max_angle[0];
  }
  if (min_max_angle[1] > max_angle)
  {
    max_angle = min_max_angle[1];
  }

  quad[0][0] = coordinates[3][0];
  quad[0][1] = coordinates[3][1];
  quad[0][2] = coordinates[3][2];
  quad[1][0] = coordinates[2][0];
  quad[1][1] = coordinates[2][1];
  quad[1][2] = coordinates[2][2];
  quad[2][0] = coordinates[1][0];
  quad[2][1] = coordinates[1][1];
  quad[2][2] = coordinates[1][2];
  quad[3][0] = coordinates[0][0];
  quad[3][1] = coordinates[0][1];
  quad[3][2] = coordinates[0][2];

  quad_minimum_maximum_angle(min_max_angle, quad);
  if (min_max_angle[0] < min_angle)
  {
    min_angle = min_max_angle[0];
  }
  if (min_max_angle[1] > max_angle)
  {
    max_angle = min_max_angle[1];
  }

  double skew_max = (max_angle - 90.0) / 90.0;
  double skew_min = (90.0 - min_angle) / 90.0;

  if (skew_max > skew_min)
  {
    return skew_max;
  }
  return skew_min;
}

/*!
  skew of a hex

  Maximum ||cosA|| where A is the angle between edges at hex center.
 */
double hex_skew(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  double skew_1, skew_2, skew_3;

  VerdictVector efg1 = calc_hex_efg(1, node_pos);
  VerdictVector efg2 = calc_hex_efg(2, node_pos);
  VerdictVector efg3 = calc_hex_efg(3, node_pos);

  if (efg1.normalize() <= VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }
  if (efg2.normalize() <= VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }
  if (efg3.normalize() <= VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }

  skew_1 = std::abs(VerdictVector::Dot(efg1, efg2));
  skew_2 = std::abs(VerdictVector::Dot(efg1, efg3));
  skew_3 = std::abs(VerdictVector::Dot(efg2, efg3));

  double skew = std::max({ skew_1, skew_2, skew_3 });

  if (skew > 0)
  {
    return (double)std::min(skew, VERDICT_DBL_MAX);
  }
  return (double)std::max(skew, -VERDICT_DBL_MAX);
}

/*!
  taper of a hex

  Maximum ratio of lengths derived from opposite edges.
 */
double hex_taper(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  VerdictVector efg1 = calc_hex_efg(1, node_pos);
  VerdictVector efg2 = calc_hex_efg(2, node_pos);
  VerdictVector efg3 = calc_hex_efg(3, node_pos);

  VerdictVector efg12 = calc_hex_efg(12, node_pos);
  VerdictVector efg13 = calc_hex_efg(13, node_pos);
  VerdictVector efg23 = calc_hex_efg(23, node_pos);

  double taper_1 = std::abs(safe_ratio(efg12.length(), std::min(efg1.length(), efg2.length())));
  double taper_2 = std::abs(safe_ratio(efg13.length(), std::min(efg1.length(), efg3.length())));
  double taper_3 = std::abs(safe_ratio(efg23.length(), std::min(efg2.length(), efg3.length())));

  double taper = std::max({ taper_1, taper_2, taper_3 });

  if (taper > 0)
  {
    return (double)std::min(taper, VERDICT_DBL_MAX);
  }
  return (double)std::max(taper, -VERDICT_DBL_MAX);
}

/*!
  volume of a hex
  Split the hex into 24 tets.
  sum the volume of each tet.
 */
double hex_volume(int num_nodes, const double coordinates[][3])
{
  double volume = 0.0;

  if (num_nodes > 9)
  {
    int(*subtet_conn_array)[4];
    int num_subtets = 0;
    if (27 == num_nodes)
    {
      num_subtets = 48;
      subtet_conn_array = hex27_subtet_conn;
    }
    else if (20 == num_nodes)
    {
      num_subtets = 36;
      subtet_conn_array = hex20_subtet_conn;
    }
    else
    {
      return 0.0;
    }

    VerdictVector aux_node = hex20_auxillary_node_coordinate(coordinates);

    for (int k = 0; k < num_subtets; k++)
    {
      VerdictVector v1(
        coordinates[subtet_conn_array[k][1]][0] - coordinates[subtet_conn_array[k][0]][0],
        coordinates[subtet_conn_array[k][1]][1] - coordinates[subtet_conn_array[k][0]][1],
        coordinates[subtet_conn_array[k][1]][2] - coordinates[subtet_conn_array[k][0]][2]);

      VerdictVector v2(
        coordinates[subtet_conn_array[k][2]][0] - coordinates[subtet_conn_array[k][0]][0],
        coordinates[subtet_conn_array[k][2]][1] - coordinates[subtet_conn_array[k][0]][1],
        coordinates[subtet_conn_array[k][2]][2] - coordinates[subtet_conn_array[k][0]][2]);

      VerdictVector v3(aux_node.x() - coordinates[subtet_conn_array[k][0]][0],
        aux_node.y() - coordinates[subtet_conn_array[k][0]][1],
        aux_node.z() - coordinates[subtet_conn_array[k][0]][2]);

      volume += compute_tet_volume(v1, v2, v3);
    }
  }
  else
  {

    VerdictVector node_pos[8];
    make_hex_nodes(coordinates, node_pos);

    // define the nodes of each face of the hex
    int faces[6][4] = {
      { 0, 1, 5, 4 },
      { 1, 2, 6, 5 },
      { 2, 3, 7, 6 },
      { 3, 0, 4, 7 },
      { 3, 2, 1, 0 },
      { 4, 5, 6, 7 },
    };

    // calculate the center of each face
    VerdictVector fcenter[6];
    for (int f = 0; f < 6; f++)
    {
      fcenter[f] = (node_pos[faces[f][0]] + node_pos[faces[f][1]] + node_pos[faces[f][2]] +
                     node_pos[faces[f][3]]) *
        0.25;
    }

    // calculate the center of the hex
    VerdictVector hcenter = (node_pos[0] + node_pos[1] + node_pos[2] + node_pos[3] + node_pos[4] +
                              node_pos[5] + node_pos[6] + node_pos[7]) *
      0.125;

    for (int i = 0; i < 6; i++)
    {
      // for each face calculate the vectors from the nodes and center of the face to the center of
      // the hex. These vectors define three of the sides of the tets.
      VerdictVector side[5];
      side[4] = hcenter - fcenter[i]; // vector from center of face to center of hex.
      for (int s = 0; s < 4; s++)
      {
        side[s] = hcenter - node_pos[faces[i][s]]; // vector from face node to center of hex.
      }

      // for each of the four tets that originate from this face.
      // calculate the volume of the tet.
      // This is done by calculating the triple product of three vectors that originate from a
      // corner node of the tet. This is also the jacobain at the corner node of the tet. The volume
      // is 1/6 of jacobian at a corner node.
      for (int j = 0; j < 3; j++) // first three tets
      {
        volume += (double)(VerdictVector::Dot(side[4], (side[j + 1] * side[j])) / 6.0);
      }
      volume += (double)(VerdictVector::Dot(side[4], (side[0] * side[3])) / 6.0); // fourth tet.
    }
  }

  if (volume > 0)
  {
    return (double)std::min(volume, VERDICT_DBL_MAX);
  }
  return (double)std::max(volume, -VERDICT_DBL_MAX);
}

/*!
  stretch of a hex

  sqrt(3) * minimum edge length / maximum diagonal length
 */
double hex_stretch(int /*num_nodes*/, const double coordinates[][3])
{
  double min_edge = hex_edge_length(0, coordinates);
  double max_diag = diag_length(1, coordinates);

  double stretch = sqrt3 * safe_ratio(min_edge, max_diag);

  if (stretch > 0)
  {
    return (double)std::min(stretch, VERDICT_DBL_MAX);
  }
  return (double)std::max(stretch, -VERDICT_DBL_MAX);
}

/*!
  diagonal ratio of a hex

  Minimum diagonal length / maximum diagonal length
 */
double hex_diagonal(int /*num_nodes*/, const double coordinates[][3])
{
  double min_diag = diag_length(0, coordinates);
  double max_diag = diag_length(1, coordinates);

  double diagonal = safe_ratio(min_diag, max_diag);

  if (diagonal > 0)
  {
    return (double)std::min(diagonal, VERDICT_DBL_MAX);
  }
  return (double)std::max(diagonal, -VERDICT_DBL_MAX);
}

#define SQR(x) ((x) * (x))

/*!
  dimension of a hex

  Pronto-specific characteristic length for stable time step calculation.
  Char_length = Volume / 2 grad Volume
*/
double hex_dimension(int /*num_nodes*/, const double coordinates[][3])
{
  double gradop[9][4];

  double x1 = coordinates[0][0];
  double x2 = coordinates[1][0];
  double x3 = coordinates[2][0];
  double x4 = coordinates[3][0];
  double x5 = coordinates[4][0];
  double x6 = coordinates[5][0];
  double x7 = coordinates[6][0];
  double x8 = coordinates[7][0];

  double y1 = coordinates[0][1];
  double y2 = coordinates[1][1];
  double y3 = coordinates[2][1];
  double y4 = coordinates[3][1];
  double y5 = coordinates[4][1];
  double y6 = coordinates[5][1];
  double y7 = coordinates[6][1];
  double y8 = coordinates[7][1];

  double z1 = coordinates[0][2];
  double z2 = coordinates[1][2];
  double z3 = coordinates[2][2];
  double z4 = coordinates[3][2];
  double z5 = coordinates[4][2];
  double z6 = coordinates[5][2];
  double z7 = coordinates[6][2];
  double z8 = coordinates[7][2];

  double z24 = z2 - z4;
  double z52 = z5 - z2;
  double z45 = z4 - z5;
  gradop[1][1] = (y2 * (z6 - z3 - z45) + y3 * z24 + y4 * (z3 - z8 - z52) + y5 * (z8 - z6 - z24) +
                   y6 * z52 + y8 * z45) /
    12.0;

  double z31 = z3 - z1;
  double z63 = z6 - z3;
  double z16 = z1 - z6;
  gradop[2][1] = (y3 * (z7 - z4 - z16) + y4 * z31 + y1 * (z4 - z5 - z63) + y6 * (z5 - z7 - z31) +
                   y7 * z63 + y5 * z16) /
    12.0;

  double z42 = z4 - z2;
  double z74 = z7 - z4;
  double z27 = z2 - z7;
  gradop[3][1] = (y4 * (z8 - z1 - z27) + y1 * z42 + y2 * (z1 - z6 - z74) + y7 * (z6 - z8 - z42) +
                   y8 * z74 + y6 * z27) /
    12.0;

  double z13 = z1 - z3;
  double z81 = z8 - z1;
  double z38 = z3 - z8;
  gradop[4][1] = (y1 * (z5 - z2 - z38) + y2 * z13 + y3 * (z2 - z7 - z81) + y8 * (z7 - z5 - z13) +
                   y5 * z81 + y7 * z38) /
    12.0;

  double z86 = z8 - z6;
  double z18 = z1 - z8;
  double z61 = z6 - z1;
  gradop[5][1] = (y8 * (z4 - z7 - z61) + y7 * z86 + y6 * (z7 - z2 - z18) + y1 * (z2 - z4 - z86) +
                   y4 * z18 + y2 * z61) /
    12.0;

  double z57 = z5 - z7;
  double z25 = z2 - z5;
  double z72 = z7 - z2;
  gradop[6][1] = (y5 * (z1 - z8 - z72) + y8 * z57 + y7 * (z8 - z3 - z25) + y2 * (z3 - z1 - z57) +
                   y1 * z25 + y3 * z72) /
    12.0;

  double z68 = z6 - z8;
  double z36 = z3 - z6;
  double z83 = z8 - z3;
  gradop[7][1] = (y6 * (z2 - z5 - z83) + y5 * z68 + y8 * (z5 - z4 - z36) + y3 * (z4 - z2 - z68) +
                   y2 * z36 + y4 * z83) /
    12.0;

  double z75 = z7 - z5;
  double z47 = z4 - z7;
  double z54 = z5 - z4;
  gradop[8][1] = (y7 * (z3 - z6 - z54) + y6 * z75 + y5 * (z6 - z1 - z47) + y4 * (z1 - z3 - z75) +
                   y3 * z47 + y1 * z54) /
    12.0;

  double x24 = x2 - x4;
  double x52 = x5 - x2;
  double x45 = x4 - x5;
  gradop[1][2] = (z2 * (x6 - x3 - x45) + z3 * x24 + z4 * (x3 - x8 - x52) + z5 * (x8 - x6 - x24) +
                   z6 * x52 + z8 * x45) /
    12.0;

  double x31 = x3 - x1;
  double x63 = x6 - x3;
  double x16 = x1 - x6;
  gradop[2][2] = (z3 * (x7 - x4 - x16) + z4 * x31 + z1 * (x4 - x5 - x63) + z6 * (x5 - x7 - x31) +
                   z7 * x63 + z5 * x16) /
    12.0;

  double x42 = x4 - x2;
  double x74 = x7 - x4;
  double x27 = x2 - x7;
  gradop[3][2] = (z4 * (x8 - x1 - x27) + z1 * x42 + z2 * (x1 - x6 - x74) + z7 * (x6 - x8 - x42) +
                   z8 * x74 + z6 * x27) /
    12.0;

  double x13 = x1 - x3;
  double x81 = x8 - x1;
  double x38 = x3 - x8;
  gradop[4][2] = (z1 * (x5 - x2 - x38) + z2 * x13 + z3 * (x2 - x7 - x81) + z8 * (x7 - x5 - x13) +
                   z5 * x81 + z7 * x38) /
    12.0;

  double x86 = x8 - x6;
  double x18 = x1 - x8;
  double x61 = x6 - x1;
  gradop[5][2] = (z8 * (x4 - x7 - x61) + z7 * x86 + z6 * (x7 - x2 - x18) + z1 * (x2 - x4 - x86) +
                   z4 * x18 + z2 * x61) /
    12.0;

  double x57 = x5 - x7;
  double x25 = x2 - x5;
  double x72 = x7 - x2;
  gradop[6][2] = (z5 * (x1 - x8 - x72) + z8 * x57 + z7 * (x8 - x3 - x25) + z2 * (x3 - x1 - x57) +
                   z1 * x25 + z3 * x72) /
    12.0;

  double x68 = x6 - x8;
  double x36 = x3 - x6;
  double x83 = x8 - x3;
  gradop[7][2] = (z6 * (x2 - x5 - x83) + z5 * x68 + z8 * (x5 - x4 - x36) + z3 * (x4 - x2 - x68) +
                   z2 * x36 + z4 * x83) /
    12.0;

  double x75 = x7 - x5;
  double x47 = x4 - x7;
  double x54 = x5 - x4;
  gradop[8][2] = (z7 * (x3 - x6 - x54) + z6 * x75 + z5 * (x6 - x1 - x47) + z4 * (x1 - x3 - x75) +
                   z3 * x47 + z1 * x54) /
    12.0;

  double y24 = y2 - y4;
  double y52 = y5 - y2;
  double y45 = y4 - y5;
  gradop[1][3] = (x2 * (y6 - y3 - y45) + x3 * y24 + x4 * (y3 - y8 - y52) + x5 * (y8 - y6 - y24) +
                   x6 * y52 + x8 * y45) /
    12.0;

  double y31 = y3 - y1;
  double y63 = y6 - y3;
  double y16 = y1 - y6;
  gradop[2][3] = (x3 * (y7 - y4 - y16) + x4 * y31 + x1 * (y4 - y5 - y63) + x6 * (y5 - y7 - y31) +
                   x7 * y63 + x5 * y16) /
    12.0;

  double y42 = y4 - y2;
  double y74 = y7 - y4;
  double y27 = y2 - y7;
  gradop[3][3] = (x4 * (y8 - y1 - y27) + x1 * y42 + x2 * (y1 - y6 - y74) + x7 * (y6 - y8 - y42) +
                   x8 * y74 + x6 * y27) /
    12.0;

  double y13 = y1 - y3;
  double y81 = y8 - y1;
  double y38 = y3 - y8;
  gradop[4][3] = (x1 * (y5 - y2 - y38) + x2 * y13 + x3 * (y2 - y7 - y81) + x8 * (y7 - y5 - y13) +
                   x5 * y81 + x7 * y38) /
    12.0;

  double y86 = y8 - y6;
  double y18 = y1 - y8;
  double y61 = y6 - y1;
  gradop[5][3] = (x8 * (y4 - y7 - y61) + x7 * y86 + x6 * (y7 - y2 - y18) + x1 * (y2 - y4 - y86) +
                   x4 * y18 + x2 * y61) /
    12.0;

  double y57 = y5 - y7;
  double y25 = y2 - y5;
  double y72 = y7 - y2;
  gradop[6][3] = (x5 * (y1 - y8 - y72) + x8 * y57 + x7 * (y8 - y3 - y25) + x2 * (y3 - y1 - y57) +
                   x1 * y25 + x3 * y72) /
    12.0;

  double y68 = y6 - y8;
  double y36 = y3 - y6;
  double y83 = y8 - y3;
  gradop[7][3] = (x6 * (y2 - y5 - y83) + x5 * y68 + x8 * (y5 - y4 - y36) + x3 * (y4 - y2 - y68) +
                   x2 * y36 + x4 * y83) /
    12.0;

  double y75 = y7 - y5;
  double y47 = y4 - y7;
  double y54 = y5 - y4;
  gradop[8][3] = (x7 * (y3 - y6 - y54) + x6 * y75 + x5 * (y6 - y1 - y47) + x4 * (y1 - y3 - y75) +
                   x3 * y47 + x1 * y54) /
    12.0;

  //     calculate element volume and characteristic element aspect ratio
  //     (used in time step and hourglass control) -

  double volume = coordinates[0][0] * gradop[1][1] + coordinates[1][0] * gradop[2][1] +
    coordinates[2][0] * gradop[3][1] + coordinates[3][0] * gradop[4][1] +
    coordinates[4][0] * gradop[5][1] + coordinates[5][0] * gradop[6][1] +
    coordinates[6][0] * gradop[7][1] + coordinates[7][0] * gradop[8][1];
  double aspect = .5 * SQR(volume) /
    (SQR(gradop[1][1]) + SQR(gradop[2][1]) + SQR(gradop[3][1]) + SQR(gradop[4][1]) +
      SQR(gradop[5][1]) + SQR(gradop[6][1]) + SQR(gradop[7][1]) + SQR(gradop[8][1]) +
      SQR(gradop[1][2]) + SQR(gradop[2][2]) + SQR(gradop[3][2]) + SQR(gradop[4][2]) +
      SQR(gradop[5][2]) + SQR(gradop[6][2]) + SQR(gradop[7][2]) + SQR(gradop[8][2]) +
      SQR(gradop[1][3]) + SQR(gradop[2][3]) + SQR(gradop[3][3]) + SQR(gradop[4][3]) +
      SQR(gradop[5][3]) + SQR(gradop[6][3]) + SQR(gradop[7][3]) + SQR(gradop[8][3]));

  return (double)std::sqrt(aspect);
}

/*!
  oddy of a hex

  General distortion measure based on left Cauchy-Green Tensor
 */
double hex_oddy(int /*num_nodes*/, const double coordinates[][3])
{
  double oddy = 0.0, current_oddy;
  VerdictVector xxi, xet, xze;

  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  xxi = calc_hex_efg(1, node_pos);
  xet = calc_hex_efg(2, node_pos);
  xze = calc_hex_efg(3, node_pos);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  xxi.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  xet.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  xze.set(coordinates[4][0] - coordinates[0][0], coordinates[4][1] - coordinates[0][1],
    coordinates[4][2] - coordinates[0][2]);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  xxi.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  xet.set(coordinates[0][0] - coordinates[1][0], coordinates[0][1] - coordinates[1][1],
    coordinates[0][2] - coordinates[1][2]);

  xze.set(coordinates[5][0] - coordinates[1][0], coordinates[5][1] - coordinates[1][1],
    coordinates[5][2] - coordinates[1][2]);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  xxi.set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  xet.set(coordinates[1][0] - coordinates[2][0], coordinates[1][1] - coordinates[2][1],
    coordinates[1][2] - coordinates[2][2]);

  xze.set(coordinates[6][0] - coordinates[2][0], coordinates[6][1] - coordinates[2][1],
    coordinates[6][2] - coordinates[2][2]);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  xxi.set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);

  xet.set(coordinates[2][0] - coordinates[3][0], coordinates[2][1] - coordinates[3][1],
    coordinates[2][2] - coordinates[3][2]);

  xze.set(coordinates[7][0] - coordinates[3][0], coordinates[7][1] - coordinates[3][1],
    coordinates[7][2] - coordinates[3][2]);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  xxi.set(coordinates[7][0] - coordinates[4][0], coordinates[7][1] - coordinates[4][1],
    coordinates[7][2] - coordinates[4][2]);

  xet.set(coordinates[5][0] - coordinates[4][0], coordinates[5][1] - coordinates[4][1],
    coordinates[5][2] - coordinates[4][2]);

  xze.set(coordinates[0][0] - coordinates[4][0], coordinates[0][1] - coordinates[4][1],
    coordinates[0][2] - coordinates[4][2]);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  xxi.set(coordinates[4][0] - coordinates[5][0], coordinates[4][1] - coordinates[5][1],
    coordinates[4][2] - coordinates[5][2]);

  xet.set(coordinates[6][0] - coordinates[5][0], coordinates[6][1] - coordinates[5][1],
    coordinates[6][2] - coordinates[5][2]);

  xze.set(coordinates[1][0] - coordinates[5][0], coordinates[1][1] - coordinates[5][1],
    coordinates[1][2] - coordinates[5][2]);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  xxi.set(coordinates[5][0] - coordinates[6][0], coordinates[5][1] - coordinates[6][1],
    coordinates[5][2] - coordinates[6][2]);

  xet.set(coordinates[7][0] - coordinates[6][0], coordinates[7][1] - coordinates[6][1],
    coordinates[7][2] - coordinates[6][2]);

  xze.set(coordinates[2][0] - coordinates[6][0], coordinates[2][1] - coordinates[6][1],
    coordinates[2][2] - coordinates[6][2]);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  xxi.set(coordinates[6][0] - coordinates[7][0], coordinates[6][1] - coordinates[7][1],
    coordinates[6][2] - coordinates[7][2]);

  xet.set(coordinates[4][0] - coordinates[7][0], coordinates[4][1] - coordinates[7][1],
    coordinates[4][2] - coordinates[7][2]);

  xze.set(coordinates[3][0] - coordinates[7][0], coordinates[3][1] - coordinates[7][1],
    coordinates[3][2] - coordinates[7][2]);

  current_oddy = oddy_comp(xxi, xet, xze);
  if (current_oddy > oddy)
  {
    oddy = current_oddy;
  }

  if (oddy > 0)
  {
    return (double)std::min(oddy, VERDICT_DBL_MAX);
  }
  return (double)std::max(oddy, -VERDICT_DBL_MAX);
}

/*!
   the average Frobenius aspect of a hex

   NB (P. Pebay 01/20/07):
     this function is calculated by averaging the 8 Frobenius aspects at
     each corner of the hex, when the reference corner is right isosceles.
 */
double hex_med_aspect_frobenius(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  VerdictVector xxi, xet, xze;

  // J(0,0,0):
  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  double med_aspect_frobenius = condition_comp(xxi, xet, xze);

  // J(1,0,0):
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  med_aspect_frobenius += condition_comp(xxi, xet, xze);

  // J(1,1,0):
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  med_aspect_frobenius += condition_comp(xxi, xet, xze);

  // J(0,1,0):
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  med_aspect_frobenius += condition_comp(xxi, xet, xze);

  // J(0,0,1):
  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  med_aspect_frobenius += condition_comp(xxi, xet, xze);

  // J(1,0,1):
  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  med_aspect_frobenius += condition_comp(xxi, xet, xze);

  // J(1,1,1):
  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  med_aspect_frobenius += condition_comp(xxi, xet, xze);

  // J(1,1,1):
  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  med_aspect_frobenius += condition_comp(xxi, xet, xze);

  if (med_aspect_frobenius >= VERDICT_DBL_MAX)
  {
    return VERDICT_DBL_MAX;
  }
  if (med_aspect_frobenius <= -VERDICT_DBL_MAX)
  {
    return -VERDICT_DBL_MAX;
  }

  return med_aspect_frobenius / 24.;
}

/*!
  maximum Frobenius condition number of a hex

  Maximum Frobenius condition number of the Jacobian matrix at 8 corners
   NB (P. Pebay 01/25/07):
     this function is calculated by taking the maximum of the 8 Frobenius aspects at
     each corner of the hex, when the reference corner is right isosceles.
 */
double hex_max_aspect_frobenius(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  VerdictVector xxi, xet, xze;

  // J(0,0,0):
  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  double condition = condition_comp(xxi, xet, xze);

  // J(1,0,0):
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  double current_condition = condition_comp(xxi, xet, xze);
  if (current_condition > condition)
  {
    condition = current_condition;
  }

  // J(1,1,0):
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  current_condition = condition_comp(xxi, xet, xze);
  if (current_condition > condition)
  {
    condition = current_condition;
  }

  // J(0,1,0):
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  current_condition = condition_comp(xxi, xet, xze);
  if (current_condition > condition)
  {
    condition = current_condition;
  }

  // J(0,0,1):
  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  current_condition = condition_comp(xxi, xet, xze);
  if (current_condition > condition)
  {
    condition = current_condition;
  }

  // J(1,0,1):
  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  current_condition = condition_comp(xxi, xet, xze);
  if (current_condition > condition)
  {
    condition = current_condition;
  }

  // J(1,1,1):
  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  current_condition = condition_comp(xxi, xet, xze);
  if (current_condition > condition)
  {
    condition = current_condition;
  }

  // J(1,1,1):
  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  current_condition = condition_comp(xxi, xet, xze);
  if (current_condition > condition)
  {
    condition = current_condition;
  }

  if (condition >= VERDICT_DBL_MAX)
  {
    return VERDICT_DBL_MAX;
  }
  if (condition <= -VERDICT_DBL_MAX)
  {
    return -VERDICT_DBL_MAX;
  }

  return condition / 3.;
}

/*!
  The maximum Frobenius condition of a hex, a.k.a. condition
  NB (P. Pebay 01/25/07):
     this method is maintained for backwards compatibility only.
     It will become deprecated at some point.

 */
double hex_condition(int /*num_nodes*/, const double coordinates[][3])
{
  return hex_max_aspect_frobenius(8, coordinates);
}

/*!
  jacobian of a hex

  Minimum pointwise volume of local map at 8 corners & center of hex
 */
double hex_jacobian(int num_nodes, const double coordinates[][3])
{
  if (num_nodes == 27)
  {
    double dhdr[27];
    double dhds[27];
    double dhdt[27];
    double min_determinant = VERDICT_DBL_MAX;

    for (int i = 0; i < 27; i++)
    {
      HEX27_gradients_of_the_shape_functions_for_RST(HEX27_node_local_coord[i], dhdr, dhds, dhdt);
      double jacobian[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };

      for (int j = 0; j < 27; j++)
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
      double det = VerdictVector::Dot(
        (VerdictVector(jacobian[0]) * VerdictVector(jacobian[1])), VerdictVector(jacobian[2]));
      min_determinant = std::min(det, min_determinant);
    }
    return min_determinant;
  }
  else
  {
    VerdictVector node_pos[8];
    make_hex_nodes(coordinates, node_pos);

    double jacobian = VERDICT_DBL_MAX;
    double current_jacobian;
    VerdictVector xxi, xet, xze;

    xxi = calc_hex_efg(1, node_pos);
    xet = calc_hex_efg(2, node_pos);
    xze = calc_hex_efg(3, node_pos);

    current_jacobian = VerdictVector::Dot(xxi, (xet * xze)) / 64.0;
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    // J(0,0,0):
    xxi = node_pos[1] - node_pos[0];
    xet = node_pos[3] - node_pos[0];
    xze = node_pos[4] - node_pos[0];

    current_jacobian = VerdictVector::Dot(xxi, (xet * xze));
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    // J(1,0,0):
    xxi = node_pos[2] - node_pos[1];
    xet = node_pos[0] - node_pos[1];
    xze = node_pos[5] - node_pos[1];

    current_jacobian = VerdictVector::Dot(xxi, (xet * xze));
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    // J(1,1,0):
    xxi = node_pos[3] - node_pos[2];
    xet = node_pos[1] - node_pos[2];
    xze = node_pos[6] - node_pos[2];

    current_jacobian = VerdictVector::Dot(xxi, (xet * xze));
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    // J(0,1,0):
    xxi = node_pos[0] - node_pos[3];
    xet = node_pos[2] - node_pos[3];
    xze = node_pos[7] - node_pos[3];

    current_jacobian = VerdictVector::Dot(xxi, (xet * xze));
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    // J(0,0,1):
    xxi = node_pos[7] - node_pos[4];
    xet = node_pos[5] - node_pos[4];
    xze = node_pos[0] - node_pos[4];

    current_jacobian = xxi % (xet * xze);
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    // J(1,0,1):
    xxi = node_pos[4] - node_pos[5];
    xet = node_pos[6] - node_pos[5];
    xze = node_pos[1] - node_pos[5];

    current_jacobian = xxi % (xet * xze);
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    // J(1,1,1):
    xxi = node_pos[5] - node_pos[6];
    xet = node_pos[7] - node_pos[6];
    xze = node_pos[2] - node_pos[6];

    current_jacobian = xxi % (xet * xze);
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    // J(0,1,1):
    xxi = node_pos[6] - node_pos[7];
    xet = node_pos[4] - node_pos[7];
    xze = node_pos[3] - node_pos[7];

    current_jacobian = xxi % (xet * xze);
    if (current_jacobian < jacobian)
    {
      jacobian = current_jacobian;
    }

    if (jacobian > 0)
    {
      return (double)std::min(jacobian, VERDICT_DBL_MAX);
    }
    return (double)std::max(jacobian, -VERDICT_DBL_MAX);
  }
}

/*!
  scaled jacobian of a hex

  Minimum Jacobian divided by the lengths of the 3 edge vectors
 */
double hex_scaled_jacobian(int num_nodes, const double coordinates[][3])
{
  double jacobi, min_norm_jac = VERDICT_DBL_MAX;

#if 0
  if(num_nodes == 27)
  {
    double dhdr[27];
    double dhds[27];
    double dhdt[27];

    for(int i=0; i<27; i++)
    {
      HEX27_gradients_of_the_shape_functions_for_RST(HEX27_node_local_coord[i], dhdr, dhds, dhdt);
      double jacobian[3][3] = {{0,0,0},{0,0,0},{0,0,0}};

      for(int j=0; j<27; j++)
      {
        jacobian[0][0]+=coordinates[j][0]*dhdr[j];
        jacobian[1][0]+=coordinates[j][0]*dhds[j];
        jacobian[2][0]+=coordinates[j][0]*dhdt[j];
        jacobian[0][1]+=coordinates[j][1]*dhdr[j];
        jacobian[1][1]+=coordinates[j][1]*dhds[j];
        jacobian[2][1]+=coordinates[j][1]*dhdt[j];
        jacobian[0][2]+=coordinates[j][2]*dhdr[j];
        jacobian[1][2]+=coordinates[j][2]*dhds[j];
        jacobian[2][2]+=coordinates[j][2]*dhdt[j];
      }
      VerdictVector xxi(jacobian[0]);
      VerdictVector xet(jacobian[1]);
      VerdictVector xze(jacobian[2]);
      jacobi = xxi % ( xet * xze );
      double scale = std::sqrt(xxi.length_squared() * xet.length_squared() * xze.length_squared());
      jacobi /= scale;
      if(jacobi < min_norm_jac)
      {
        min_norm_jac = jacobi;
      }
    }
  }
  else
#endif
  {
    double min_jacobi = VERDICT_DBL_MAX;
    double temp_norm_jac, lengths;
    double len1_sq, len2_sq, len3_sq;
    VerdictVector xxi, xet, xze;

    VerdictVector node_pos[8];
    make_hex_nodes(coordinates, node_pos);

    xxi = calc_hex_efg(1, node_pos);
    xet = calc_hex_efg(2, node_pos);
    xze = calc_hex_efg(3, node_pos);

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;

    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }

    // J(0,0,0):
    xxi = node_pos[1] - node_pos[0];
    xet = node_pos[3] - node_pos[0];
    xze = node_pos[4] - node_pos[0];

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;
    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }

    // J(1,0,0):
    xxi = node_pos[2] - node_pos[1];
    xet = node_pos[0] - node_pos[1];
    xze = node_pos[5] - node_pos[1];

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;
    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }

    // J(1,1,0):
    xxi = node_pos[3] - node_pos[2];
    xet = node_pos[1] - node_pos[2];
    xze = node_pos[6] - node_pos[2];

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;
    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }

    // J(0,1,0):
    xxi = node_pos[0] - node_pos[3];
    xet = node_pos[2] - node_pos[3];
    xze = node_pos[7] - node_pos[3];

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;
    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }

    // J(0,0,1):
    xxi = node_pos[7] - node_pos[4];
    xet = node_pos[5] - node_pos[4];
    xze = node_pos[0] - node_pos[4];

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;
    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }

    // J(1,0,1):
    xxi = node_pos[4] - node_pos[5];
    xet = node_pos[6] - node_pos[5];
    xze = node_pos[1] - node_pos[5];

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;
    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }

    // J(1,1,1):
    xxi = node_pos[5] - node_pos[6];
    xet = node_pos[7] - node_pos[6];
    xze = node_pos[2] - node_pos[6];

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;
    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }

    // J(0,1,1):
    xxi = node_pos[6] - node_pos[7];
    xet = node_pos[4] - node_pos[7];
    xze = node_pos[3] - node_pos[7];

    jacobi = VerdictVector::Dot(xxi, (xet * xze));
    if (jacobi < min_jacobi)
    {
      min_jacobi = jacobi;
    }

    len1_sq = xxi.length_squared();
    len2_sq = xet.length_squared();
    len3_sq = xze.length_squared();

    if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
    {
      return (double)VERDICT_DBL_MAX;
    }

    lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
    temp_norm_jac = jacobi / lengths;
    if (temp_norm_jac < min_norm_jac)
    {
      min_norm_jac = temp_norm_jac;
    }
    else
    {
      temp_norm_jac = jacobi;
    }
  }

  if (min_norm_jac > 0)
  {
    return (double)std::min(min_norm_jac, VERDICT_DBL_MAX);
  }
  return (double)std::max(min_norm_jac, -VERDICT_DBL_MAX);
}

/*!
  Nodal jacobian ratio of a hex
  Minimum nodal jacobian divided by the maximum.  Detects element skewness.
 */
inline std::pair<double /*min*/, double /*max*/> minMaxVal(const double a1, const double a2)
{
  return (a1 < a2) ? std::pair<double, double>(a1, a2) : std::pair<double, double>(a2, a1);
}

inline std::pair<double, double> minMaxValPair(
  const std::pair<double, double>& a1, const std::pair<double, double>& a2)
{
  return std::pair<double, double>(std::min(a1.first, a2.first), std::max(a1.second, a2.second));
}

double hex_nodal_jacobian_ratio(int num_nodes, const double coordinates[][3])
{
  return verdict::hex_nodal_jacobian_ratio2(num_nodes, (double*)coordinates);
}

double hex_nodal_jacobian_ratio2(int /*num_nodes*/, const double* coordinates)
{
  double Jdet8x[8];
  verdict::hex_nodal_jacobians(coordinates, Jdet8x);
  //
  //  Compute the minimum and maximum nodal determinates, use an optimal algorithm
  //
  // std::pair<double, double> minMaxResult = std::minmax_element(Jdet8x, Jdet8x+8);

  std::pair<double, double> m01(minMaxVal(Jdet8x[0], Jdet8x[1]));
  std::pair<double, double> m23(minMaxVal(Jdet8x[2], Jdet8x[3]));
  std::pair<double, double> m45(minMaxVal(Jdet8x[4], Jdet8x[5]));
  std::pair<double, double> m67(minMaxVal(Jdet8x[6], Jdet8x[7]));

  std::pair<double, double> m0123(minMaxValPair(m01, m23));
  std::pair<double, double> m4567(minMaxValPair(m45, m67));

  std::pair<double, double> m01234567(minMaxValPair(m0123, m4567));

  //
  //  Turn the determinates into a normalized quality ratio.
  //  If the maximum nodal jacobian is negative the element is fully inverted, return huge negative
  //  number Otherwise return ratio of the minimal nodal determinate to the maximum
  //

  if (m01234567.second <= VERDICT_DBL_MIN)
  {
    return -VERDICT_DBL_MAX;
  }
  else
  {
    return m01234567.first / m01234567.second;
  }
}

/*!
  shear of a hex

  3/Condition number of Jacobian Skew matrix
 */
double hex_shear(int /*num_nodes*/, const double coordinates[][3])
{
  double shear;
  double min_shear = 1.0;
  VerdictVector xxi, xet, xze;
  double det, len1_sq, len2_sq, len3_sq, lengths;

  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  // J(0,0,0):
  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
  {
    return 0;
  }

  lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det < VERDICT_DBL_MIN)
  {
    return 0;
  }

  shear = det / lengths;
  min_shear = std::min(shear, min_shear);

  // J(1,0,0):
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
  {
    return 0;
  }

  lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det < VERDICT_DBL_MIN)
  {
    return 0;
  }

  shear = det / lengths;
  min_shear = std::min(shear, min_shear);

  // J(1,1,0):
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
  {
    return 0;
  }

  lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det < VERDICT_DBL_MIN)
  {
    return 0;
  }

  shear = det / lengths;
  min_shear = std::min(shear, min_shear);

  // J(0,1,0):
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
  {
    return 0;
  }

  lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det < VERDICT_DBL_MIN)
  {
    return 0;
  }

  shear = det / lengths;
  min_shear = std::min(shear, min_shear);

  // J(0,0,1):
  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
  {
    return 0;
  }

  lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det < VERDICT_DBL_MIN)
  {
    return 0;
  }

  shear = det / lengths;
  min_shear = std::min(shear, min_shear);

  // J(1,0,1):
  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
  {
    return 0;
  }

  lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det < VERDICT_DBL_MIN)
  {
    return 0;
  }

  shear = det / lengths;
  min_shear = std::min(shear, min_shear);

  // J(1,1,1):
  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
  {
    return 0;
  }

  lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det < VERDICT_DBL_MIN)
  {
    return 0;
  }

  shear = det / lengths;
  min_shear = std::min(shear, min_shear);

  // J(0,1,1):
  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if (len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN || len3_sq <= VERDICT_DBL_MIN)
  {
    return 0;
  }

  lengths = std::sqrt(len1_sq * len2_sq * len3_sq);
  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det < VERDICT_DBL_MIN)
  {
    return 0;
  }

  shear = det / lengths;
  min_shear = std::min(shear, min_shear);

  if (min_shear <= VERDICT_DBL_MIN)
  {
    min_shear = 0;
  }

  if (min_shear > 0)
  {
    return (double)std::min(min_shear, VERDICT_DBL_MAX);
  }
  return (double)std::max(min_shear, -VERDICT_DBL_MAX);
}

/*!
  shape of a hex

  3/Condition number of weighted Jacobian matrix
 */
double hex_shape(int /*num_nodes*/, const double coordinates[][3])
{
  double det, shape;
  double min_shape = 1.0;

  VerdictVector xxi, xet, xze;

  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  // J(0,0,0):
  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det > VERDICT_DBL_MIN)
  {
    shape = 3 * std::pow(det, two_thirds) /
      (VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze));
  }
  else
  {
    return 0;
  }

  if (shape < min_shape)
  {
    min_shape = shape;
  }

  // J(1,0,0):
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det > VERDICT_DBL_MIN)
  {
    shape = 3 * std::pow(det, two_thirds) /
      (VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze));
  }
  else
  {
    return 0;
  }

  if (shape < min_shape)
  {
    min_shape = shape;
  }

  // J(1,1,0):
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det > VERDICT_DBL_MIN)
  {
    shape = 3 * std::pow(det, two_thirds) /
      (VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze));
  }
  else
  {
    return 0;
  }

  if (shape < min_shape)
  {
    min_shape = shape;
  }

  // J(0,1,0):
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det > VERDICT_DBL_MIN)
  {
    shape = 3 * std::pow(det, two_thirds) /
      (VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze));
  }
  else
  {
    return 0;
  }

  if (shape < min_shape)
  {
    min_shape = shape;
  }

  // J(0,0,1):
  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det > VERDICT_DBL_MIN)
  {
    shape = 3 * std::pow(det, two_thirds) /
      (VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze));
  }
  else
  {
    return 0;
  }

  if (shape < min_shape)
  {
    min_shape = shape;
  }

  // J(1,0,1):
  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det > VERDICT_DBL_MIN)
  {
    shape = 3 * std::pow(det, two_thirds) /
      (VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze));
  }
  else
  {
    return 0;
  }

  if (shape < min_shape)
  {
    min_shape = shape;
  }

  // J(1,1,1):
  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det > VERDICT_DBL_MIN)
  {
    shape = 3 * std::pow(det, two_thirds) /
      (VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze));
  }
  else
  {
    return 0;
  }

  if (shape < min_shape)
  {
    min_shape = shape;
  }

  // J(1,1,1):
  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  det = VerdictVector::Dot(xxi, (xet * xze));
  if (det > VERDICT_DBL_MIN)
  {
    shape = 3 * std::pow(det, two_thirds) /
      (VerdictVector::Dot(xxi, xxi) + VerdictVector::Dot(xet, xet) + VerdictVector::Dot(xze, xze));
  }
  else
  {
    return 0;
  }

  if (shape < min_shape)
  {
    min_shape = shape;
  }

  if (min_shape <= VERDICT_DBL_MIN)
  {
    min_shape = 0;
  }

  if (min_shape > 0)
  {
    return (double)std::min(min_shape, VERDICT_DBL_MAX);
  }
  return (double)std::max(min_shape, -VERDICT_DBL_MAX);
}

/*!
  relative size of a hex

  Min( J, 1/J ), where J is determinant of weighted Jacobian matrix
 */
double hex_relative_size_squared(
  int /*num_nodes*/, const double coordinates[][3], double average_hex_volume)
{
  double size = 0;
  double tau;

  VerdictVector xxi, xet, xze;
  double det, det_sum = 0;

  hex_get_weight(xxi, xet, xze, average_hex_volume);

  // This is the average relative size
  double detw = VerdictVector::Dot(xxi, (xet * xze));

  if (detw < VERDICT_DBL_MIN)
  {
    return 0;
  }

  VerdictVector node_pos[8];
  make_hex_nodes(coordinates, node_pos);

  // J(0,0,0):
  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  det = VerdictVector::Dot(xxi, (xet * xze));
  det_sum += det;

  // J(1,0,0):
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  det = VerdictVector::Dot(xxi, (xet * xze));
  det_sum += det;

  // J(0,1,0):
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  det = VerdictVector::Dot(xxi, (xet * xze));
  det_sum += det;

  // J(1,1,0):
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  det = VerdictVector::Dot(xxi, (xet * xze));
  det_sum += det;

  // J(0,1,0):
  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  det = VerdictVector::Dot(xxi, (xet * xze));
  det_sum += det;

  // J(1,0,1):
  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  det = VerdictVector::Dot(xxi, (xet * xze));
  det_sum += det;

  // J(1,1,1):
  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  det = VerdictVector::Dot(xxi, (xet * xze));
  det_sum += det;

  // J(1,1,1):
  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  det = VerdictVector::Dot(xxi, (xet * xze));
  det_sum += det;

  if (det_sum > VERDICT_DBL_MIN)
  {
    tau = det_sum / (8 * detw);

    tau = std::min(tau, 1.0 / tau);

    size = tau * tau;
  }

  if (size > 0)
  {
    return (double)std::min(size, VERDICT_DBL_MAX);
  }
  return (double)std::max(size, -VERDICT_DBL_MAX);
}

/*!
  shape and size of a hex

  Product of Shape and Relative Size
 */
double hex_shape_and_size(int num_nodes, const double coordinates[][3], double average_hex_volume)
{
  double size = hex_relative_size_squared(num_nodes, coordinates, average_hex_volume);
  double shape = hex_shape(num_nodes, coordinates);

  double shape_size = size * shape;

  if (shape_size > 0)
  {
    return (double)std::min(shape_size, VERDICT_DBL_MAX);
  }
  return (double)std::max(shape_size, -VERDICT_DBL_MAX);
}

/*!
  shear and size of a hex

  Product of Shear and Relative Size
 */
double hex_shear_and_size(int num_nodes, const double coordinates[][3], double average_hex_volume)
{
  double size = hex_relative_size_squared(num_nodes, coordinates, average_hex_volume);
  double shear = hex_shear(num_nodes, coordinates);

  double shear_size = shear * size;

  if (shear_size > 0)
  {
    return (double)std::min(shear_size, VERDICT_DBL_MAX);
  }
  return (double)std::max(shear_size, -VERDICT_DBL_MAX);
}

/*!
  distortion of a hex
 */
double hex_distortion(int num_nodes, const double coordinates[][3])
{
  // use 2x2 gauss points for linear hex and 3x3 for 2nd order hex
  int number_of_gauss_points = 0;
  if (num_nodes < 20)
  {
    // 2x2 quadrature rule
    number_of_gauss_points = 2;
    num_nodes = 8;
  }
  else if (num_nodes >= 20)
  {
    // 3x3 quadrature rule
    number_of_gauss_points = 3;
    num_nodes = 20;
  }

  int number_dimension = 3;
  int total_number_of_gauss_points =
    number_of_gauss_points * number_of_gauss_points * number_of_gauss_points;
  double distortion = VERDICT_DBL_MAX;

  // maxTotalNumberGaussPoints =27, maxNumberNodes = 20
  // they are defined in GaussIntegration.hpp
  // This is used to make these arrays static.
  // I tried dynamically allocated arrays but the new and delete
  // was very expensive

  double shape_function[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy1[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy2[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy3[maxTotalNumberGaussPoints][maxNumberNodes];
  double weight[maxTotalNumberGaussPoints];

  // create an object of GaussIntegration
  GaussIntegration gint{};
  gint.initialize(number_of_gauss_points, num_nodes, number_dimension);
  gint.calculate_shape_function_3d_hex();
  gint.get_shape_func(shape_function[0], dndy1[0], dndy2[0], dndy3[0], weight);

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

    jacobian = VerdictVector::Dot(xxi, (xet * xze));
    if (minimum_jacobian > jacobian)
    {
      minimum_jacobian = jacobian;
    }

    element_volume += weight[ife] * jacobian;
  }

  // loop through all nodes
  double dndy1_at_node[maxNumberNodes][maxNumberNodes];
  double dndy2_at_node[maxNumberNodes][maxNumberNodes];
  double dndy3_at_node[maxNumberNodes][maxNumberNodes];

  gint.calculate_derivative_at_nodes_3d(dndy1_at_node, dndy2_at_node, dndy3_at_node);
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

    jacobian = VerdictVector::Dot(xxi, (xet * xze));
    if (minimum_jacobian > jacobian)
    {
      minimum_jacobian = jacobian;
    }
  }
  if (std::abs(element_volume) > 0.0)
  {
    distortion = minimum_jacobian / element_volume * 8.;
  }
  if (distortion > VERDICT_DBL_MAX)
  {
    distortion = VERDICT_DBL_MAX;
  }
  else if (distortion < -VERDICT_DBL_MAX)
  {
    distortion = -VERDICT_DBL_MAX;
  }
  else if (std::isnan(distortion))
  {
    distortion = VERDICT_DBL_MAX; // 0/0, or should we return some other value?
  }
  return (double)distortion;
}

double hex_timestep(int num_nodes, const double coordinates[][3], double density,
  double poissons_ratio, double youngs_modulus)
{
  double char_length = hex_dimension(num_nodes, coordinates);
  double M =
    youngs_modulus * (1 - poissons_ratio) / ((1 - 2 * poissons_ratio) * (1 + poissons_ratio));
  double denominator = std::sqrt(M / density);

  return char_length / denominator;
}

/*
double hex_jac_normjac_oddy_cond( int choices[],
                      const double coordinates[][3],
                      double answers[4]  )
{
  //Define variables
  int i;

  double xxi[3], xet[3], xze[3];
  double norm_jacobian = 0.0, current_norm_jac = 0.0;
        double jacobian = 0.0, current_jacobian = 0.0;
  double oddy = 0.0, current_oddy = 0.0;
  double condition = 0.0, current_condition = 0.0;

  for( i=0; i<3; i++)
    xxi[i] = calc_hex_efg( 2, i, coordinates );
  for( i=0; i<3; i++)
    xet[i] = calc_hex_efg( 3, i, coordinates );
  for( i=0; i<3; i++)
    xze[i] = calc_hex_efg( 6, i, coordinates );

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze  );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }

    current_jacobian /= 64.0;
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  for( i=0; i<3; i++)
  {
    xxi[i] = coordinates[1][i] - coordinates[0][i];
    xet[i] = coordinates[3][i] - coordinates[0][i];
    xze[i] = coordinates[4][i] - coordinates[0][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze  );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[1][i] - coordinates[0][i];
          xet[i] = coordinates[2][i] - coordinates[1][i];
          xze[i] = coordinates[5][i] - coordinates[1][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi,  xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }


  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[2][i] - coordinates[3][i];
          xet[i] = coordinates[3][i] - coordinates[0][i];
          xze[i] = coordinates[7][i] - coordinates[3][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[5][i] - coordinates[4][i];
          xet[i] = coordinates[7][i] - coordinates[4][i];
          xze[i] = coordinates[4][i] - coordinates[0][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[2][i] - coordinates[3][i];
          xet[i] = coordinates[2][i] - coordinates[1][i];
          xze[i] = coordinates[6][i] - coordinates[2][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[5][i] - coordinates[4][i];
          xet[i] = coordinates[6][i] - coordinates[5][i];
          xze[i] = coordinates[5][i] - coordinates[1][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[6][i] - coordinates[7][i];
          xet[i] = coordinates[7][i] - coordinates[4][i];
          xze[i] = coordinates[7][i] - coordinates[3][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[6][i] - coordinates[7][i];
          xet[i] = coordinates[6][i] - coordinates[5][i];
          xze[i] = coordinates[6][i] - coordinates[2][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );

    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; }
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy
  if ( choices[2] )
  {
    current_oddy = oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }

    condition /= 3.0;
  }

  answers[0] = jacobian;
  answers[1] = norm_jacobian;
  answers[2] = oddy;
  answers[3] = condition;

  return 1.0;
}
*/
} // namespace verdict
