/*=========================================================================

  Module:    V_TriMetric.cpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * TriMetric.cpp contains quality calculations for Tris
 *
 * This file is part of VERDICT
 *
 */

#include "V_GaussIntegration.hpp"
#include "VerdictVector.hpp"
#include "verdict.h"
#include "verdict_defines.hpp"

#include <algorithm>

namespace VERDICT_NAMESPACE
{
static const double sqrt3 = std::sqrt(3.0);
static const double aspect_ratio_normal_coeff = sqrt3 / 6.;
static const double two_times_sqrt3 = 2 * sqrt3;
static const double two_over_sqrt3 = 2. / sqrt3;

/*!
  get weights based on the average area of a set of
  tris
*/
static int tri_get_weight(
  double& m11, double& m21, double& m12, double& m22, double average_tri_area)
{
  m11 = 1;
  m21 = 0;
  m12 = 0.5;
  m22 = 0.5 * sqrt3;
  double scale = std::sqrt(2.0 * average_tri_area / (m11 * m22 - m21 * m12));

  m11 *= scale;
  m21 *= scale;
  m12 *= scale;
  m22 *= scale;

  return 1;
}

/*!
   the edge ratio of a triangle

   NB (P. Pebay 01/14/07):
     Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
     minimum edge lengths

*/
double tri_edge_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  // three vectors for each side
  VerdictVector a(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  VerdictVector b(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  VerdictVector c(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  double a2 = a.length_squared();
  double b2 = b.length_squared();
  double c2 = c.length_squared();

  double m2, M2;
  if (a2 < b2)
  {
    if (b2 < c2)
    {
      m2 = a2;
      M2 = c2;
    }
    else // b2 <= a2
    {
      if (a2 < c2)
      {
        m2 = a2;
        M2 = b2;
      }
      else // c2 <= a2
      {
        m2 = c2;
        M2 = b2;
      }
    }
  }
  else // b2 <= a2
  {
    if (a2 < c2)
    {
      m2 = b2;
      M2 = c2;
    }
    else // c2 <= a2
    {
      if (b2 < c2)
      {
        m2 = b2;
        M2 = a2;
      }
      else // c2 <= b2
      {
        m2 = c2;
        M2 = a2;
      }
    }
  }

  if (m2 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }
  else
  {
    double edge_ratio;
    edge_ratio = std::sqrt(M2 / m2);

    if (edge_ratio > 0)
    {
      return (double)std::min(edge_ratio, VERDICT_DBL_MAX);
    }
    return (double)std::max(edge_ratio, -VERDICT_DBL_MAX);
  }
}

/*!
   the aspect ratio of a triangle

   NB (P. Pebay 01/14/07):
     Hmax / ( 2.0 * sqrt(3.0) * IR) where Hmax is the maximum edge length
     and IR is the inradius

     note that previous incarnations of verdict used "v_tri_aspect_ratio" to denote
     what is now called "v_tri_aspect_frobenius"

 */
double tri_aspect_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  // three vectors for each side
  VerdictVector a(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  VerdictVector b(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  VerdictVector c(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  double a1 = a.length();
  double b1 = b.length();
  double c1 = c.length();

  double hm = a1 > b1 ? a1 : b1;
  hm = hm > c1 ? hm : c1;

  VerdictVector ab = a * b;
  double denominator = ab.length();

  if (denominator < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }
  else
  {
    double aspect_ratio;
    aspect_ratio = aspect_ratio_normal_coeff * hm * (a1 + b1 + c1) / denominator;

    if (aspect_ratio > 0)
    {
      return (double)std::min(aspect_ratio, VERDICT_DBL_MAX);
    }
    return (double)std::max(aspect_ratio, -VERDICT_DBL_MAX);
  }
}

/*!
   the radius ratio of a triangle

   NB (P. Pebay 01/13/07):
     CR / (2.0*IR) where CR is the circumradius and IR is the inradius

     The radius ratio is also known to VERDICT, for tetrahedral elements only,
     as the "aspect beta".

 */
double tri_radius_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  // three vectors for each side
  VerdictVector a(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  VerdictVector b(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  VerdictVector c(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  double a1 = a.length();
  double b1 = b.length();
  double c1 = c.length();

  VerdictVector ab = a * b;
  double denominator = ab.length_squared();

  if (denominator < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double radius_ratio;
  radius_ratio = .25 * a1 * b1 * c1 * (a1 + b1 + c1) / denominator;

  if (radius_ratio > 0)
  {
    return (double)std::min(radius_ratio, VERDICT_DBL_MAX);
  }
  return (double)std::max(radius_ratio, -VERDICT_DBL_MAX);
}

/*!
   the Frobenius aspect of a tri

   srms^2/(2 * sqrt(3.0) * area)
   where srms^2 is sum of the lengths squared

   NB (P. Pebay 01/14/07):
     this method was called "aspect ratio" in earlier incarnations of VERDICT

 */
double tri_aspect_frobenius(int /*num_nodes*/, const double coordinates[][3])
{

  // three vectors for each side
  VerdictVector side1(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  VerdictVector side2(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  VerdictVector side3(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  // sum the lengths squared of each side
  double srms = (side1.length_squared() + side2.length_squared() + side3.length_squared());

  // find two times the area of the triangle by cross product
  double areaX2 = ((side1 * (-side3)).length());

  if (areaX2 == 0.0)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double aspect = (double)(srms / (two_times_sqrt3 * (areaX2)));
  if (aspect > 0)
  {
    return (double)std::min(aspect, VERDICT_DBL_MAX);
  }
  return (double)std::max(aspect, -VERDICT_DBL_MAX);
}

/*!
  The area of a tri

  0.5 * jacobian at a node
 */
double tri_area(int /*num_nodes*/, const double coordinates[][3])
{
  // two vectors for two sides
  VerdictVector side1(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  VerdictVector side3(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  // the cross product of the two vectors representing two sides of the
  // triangle
  VerdictVector tmp = side1 * side3;

  // return the magnitude of the vector divided by two
  double area = 0.5 * tmp.length();
  if (area > 0)
  {
    return (double)std::min(area, VERDICT_DBL_MAX);
  }
  return (double)std::max(area, -VERDICT_DBL_MAX);
}

/*!
  The minimum angle of a tri

  The minimum angle of a tri is the minimum angle between
  two adjacents sides out of all three corners of the triangle.
 */
double tri_minimum_angle(int /*num_nodes*/, const double coordinates[][3])
{
  // vectors for all the sides
  VerdictVector sides[4];
  sides[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  sides[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  sides[2].set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  // in case we need to find the interior angle
  // between sides 0 and 1
  sides[3] = -sides[1];

  // calculate the lengths squared of the sides
  double sides_lengths[3];
  sides_lengths[0] = sides[0].length_squared();
  sides_lengths[1] = sides[1].length_squared();
  sides_lengths[2] = sides[2].length_squared();

  if (sides_lengths[0] == 0.0 || sides_lengths[1] == 0.0 || sides_lengths[2] == 0.0)
  {
    return 0.0;
  }

  // using the law of sines, we know that the minimum
  // angle is opposite of the shortest side

  // find the shortest side
  int short_side = 0;
  if (sides_lengths[1] < sides_lengths[0])
  {
    short_side = 1;
  }
  if (sides_lengths[2] < sides_lengths[short_side])
  {
    short_side = 2;
  }

  // from the shortest side, calculate the angle of the
  // opposite angle
  double min_angle;
  if (short_side == 0)
  {
    min_angle = sides[2].interior_angle(sides[1]);
  }
  else if (short_side == 1)
  {
    min_angle = sides[0].interior_angle(sides[2]);
  }
  else
  {
    min_angle = sides[0].interior_angle(sides[3]);
  }

  if (min_angle > 0)
  {
    return (double)std::min(min_angle, VERDICT_DBL_MAX);
  }
  return (double)std::max(min_angle, -VERDICT_DBL_MAX);
}

/*!
  The maximum angle of a tri

  The maximum angle of a tri is the maximum angle between
  two adjacents sides out of all three corners of the triangle.
 */
double tri_maximum_angle(int /*num_nodes*/, const double coordinates[][3])
{

  // vectors for all the sides
  VerdictVector sides[4];
  sides[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  sides[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  sides[2].set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  // in case we need to find the interior angle
  // between sides 0 and 1
  sides[3] = -sides[1];

  // calculate the lengths squared of the sides
  double sides_lengths[3];
  sides_lengths[0] = sides[0].length_squared();
  sides_lengths[1] = sides[1].length_squared();
  sides_lengths[2] = sides[2].length_squared();

  if (sides_lengths[0] == 0.0 || sides_lengths[1] == 0.0 || sides_lengths[2] == 0.0)
  {
    return 0.0;
  }

  // using the law of sines, we know that the maximum
  // angle is opposite of the longest side

  // find the longest side
  int short_side = 0;
  if (sides_lengths[1] > sides_lengths[0])
  {
    short_side = 1;
  }
  if (sides_lengths[2] > sides_lengths[short_side])
  {
    short_side = 2;
  }

  // from the longest side, calculate the angle of the
  // opposite angle
  double max_angle;
  if (short_side == 0)
  {
    max_angle = sides[2].interior_angle(sides[1]);
  }
  else if (short_side == 1)
  {
    max_angle = sides[0].interior_angle(sides[2]);
  }
  else
  {
    max_angle = sides[0].interior_angle(sides[3]);
  }

  if (max_angle > 0)
  {
    return (double)std::min(max_angle, VERDICT_DBL_MAX);
  }
  return (double)std::max(max_angle, -VERDICT_DBL_MAX);
}

double tri_equiangle_skew(int num_nodes, const double coordinates[][3])
{
  double min_angle = 360.0;
  double max_angle = 0.0;

  min_angle = tri_minimum_angle(num_nodes, coordinates);
  max_angle = tri_maximum_angle(num_nodes, coordinates);

  double skew_max = (max_angle - 60.0) / 120.0;
  double skew_min = (60.0 - min_angle) / 60.0;

  if (skew_max > skew_min)
  {
    return skew_max;
  }
  return skew_min;
}

/*!
  The condition of a tri

  Condition number of the jacobian matrix at any corner
 */
double tri_condition(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector v1(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  VerdictVector v2(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  VerdictVector tri_normal = v1 * v2;
  double areax2 = tri_normal.length();

  if (areax2 == 0.0)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double condition = (double)(((v1 % v1) + (v2 % v2) - (v1 % v2)) / (areax2 * sqrt3));

  return (double)std::min(condition, VERDICT_DBL_MAX);
}

/*!
  The scaled jacobian of a tri

  minimum of the jacobian divided by the lengths of 2 edge vectors
 */
double tri_scaled_jacobian(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector first, second;
  double jacobian;

  VerdictVector edge[3];
  edge[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  edge[1].set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  edge[2].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  first = edge[1] - edge[0];
  second = edge[2] - edge[0];

  VerdictVector cross = first * second;
  jacobian = cross.length();

  double max_edge_length_product;
  max_edge_length_product = std::max(edge[0].length() * edge[1].length(),
    std::max(edge[1].length() * edge[2].length(), edge[0].length() * edge[2].length()));

  if (max_edge_length_product < VERDICT_DBL_MIN)
  {
    return (double)0.0;
  }

  jacobian *= two_over_sqrt3;
  jacobian /= max_edge_length_product;

  if (jacobian > 0)
  {
    return (double)std::min(jacobian, VERDICT_DBL_MAX);
  }
  return (double)std::max(jacobian, -VERDICT_DBL_MAX);
}

/*!
  The shape of a tri

  2 / condition number of weighted jacobian matrix
 */
double tri_shape(int num_nodes, const double coordinates[][3])
{
  double condition = tri_condition(num_nodes, coordinates);

  double shape;
  if (condition <= VERDICT_DBL_MIN)
  {
    shape = VERDICT_DBL_MAX;
  }
  else
  {
    shape = (1 / condition);
  }

  if (shape > 0)
  {
    return (double)std::min(shape, VERDICT_DBL_MAX);
  }
  return (double)std::max(shape, -VERDICT_DBL_MAX);
}

/*!
  The relative size of a tri

  Min(J,1/J) where J is the determinant of the weighted jacobian matrix.
*/
double tri_relative_size_squared(
  int /*num_nodes*/, const double coordinates[][3], double average_tri_area)
{
  double w11, w21, w12, w22;

  VerdictVector xxi, xet, tri_normal;

  tri_get_weight(w11, w21, w12, w22, average_tri_area);

  double detw = determinant(w11, w21, w12, w22);

  if (detw == 0.0)
  {
    return 0.0;
  }

  xxi.set(coordinates[0][0] - coordinates[1][0], coordinates[0][1] - coordinates[1][1],
    coordinates[0][2] - coordinates[1][2]);

  xet.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  tri_normal = xxi * xet;

  double deta = tri_normal.length();
  if (deta == 0.0 || detw == 0.0)
  {
    return 0.0;
  }

  double size = std::pow(deta / detw, 2);

  double rel_size = std::min(size, 1.0 / size);

  if (rel_size > 0)
  {
    return (double)std::min(rel_size, VERDICT_DBL_MAX);
  }
  return (double)std::max(rel_size, -VERDICT_DBL_MAX);
}

/*!
  The shape and size of a tri

  Product of the Shape and Relative Size
 */
double tri_shape_and_size(int num_nodes, const double coordinates[][3], double average_tri_area)
{
  double size, shape;

  size = tri_relative_size_squared(num_nodes, coordinates, average_tri_area);
  shape = tri_shape(num_nodes, coordinates);

  double shape_and_size = size * shape;

  if (shape_and_size > 0)
  {
    return (double)std::min(shape_and_size, VERDICT_DBL_MAX);
  }
  return (double)std::max(shape_and_size, -VERDICT_DBL_MAX);
}

/*!
  The distortion of a tri

  TODO:  make a short definition of the distortion and comment below
 */
double tri_distortion(int num_nodes, const double coordinates[][3])
{

  double distortion;
  int total_number_of_gauss_points = 0;
  VerdictVector aa, bb, cc, normal_at_point, xin;
  double element_area = 0.;

  aa.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  bb.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  VerdictVector tri_normal = aa * bb;

  int number_of_gauss_points = 0;
  if (num_nodes == 3)
  {
    distortion = 1.0;
    return (double)distortion;
  }

  else if (num_nodes == 6)
  {
    total_number_of_gauss_points = 6;
    number_of_gauss_points = 6;
  }

  distortion = VERDICT_DBL_MAX;
  double shape_function[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy1[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy2[maxTotalNumberGaussPoints][maxNumberNodes];
  double weight[maxTotalNumberGaussPoints];

  // create an object of GaussIntegration
  int number_dims = 2;
  int is_tri = 1;
  GaussIntegration gint{};
  gint.initialize(number_of_gauss_points, num_nodes, number_dims, is_tri);
  gint.calculate_shape_function_2d_tri();
  gint.get_shape_func(shape_function[0], dndy1[0], dndy2[0], weight);

  // calculate element area
  int ife, ja;
  for (ife = 0; ife < total_number_of_gauss_points; ife++)
  {
    aa.set(0.0, 0.0, 0.0);
    bb.set(0.0, 0.0, 0.0);

    for (ja = 0; ja < num_nodes; ja++)
    {
      xin.set(coordinates[ja][0], coordinates[ja][1], coordinates[ja][2]);
      aa += dndy1[ife][ja] * xin;
      bb += dndy2[ife][ja] * xin;
    }
    normal_at_point = aa * bb;
    double jacobian = normal_at_point.length();
    element_area += weight[ife] * jacobian;
  }

  element_area *= 0.8660254;
  double dndy1_at_node[maxNumberNodes][maxNumberNodes];
  double dndy2_at_node[maxNumberNodes][maxNumberNodes];

  gint.calculate_derivative_at_nodes_2d_tri(dndy1_at_node, dndy2_at_node);

  VerdictVector normal_at_nodes[7];

  // evaluate normal at nodes and distortion values at nodes
  int jai = 0;
  for (ja = 0; ja < num_nodes; ja++)
  {
    aa.set(0.0, 0.0, 0.0);
    bb.set(0.0, 0.0, 0.0);
    for (jai = 0; jai < num_nodes; jai++)
    {
      xin.set(coordinates[jai][0], coordinates[jai][1], coordinates[jai][2]);
      aa += dndy1_at_node[ja][jai] * xin;
      bb += dndy2_at_node[ja][jai] * xin;
    }
    normal_at_nodes[ja] = aa * bb;
    normal_at_nodes[ja].normalize();
  }

  // determine if element is flat
  bool flat_element = true;
  double dot_product;

  for (ja = 0; ja < num_nodes; ja++)
  {
    dot_product = normal_at_nodes[0] % normal_at_nodes[ja];
    if (std::abs(dot_product) < 0.99)
    {
      flat_element = false;
      break;
    }
  }

  // take into consideration the thickness of the element
  double thickness, thickness_gauss;
  double distrt;
  // get_tri_thickness(tri, element_area, thickness );
  thickness = 0.001 * std::sqrt(element_area);

  // set thickness gauss point location
  double zl = 0.5773502691896;
  if (flat_element)
  {
    zl = 0.0;
  }

  int no_gauss_pts_z = (flat_element) ? 1 : 2;
  double thickness_z;

  // loop on integration points
  int igz;
  for (ife = 0; ife < total_number_of_gauss_points; ife++)
  {
    // loop on the thickness direction gauss points
    for (igz = 0; igz < no_gauss_pts_z; igz++)
    {
      zl = -zl;
      thickness_z = zl * thickness / 2.0;

      aa.set(0.0, 0.0, 0.0);
      bb.set(0.0, 0.0, 0.0);
      cc.set(0.0, 0.0, 0.0);

      for (ja = 0; ja < num_nodes; ja++)
      {
        xin.set(coordinates[ja][0], coordinates[ja][1], coordinates[ja][2]);
        xin += thickness_z * normal_at_nodes[ja];
        aa += dndy1[ife][ja] * xin;
        bb += dndy2[ife][ja] * xin;
        thickness_gauss = shape_function[ife][ja] * thickness / 2.0;
        cc += thickness_gauss * normal_at_nodes[ja];
      }

      normal_at_point = aa * bb;
      distrt = cc % normal_at_point;
      if (distrt < distortion)
      {
        distortion = distrt;
      }
    }
  }

  // loop through nodal points
  for (ja = 0; ja < num_nodes; ja++)
  {
    for (igz = 0; igz < no_gauss_pts_z; igz++)
    {
      zl = -zl;
      thickness_z = zl * thickness / 2.0;

      aa.set(0.0, 0.0, 0.0);
      bb.set(0.0, 0.0, 0.0);
      cc.set(0.0, 0.0, 0.0);

      for (jai = 0; jai < num_nodes; jai++)
      {
        xin.set(coordinates[jai][0], coordinates[jai][1], coordinates[jai][2]);
        xin += thickness_z * normal_at_nodes[ja];
        aa += dndy1_at_node[ja][jai] * xin;
        bb += dndy2_at_node[ja][jai] * xin;
        if (jai == ja)
        {
          thickness_gauss = thickness / 2.0;
        }
        else
        {
          thickness_gauss = 0.;
        }
        cc += thickness_gauss * normal_at_nodes[jai];
      }
    }

    normal_at_point = aa * bb;
    double sign_jacobian = (tri_normal % normal_at_point) > 0 ? 1. : -1.;
    distrt = sign_jacobian * (cc % normal_at_point);

    if (distrt < distortion)
    {
      distortion = distrt;
    }
  }
  if (element_area * thickness != 0)
  {
    distortion *= 1. / (element_area * thickness);
  }
  else
  {
    distortion *= 1.;
  }

  if (distortion > 0)
  {
    return (double)std::min(distortion, VERDICT_DBL_MAX);
  }
  return (double)std::max(distortion, -VERDICT_DBL_MAX);
}

double tri_inradius(const double coordinates[][3])
{
  double sp = 0.0;
  VerdictVector sides[3];
  for (int i = 0; i < 3; i++)
  {
    int j = (i + 1) % 3;
    sides[i].set(coordinates[j][0] - coordinates[i][0], coordinates[j][1] - coordinates[i][1],
      coordinates[j][2] - coordinates[i][2]);
    sp += sides[i].length();
  }
  sp /= 2.0;

  VerdictVector cross = sides[1] * sides[0];
  double area = cross.length() / 2.0;

  double ir = area / sp;
  return ir;
}

double tri6_min_inradius(const double coordinates[][3])
{
  static int SUBTRI_NODES[4][3] = { { 0, 3, 5 }, { 3, 1, 4 }, { 5, 4, 2 }, { 3, 4, 5 } };
  double min_inrad = VERDICT_DBL_MAX;
  for (int i = 0; i < 4; i++)
  {
    double subtri_coords[3][3];
    for (int j = 0; j < 3; j++)
    {
      int idx = SUBTRI_NODES[i][j];
      subtri_coords[j][0] = coordinates[idx][0];
      subtri_coords[j][1] = coordinates[idx][1];
      subtri_coords[j][2] = coordinates[idx][2];
    }
    double subtri_inrad = tri_inradius(subtri_coords);
    if (subtri_inrad < min_inrad)
    {
      min_inrad = subtri_inrad;
    }
  }
  return min_inrad;
}

double calculate_tri3_outer_radius(const double coordinates[][3])
{
  double sp = 0.0;
  VerdictVector sides[3];
  double slen[3];
  for (int i = 0; i < 3; i++)
  {
    int j = (i + 1) % 3;
    sides[i].set(coordinates[j][0] - coordinates[i][0], coordinates[j][1] - coordinates[i][1],
      coordinates[j][2] - coordinates[i][2]);
    slen[i] = sides[i].length();
    sp += slen[i];
  }
  sp /= 2.0;

  VerdictVector cross = sides[1] * sides[0];
  double area = cross.length() / 2.0;
  double ir = area / sp;

  double cr = (slen[0] * slen[1] * slen[2]) / (4.0 * ir * sp);
  return cr;
}

double tri6_normalized_inradius(const double coordinates[][3])
{
  double min_inradius_for_subtri = tri6_min_inradius(coordinates);
  double outer_radius = calculate_tri3_outer_radius(coordinates);
  double normalized_inradius = 4.0 * min_inradius_for_subtri / outer_radius;

  return normalized_inradius;
}

double tri3_normalized_inradius(const double coordinates[][3])
{
  double tri6_coords[6][3];
  for (int i = 0; i < 3; i++)
  {
    tri6_coords[i][0] = coordinates[i][0];
    tri6_coords[i][1] = coordinates[i][1];
    tri6_coords[i][2] = coordinates[i][2];
  }

  static int eidx[3][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 } };
  for (int i = 3; i < 6; i++)
  {
    int i0 = eidx[i - 3][0];
    int i1 = eidx[i - 3][1];
    tri6_coords[i][0] = (coordinates[i0][0] + coordinates[i1][0]) * 0.5;
    tri6_coords[i][1] = (coordinates[i0][1] + coordinates[i1][1]) * 0.5;
    tri6_coords[i][2] = (coordinates[i0][2] + coordinates[i1][2]) * 0.5;
  }
  return tri6_normalized_inradius(tri6_coords);
}

//! Calculates the minimum normalized inner radius of a tri6
/** Ratio of the minimum subtri inner radius to tri outer radius*/
/* Currently, supports tri 6 and 3.*/
double tri_normalized_inradius(int num_nodes, const double coordinates[][3])
{
  if (num_nodes == 3)
  {
    return tri3_normalized_inradius(coordinates);
  }
  else if (num_nodes == 6)
  {
    return tri6_normalized_inradius(coordinates);
  }
  return 0.0;
}
} // namespace verdict
