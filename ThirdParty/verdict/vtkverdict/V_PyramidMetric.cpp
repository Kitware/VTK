/*=========================================================================

  Module:    V_PyramidMetric.cpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * PyramidMetrics.cpp contains quality calculations for Pyramids
 *
 * This file is part of VERDICT
 *
 */

#include "VerdictVector.hpp"
#include "verdict.h"

#include <algorithm>
#include <array>

namespace VERDICT_NAMESPACE
{
extern double quad_equiangle_skew(int num_nodes, const double coordinates[][3]);
extern double tri_equiangle_skew(int num_nodes, const double coordinates[][3]);

static const double sqrt2_2 = std::sqrt(2.0) / 2.0;

// local methods
void make_pyramid_tets(const double coordinates[][3], double tet1_coords[][3],
  double tet2_coords[][3], double tet3_coords[][3], double tet4_coords[][3]);
void make_pyramid_faces(const double coordinates[][3], double base[][3], double tri1[][3],
  double tri2[][3], double tri3[][3], double tri4[][3]);
void make_pyramid_edges(VerdictVector edges[8], const double coordinates[][3]);
double distance_point_to_pyramid_base(
  int num_nodes, const double coordinates[][3], double& cos_angle);
double largest_pyramid_edge(const double coordinates[][3]);

/*
  the pyramid element

       5
       ^
       |\
      /| \\_
     |  \   \
     |  | \_ \_
     /   \/4\  \
    |   /|    \ \_
    | /  \      \ \
    /     |       \
  1 \_    |      _/3
      \_   \   _/
        \_ | _/
          \_/
          2

    a quadrilateral base and a pointy peak like a pyramid
*/

double pyramid_equiangle_skew(int /*num_nodes*/, const double coordinates[][3])
{
  double base[4][3];
  double tri1[3][3];
  double tri2[3][3];
  double tri3[3][3];
  double tri4[3][3];
  make_pyramid_faces(coordinates, base, tri1, tri2, tri3, tri4);

  double quad_skew = quad_equiangle_skew(4, base);
  double tri1_skew = tri_equiangle_skew(3, tri1);
  double tri2_skew = tri_equiangle_skew(3, tri2);
  double tri3_skew = tri_equiangle_skew(3, tri3);
  double tri4_skew = tri_equiangle_skew(3, tri4);

  double max_skew = quad_skew;
  max_skew = max_skew > tri1_skew ? max_skew : tri1_skew;
  max_skew = max_skew > tri2_skew ? max_skew : tri2_skew;
  max_skew = max_skew > tri3_skew ? max_skew : tri3_skew;
  max_skew = max_skew > tri4_skew ? max_skew : tri4_skew;

  return max_skew;
}

/*!
  the volume of a pyramid

  the volume is calculated by dividing the pyramid into
  2 tets and summing the volumes of the 2 tets.
 */
double pyramid_volume(int /*num_nodes*/, const double coordinates[][3])
{
  double center_coords[3];
  // calculate the center of the quads
  center_coords[0] =
    (coordinates[0][0] + coordinates[1][0] + coordinates[2][0] + coordinates[3][0]) / 4;
  center_coords[1] =
    (coordinates[0][1] + coordinates[1][1] + coordinates[2][1] + coordinates[3][1]) / 4;
  center_coords[2] =
    (coordinates[0][2] + coordinates[1][2] + coordinates[2][2] + coordinates[3][2]) / 4;

  double tet_coords[4][4][3];
  tet_coords[0][0][0] = coordinates[0][0];
  tet_coords[0][0][1] = coordinates[0][1];
  tet_coords[0][0][2] = coordinates[0][2];
  tet_coords[0][1][0] = coordinates[1][0];
  tet_coords[0][1][1] = coordinates[1][1];
  tet_coords[0][1][2] = coordinates[1][2];
  tet_coords[0][2][0] = center_coords[0];
  tet_coords[0][2][1] = center_coords[1];
  tet_coords[0][2][2] = center_coords[2];
  tet_coords[0][3][0] = coordinates[4][0];
  tet_coords[0][3][1] = coordinates[4][1];
  tet_coords[0][3][2] = coordinates[4][2];

  tet_coords[1][0][0] = coordinates[1][0];
  tet_coords[1][0][1] = coordinates[1][1];
  tet_coords[1][0][2] = coordinates[1][2];
  tet_coords[1][1][0] = coordinates[2][0];
  tet_coords[1][1][1] = coordinates[2][1];
  tet_coords[1][1][2] = coordinates[2][2];
  tet_coords[1][2][0] = center_coords[0];
  tet_coords[1][2][1] = center_coords[1];
  tet_coords[1][2][2] = center_coords[2];
  tet_coords[1][3][0] = coordinates[4][0];
  tet_coords[1][3][1] = coordinates[4][1];
  tet_coords[1][3][2] = coordinates[4][2];

  tet_coords[2][0][0] = coordinates[2][0];
  tet_coords[2][0][1] = coordinates[2][1];
  tet_coords[2][0][2] = coordinates[2][2];
  tet_coords[2][1][0] = coordinates[3][0];
  tet_coords[2][1][1] = coordinates[3][1];
  tet_coords[2][1][2] = coordinates[3][2];
  tet_coords[2][2][0] = center_coords[0];
  tet_coords[2][2][1] = center_coords[1];
  tet_coords[2][2][2] = center_coords[2];
  tet_coords[2][3][0] = coordinates[4][0];
  tet_coords[2][3][1] = coordinates[4][1];
  tet_coords[2][3][2] = coordinates[4][2];

  tet_coords[3][0][0] = coordinates[3][0];
  tet_coords[3][0][1] = coordinates[3][1];
  tet_coords[3][0][2] = coordinates[3][2];
  tet_coords[3][1][0] = coordinates[0][0];
  tet_coords[3][1][1] = coordinates[0][1];
  tet_coords[3][1][2] = coordinates[0][2];
  tet_coords[3][2][0] = center_coords[0];
  tet_coords[3][2][1] = center_coords[1];
  tet_coords[3][2][2] = center_coords[2];
  tet_coords[3][3][0] = coordinates[4][0];
  tet_coords[3][3][1] = coordinates[4][1];
  tet_coords[3][3][2] = coordinates[4][2];

  double volume = 0.0;
  for (int t = 0; t < 4; t++)
  {
    volume += tet_volume(4, tet_coords[t]);
  }
  return (double)volume;
}

double pyramid_jacobian(int /*num_nodes*/, const double coordinates[][3])
{
  // break the pyramid into four tets return the minimum jacobian of the two tets
  double tet1_coords[4][3];
  double tet2_coords[4][3];
  double tet3_coords[4][3];
  double tet4_coords[4][3];

  make_pyramid_tets(coordinates, tet1_coords, tet2_coords, tet3_coords, tet4_coords);

  double j1 = tet_jacobian(4, tet1_coords);
  double j2 = tet_jacobian(4, tet2_coords);
  double j3 = tet_jacobian(4, tet3_coords);
  double j4 = tet_jacobian(4, tet4_coords);

  double p1 = j1 < j2 ? j1 : j2;
  double p2 = j3 < j4 ? j3 : j4;

  return p1 < p2 ? p1 : p2;
}

double pyramid_scaled_jacobian(int /*num_nodes*/, const double coordinates[][3])
{
  // break the pyramid into four tets return the minimum scaled jacobian of the tets
  double tet1_coords[4][3];
  double tet2_coords[4][3];
  double tet3_coords[4][3];
  double tet4_coords[4][3];

  make_pyramid_tets(coordinates, tet1_coords, tet2_coords, tet3_coords, tet4_coords);

  std::array<double, 4> scaled_jacob{};
  scaled_jacob[0] = tet_scaled_jacobian(4, tet1_coords);
  scaled_jacob[1] = tet_scaled_jacobian(4, tet2_coords);
  scaled_jacob[2] = tet_scaled_jacobian(4, tet3_coords);
  scaled_jacob[3] = tet_scaled_jacobian(4, tet4_coords);

  auto iter = std::min_element(scaled_jacob.begin(), scaled_jacob.end());

  // scale the minimum scaled jacobian so that a perfect pyramid has
  // a value of 1 and cap it to make sure it is not > 1.0 or < 0.0
  if (*iter <= 0.0)
  {
    return 0.0;
  }

  double min_jac = (*iter) * 2.0 / std::sqrt(2.0);
  return min_jac < 1.0 ? min_jac : 1.0 - (min_jac - 1.0);
}

double pyramid_shape(int num_nodes, const double coordinates[][3])
{
  // ideally there will be four equilateral triangles and one square.
  // Test each face
  double base[4][3];
  double tri1[3][3];
  double tri2[3][3];
  double tri3[3][3];
  double tri4[3][3];

  make_pyramid_faces(coordinates, base, tri1, tri2, tri3, tri4);

  double s1 = quad_shape(4, base);

  if (s1 == 0.0)
  {
    return 0.0;
  }

  double cos_angle;
  double dist_to_base = distance_point_to_pyramid_base(num_nodes, coordinates, cos_angle);

  if (dist_to_base <= 0 || cos_angle <= 0.0)
  {
    return 0.0;
  }

  double longest_edge = largest_pyramid_edge(coordinates) * sqrt2_2;
  if (dist_to_base < longest_edge)
  {
    dist_to_base = dist_to_base / longest_edge;
  }
  else
  {
    dist_to_base = longest_edge / dist_to_base;
  }

  double shape = s1 * cos_angle * dist_to_base;

  return shape;
}

void make_pyramid_tets(const double coordinates[][3], double tet1_coords[][3],
  double tet2_coords[][3], double tet3_coords[][3], double tet4_coords[][3])
{
  // tet1
  tet1_coords[0][0] = coordinates[0][0];
  tet1_coords[0][1] = coordinates[0][1];
  tet1_coords[0][2] = coordinates[0][2];

  tet1_coords[1][0] = coordinates[1][0];
  tet1_coords[1][1] = coordinates[1][1];
  tet1_coords[1][2] = coordinates[1][2];

  tet1_coords[2][0] = coordinates[2][0];
  tet1_coords[2][1] = coordinates[2][1];
  tet1_coords[2][2] = coordinates[2][2];

  tet1_coords[3][0] = coordinates[4][0];
  tet1_coords[3][1] = coordinates[4][1];
  tet1_coords[3][2] = coordinates[4][2];

  // tet2
  tet2_coords[0][0] = coordinates[0][0];
  tet2_coords[0][1] = coordinates[0][1];
  tet2_coords[0][2] = coordinates[0][2];

  tet2_coords[1][0] = coordinates[2][0];
  tet2_coords[1][1] = coordinates[2][1];
  tet2_coords[1][2] = coordinates[2][2];

  tet2_coords[2][0] = coordinates[3][0];
  tet2_coords[2][1] = coordinates[3][1];
  tet2_coords[2][2] = coordinates[3][2];

  tet2_coords[3][0] = coordinates[4][0];
  tet2_coords[3][1] = coordinates[4][1];
  tet2_coords[3][2] = coordinates[4][2];

  // tet3
  tet3_coords[0][0] = coordinates[0][0];
  tet3_coords[0][1] = coordinates[0][1];
  tet3_coords[0][2] = coordinates[0][2];

  tet3_coords[1][0] = coordinates[1][0];
  tet3_coords[1][1] = coordinates[1][1];
  tet3_coords[1][2] = coordinates[1][2];

  tet3_coords[2][0] = coordinates[3][0];
  tet3_coords[2][1] = coordinates[3][1];
  tet3_coords[2][2] = coordinates[3][2];

  tet3_coords[3][0] = coordinates[4][0];
  tet3_coords[3][1] = coordinates[4][1];
  tet3_coords[3][2] = coordinates[4][2];

  // tet4
  tet4_coords[0][0] = coordinates[1][0];
  tet4_coords[0][1] = coordinates[1][1];
  tet4_coords[0][2] = coordinates[1][2];

  tet4_coords[1][0] = coordinates[2][0];
  tet4_coords[1][1] = coordinates[2][1];
  tet4_coords[1][2] = coordinates[2][2];

  tet4_coords[2][0] = coordinates[3][0];
  tet4_coords[2][1] = coordinates[3][1];
  tet4_coords[2][2] = coordinates[3][2];

  tet4_coords[3][0] = coordinates[4][0];
  tet4_coords[3][1] = coordinates[4][1];
  tet4_coords[3][2] = coordinates[4][2];
}

void make_pyramid_faces(const double coordinates[][3], double base[][3], double tri1[][3],
  double tri2[][3], double tri3[][3], double tri4[][3])
{
  // base
  base[0][0] = coordinates[0][0];
  base[0][1] = coordinates[0][1];
  base[0][2] = coordinates[0][2];

  base[1][0] = coordinates[1][0];
  base[1][1] = coordinates[1][1];
  base[1][2] = coordinates[1][2];

  base[2][0] = coordinates[2][0];
  base[2][1] = coordinates[2][1];
  base[2][2] = coordinates[2][2];

  base[3][0] = coordinates[3][0];
  base[3][1] = coordinates[3][1];
  base[3][2] = coordinates[3][2];

  // tri1
  tri1[0][0] = coordinates[0][0];
  tri1[0][1] = coordinates[0][1];
  tri1[0][2] = coordinates[0][2];

  tri1[1][0] = coordinates[1][0];
  tri1[1][1] = coordinates[1][1];
  tri1[1][2] = coordinates[1][2];

  tri1[2][0] = coordinates[4][0];
  tri1[2][1] = coordinates[4][1];
  tri1[2][2] = coordinates[4][2];

  // tri2
  tri2[0][0] = coordinates[1][0];
  tri2[0][1] = coordinates[1][1];
  tri2[0][2] = coordinates[1][2];

  tri2[1][0] = coordinates[2][0];
  tri2[1][1] = coordinates[2][1];
  tri2[1][2] = coordinates[2][2];

  tri2[2][0] = coordinates[4][0];
  tri2[2][1] = coordinates[4][1];
  tri2[2][2] = coordinates[4][2];

  // tri3
  tri3[0][0] = coordinates[2][0];
  tri3[0][1] = coordinates[2][1];
  tri3[0][2] = coordinates[2][2];

  tri3[1][0] = coordinates[3][0];
  tri3[1][1] = coordinates[3][1];
  tri3[1][2] = coordinates[3][2];

  tri3[2][0] = coordinates[4][0];
  tri3[2][1] = coordinates[4][1];
  tri3[2][2] = coordinates[4][2];

  // tri4
  tri4[0][0] = coordinates[3][0];
  tri4[0][1] = coordinates[3][1];
  tri4[0][2] = coordinates[3][2];

  tri4[1][0] = coordinates[0][0];
  tri4[1][1] = coordinates[0][1];
  tri4[1][2] = coordinates[0][2];

  tri4[2][0] = coordinates[4][0];
  tri4[2][1] = coordinates[4][1];
  tri4[2][2] = coordinates[4][2];
}

void make_pyramid_edges(VerdictVector edges[8], const double coordinates[][3])
{
  edges[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  edges[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  edges[2].set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);
  edges[3].set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);
  edges[4].set(coordinates[4][0] - coordinates[0][0], coordinates[4][1] - coordinates[0][1],
    coordinates[4][2] - coordinates[0][2]);
  edges[5].set(coordinates[4][0] - coordinates[1][0], coordinates[4][1] - coordinates[1][1],
    coordinates[4][2] - coordinates[1][2]);
  edges[6].set(coordinates[4][0] - coordinates[2][0], coordinates[4][1] - coordinates[2][1],
    coordinates[4][2] - coordinates[2][2]);
  edges[7].set(coordinates[4][0] - coordinates[3][0], coordinates[4][1] - coordinates[3][1],
    coordinates[4][2] - coordinates[3][2]);
}

double largest_pyramid_edge(const double coordinates[][3])
{
  VerdictVector edges[8];
  make_pyramid_edges(edges, coordinates);
  double l0 = edges[0].length_squared();
  double l1 = edges[1].length_squared();
  double l2 = edges[2].length_squared();
  double l3 = edges[3].length_squared();
  double l4 = edges[4].length_squared();
  double l5 = edges[5].length_squared();
  double l6 = edges[6].length_squared();
  double l7 = edges[7].length_squared();

  double max = std::min(l0, l1);
  max = std::max(max, l2);
  max = std::max(max, l3);
  max = std::max(max, l4);
  max = std::max(max, l5);
  max = std::max(max, l6);
  max = std::max(max, l7);

  return std::sqrt(max);
}

double distance_point_to_pyramid_base(
  int /*num_nodes*/, const double coordinates[][3], double& cos_angle)
{
  VerdictVector a(coordinates[0]);
  VerdictVector b(coordinates[1]);
  VerdictVector c(coordinates[2]);
  VerdictVector d(coordinates[3]);
  VerdictVector peak(coordinates[4]);

  VerdictVector centroid = (a + b + c + d) / 4.0;
  VerdictVector t1 = b - a;
  VerdictVector t2 = d - a;

  VerdictVector normal = t1 * t2;
  double normal_length = normal.length();

  VerdictVector pq = peak - centroid;

  double distance = (pq % normal) / normal_length;
  cos_angle = distance / pq.length();

  return distance;
}
} // namespace verdict
