/*=========================================================================

  Module:    V_QuadMetric.cpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * QuadMetric.cpp contains quality calculations for Quads
 *
 * This file is part of VERDICT
 *
 */

#include "V_GaussIntegration.hpp"
#include "VerdictVector.hpp"
#include "verdict.h"
#include "verdict_defines.hpp"

namespace VERDICT_NAMESPACE
{
static constexpr double radius_ratio_normal_coeff = 1. / (2. * sqrt2);

/*!
  weights based on the average size of a quad
 */
VERDICT_HOST_DEVICE static int quad_get_weight(
  double& m11, double& m21, double& m12, double& m22, double average_quad_size)
{

  m11 = 1;
  m21 = 0;
  m12 = 0;
  m22 = 1;

  double scale = sqrt(average_quad_size / (m11 * m22 - m21 * m12));

  m11 *= scale;
  m21 *= scale;
  m12 *= scale;
  m22 *= scale;

  return 1;
}

//! returns whether the quad is collapsed or not
VERDICT_HOST_DEVICE static VerdictBoolean is_collapsed_quad(const double coordinates[][3])
{
  if (coordinates[3][0] == coordinates[2][0] && coordinates[3][1] == coordinates[2][1] &&
    coordinates[3][2] == coordinates[2][2])
  {
    return VERDICT_TRUE;
  }
  else
  {
    return VERDICT_FALSE;
  }
}

VERDICT_HOST_DEVICE static void make_quad_edges(VerdictVector edges[4], const double coordinates[][3])
{

  edges[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  edges[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  edges[2].set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);
  edges[3].set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);
}

VERDICT_HOST_DEVICE static void signed_corner_areas(double areas[4], const double coordinates[][3])
{
  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  VerdictVector corner_normals[4];
  corner_normals[0] = edges[3] * edges[0];
  corner_normals[1] = edges[0] * edges[1];
  corner_normals[2] = edges[1] * edges[2];
  corner_normals[3] = edges[2] * edges[3];

  // principal axes
  VerdictVector principal_axes[2];
  principal_axes[0] = edges[0] - edges[2];
  principal_axes[1] = edges[1] - edges[3];

  // quad center unit normal
  VerdictVector unit_center_normal;
  unit_center_normal = principal_axes[0] * principal_axes[1];
  unit_center_normal.normalize();

  areas[0] = unit_center_normal % corner_normals[0];
  areas[1] = unit_center_normal % corner_normals[1];
  areas[2] = unit_center_normal % corner_normals[2];
  areas[3] = unit_center_normal % corner_normals[3];
}

#if 0  /* Not currently used and not exposed in verdict.h */
/*!
  localize the coordinates of a quad

  localizing puts the centriod of the quad
  at the orgin and also rotates the quad
  such that edge (0,1) is aligned with the x axis 
  and the quad normal lines up with the y axis.

 */
static void localize_quad_coordinates(VerdictVector nodes[4])
{
  int i;
  VerdictVector global[4] = { nodes[0], nodes[1], nodes[2], nodes[3] };
  
  VerdictVector center = (global[0] + global[1] + global[2] + global[3]) / 4.0;
  for(i=0; i<4; i++)
  {
    global[i] -= center;
  }
  
  VerdictVector vector_diff;
  VerdictVector vector_sum;
  VerdictVector ref_point(0.0,0.0,0.0);
  VerdictVector tmp_vector, normal(0.0,0.0,0.0);
  VerdictVector vector1, vector2;
  
  for(i=0; i<4; i++)
  {
    vector1 = global[i];
    vector2 = global[(i+1)%4];
    vector_diff = vector2 - vector1;
    ref_point += vector1;
    vector_sum = vector1 + vector2;
    
    tmp_vector.set(vector_diff.y() * vector_sum.z(),
        vector_diff.z() * vector_sum.x(),
        vector_diff.x() * vector_sum.y());
    normal += tmp_vector;
  }
  
  normal.normalize();
  normal *= -1.0;
  
  
  VerdictVector local_x_axis = global[1] - global[0];
  local_x_axis.normalize();
  
  VerdictVector local_y_axis = normal * local_x_axis;
  local_y_axis.normalize();
  
  for (i=0; i < 4; i++)
  {
    nodes[i].x(global[i] % local_x_axis);
    nodes[i].y(global[i] % local_y_axis);
    nodes[i].z(global[i] % normal);
  }
}

/*! 
  moves and rotates the quad such that it enables us to 
  use components of ef's
 */
static void localize_quad_for_ef( VerdictVector node_pos[4] )
{

  VerdictVector centroid(node_pos[0]);
  centroid += node_pos[1];
  centroid += node_pos[2];
  centroid += node_pos[3];
  
  centroid /= 4.0;

  node_pos[0] -= centroid;
  node_pos[1] -= centroid;
  node_pos[2] -= centroid;
  node_pos[3] -= centroid;

  VerdictVector rotate = node_pos[1] + node_pos[2] - node_pos[3] - node_pos[0];
  rotate.normalize();

  double cosine = rotate.x();
  double   sine = rotate.y();
 
  double xnew;
 
  for (int i=0; i < 4; i++) 
  {
    xnew =  cosine * node_pos[i].x() +   sine * node_pos[i].y();
    node_pos[i].y( -sine * node_pos[i].x() + cosine * node_pos[i].y() );
    node_pos[i].x(xnew);
  }
}
#endif /* Not currently used and not exposed in verdict.h */

/*!
  returns the normal vector of a quad
 */
VERDICT_HOST_DEVICE static VerdictVector quad_normal(const double coordinates[][3])
{
  // get normal at node 0
  VerdictVector edge0, edge1;

  edge0.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  edge1.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  VerdictVector norm0 = edge0 * edge1;
  norm0.normalize();

  // because some faces may have obtuse angles, check another normal at
  // node 2 for consistent sense

  edge0.set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);

  edge1.set(coordinates[1][0] - coordinates[2][0], coordinates[1][1] - coordinates[2][1],
    coordinates[1][2] - coordinates[2][2]);

  VerdictVector norm2 = edge0 * edge1;
  norm2.normalize();

  // if these two agree, we are done, else test a third to decide

  if ((norm0 % norm2) > 0.0)
  {
    norm0 += norm2;
    norm0 *= 0.5;
    return norm0;
  }

  // test normal at node1

  edge0.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  edge1.set(coordinates[0][0] - coordinates[1][0], coordinates[0][1] - coordinates[1][1],
    coordinates[0][2] - coordinates[1][2]);

  VerdictVector norm1 = edge0 * edge1;
  norm1.normalize();

  if ((norm0 % norm1) > 0.0)
  {
    norm0 += norm1;
    norm0 *= 0.5;
    return norm0;
  }
  else
  {
    norm2 += norm1;
    norm2 *= 0.5;
    return norm2;
  }
}

VERDICT_HOST_DEVICE void quad_minimum_maximum_angle(double min_max_angles[2], const double coordinates[][3])
{
  // if this is a collapsed quad, just pass it on to
  // the tri_largest_angle routine
  if (is_collapsed_quad(coordinates) == VERDICT_TRUE)
  {
    min_max_angles[0] = tri_minimum_angle(3, coordinates);
    min_max_angles[1] = tri_maximum_angle(3, coordinates);
    return;
  }

  double angle;
  double max_angle = 0.0;
  double min_angle = 360.0;

  VerdictVector edges[4];
  edges[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  edges[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  edges[2].set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);
  edges[3].set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);

  // go around each node and calculate the angle
  // at each node
  double length[4];
  length[0] = edges[0].length();
  length[1] = edges[1].length();
  length[2] = edges[2].length();
  length[3] = edges[3].length();

  if (length[0] <= VERDICT_DBL_MIN || length[1] <= VERDICT_DBL_MIN ||
    length[2] <= VERDICT_DBL_MIN || length[3] <= VERDICT_DBL_MIN)
  {
    min_max_angles[0] = 360.0;
    min_max_angles[1] = 0.0;
    return;
  }

  angle = acos(-(edges[0] % edges[1]) / (length[0] * length[1]));
  min_angle = fmin(angle, min_angle);
  max_angle = fmax(angle, max_angle);

  angle = acos(-(edges[1] % edges[2]) / (length[1] * length[2]));
  min_angle = fmin(angle, min_angle);
  max_angle = fmax(angle, max_angle);

  angle = acos(-(edges[2] % edges[3]) / (length[2] * length[3]));
  min_angle = fmin(angle, min_angle);
  max_angle = fmax(angle, max_angle);

  angle = acos(-(edges[3] % edges[0]) / (length[3] * length[0]));
  min_angle = fmin(angle, min_angle);
  max_angle = fmax(angle, max_angle);

  max_angle = max_angle * 180.0 / VERDICT_PI;
  min_angle = min_angle * 180.0 / VERDICT_PI;

  if (min_angle > 0)
  {
    min_max_angles[0] = (double)fmin(min_angle, VERDICT_DBL_MAX);
  }
  min_max_angles[0] = (double)fmax(min_angle, -VERDICT_DBL_MAX);

  // if any signed areas are < 0, then you are getting the wrong angle
  double areas[4];
  signed_corner_areas(areas, coordinates);

  if (areas[0] < 0 || areas[1] < 0 || areas[2] < 0 || areas[3] < 0)
  {
    max_angle = 360 - max_angle;
  }

  if (max_angle > 0)
  {
    min_max_angles[1] = (double)fmin(max_angle, VERDICT_DBL_MAX);
  }
  min_max_angles[1] = (double)fmax(max_angle, -VERDICT_DBL_MAX);
}

/*!
   the edge ratio of a quad

   NB (P. Pebay 01/19/07):
     Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
     minimum edge lengths
 */
VERDICT_HOST_DEVICE double quad_edge_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  apply_elem_scaling_on_edges(4, coordinates, 4, edges);

  double a2 = edges[0].length_squared();
  double b2 = edges[1].length_squared();
  double c2 = edges[2].length_squared();
  double d2 = edges[3].length_squared();

  double mab, Mab, mcd, Mcd, m2, M2;
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
  m2 = mab < mcd ? mab : mcd;
  M2 = Mab > Mcd ? Mab : Mcd;

  if (m2 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }
  else
  {
    double edge_ratio = sqrt(M2 / m2);

    if (edge_ratio > 0)
    {
      return (double)fmin(edge_ratio, VERDICT_DBL_MAX);
    }
    return (double)fmax(edge_ratio, -VERDICT_DBL_MAX);
  }
}

/*!
  maximum of edge ratio of a quad

  maximum edge length ratio at quad center
 */
VERDICT_HOST_DEVICE double quad_max_edge_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector quad_nodes[4];
  quad_nodes[0].set(coordinates[0][0], coordinates[0][1], coordinates[0][2]);
  quad_nodes[1].set(coordinates[1][0], coordinates[1][1], coordinates[1][2]);
  quad_nodes[2].set(coordinates[2][0], coordinates[2][1], coordinates[2][2]);
  quad_nodes[3].set(coordinates[3][0], coordinates[3][1], coordinates[3][2]);

  apply_elem_scaling_on_points(4, coordinates, 4, quad_nodes);

  VerdictVector principal_axes[2];
  principal_axes[0] = quad_nodes[1] + quad_nodes[2] - quad_nodes[0] - quad_nodes[3];
  principal_axes[1] = quad_nodes[2] + quad_nodes[3] - quad_nodes[0] - quad_nodes[1];

  double len1 = principal_axes[0].length();
  double len2 = principal_axes[1].length();

  if (len1 < VERDICT_DBL_MIN || len2 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double max_edge_ratio = fmax(len1 / len2, len2 / len1);

  if (max_edge_ratio > 0)
  {
    return (double)fmin(max_edge_ratio, VERDICT_DBL_MAX);
  }
  return (double)fmax(max_edge_ratio, -VERDICT_DBL_MAX);
}

/*!
   the aspect ratio of a quad

   NB (P. Pebay 01/20/07):
     this is a generalization of the triangle aspect ratio
     using Heron's formula.
 */
VERDICT_HOST_DEVICE double quad_aspect_ratio(int /*num_nodes*/, const double coordinates[][3])
{

  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  double char_size = apply_elem_scaling_on_edges(4, coordinates, 4, edges);

  double a1 = edges[0].length();
  double b1 = edges[1].length();
  double c1 = edges[2].length();
  double d1 = edges[3].length();

  double ma = a1 > b1 ? a1 : b1;
  double mb = c1 > d1 ? c1 : d1;
  double hm = ma > mb ? ma : mb;

  double corner_areas[4];
  signed_corner_areas(corner_areas, coordinates);
  corner_areas[0] /= (char_size * char_size);
  corner_areas[1] /= (char_size * char_size);
  corner_areas[2] /= (char_size * char_size);
  corner_areas[3] /= (char_size * char_size);

  double aspect_ratio = hm * (a1 + b1 + c1 + d1) / (corner_areas[0] + corner_areas[1] + corner_areas[2] + corner_areas[3]);

  if (aspect_ratio > 0)
  {
    return (double)fmin(aspect_ratio, VERDICT_DBL_MAX);
  }
  return (double)fmax(aspect_ratio, -VERDICT_DBL_MAX);
}

/*!
   the radius ratio of a quad

   NB (P. Pebay 01/19/07):
     this function is called "radius ratio" by extension of a concept that does
     not exist in general with quads -- although a different name should probably
     be used in the future.
 */
VERDICT_HOST_DEVICE double quad_radius_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  double char_size = apply_elem_scaling_on_edges(4, coordinates, 4, edges);

  double a2 = edges[0].length_squared();
  double b2 = edges[1].length_squared();
  double c2 = edges[2].length_squared();
  double d2 = edges[3].length_squared();

  VerdictVector diag;
  diag.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);
  diag /= char_size;
  double m2 = diag.length_squared();

  diag.set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);
  diag /= char_size;
  double n2 = diag.length_squared();

  double t0 = a2 > b2 ? a2 : b2;
  double t1 = c2 > d2 ? c2 : d2;
  double t2 = m2 > n2 ? m2 : n2;
  double h2 = t0 > t1 ? t0 : t1;
  h2 = h2 > t2 ? h2 : t2;

  VerdictVector ab = edges[0] * edges[1];
  VerdictVector bc = edges[1] * edges[2];
  VerdictVector cd = edges[2] * edges[3];
  VerdictVector da = edges[3] * edges[0];

  t0 = da.length();
  t1 = ab.length();
  t2 = bc.length();
  double t3 = cd.length();

  t0 = t0 < t1 ? t0 : t1;
  t2 = t2 < t3 ? t2 : t3;
  t0 = t0 < t2 ? t0 : t2;

  if (t0 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double radius_ratio = radius_ratio_normal_coeff * sqrt((a2 + b2 + c2 + d2) * h2) / t0;

  if (radius_ratio > 0)
  {
    return (double)fmin(radius_ratio, VERDICT_DBL_MAX);
  }
  return (double)fmax(radius_ratio, -VERDICT_DBL_MAX);
}

/*!
   the average Frobenius aspect of a quad

   NB (P. Pebay 01/20/07):
     this function is calculated by averaging the 4 Frobenius aspects at
     each corner of the quad, when the reference triangle is right isosceles.
 */
VERDICT_HOST_DEVICE double quad_med_aspect_frobenius(int /*num_nodes*/, const double coordinates[][3])
{

  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  apply_elem_scaling_on_edges(4, coordinates, 4, edges);

  double a2 = edges[0].length_squared();
  double b2 = edges[1].length_squared();
  double c2 = edges[2].length_squared();
  double d2 = edges[3].length_squared();

  VerdictVector ab = edges[0] * edges[1];
  VerdictVector bc = edges[1] * edges[2];
  VerdictVector cd = edges[2] * edges[3];
  VerdictVector da = edges[3] * edges[0];

  double ab1 = ab.length();
  double bc1 = bc.length();
  double cd1 = cd.length();
  double da1 = da.length();

  if (ab1 < VERDICT_DBL_MIN || bc1 < VERDICT_DBL_MIN || cd1 < VERDICT_DBL_MIN ||
    da1 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double qsum = (a2 + b2) / ab1;
  qsum += (b2 + c2) / bc1;
  qsum += (c2 + d2) / cd1;
  qsum += (d2 + a2) / da1;

  double med_aspect_frobenius = .125 * qsum;

  if (med_aspect_frobenius > 0)
  {
    return (double)fmin(med_aspect_frobenius, VERDICT_DBL_MAX);
  }
  return (double)fmax(med_aspect_frobenius, -VERDICT_DBL_MAX);
}

/*!
   the maximum Frobenius aspect of a quad

   NB (P. Pebay 01/20/07):
     this function is calculated by taking the maximum of the 4 Frobenius aspects at
     each corner of the quad, when the reference triangle is right isosceles.
 */
VERDICT_HOST_DEVICE double quad_max_aspect_frobenius(int /*num_nodes*/, const double coordinates[][3])
{

  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  apply_elem_scaling_on_edges(4, coordinates, 4, edges);

  double a2 = edges[0].length_squared();
  double b2 = edges[1].length_squared();
  double c2 = edges[2].length_squared();
  double d2 = edges[3].length_squared();

  VerdictVector ab = edges[0] * edges[1];
  VerdictVector bc = edges[1] * edges[2];
  VerdictVector cd = edges[2] * edges[3];
  VerdictVector da = edges[3] * edges[0];

  double ab1 = ab.length();
  double bc1 = bc.length();
  double cd1 = cd.length();
  double da1 = da.length();

  if (ab1 < VERDICT_DBL_MIN || bc1 < VERDICT_DBL_MIN || cd1 < VERDICT_DBL_MIN ||
    da1 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }

  double qmax = (a2 + b2) / ab1;

  double qcur = (b2 + c2) / bc1;
  qmax = qmax > qcur ? qmax : qcur;

  qcur = (c2 + d2) / cd1;
  qmax = qmax > qcur ? qmax : qcur;

  qcur = (d2 + a2) / da1;
  qmax = qmax > qcur ? qmax : qcur;

  double max_aspect_frobenius = .5 * qmax;

  if (max_aspect_frobenius > 0)
  {
    return (double)fmin(max_aspect_frobenius, VERDICT_DBL_MAX);
  }
  return (double)fmax(max_aspect_frobenius, -VERDICT_DBL_MAX);
}

/*!
  skew of a quad

  maximum ||cos A|| where A is the angle between edges at quad center
 */
VERDICT_HOST_DEVICE double quad_skew(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector node_pos[4];
  for (int i = 0; i < 4; i++)
  {
    node_pos[i].set(coordinates[i][0], coordinates[i][1], coordinates[i][2]);
  }

  apply_elem_scaling_on_points(4, coordinates, 4, node_pos);

  VerdictVector principle_axes[2];
  principle_axes[0] = node_pos[1] + node_pos[2] - node_pos[3] - node_pos[0];
  principle_axes[1] = node_pos[2] + node_pos[3] - node_pos[0] - node_pos[1];

  if (principle_axes[0].normalize() < VERDICT_DBL_MIN)
  {
    return 0.0;
  }
  if (principle_axes[1].normalize() < VERDICT_DBL_MIN)
  {
    return 0.0;
  }

  double skew = fabs(principle_axes[0] % principle_axes[1]);
  return (double)fmin(skew, VERDICT_DBL_MAX);
}

/*!
  taper of a quad

  maximum ratio of lengths derived from opposite edges
 */
VERDICT_HOST_DEVICE double quad_taper(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector node_pos[4];
  for (int i = 0; i < 4; i++)
  {
    node_pos[i].set(coordinates[i][0], coordinates[i][1], coordinates[i][2]);
  }

  apply_elem_scaling_on_points(4, coordinates, 4, node_pos);

  VerdictVector principle_axes[2];
  principle_axes[0] = node_pos[1] + node_pos[2] - node_pos[3] - node_pos[0];
  principle_axes[1] = node_pos[2] + node_pos[3] - node_pos[0] - node_pos[1];

  VerdictVector cross_derivative = node_pos[0] + node_pos[2] - node_pos[1] - node_pos[3];

  double lengths[2];
  lengths[0] = principle_axes[0].length();
  lengths[1] = principle_axes[1].length();

  // get min length
  lengths[0] = fmin(lengths[0], lengths[1]);

  if (lengths[0] < VERDICT_DBL_MIN)
  {
    return VERDICT_DBL_MAX;
  }

  double taper = cross_derivative.length() / lengths[0];
  return (double)fmin(taper, VERDICT_DBL_MAX);
}

/*!
  warpage of a quad

  deviation of element from planarity
 */
VERDICT_HOST_DEVICE double quad_warpage(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  apply_elem_scaling_on_edges(4, coordinates, 4, edges);

  VerdictVector corner_normals[4];
  corner_normals[0] = edges[3] * edges[0];
  corner_normals[1] = edges[0] * edges[1];
  corner_normals[2] = edges[1] * edges[2];
  corner_normals[3] = edges[2] * edges[3];

  if (corner_normals[0].normalize() < VERDICT_DBL_MIN ||
    corner_normals[1].normalize() < VERDICT_DBL_MIN ||
    corner_normals[2].normalize() < VERDICT_DBL_MIN ||
    corner_normals[3].normalize() < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MIN;
  }

  double warpage = fmin(corner_normals[0] % corner_normals[2], corner_normals[1] % corner_normals[3]);
  warpage = warpage * warpage * warpage;

  if (warpage > 0)
  {
    return (double)fmin(warpage, VERDICT_DBL_MAX);
  }
  return (double)fmax(warpage, -VERDICT_DBL_MAX);
}

/*!
  the area of a quad

  jacobian at quad center
 */
VERDICT_HOST_DEVICE double quad_area(int num_nodes, const double coordinates[][3])
{
  if (4 == num_nodes)
  {
    double corner_areas[4];
    signed_corner_areas(corner_areas, coordinates);

    double area = 0.25 * (corner_areas[0] + corner_areas[1] + corner_areas[2] + corner_areas[3]);

    if (area > 0)
    {
      return (double)fmin(area, VERDICT_DBL_MAX);
    }
    return (double)fmax(area, -VERDICT_DBL_MAX);
  }
  else 
  {
    double area = 0;
    double tmp_coords[4][3];
    
    if (5 == num_nodes)
    {
      int tri_conn[4][2] = { {0,1}, {1,2}, {2,3}, {3,0} };

      //center node 4
      tmp_coords[2][0] = coordinates[4][0];
      tmp_coords[2][1] = coordinates[4][1];
      tmp_coords[2][2] = coordinates[4][2];

      for (auto v : tri_conn)
      {
        tmp_coords[0][0] = coordinates[v[0]][0];
        tmp_coords[0][1] = coordinates[v[0]][1];
        tmp_coords[0][2] = coordinates[v[0]][2];
        tmp_coords[1][0] = coordinates[v[1]][0];
        tmp_coords[1][1] = coordinates[v[1]][1];
        tmp_coords[1][2] = coordinates[v[1]][2];
        area += tri_area(3, tmp_coords);
      }     
    }
    else if (8 == num_nodes)
    {
      int tri_conn[4][3] = { {0,4,7}, {4,1,5}, {5,2,6}, {6,3,7} };

      for (auto v : tri_conn)
      {
        tmp_coords[0][0] = coordinates[v[0]][0];
        tmp_coords[0][1] = coordinates[v[0]][1];
        tmp_coords[0][2] = coordinates[v[0]][2];
        tmp_coords[1][0] = coordinates[v[1]][0];
        tmp_coords[1][1] = coordinates[v[1]][1];
        tmp_coords[1][2] = coordinates[v[1]][2];
        tmp_coords[2][0] = coordinates[v[2]][0];
        tmp_coords[2][1] = coordinates[v[2]][1];
        tmp_coords[2][2] = coordinates[v[2]][2];
        area += tri_area(3, tmp_coords);
      }

      //interior quad 4567
      tmp_coords[0][0] = coordinates[4][0];
      tmp_coords[0][1] = coordinates[4][1];
      tmp_coords[0][2] = coordinates[4][2];
      tmp_coords[1][0] = coordinates[5][0];
      tmp_coords[1][1] = coordinates[5][1];
      tmp_coords[1][2] = coordinates[5][2];
      tmp_coords[2][0] = coordinates[6][0];
      tmp_coords[2][1] = coordinates[6][1];
      tmp_coords[2][2] = coordinates[6][2];
      tmp_coords[3][0] = coordinates[7][0];
      tmp_coords[3][1] = coordinates[7][1];
      tmp_coords[3][2] = coordinates[7][2];
      area += quad_area(4, tmp_coords);
    }
    else if (9 == num_nodes)
    {
      int tri_conn[8][2] = { {0,4}, {4,1}, {1,5}, {5,2}, {2,6}, {6,3}, {3,7}, {7,0} };

      //quad center node
      tmp_coords[2][0] = coordinates[8][0];
      tmp_coords[2][1] = coordinates[8][1];
      tmp_coords[2][2] = coordinates[8][2];

      for (auto v : tri_conn)
      {
        tmp_coords[0][0] = coordinates[v[0]][0];
        tmp_coords[0][1] = coordinates[v[0]][1];
        tmp_coords[0][2] = coordinates[v[0]][2];
        tmp_coords[1][0] = coordinates[v[1]][0];
        tmp_coords[1][1] = coordinates[v[1]][1];
        tmp_coords[1][2] = coordinates[v[1]][2];
        area += tri_area(3, tmp_coords);
      }
    }
    return area;
  }
}

/*!
  the stretch of a quad

  sqrt(2) * minimum edge length / maximum diagonal length
 */
VERDICT_HOST_DEVICE double quad_stretch(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector edges[4], temp;
  make_quad_edges(edges, coordinates);

  double char_size = apply_elem_scaling_on_edges(4, coordinates, 4, edges);

  double lengths_squared[4];
  lengths_squared[0] = edges[0].length_squared();
  lengths_squared[1] = edges[1].length_squared();
  lengths_squared[2] = edges[2].length_squared();
  lengths_squared[3] = edges[3].length_squared();

  temp.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);
  temp /= char_size;
  double diag02 = temp.length_squared();

  temp.set(coordinates[3][0] - coordinates[1][0], coordinates[3][1] - coordinates[1][1],
    coordinates[3][2] - coordinates[1][2]);
  temp /= char_size;
  double diag13 = temp.length_squared();

  // 'diag02' is now the max diagonal of the quad
  diag02 = fmax(diag02, diag13);

  if (diag02 < VERDICT_DBL_MIN)
  {
    return (double)VERDICT_DBL_MAX;
  }
  else
  {
    double stretch = (double)(sqrt2 *
      sqrt(fmin(fmin(lengths_squared[0], lengths_squared[1]),
                  fmin(lengths_squared[2], lengths_squared[3])) /
        diag02));

    return (double)fmin(stretch, VERDICT_DBL_MAX);
  }
}

/*!
  the largest angle of a quad

  largest included quad area (degrees)
 */
VERDICT_HOST_DEVICE double quad_maximum_angle(int /*num_nodes*/, const double coordinates[][3])
{
  // if this is a collapsed quad, just pass it on to
  // the tri_largest_angle routine
  if (is_collapsed_quad(coordinates) == VERDICT_TRUE)
  {
    return tri_maximum_angle(3, coordinates);
  }

  double angle;
  double max_angle = 0.0;

  VerdictVector edges[4];
  edges[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  edges[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  edges[2].set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);
  edges[3].set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);

  // go around each node and calculate the angle
  // at each node
  double length[4];
  length[0] = edges[0].length();
  length[1] = edges[1].length();
  length[2] = edges[2].length();
  length[3] = edges[3].length();

  if (length[0] <= VERDICT_DBL_MIN || length[1] <= VERDICT_DBL_MIN ||
    length[2] <= VERDICT_DBL_MIN || length[3] <= VERDICT_DBL_MIN)
  {
    return 0.0;
  }

  angle = acos(-(edges[0] % edges[1]) / (length[0] * length[1]));
  max_angle = fmax(angle, max_angle);

  angle = acos(-(edges[1] % edges[2]) / (length[1] * length[2]));
  max_angle = fmax(angle, max_angle);

  angle = acos(-(edges[2] % edges[3]) / (length[2] * length[3]));
  max_angle = fmax(angle, max_angle);

  angle = acos(-(edges[3] % edges[0]) / (length[3] * length[0]));
  max_angle = fmax(angle, max_angle);

  max_angle = max_angle * 180.0 / VERDICT_PI;

  // if any signed areas are < 0, then you are getting the wrong angle
  double areas[4];
  signed_corner_areas(areas, coordinates);

  if (areas[0] < 0 || areas[1] < 0 || areas[2] < 0 || areas[3] < 0)
  {
    max_angle = 360 - max_angle;
  }

  if (max_angle > 0)
  {
    return (double)fmin(max_angle, VERDICT_DBL_MAX);
  }
  return (double)fmax(max_angle, -VERDICT_DBL_MAX);
}

/*!
  the smallest angle of a quad

  smallest included quad angle (degrees)
 */
VERDICT_HOST_DEVICE double quad_minimum_angle(int /*num_nodes*/, const double coordinates[][3])
{
  // if this quad is a collapsed quad, then just
  // send it to the tri_smallest_angle routine
  if (is_collapsed_quad(coordinates) == VERDICT_TRUE)
  {
    return tri_minimum_angle(3, coordinates);
  }

  double angle;
  double min_angle = 360.0;

  VerdictVector edges[4];
  edges[0].set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);
  edges[1].set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);
  edges[2].set(coordinates[3][0] - coordinates[2][0], coordinates[3][1] - coordinates[2][1],
    coordinates[3][2] - coordinates[2][2]);
  edges[3].set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);

  // go around each node and calculate the angle
  // at each node
  double length[4];
  length[0] = edges[0].length();
  length[1] = edges[1].length();
  length[2] = edges[2].length();
  length[3] = edges[3].length();

  if (length[0] <= VERDICT_DBL_MIN || length[1] <= VERDICT_DBL_MIN ||
    length[2] <= VERDICT_DBL_MIN || length[3] <= VERDICT_DBL_MIN)
  {
    return 360.0;
  }

  angle = acos(-(edges[0] % edges[1]) / (length[0] * length[1]));
  min_angle = fmin(angle, min_angle);

  angle = acos(-(edges[1] % edges[2]) / (length[1] * length[2]));
  min_angle = fmin(angle, min_angle);

  angle = acos(-(edges[2] % edges[3]) / (length[2] * length[3]));
  min_angle = fmin(angle, min_angle);

  angle = acos(-(edges[3] % edges[0]) / (length[3] * length[0]));
  min_angle = fmin(angle, min_angle);

  min_angle = min_angle * 180.0 / VERDICT_PI;

  if (min_angle > 0)
  {
    return (double)fmin(min_angle, VERDICT_DBL_MAX);
  }
  return (double)fmax(min_angle, -VERDICT_DBL_MAX);
}

VERDICT_HOST_DEVICE double quad_equiangle_skew(int /*num_nodes*/, const double coordinates[][3])
{
  double min_max_angle[2];

  quad_minimum_maximum_angle(min_max_angle, coordinates);

  double skew_max = (min_max_angle[1] - 90.0) / 90.0;
  double skew_min = (90.0 - min_max_angle[0]) / 90.0;

  if (skew_max > skew_min)
  {
    return skew_max;
  }
  return skew_min;
}

/*!
  the oddy of a quad

  general distortion measure based on left Cauchy-Green Tensor
 */
VERDICT_HOST_DEVICE double quad_oddy(int /*num_nodes*/, const double coordinates[][3])
{
  double max_oddy = 0.;

  VerdictVector first, second, node_pos[4];

  double g, g11, g12, g22, cur_oddy;
  int i;

  for (i = 0; i < 4; i++)
  {
    node_pos[i].set(coordinates[i][0], coordinates[i][1], coordinates[i][2]);
  }

  apply_elem_scaling_on_points(4, coordinates, 4, node_pos);

  for (i = 0; i < 4; i++)
  {
    first = node_pos[i] - node_pos[(i + 1) % 4];
    second = node_pos[i] - node_pos[(i + 3) % 4];

    g11 = first % first;
    g12 = first % second;
    g22 = second % second;
    g = g11 * g22 - g12 * g12;

    if (g < VERDICT_DBL_MIN)
    {
      cur_oddy = VERDICT_DBL_MAX;
    }
    else
    {
      cur_oddy = ((g11 - g22) * (g11 - g22) + 4. * g12 * g12) / 2. / g;
    }
    max_oddy = fmax(max_oddy, cur_oddy);
  }

  if (max_oddy > 0)
  {
    return (double)fmin(max_oddy, VERDICT_DBL_MAX);
  }
  return (double)fmax(max_oddy, -VERDICT_DBL_MAX);
}

/*!
  the condition of a quad

  maximum condition number of the Jacobian matrix at 4 corners
 */
VERDICT_HOST_DEVICE double quad_condition(int /*num_nodes*/, const double coordinates[][3])
{
  if (is_collapsed_quad(coordinates) == VERDICT_TRUE)
  {
    return tri_condition(3, coordinates);
  }

  double char_size = elem_scaling(4, coordinates).scale;

  double areas[4];
  signed_corner_areas(areas, coordinates);
  areas[0] /= (char_size * char_size);
  areas[1] /= (char_size * char_size);
  areas[2] /= (char_size * char_size);
  areas[3] /= (char_size * char_size);

  double max_condition = 0.;

  VerdictVector xxi, xet;

  double condition;

  for (int i = 0; i < 4; i++)
  {
    xxi.set(coordinates[i][0] - coordinates[(i + 1) % 4][0],
      coordinates[i][1] - coordinates[(i + 1) % 4][1],
      coordinates[i][2] - coordinates[(i + 1) % 4][2]);
    xxi /= char_size;

    xet.set(coordinates[i][0] - coordinates[(i + 3) % 4][0],
      coordinates[i][1] - coordinates[(i + 3) % 4][1],
      coordinates[i][2] - coordinates[(i + 3) % 4][2]);
    xet /= char_size;

    if (areas[i] < VERDICT_DBL_MIN)
    {
      condition = VERDICT_DBL_MAX;
    }
    else
    {
      condition = (xxi % xxi + xet % xet) / areas[i];
    }
    max_condition = fmax(max_condition, condition);
  }

  if (max_condition >= VERDICT_DBL_MAX)
  {
    return VERDICT_DBL_MAX;
  }
  if (max_condition <= -VERDICT_DBL_MAX)
  {
    return -VERDICT_DBL_MAX;
  }
  return max_condition / 2.;
}

/*!
  the jacobian of a quad

  minimum pointwise volume of local map at 4 corners and center of quad
*/
VERDICT_HOST_DEVICE double quad_jacobian(int /*num_nodes*/, const double coordinates[][3])
{

  if (is_collapsed_quad(coordinates) == VERDICT_TRUE)
  {
    return (double)(tri_area(3, coordinates) * 2.0);
  }

  double areas[4];
  signed_corner_areas(areas, coordinates);

  double jacobian = fmin(fmin(areas[0], areas[1]), fmin(areas[2], areas[3]));
  if (jacobian > 0)
  {
    return (double)fmin(jacobian, VERDICT_DBL_MAX);
  }
  return (double)fmax(jacobian, -VERDICT_DBL_MAX);
}

/*!
  scaled jacobian of a quad

  Minimum Jacobian divided by the lengths of the 2 edge vector
 */
VERDICT_HOST_DEVICE double quad_scaled_jacobian(int /*num_nodes*/, const double coordinates[][3])
{
  if (is_collapsed_quad(coordinates) == VERDICT_TRUE)
  {
    return tri_scaled_jacobian(3, coordinates);
  }

  double corner_areas[4], min_scaled_jac = VERDICT_DBL_MAX, scaled_jac;
  signed_corner_areas(corner_areas, coordinates);

  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  double char_size = apply_elem_scaling_on_edges(4, coordinates, 4, edges);
  corner_areas[0] /= (char_size * char_size);
  corner_areas[1] /= (char_size * char_size);
  corner_areas[2] /= (char_size * char_size);
  corner_areas[3] /= (char_size * char_size);

  double length[4];
  length[0] = edges[0].length();
  length[1] = edges[1].length();
  length[2] = edges[2].length();
  length[3] = edges[3].length();

  if (length[0] < VERDICT_DBL_MIN || length[1] < VERDICT_DBL_MIN || length[2] < VERDICT_DBL_MIN ||
    length[3] < VERDICT_DBL_MIN)
  {
    return 0.0;
  }

  scaled_jac = corner_areas[0] / (length[0] * length[3]);
  min_scaled_jac = fmin(scaled_jac, min_scaled_jac);

  scaled_jac = corner_areas[1] / (length[1] * length[0]);
  min_scaled_jac = fmin(scaled_jac, min_scaled_jac);

  scaled_jac = corner_areas[2] / (length[2] * length[1]);
  min_scaled_jac = fmin(scaled_jac, min_scaled_jac);

  scaled_jac = corner_areas[3] / (length[3] * length[2]);
  min_scaled_jac = fmin(scaled_jac, min_scaled_jac);

  if (min_scaled_jac > 0)
  {
    return (double)fmin(min_scaled_jac, VERDICT_DBL_MAX);
  }
  return (double)fmax(min_scaled_jac, -VERDICT_DBL_MAX);
}

/*!
  the shear of a quad

  2/Condition number of Jacobian Skew matrix
 */
VERDICT_HOST_DEVICE double quad_shear(int /*num_nodes*/, const double coordinates[][3])
{
  double scaled_jacobian = quad_scaled_jacobian(4, coordinates);

  if (scaled_jacobian <= VERDICT_DBL_MIN)
  {
    return 0.0;
  }
  else
  {
    return (double)fmin(scaled_jacobian, VERDICT_DBL_MAX);
  }
}

/*!
  the shape of a quad

   2/Condition number of weighted Jacobian matrix
 */
VERDICT_HOST_DEVICE double quad_shape(int /*num_nodes*/, const double coordinates[][3])
{

  double corner_areas[4], min_shape = VERDICT_DBL_MAX, shape;
  signed_corner_areas(corner_areas, coordinates);

  VerdictVector edges[4];
  make_quad_edges(edges, coordinates);

  double char_size = apply_elem_scaling_on_edges(4, coordinates, 4, edges);
  corner_areas[0] /= (char_size * char_size);
  corner_areas[1] /= (char_size * char_size);
  corner_areas[2] /= (char_size * char_size);
  corner_areas[3] /= (char_size * char_size);

  double length_squared[4];
  length_squared[0] = edges[0].length_squared();
  length_squared[1] = edges[1].length_squared();
  length_squared[2] = edges[2].length_squared();
  length_squared[3] = edges[3].length_squared();

  if (length_squared[0] <= VERDICT_DBL_MIN || length_squared[1] <= VERDICT_DBL_MIN ||
    length_squared[2] <= VERDICT_DBL_MIN || length_squared[3] <= VERDICT_DBL_MIN)
  {
    return 0.0;
  }

  shape = corner_areas[0] / (length_squared[0] + length_squared[3]);
  min_shape = fmin(shape, min_shape);

  shape = corner_areas[1] / (length_squared[1] + length_squared[0]);
  min_shape = fmin(shape, min_shape);

  shape = corner_areas[2] / (length_squared[2] + length_squared[1]);
  min_shape = fmin(shape, min_shape);

  shape = corner_areas[3] / (length_squared[3] + length_squared[2]);
  min_shape = fmin(shape, min_shape);

  min_shape *= 2;

  if (min_shape < VERDICT_DBL_MIN)
  {
    min_shape = 0;
  }

  if (min_shape > 0)
  {
    return (double)fmin(min_shape, VERDICT_DBL_MAX);
  }
  return (double)fmax(min_shape, -VERDICT_DBL_MAX);
}

/*!
  the relative size of a quad

  Min( J, 1/J ), where J is determinant of weighted Jacobian matrix
*/
VERDICT_HOST_DEVICE double quad_relative_size_squared(
  int /*num_nodes*/, const double coordinates[][3], double average_quad_area)
{
  double the_quad_area = quad_area(4, coordinates);
  double rel_size = 0;

  double w11, w21, w12, w22;
  quad_get_weight(w11, w21, w12, w22, average_quad_area);
  double avg_area = determinant(w11, w21, w12, w22);

  if (avg_area > VERDICT_DBL_MIN)
  {
    w11 = the_quad_area / avg_area;

    if (w11 > VERDICT_DBL_MIN)
    {
      rel_size = fmin(w11, 1 / w11);
      rel_size *= rel_size;
    }
  }

  if (rel_size > 0)
  {
    return (double)fmin(rel_size, VERDICT_DBL_MAX);
  }
  return (double)fmax(rel_size, -VERDICT_DBL_MAX);
}

/*!
  the relative shape and size of a quad

  Product of Shape and Relative Size
 */
VERDICT_HOST_DEVICE double quad_shape_and_size(int num_nodes, const double coordinates[][3], double average_quad_area)
{
  double shape, size;
  size = quad_relative_size_squared(num_nodes, coordinates, average_quad_area);
  shape = quad_shape(num_nodes, coordinates);

  double shape_and_size = shape * size;

  if (shape_and_size > 0)
  {
    return (double)fmin(shape_and_size, VERDICT_DBL_MAX);
  }
  return (double)fmax(shape_and_size, -VERDICT_DBL_MAX);
}

/*!
  the shear and size of a quad

  product of shear and relative size
 */
VERDICT_HOST_DEVICE double quad_shear_and_size(int num_nodes, const double coordinates[][3], double average_quad_area)
{
  double shear, size;
  shear = quad_shear(num_nodes, coordinates);
  size = quad_relative_size_squared(num_nodes, coordinates, average_quad_area);

  double shear_and_size = shear * size;

  if (shear_and_size > 0)
  {
    return (double)fmin(shear_and_size, VERDICT_DBL_MAX);
  }
  return (double)fmax(shear_and_size, -VERDICT_DBL_MAX);
}

/*!
  the distortion of a quad
*/
VERDICT_HOST_DEVICE double quad_distortion(int num_nodes, const double coordinates[][3])
{
  // To calculate distortion for linear and 2nd order quads
  // distortion = {min(|J|)/actual area}*{parent area}
  // parent area = 4 for a quad.
  // min |J| is the minimum over nodes and gaussian integration points
  // created by Ling Pan, CAT on 4/30/01

  double element_area = 0.0, distrt, thickness_gauss;
  double cur_jacobian = 0., sign_jacobian, jacobian;
  VerdictVector aa, bb, cc, normal_at_point, xin;

  // use 2x2 gauss points for linear quads and 3x3 for 2nd order quads
  int number_of_gauss_points = 0;
  if (num_nodes == 4)
  { // 2x2 quadrature rule
    number_of_gauss_points = 2;
  }
  else if (num_nodes == 8)
  { // 3x3 quadrature rule
    number_of_gauss_points = 3;
  }

  int total_number_of_gauss_points = number_of_gauss_points * number_of_gauss_points;

  VerdictVector face_normal = quad_normal(coordinates);

  double distortion = VERDICT_DBL_MAX;

  VerdictVector first, second;

  int i;
  // Will work out the case for collapsed quad later
  if (is_collapsed_quad(coordinates) == VERDICT_TRUE)
  {
    for (i = 0; i < 3; i++)
    {
      first.set(coordinates[i][0] - coordinates[(i + 1) % 3][0],
        coordinates[i][1] - coordinates[(i + 1) % 3][1],
        coordinates[i][2] - coordinates[(i + 1) % 3][2]);

      second.set(coordinates[i][0] - coordinates[(i + 2) % 3][0],
        coordinates[i][1] - coordinates[(i + 2) % 3][1],
        coordinates[i][2] - coordinates[(i + 2) % 3][2]);

      sign_jacobian = (face_normal % (first * second)) > 0 ? 1. : -1.;
      cur_jacobian = sign_jacobian * (first * second).length();
      distortion = fmin(distortion, cur_jacobian);
    }
    element_area = (first * second).length() / 2.0;
    distortion /= element_area;
  }
  else
  {
    double shape_function[maxTotalNumberGaussPoints][maxNumberNodes];
    double dndy1[maxTotalNumberGaussPoints][maxNumberNodes];
    double dndy2[maxTotalNumberGaussPoints][maxNumberNodes];
    double weight[maxTotalNumberGaussPoints];

    // create an object of GaussIntegration
    GaussIntegration gint{};
    gint.initialize(number_of_gauss_points, num_nodes);
    gint.calculate_shape_function_2d_quad();
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
      jacobian = normal_at_point.length();
      element_area += weight[ife] * jacobian;
    }

    double dndy1_at_node[maxNumberNodes][maxNumberNodes];
    double dndy2_at_node[maxNumberNodes][maxNumberNodes];

    gint.calculate_derivative_at_nodes(dndy1_at_node, dndy2_at_node);

    VerdictVector normal_at_nodes[9];

    // evaluate normal at nodes and distortion values at nodes
    int jai;
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
      if (fabs(dot_product) < 0.99)
      {
        flat_element = false;
        break;
      }
    }

    // take into consideration the thickness of the element
    double thickness;
    // get_quad_thickness(face, element_area, thickness );
    thickness = 0.001 * sqrt(element_area);

    // set thickness gauss point location
    double zl = 0.5773502691896;
    if (flat_element)
    {
      zl = 0.0;
    }

    int no_gauss_pts_z = (flat_element) ? 1 : 2;
    double thickness_z;
    int igz;
    // loop on Gauss points
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
        jacobian = normal_at_point.length();
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
      sign_jacobian = (face_normal % normal_at_point) > 0 ? 1. : -1.;
      distrt = sign_jacobian * (cc % normal_at_point);

      if (distrt < distortion)
      {
        distortion = distrt;
      }
    }

    if (element_area * thickness != 0)
    {
      distortion *= 8. / (element_area * thickness);
    }
    else
    {
      distortion *= 8.;
    }
  }
  return (double)distortion;
}
} // namespace verdict
