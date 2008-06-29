/*=========================================================================

  Module:    V_QuadMetric.cpp

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 *
 * QuadMetric.cpp contains quality calculations for Quads
 *
 * This file is part of VERDICT
 *
 */


#define VERDICT_EXPORTS

#include "verdict.h"
#include "VerdictVector.hpp"
#include "verdict_defines.hpp"
#include "V_GaussIntegration.hpp"
#include <memory.h>


//! the average area of a quad
static double v_quad_size = 0;

/*!
  weights based on the average size of a quad
*/
static int v_quad_get_weight (
  double &m11, double &m21, double &m12, double &m22 )
{
  
  m11=1;
  m21=0;
  m12=0;
  m22=1;
  
  double scale = sqrt( v_quad_size/(m11*m22-m21*m12));

  m11 *= scale;
  m21 *= scale;
  m12 *= scale;
  m22 *= scale;
  
  return 1;

}

//! return the average area of a quad
C_FUNC_DEF void v_set_quad_size( double size )
{
  v_quad_size = size;
}

//! returns whether the quad is collapsed or not
static VerdictBoolean v_is_collapsed_quad ( double coordinates[][3] )
{
  if( coordinates[3][0] == coordinates[2][0] &&
      coordinates[3][1] == coordinates[2][1] &&
      coordinates[3][2] == coordinates[2][2] )
    return VERDICT_TRUE;
  
  else
    return VERDICT_FALSE;
}

static void v_make_quad_edges( VerdictVector edges[4], double coordinates[][3] )
{

  edges[0].set(
      coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]
      );
  edges[1].set(
      coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2]
      );
  edges[2].set(
      coordinates[3][0] - coordinates[2][0],
      coordinates[3][1] - coordinates[2][1],
      coordinates[3][2] - coordinates[2][2]
      );
  edges[3].set(
      coordinates[0][0] - coordinates[3][0],
      coordinates[0][1] - coordinates[3][1],
      coordinates[0][2] - coordinates[3][2]
      );
}

static void v_signed_corner_areas( double areas[4], double coordinates[][3] )
{
  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

  VerdictVector corner_normals[4];
  corner_normals[0] = edges[3] * edges[0];
  corner_normals[1] = edges[0] * edges[1];
  corner_normals[2] = edges[1] * edges[2];
  corner_normals[3] = edges[2] * edges[3];
 
  //principal axes 
  VerdictVector principal_axes[2];
  principal_axes[0] = edges[0] - edges[2];   
  principal_axes[1] = edges[1] - edges[3];   

  //quad center unit normal
  VerdictVector unit_center_normal;
  unit_center_normal = principal_axes[0] * principal_axes[1]; 
  unit_center_normal.normalize(); 

  areas[0] =  unit_center_normal % corner_normals[0];
  areas[1] =  unit_center_normal % corner_normals[1];
  areas[2] =  unit_center_normal % corner_normals[2];
  areas[3] =  unit_center_normal % corner_normals[3];

}

#if 0 /* Not currently used and not exposed in verdict.h */
/*!
  localize the coordinates of a quad

  localizing puts the centriod of the quad
  at the orgin and also rotates the quad
  such that edge (0,1) is aligned with the x axis 
  and the quad normal lines up with the y axis.

*/
static void v_localize_quad_coordinates(VerdictVector nodes[4])
{
  int i;
  VerdictVector global[4] = { nodes[0], nodes[1], nodes[2], nodes[3] };
  
  VerdictVector center = (global[0] + global[1] + global[2] + global[3]) / 4.0;
  for(i=0; i<4; i++)
    global[i] -= center;
  
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
static void v_localize_quad_for_ef( VerdictVector node_pos[4] )
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
static VerdictVector v_quad_normal( double coordinates[][3] )
{
  // get normal at node 0
  VerdictVector edge0, edge1;
  
  edge0.set( coordinates[1][0] - coordinates[0][0], 
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2] );
  
  
  edge1.set( coordinates[3][0] - coordinates[0][0], 
      coordinates[3][1] - coordinates[0][1],
      coordinates[3][2] - coordinates[0][2] );
  
  VerdictVector norm0 = edge0 * edge1 ;
  norm0.normalize();
  
  // because some faces may have obtuse angles, check another normal at
  // node 2 for consistent sense


  edge0.set ( coordinates[2][0] - coordinates[3][0], 
      coordinates[2][1] - coordinates[3][1],
      coordinates[2][2] - coordinates[3][2] );
  
  
  edge1.set ( coordinates[2][0] - coordinates[1][0], 
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2] );
  
  VerdictVector norm2 = edge0 * edge1 ;
  norm2.normalize();
  
  // if these two agree, we are done, else test a third to decide

  if ( (norm0 % norm2) > 0.0 )
  {
    norm0 += norm2;
    norm0 *= 0.5;
    return norm0;
  }
  
  // test normal at node1


  edge0.set ( coordinates[1][0] - coordinates[2][0], 
      coordinates[1][1] - coordinates[2][1],
      coordinates[1][2] - coordinates[2][2] );
  
  
  edge1.set ( coordinates[1][0] - coordinates[0][0], 
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2] );
  
  VerdictVector norm1 = edge0 * edge1 ;
  norm1.normalize();
  
  if ( (norm0 % norm1) > 0.0 )
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

/*!
   the edge ratio of a quad

   NB (P. Pebay 01/19/07): 
     Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
     minimum edge lengths
*/
C_FUNC_DEF double v_quad_edge_ratio( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

  double a2 = edges[0].length_squared();
  double b2 = edges[1].length_squared();
  double c2 = edges[2].length_squared();
  double d2 = edges[3].length_squared();

  double mab,Mab,mcd,Mcd,m2,M2;
  if ( a2 < b2 )
    {
      mab = a2;
      Mab = b2;
    }
  else // b2 <= a2
    {
      mab = b2;
      Mab = a2;
    }
  if ( c2 < d2 )
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

  if( m2 < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;
  else
  {
    double edge_ratio = sqrt( M2 / m2 );
    
    if( edge_ratio > 0 )
      return (double) VERDICT_MIN( edge_ratio, VERDICT_DBL_MAX );
    return (double) VERDICT_MAX( edge_ratio, -VERDICT_DBL_MAX );
  }
}

/*!
  maximum of edge ratio of a quad

  maximum edge length ratio at quad center
*/
C_FUNC_DEF double v_quad_max_edge_ratio( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector quad_nodes[4];
  quad_nodes[0].set( coordinates[0][0], coordinates[0][1], coordinates[0][2] );
  quad_nodes[1].set( coordinates[1][0], coordinates[1][1], coordinates[1][2] );
  quad_nodes[2].set( coordinates[2][0], coordinates[2][1], coordinates[2][2] );
  quad_nodes[3].set( coordinates[3][0], coordinates[3][1], coordinates[3][2] );

  VerdictVector principal_axes[2];
  principal_axes[0] = quad_nodes[1] + quad_nodes[2] - quad_nodes[0] - quad_nodes[3];
  principal_axes[1] = quad_nodes[2] + quad_nodes[3] - quad_nodes[0] - quad_nodes[1];

  double len1 = principal_axes[0].length();
  double len2 = principal_axes[1].length();

  if( len1 < VERDICT_DBL_MIN || len2 < VERDICT_DBL_MIN )
    return (double)VERDICT_DBL_MAX;

  double max_edge_ratio = VERDICT_MAX( len1 / len2, len2 / len1 );

  if( max_edge_ratio > 0 )
    return (double) VERDICT_MIN( max_edge_ratio, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( max_edge_ratio, -VERDICT_DBL_MAX );
}

/*!
   the aspect ratio of a quad

   NB (P. Pebay 01/20/07): 
     this is a generalization of the triangle aspect ratio
     using Heron's formula.
*/
C_FUNC_DEF double v_quad_aspect_ratio( int /*num_nodes*/, double coordinates[][3] )
{

  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

  double a1 = edges[0].length();
  double b1 = edges[1].length();
  double c1 = edges[2].length();
  double d1 = edges[3].length();

  double ma = a1 > b1 ? a1 : b1;
  double mb = c1 > d1 ? c1 : d1;
  double hm = ma > mb ? ma : mb;

  VerdictVector ab = edges[0] * edges[1];
  VerdictVector cd = edges[2] * edges[3];
  double denominator = ab.length() + cd.length();

  if( denominator < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;

  double aspect_ratio = .5 * hm * ( a1 + b1 + c1 + d1 ) / denominator;
  
  if( aspect_ratio > 0 )
    return (double) VERDICT_MIN( aspect_ratio, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( aspect_ratio, -VERDICT_DBL_MAX );
}

/*!
   the radius ratio of a quad

   NB (P. Pebay 01/19/07): 
     this function is called "radius ratio" by extension of a concept that does
     not exist in general with quads -- although a different name should probably
     be used in the future.
*/
C_FUNC_DEF double v_quad_radius_ratio( int /*num_nodes*/, double coordinates[][3] )
{
  static const double normal_coeff = 1. / ( 2. * sqrt( 2. ) );

  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

  double a2 = edges[0].length_squared();
  double b2 = edges[1].length_squared();
  double c2 = edges[2].length_squared();
  double d2 = edges[3].length_squared();

  VerdictVector diag;
  diag.set( coordinates[2][0] - coordinates[0][0],
            coordinates[2][1] - coordinates[0][1],
            coordinates[2][2] - coordinates[0][2]);
  double m2 = diag.length_squared();

  diag.set( coordinates[3][0] - coordinates[1][0],
            coordinates[3][1] - coordinates[1][1],
            coordinates[3][2] - coordinates[1][2]);
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

  if( t0 < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;

  double radius_ratio = normal_coeff * sqrt( ( a2 + b2 + c2 + d2 ) * h2 ) / t0;
  
  if( radius_ratio > 0 )
    return (double) VERDICT_MIN( radius_ratio, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( radius_ratio, -VERDICT_DBL_MAX );
}

/*!
   the average Frobenius aspect of a quad

   NB (P. Pebay 01/20/07): 
     this function is calculated by averaging the 4 Frobenius aspects at
     each corner of the quad, when the reference triangle is right isosceles.
*/
C_FUNC_DEF double v_quad_med_aspect_frobenius( int /*num_nodes*/, double coordinates[][3] )
{

  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

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

  if( ab1 < VERDICT_DBL_MIN ||
      bc1 < VERDICT_DBL_MIN ||
      cd1 < VERDICT_DBL_MIN ||
      da1 < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;

  double qsum  = ( a2 + b2 ) / ab1;
  qsum += ( b2 + c2 ) / bc1;
  qsum += ( c2 + d2 ) / cd1;
  qsum += ( d2 + a2 ) / da1;

  double med_aspect_frobenius = .125 * qsum;

  if( med_aspect_frobenius > 0 )
    return (double) VERDICT_MIN( med_aspect_frobenius, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( med_aspect_frobenius, -VERDICT_DBL_MAX );
}

/*!
   the maximum Frobenius aspect of a quad

   NB (P. Pebay 01/20/07): 
     this function is calculated by taking the maximum of the 4 Frobenius aspects at
     each corner of the quad, when the reference triangle is right isosceles.
*/
C_FUNC_DEF double v_quad_max_aspect_frobenius( int /*num_nodes*/, double coordinates[][3] )
{

  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

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

  if( ab1 < VERDICT_DBL_MIN ||
      bc1 < VERDICT_DBL_MIN ||
      cd1 < VERDICT_DBL_MIN ||
      da1 < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;

  double qmax = ( a2 + b2 ) / ab1;

  double qcur = ( b2 + c2 ) / bc1;
  qmax = qmax > qcur ? qmax : qcur;

  qcur = ( c2 + d2 ) / cd1;
  qmax = qmax > qcur ? qmax : qcur;

  qcur = ( d2 + a2 ) / da1;
  qmax = qmax > qcur ? qmax : qcur;

  double max_aspect_frobenius = .5 * qmax;

  if( max_aspect_frobenius > 0 )
    return (double) VERDICT_MIN( max_aspect_frobenius, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( max_aspect_frobenius, -VERDICT_DBL_MAX );
}

/*!
  skew of a quad

  maximum ||cos A|| where A is the angle between edges at quad center
*/
C_FUNC_DEF double v_quad_skew( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector node_pos[4];
  for(int i = 0; i < 4; i++ )
    node_pos[i].set(coordinates[i][0], coordinates[i][1], coordinates[i][2]);

  VerdictVector principle_axes[2];
  principle_axes[0] = node_pos[1] + node_pos[2] - node_pos[3] - node_pos[0]; 
  principle_axes[1] = node_pos[2] + node_pos[3] - node_pos[0] - node_pos[1]; 

  if( principle_axes[0].normalize() < VERDICT_DBL_MIN )
    return 0.0;
  if( principle_axes[1].normalize() < VERDICT_DBL_MIN )
    return 0.0;

  double skew = fabs( principle_axes[0] % principle_axes[1] );

  return (double) VERDICT_MIN( skew, VERDICT_DBL_MAX );
}

/*! 
  taper of a quad

  maximum ratio of lengths derived from opposite edges
*/
C_FUNC_DEF double v_quad_taper( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector node_pos[4];
  for(int i = 0; i < 4; i++ )
    node_pos[i].set(coordinates[i][0], coordinates[i][1], coordinates[i][2]);

  VerdictVector principle_axes[2];
  principle_axes[0] = node_pos[1] + node_pos[2] - node_pos[3] - node_pos[0]; 
  principle_axes[1] = node_pos[2] + node_pos[3] - node_pos[0] - node_pos[1]; 

  VerdictVector cross_derivative = node_pos[0] + node_pos[2] - node_pos[1] - node_pos[3];

  double lengths[2];
  lengths[0] = principle_axes[0].length();
  lengths[1] = principle_axes[1].length();

  //get min length
  lengths[0] = VERDICT_MIN( lengths[0], lengths[1] );

  if( lengths[0] < VERDICT_DBL_MIN )
    return VERDICT_DBL_MAX;

  double taper = cross_derivative.length()/ lengths[0];
  return (double) VERDICT_MIN( taper, VERDICT_DBL_MAX );

}

/*!
  warpage of a quad

  deviation of element from planarity
*/
C_FUNC_DEF double v_quad_warpage( int /*num_nodes*/, double coordinates[][3] )
{

  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

  VerdictVector corner_normals[4];
  corner_normals[0] = edges[3] * edges[0];
  corner_normals[1] = edges[0] * edges[1];
  corner_normals[2] = edges[1] * edges[2];
  corner_normals[3] = edges[2] * edges[3];

  if( corner_normals[0].normalize() < VERDICT_DBL_MIN ||
      corner_normals[1].normalize() < VERDICT_DBL_MIN ||
      corner_normals[2].normalize() < VERDICT_DBL_MIN ||
      corner_normals[3].normalize() < VERDICT_DBL_MIN )
    return (double) VERDICT_DBL_MIN;

  double warpage = pow( 
    VERDICT_MIN( corner_normals[0]%corner_normals[2],
                 corner_normals[1]%corner_normals[3]), 3 );

  if( warpage > 0 )
    return (double) VERDICT_MIN( warpage, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( warpage, -VERDICT_DBL_MAX );

}

/*!
  the area of a quad

  jacobian at quad center
*/
C_FUNC_DEF double v_quad_area( int /*num_nodes*/, double coordinates[][3] )
{    

  double corner_areas[4];
  v_signed_corner_areas( corner_areas, coordinates );

  double area = 0.25 * (corner_areas[0] + corner_areas[1] + corner_areas[2] + corner_areas[3]);

  if( area  > 0 )
    return (double) VERDICT_MIN( area, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( area, -VERDICT_DBL_MAX );

}

/*!
  the stretch of a quad

  sqrt(2) * minimum edge length / maximum diagonal length
*/
C_FUNC_DEF double v_quad_stretch( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector edges[4], temp;
  v_make_quad_edges( edges, coordinates );

  double lengths_squared[4];
  lengths_squared[0] = edges[0].length_squared();
  lengths_squared[1] = edges[1].length_squared();
  lengths_squared[2] = edges[2].length_squared();
  lengths_squared[3] = edges[3].length_squared();

  temp.set( coordinates[2][0] - coordinates[0][0],
            coordinates[2][1] - coordinates[0][1],
            coordinates[2][2] - coordinates[0][2]);
  double diag02 = temp.length_squared();

  temp.set( coordinates[3][0] - coordinates[1][0],
            coordinates[3][1] - coordinates[1][1],
            coordinates[3][2] - coordinates[1][2]);
  double diag13 = temp.length_squared();
  
  static const double QUAD_STRETCH_FACTOR = sqrt(2.0);

  // 'diag02' is now the max diagonal of the quad
  diag02 = VERDICT_MAX( diag02, diag13 );

  if( diag02 < VERDICT_DBL_MIN )
    return (double) VERDICT_DBL_MAX;
  else
  {
    double stretch = (double) ( QUAD_STRETCH_FACTOR *
                           sqrt( VERDICT_MIN(
                                  VERDICT_MIN( lengths_squared[0], lengths_squared[1] ),
                                  VERDICT_MIN( lengths_squared[2], lengths_squared[3] ) ) /
                                diag02 ));

    return (double) VERDICT_MIN( stretch, VERDICT_DBL_MAX );
  }
}

/*!
  the largest angle of a quad

  largest included quad area (degrees)
*/
C_FUNC_DEF double v_quad_maximum_angle( int /*num_nodes*/, double coordinates[][3] )
{

  // if this is a collapsed quad, just pass it on to 
  // the tri_largest_angle routine
  if( v_is_collapsed_quad(coordinates) == VERDICT_TRUE )
    return v_tri_maximum_angle(3, coordinates);

  double angle;
  double max_angle = 0.0;
  
  VerdictVector edges[4];
  edges[0].set(
      coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]
      );
  edges[1].set(
      coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2]
      );
  edges[2].set(
      coordinates[3][0] - coordinates[2][0],
      coordinates[3][1] - coordinates[2][1],
      coordinates[3][2] - coordinates[2][2]
      );
  edges[3].set(
      coordinates[0][0] - coordinates[3][0],
      coordinates[0][1] - coordinates[3][1],
      coordinates[0][2] - coordinates[3][2]
      );

  // go around each node and calculate the angle
  // at each node
  double length[4];
  length[0] = edges[0].length();
  length[1] = edges[1].length();
  length[2] = edges[2].length();
  length[3] = edges[3].length();

  if( length[0] <= VERDICT_DBL_MIN ||
      length[1] <= VERDICT_DBL_MIN ||
      length[2] <= VERDICT_DBL_MIN ||
      length[3] <= VERDICT_DBL_MIN )
    return 0.0;  

  angle = acos( -(edges[0] % edges[1])/(length[0]*length[1]) );
  max_angle = VERDICT_MAX(angle, max_angle);

  angle = acos( -(edges[1] % edges[2])/(length[1]*length[2]) );
  max_angle = VERDICT_MAX(angle, max_angle);

  angle = acos( -(edges[2] % edges[3])/(length[2]*length[3]) );
  max_angle = VERDICT_MAX(angle, max_angle);

  angle = acos( -(edges[3] % edges[0])/(length[3]*length[0]) );
  max_angle = VERDICT_MAX(angle, max_angle);

  max_angle = max_angle *180.0/VERDICT_PI;

  //if any signed areas are < 0, then you are getting the wrong angle
  double areas[4]; 
  v_signed_corner_areas( areas, coordinates );

  if( areas[0] < 0 || areas[1] < 0 || 
      areas[2] < 0 || areas[3] < 0 )
  {
    max_angle = 360 - max_angle;
  } 

  if( max_angle  > 0 )
    return (double) VERDICT_MIN( max_angle, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( max_angle, -VERDICT_DBL_MAX );
}

/*!
  the smallest angle of a quad

  smallest included quad angle (degrees)
*/
C_FUNC_DEF double v_quad_minimum_angle( int /*num_nodes*/, double coordinates[][3] )
{
  // if this quad is a collapsed quad, then just
  // send it to the tri_smallest_angle routine 
  if ( v_is_collapsed_quad(coordinates) == VERDICT_TRUE )
    return v_tri_minimum_angle(3, coordinates);

  double angle;
  double min_angle = 360.0;
  
  VerdictVector edges[4];
  edges[0].set(
      coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]
      );
  edges[1].set(
      coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2]
      );
  edges[2].set(
      coordinates[3][0] - coordinates[2][0],
      coordinates[3][1] - coordinates[2][1],
      coordinates[3][2] - coordinates[2][2]
      );
  edges[3].set(
      coordinates[0][0] - coordinates[3][0],
      coordinates[0][1] - coordinates[3][1],
      coordinates[0][2] - coordinates[3][2]
      );

  // go around each node and calculate the angle
  // at each node
  double length[4];
  length[0] = edges[0].length();
  length[1] = edges[1].length();
  length[2] = edges[2].length();
  length[3] = edges[3].length();

  if( length[0] <= VERDICT_DBL_MIN ||
      length[1] <= VERDICT_DBL_MIN ||
      length[2] <= VERDICT_DBL_MIN ||
      length[3] <= VERDICT_DBL_MIN )
    return 360.0;  

  angle = acos( -(edges[0] % edges[1])/(length[0]*length[1]) );
  min_angle = VERDICT_MIN(angle, min_angle);

  angle = acos( -(edges[1] % edges[2])/(length[1]*length[2]) );
  min_angle = VERDICT_MIN(angle, min_angle);

  angle = acos( -(edges[2] % edges[3])/(length[2]*length[3]) );
  min_angle = VERDICT_MIN(angle, min_angle);

  angle = acos( -(edges[3] % edges[0])/(length[3]*length[0]) );
  min_angle = VERDICT_MIN(angle, min_angle);

  min_angle = min_angle *180.0/VERDICT_PI;

  if( min_angle  > 0 )
    return (double) VERDICT_MIN( min_angle, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_angle, -VERDICT_DBL_MAX );
}

/*!
  the oddy of a quad

  general distortion measure based on left Cauchy-Green Tensor
*/
C_FUNC_DEF double v_quad_oddy( int /*num_nodes*/, double coordinates[][3] )
{
  
  double max_oddy = 0.;
  
  VerdictVector first, second, node_pos[4];
  
  double g, g11, g12, g22, cur_oddy;
  int i;
  
  for(i = 0; i < 4; i++ )
    node_pos[i].set(coordinates[i][0], coordinates[i][1], coordinates[i][2]);
 

  for ( i = 0; i < 4; i++ )
  {
    first  = node_pos[i] - node_pos[(i+1)%4];
    second = node_pos[i] - node_pos[(i+3)%4];
    
    g11 = first % first;
    g12 = first % second;
    g22 = second % second;
    g = g11*g22 - g12*g12;
    
    if ( g < VERDICT_DBL_MIN ) 
      cur_oddy = VERDICT_DBL_MAX; 
    else 
      cur_oddy = ( (g11-g22)*(g11-g22) + 4.*g12*g12 ) / 2. / g;
  
    max_oddy = VERDICT_MAX(max_oddy, cur_oddy);
  }
  
  if( max_oddy  > 0 )
    return (double) VERDICT_MIN( max_oddy, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( max_oddy, -VERDICT_DBL_MAX );
}


/*!
  the condition of a quad

  maximum condition number of the Jacobian matrix at 4 corners
*/
C_FUNC_DEF double v_quad_condition( int /*num_nodes*/, double coordinates[][3] )
{

  if ( v_is_collapsed_quad( coordinates ) == VERDICT_TRUE ) 
    return v_tri_condition(3,coordinates);
 
  double areas[4]; 
  v_signed_corner_areas( areas, coordinates );

  double max_condition = 0.;
  
  VerdictVector xxi, xet;
  
  double condition;
  
  for ( int i=0; i<4; i++ ) 
  {
    
    xxi.set( coordinates[i][0] - coordinates[(i+1)%4][0],
        coordinates[i][1] - coordinates[(i+1)%4][1],
        coordinates[i][2] - coordinates[(i+1)%4][2] );
    
    xet.set( coordinates[i][0] - coordinates[(i+3)%4][0],
        coordinates[i][1] - coordinates[(i+3)%4][1],
        coordinates[i][2] - coordinates[(i+3)%4][2] );
    
    if ( areas[i] <  VERDICT_DBL_MIN )
      condition = VERDICT_DBL_MAX;
    else 
      condition = ( xxi % xxi + xet % xet ) / areas[i];
    
    max_condition = VERDICT_MAX(max_condition, condition);
  }
  
  max_condition /= 2;

  if( max_condition > 0 )
    return (double) VERDICT_MIN( max_condition, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( max_condition, -VERDICT_DBL_MAX );
}

/*!
  the jacobian of a quad

  minimum pointwise volume of local map at 4 corners and center of quad
*/
C_FUNC_DEF double v_quad_jacobian( int /*num_nodes*/, double coordinates[][3] )
{
   
  if ( v_is_collapsed_quad( coordinates ) == VERDICT_TRUE )
    return (double)(v_tri_area(3, coordinates) * 2.0);
  
  double areas[4]; 
  v_signed_corner_areas( areas, coordinates );

  double jacobian = VERDICT_MIN( VERDICT_MIN( areas[0], areas[1] ), 
                                 VERDICT_MIN( areas[2], areas[3] ) );
  if( jacobian > 0 )
    return (double) VERDICT_MIN( jacobian, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( jacobian, -VERDICT_DBL_MAX );

}


/*!
  scaled jacobian of a quad

  Minimum Jacobian divided by the lengths of the 2 edge vector
*/
C_FUNC_DEF double v_quad_scaled_jacobian( int /*num_nodes*/, double coordinates[][3] )
{
  if ( v_is_collapsed_quad( coordinates ) == VERDICT_TRUE ) 
    return v_tri_scaled_jacobian(3, coordinates);
 
  double corner_areas[4], min_scaled_jac = VERDICT_DBL_MAX, scaled_jac;
  v_signed_corner_areas( corner_areas, coordinates );

  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

  double length[4];
  length[0] = edges[0].length();
  length[1] = edges[1].length();
  length[2] = edges[2].length();
  length[3] = edges[3].length();

  if( length[0] < VERDICT_DBL_MIN ||
      length[1] < VERDICT_DBL_MIN ||
      length[2] < VERDICT_DBL_MIN ||
      length[3] < VERDICT_DBL_MIN )
    return 0.0;  


  scaled_jac = corner_areas[0] / (length[0] * length[3]);
  min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac );

  scaled_jac = corner_areas[1] / (length[1] * length[0]);
  min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac );

  scaled_jac = corner_areas[2] / (length[2] * length[1]);
  min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac );

  scaled_jac = corner_areas[3] / (length[3] * length[2]);
  min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac );

  if( min_scaled_jac > 0 )
    return (double) VERDICT_MIN( min_scaled_jac, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_scaled_jac, -VERDICT_DBL_MAX );

}

/*!
  the shear of a quad

  2/Condition number of Jacobian Skew matrix
*/
C_FUNC_DEF double v_quad_shear( int /*num_nodes*/, double coordinates[][3] )
{
  double scaled_jacobian = v_quad_scaled_jacobian( 4, coordinates );

  if( scaled_jacobian <= VERDICT_DBL_MIN )
    return 0.0;
  else
    return (double) VERDICT_MIN( scaled_jacobian, VERDICT_DBL_MAX );
}

/*!
  the shape of a quad

   2/Condition number of weighted Jacobian matrix
*/
C_FUNC_DEF double v_quad_shape( int /*num_nodes*/, double coordinates[][3] )
{

  double corner_areas[4], min_shape = VERDICT_DBL_MAX, shape; 
  v_signed_corner_areas( corner_areas, coordinates );

  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

  double length_squared[4];
  length_squared[0] = edges[0].length_squared();
  length_squared[1] = edges[1].length_squared();
  length_squared[2] = edges[2].length_squared();
  length_squared[3] = edges[3].length_squared();

  if( length_squared[0] <= VERDICT_DBL_MIN ||
      length_squared[1] <= VERDICT_DBL_MIN ||
      length_squared[2] <= VERDICT_DBL_MIN ||
      length_squared[3] <= VERDICT_DBL_MIN )
    return 0.0;  

  shape = corner_areas[0] / (length_squared[0] + length_squared[3]);
  min_shape = VERDICT_MIN( shape, min_shape );

  shape = corner_areas[1] / (length_squared[1] + length_squared[0]);
  min_shape = VERDICT_MIN( shape, min_shape );

  shape = corner_areas[2] / (length_squared[2] + length_squared[1]);
  min_shape = VERDICT_MIN( shape, min_shape );

  shape = corner_areas[3] / (length_squared[3] + length_squared[2]);
  min_shape = VERDICT_MIN( shape, min_shape );

  min_shape *= 2;

  if( min_shape < VERDICT_DBL_MIN )
    min_shape = 0;

  if( min_shape > 0 )
    return (double) VERDICT_MIN( min_shape, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_shape, -VERDICT_DBL_MAX );

}

/*!
  the relative size of a quad

  Min( J, 1/J ), where J is determinant of weighted Jacobian matrix
*/
C_FUNC_DEF double v_quad_relative_size_squared( int /*num_nodes*/, double coordinates[][3] )
{
 
  double quad_area = v_quad_area (4, coordinates); 
  double rel_size = 0;
  
  v_set_quad_size( quad_area );
  double w11,w21,w12,w22;
  v_quad_get_weight(w11,w21,w12,w22);
  double avg_area = v_determinant(w11,w21,w12,w22);
  
  if ( avg_area > VERDICT_DBL_MIN ) 
  {
    
    w11 = quad_area / avg_area;
      
    if ( w11 > VERDICT_DBL_MIN )
    {
      rel_size = VERDICT_MIN( w11, 1/w11 );
      rel_size *= rel_size;
    }
  }
  
  if( rel_size  > 0 )
    return (double) VERDICT_MIN( rel_size, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( rel_size, -VERDICT_DBL_MAX );

}

/*!
  the relative shape and size of a quad

  Product of Shape and Relative Size
*/
C_FUNC_DEF double v_quad_shape_and_size( int num_nodes, double coordinates[][3] )
{
  double shape, size;
  size = v_quad_relative_size_squared( num_nodes, coordinates );
  shape = v_quad_shape( num_nodes, coordinates );

  double shape_and_size = shape * size;
  
  if( shape_and_size > 0 )
    return (double) VERDICT_MIN( shape_and_size, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( shape_and_size, -VERDICT_DBL_MAX );

}

/*!
  the shear and size of a quad

  product of shear and relative size
*/
C_FUNC_DEF double v_quad_shear_and_size( int num_nodes, double coordinates[][3] )
{
  double shear, size;
  shear = v_quad_shear( num_nodes, coordinates );
  size = v_quad_relative_size_squared( num_nodes, coordinates );

  double shear_and_size = shear * size;

  if( shear_and_size > 0 )
    return (double) VERDICT_MIN( shear_and_size, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( shear_and_size, -VERDICT_DBL_MAX );

}

/*!
  the distortion of a quad
*/
C_FUNC_DEF double v_quad_distortion( int num_nodes, double coordinates[][3] )
{
  // To calculate distortion for linear and 2nd order quads
  // distortion = {min(|J|)/actual area}*{parent area}
  // parent area = 4 for a quad.
  // min |J| is the minimum over nodes and gaussian integration points
  // created by Ling Pan, CAT on 4/30/01

  double element_area =0.0,distrt,thickness_gauss;
  double cur_jacobian=0., sign_jacobian, jacobian ;
  VerdictVector  aa, bb, cc,normal_at_point, xin;


  //use 2x2 gauss points for linear quads and 3x3 for 2nd order quads
  int number_of_gauss_points = 0;
  if ( num_nodes == 4 )
    { //2x2 quadrature rule
    number_of_gauss_points = 2;
    }
  else if ( num_nodes == 8 )
    { //3x3 quadrature rule
    number_of_gauss_points = 3;
    }


  int total_number_of_gauss_points = number_of_gauss_points*number_of_gauss_points;

  VerdictVector face_normal = v_quad_normal( coordinates );

  double distortion = VERDICT_DBL_MAX;

  VerdictVector first, second;

  int i;
  //Will work out the case for collapsed quad later
  if ( v_is_collapsed_quad( coordinates ) == VERDICT_TRUE )
    {
    for (  i=0; i<3; i++ )
      {

      first.set( coordinates[i][0] - coordinates[(i+1)%3][0],
        coordinates[i][1] - coordinates[(i+1)%3][1],
        coordinates[i][2] - coordinates[(i+1)%3][2] );

      second.set( coordinates[i][0] - coordinates[(i+2)%3][0],
        coordinates[i][1] - coordinates[(i+2)%3][1],
        coordinates[i][2] - coordinates[(i+2)%3][2] );

      sign_jacobian = (face_normal % ( first * second )) > 0? 1.:-1.;
      cur_jacobian = sign_jacobian*(first * second).length();
      distortion = VERDICT_MIN(distortion, cur_jacobian);
      }
    element_area = (first*second).length()/2.0;
    distortion /= element_area;
    }
  else
    {
    double shape_function[maxTotalNumberGaussPoints][maxNumberNodes];
    double dndy1[maxTotalNumberGaussPoints][maxNumberNodes];
    double dndy2[maxTotalNumberGaussPoints][maxNumberNodes];
    double weight[maxTotalNumberGaussPoints];

    //create an object of GaussIntegration
    GaussIntegration::initialize(number_of_gauss_points,num_nodes );
    GaussIntegration::calculate_shape_function_2d_quad();
    GaussIntegration::get_shape_func(shape_function[0], dndy1[0], dndy2[0], weight);

    // calculate element area
    int ife,ja;
    for ( ife=0;ife<total_number_of_gauss_points; ife++)
      {
      aa.set(0.0,0.0,0.0);
      bb.set(0.0,0.0,0.0);

      for (ja=0;ja<num_nodes;ja++)
        {
        xin.set(coordinates[ja][0], coordinates[ja][1], coordinates[ja][2]);
        aa += dndy1[ife][ja]*xin;
        bb += dndy2[ife][ja]*xin;
        }
      normal_at_point = aa*bb;
      jacobian = normal_at_point.length();
      element_area += weight[ife]*jacobian;
      }


    double dndy1_at_node[maxNumberNodes][maxNumberNodes];
    double dndy2_at_node[maxNumberNodes][maxNumberNodes];

    GaussIntegration::calculate_derivative_at_nodes( dndy1_at_node,  dndy2_at_node);

    VerdictVector normal_at_nodes[9];


    //evaluate normal at nodes and distortion values at nodes
    int jai;
    for (ja =0; ja<num_nodes; ja++)
      {
      aa.set(0.0,0.0,0.0);
      bb.set(0.0,0.0,0.0);
      for (jai =0; jai<num_nodes; jai++)
        {
        xin.set(coordinates[jai][0], coordinates[jai][1], coordinates[jai][2]);
        aa += dndy1_at_node[ja][jai]*xin;
        bb += dndy2_at_node[ja][jai]*xin;
        }
      normal_at_nodes[ja] = aa*bb;
      normal_at_nodes[ja].normalize();

      }

    //determine if element is flat
    bool flat_element =true;
    double dot_product;

    for ( ja=0; ja<num_nodes;ja++)
      {
      dot_product = normal_at_nodes[0]%normal_at_nodes[ja];
      if (fabs(dot_product) <0.99)
        {
        flat_element = false;
        break;
        }
      }

    // take into consideration of the thickness of the element
    double thickness;
    //get_quad_thickness(face, element_area, thickness );
    thickness = 0.001*sqrt(element_area);

    //set thickness gauss point location
    double zl = 0.5773502691896;
    if (flat_element) zl =0.0;

    int no_gauss_pts_z = (flat_element)? 1 : 2;
    double thickness_z;
    int igz;
    //loop on Gauss points
    for (ife=0;ife<total_number_of_gauss_points;ife++)
      {
      //loop on the thickness direction gauss points
      for ( igz=0;igz<no_gauss_pts_z;igz++)
        {
        zl = -zl;
        thickness_z = zl*thickness/2.0;

        aa.set(0.0,0.0,0.0);
        bb.set(0.0,0.0,0.0);
        cc.set(0.0,0.0,0.0);

        for (ja=0;ja<num_nodes;ja++)
          {
          xin.set(coordinates[ja][0], coordinates[ja][1], coordinates[ja][2]);
          xin += thickness_z*normal_at_nodes[ja];
          aa  += dndy1[ife][ja]*xin;
          bb  += dndy2[ife][ja]*xin;
          thickness_gauss = shape_function[ife][ja]*thickness/2.0;
          cc  += thickness_gauss*normal_at_nodes[ja];
          }

        normal_at_point = aa*bb;
        jacobian = normal_at_point.length();
        distrt = cc%normal_at_point;
        if (distrt < distortion) distortion = distrt;
        }
      }

    //loop through nodal points
    for ( ja =0; ja<num_nodes; ja++)
      {
      for ( igz=0;igz<no_gauss_pts_z;igz++)
        {
        zl = -zl;
        thickness_z = zl*thickness/2.0;

        aa.set(0.0,0.0,0.0);
        bb.set(0.0,0.0,0.0);
        cc.set(0.0,0.0,0.0);

        for ( jai =0; jai<num_nodes; jai++)
          {
          xin.set(coordinates[jai][0], coordinates[jai][1], coordinates[jai][2]);
          xin += thickness_z*normal_at_nodes[ja];
          aa += dndy1_at_node[ja][jai]*xin;
          bb += dndy2_at_node[ja][jai]*xin;
          if (jai == ja)
            thickness_gauss = thickness/2.0;
          else
            thickness_gauss = 0.;
          cc  += thickness_gauss*normal_at_nodes[jai];
          }

        }
      normal_at_point = aa*bb;
      sign_jacobian = (face_normal % normal_at_point) > 0 ? 1. : -1.;
      distrt = sign_jacobian * (cc % normal_at_point);

      if (distrt < distortion) distortion = distrt;
      }

    if ( element_area * thickness != 0 )
      distortion *= 8. / ( element_area * thickness );
    else
      distortion *= 8.;

    }

  return (double)distortion;
}

/*!
  multiple quality measures of a quad
*/
C_FUNC_DEF void v_quad_quality( int num_nodes, double coordinates[][3], 
    unsigned int metrics_request_flag, QuadMetricVals *metric_vals )
{

  memset( metric_vals, 0, sizeof(QuadMetricVals) );

  // for starts, lets set up some basic and common information

  /*  node numbers and side numbers used below

                  2
            3 +--------- 2
             /         +
            /          |
         3 /           | 1
          /            |
         +             |
       0 -------------+ 1
             0
  */
  
  // vectors for each side
  VerdictVector edges[4];
  v_make_quad_edges( edges, coordinates );

  double areas[4]; 
  v_signed_corner_areas( areas, coordinates );

  double lengths[4];
  lengths[0] = edges[0].length();
  lengths[1] = edges[1].length();
  lengths[2] = edges[2].length();
  lengths[3] = edges[3].length();

  VerdictBoolean is_collapsed = v_is_collapsed_quad(coordinates);

  // handle collapsed quads functions here
  if(is_collapsed == VERDICT_TRUE && metrics_request_flag & 
      ( V_QUAD_MINIMUM_ANGLE | V_QUAD_MAXIMUM_ANGLE | V_QUAD_JACOBIAN |
        V_QUAD_SCALED_JACOBIAN ))
  {
    if(metrics_request_flag & V_QUAD_MINIMUM_ANGLE)
      metric_vals->minimum_angle = v_tri_minimum_angle(3, coordinates);
    if(metrics_request_flag & V_QUAD_MAXIMUM_ANGLE)
      metric_vals->maximum_angle = v_tri_maximum_angle(3, coordinates);
    if(metrics_request_flag & V_QUAD_JACOBIAN)
      metric_vals->jacobian = (double)(v_tri_area(3, coordinates) * 2.0);
    if(metrics_request_flag & V_QUAD_SCALED_JACOBIAN)
      metric_vals->jacobian = (double)(v_tri_scaled_jacobian(3, coordinates) * 2.0);
  }
  
  // calculate both largest and smallest angles
  if(metrics_request_flag & (V_QUAD_MINIMUM_ANGLE | V_QUAD_MAXIMUM_ANGLE) 
      && is_collapsed == VERDICT_FALSE )
  {
    // gather the angles
    double angles[4];
    angles[0] = acos( -(edges[0] % edges[1])/(lengths[0]*lengths[1]) );
    angles[1] = acos( -(edges[1] % edges[2])/(lengths[1]*lengths[2]) );
    angles[2] = acos( -(edges[2] % edges[3])/(lengths[2]*lengths[3]) );
    angles[3] = acos( -(edges[3] % edges[0])/(lengths[3]*lengths[0]) );

    if( lengths[0] <= VERDICT_DBL_MIN ||
        lengths[1] <= VERDICT_DBL_MIN ||
        lengths[2] <= VERDICT_DBL_MIN ||
        lengths[3] <= VERDICT_DBL_MIN )
    {
      metric_vals->minimum_angle = 360.0;
      metric_vals->maximum_angle = 0.0;
    }
    else
    {
      // if smallest angle, find the smallest angle
      if(metrics_request_flag & V_QUAD_MINIMUM_ANGLE)
      {
        metric_vals->minimum_angle = VERDICT_DBL_MAX;
        for(int i = 0; i<4; i++)
          metric_vals->minimum_angle = VERDICT_MIN(angles[i], metric_vals->minimum_angle);
        metric_vals->minimum_angle *= 180.0 / VERDICT_PI;
      }
      // if largest angle, find the largest angle
      if(metrics_request_flag & V_QUAD_MAXIMUM_ANGLE)
      {
        metric_vals->maximum_angle = 0.0;
        for(int i = 0; i<4; i++)
          metric_vals->maximum_angle = VERDICT_MAX(angles[i], metric_vals->maximum_angle);
        metric_vals->maximum_angle *= 180.0 / VERDICT_PI;

        if( areas[0] < 0 || areas[1] < 0 || 
            areas[2] < 0 || areas[3] < 0 )
          metric_vals->maximum_angle = 360 - metric_vals->maximum_angle;
      }
    }
  }

  // handle max_edge_ratio, skew, taper, and area together
  if( metrics_request_flag & ( V_QUAD_MAX_EDGE_RATIO | V_QUAD_SKEW | V_QUAD_TAPER ) )
  {
    //get principle axes
    VerdictVector principal_axes[2];
    principal_axes[0] = edges[0] - edges[2];
    principal_axes[1] = edges[1] - edges[3];

    if(metrics_request_flag & (V_QUAD_MAX_EDGE_RATIO | V_QUAD_SKEW | V_QUAD_TAPER))
    {
      double len1 = principal_axes[0].length();
      double len2 = principal_axes[1].length();

      // calculate the max_edge_ratio ratio 
      if(metrics_request_flag & V_QUAD_MAX_EDGE_RATIO)
      {
        if( len1 < VERDICT_DBL_MIN || len2 < VERDICT_DBL_MIN )
          metric_vals->max_edge_ratio = VERDICT_DBL_MAX;
        else
          metric_vals->max_edge_ratio = VERDICT_MAX( len1 / len2, len2 / len1 );
      }
    
      // calculate the taper
      if(metrics_request_flag & V_QUAD_TAPER)
      {
        double min_length = VERDICT_MIN( len1, len2 );

        VerdictVector cross_derivative = edges[1] + edges[3]; 

        if( min_length < VERDICT_DBL_MIN )
          metric_vals->taper = VERDICT_DBL_MAX;
        else
          metric_vals->taper = cross_derivative.length()/ min_length;
      }
      
      // calculate the skew 
      if(metrics_request_flag & V_QUAD_SKEW)
      {
        if( principal_axes[0].normalize() < VERDICT_DBL_MIN ||
            principal_axes[1].normalize() < VERDICT_DBL_MIN )
          metric_vals->skew = 0.0; 
        else
          metric_vals->skew = fabs( principal_axes[0] % principal_axes[1] );
      }
    }
  }

  // calculate the area
  if(metrics_request_flag & (V_QUAD_AREA | V_QUAD_RELATIVE_SIZE_SQUARED) )
  {
    metric_vals->area = 0.25 * (areas[0] + areas[1] + areas[2] + areas[3]);
  }

  // calculate the relative size 
  if(metrics_request_flag & (V_QUAD_RELATIVE_SIZE_SQUARED | V_QUAD_SHAPE_AND_SIZE |
                             V_QUAD_SHEAR_AND_SIZE ) )
  {
    double quad_area = v_quad_area (4, coordinates); 
    v_set_quad_size( quad_area );
    double w11,w21,w12,w22;
    v_quad_get_weight(w11,w21,w12,w22);
    double avg_area = v_determinant(w11,w21,w12,w22);

    if( avg_area < VERDICT_DBL_MIN )
      metric_vals->relative_size_squared = 0.0;
    else
      metric_vals->relative_size_squared =  pow( VERDICT_MIN( 
                                              metric_vals->area/avg_area,  
                                              avg_area/metric_vals->area ), 2 );
  }

  // calculate the jacobian
  if(metrics_request_flag & V_QUAD_JACOBIAN)
  {
    metric_vals->jacobian = VERDICT_MIN(
                              VERDICT_MIN( areas[0], areas[1] ),
                              VERDICT_MIN( areas[2], areas[3] ) );
  }

  if( metrics_request_flag & ( V_QUAD_SCALED_JACOBIAN | V_QUAD_SHEAR | V_QUAD_SHEAR_AND_SIZE ) )
  {
    double scaled_jac, min_scaled_jac = VERDICT_DBL_MAX;
    
    if( lengths[0] < VERDICT_DBL_MIN ||
        lengths[1] < VERDICT_DBL_MIN ||
        lengths[2] < VERDICT_DBL_MIN ||
        lengths[3] < VERDICT_DBL_MIN )
    {
      metric_vals->scaled_jacobian = 0.0;  
      metric_vals->shear = 0.0;  
    }
    else
    {
      scaled_jac = areas[0] / (lengths[0] * lengths[3]);
      min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac );

      scaled_jac = areas[1] / (lengths[1] * lengths[0]);
      min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac );

      scaled_jac = areas[2] / (lengths[2] * lengths[1]);
      min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac );

      scaled_jac = areas[3] / (lengths[3] * lengths[2]);
      min_scaled_jac = VERDICT_MIN( scaled_jac, min_scaled_jac );

      metric_vals->scaled_jacobian = min_scaled_jac;
     
      //what the heck...set shear as well 
      if( min_scaled_jac <= VERDICT_DBL_MIN )
        metric_vals->shear = 0.0;
      else
        metric_vals->shear = min_scaled_jac;
    }
  }

  if( metrics_request_flag & (V_QUAD_WARPAGE | V_QUAD_ODDY) )
  {
    VerdictVector corner_normals[4];
    corner_normals[0] = edges[3] * edges[0];
    corner_normals[1] = edges[0] * edges[1];
    corner_normals[2] = edges[1] * edges[2];
    corner_normals[3] = edges[2] * edges[3];

    if( metrics_request_flag & V_QUAD_ODDY )
    {
      double oddy, max_oddy = 0.0;

      double diff, dot_prod;

      double length_squared[4];
      length_squared[0] = corner_normals[0].length_squared();
      length_squared[1] = corner_normals[1].length_squared();
      length_squared[2] = corner_normals[2].length_squared();
      length_squared[3] = corner_normals[3].length_squared();

      if( length_squared[0] < VERDICT_DBL_MIN ||
          length_squared[1] < VERDICT_DBL_MIN ||
          length_squared[2] < VERDICT_DBL_MIN ||
          length_squared[3] < VERDICT_DBL_MIN )
       metric_vals->oddy = VERDICT_DBL_MAX; 
      else
      {
        diff = (lengths[0]*lengths[0]) - (lengths[1]*lengths[1]);
        dot_prod = edges[0]%edges[1];
        oddy = ((diff*diff) + 4*dot_prod*dot_prod ) / (2*length_squared[1]);
        max_oddy = VERDICT_MAX( oddy, max_oddy );

        diff = (lengths[1]*lengths[1]) - (lengths[2]*lengths[2]);
        dot_prod = edges[1]%edges[2];
        oddy = ((diff*diff) + 4*dot_prod*dot_prod ) / (2*length_squared[2]);
        max_oddy = VERDICT_MAX( oddy, max_oddy );

        diff = (lengths[2]*lengths[2]) - (lengths[3]*lengths[3]);
        dot_prod = edges[2]%edges[3];
        oddy = ((diff*diff) + 4*dot_prod*dot_prod ) / (2*length_squared[3]);
        max_oddy = VERDICT_MAX( oddy, max_oddy );

        diff = (lengths[3]*lengths[3]) - (lengths[0]*lengths[0]);
        dot_prod = edges[3]%edges[0];
        oddy = ((diff*diff) + 4*dot_prod*dot_prod ) / (2*length_squared[0]);
        max_oddy = VERDICT_MAX( oddy, max_oddy );

        metric_vals->oddy = max_oddy;
      }
    }

    if( metrics_request_flag & V_QUAD_WARPAGE )
    {
      if( corner_normals[0].normalize() < VERDICT_DBL_MIN ||
          corner_normals[1].normalize() < VERDICT_DBL_MIN ||
          corner_normals[2].normalize() < VERDICT_DBL_MIN ||
          corner_normals[3].normalize() < VERDICT_DBL_MIN )
        metric_vals->warpage = VERDICT_DBL_MAX;
      else 
      {
        metric_vals->warpage = pow( 
                             VERDICT_MIN( corner_normals[0]%corner_normals[2],
                                          corner_normals[1]%corner_normals[3]), 3 ); 
      }
    }
  }

  if( metrics_request_flag & V_QUAD_STRETCH )
  {
    VerdictVector temp;

    temp.set( coordinates[2][0] - coordinates[0][0],
              coordinates[2][1] - coordinates[0][1],
              coordinates[2][2] - coordinates[0][2]);
    double diag02 = temp.length_squared();

    temp.set( coordinates[3][0] - coordinates[1][0],
              coordinates[3][1] - coordinates[1][1],
              coordinates[3][2] - coordinates[1][2]);
    double diag13 = temp.length_squared();
    
    static const double QUAD_STRETCH_FACTOR = sqrt(2.0);

    // 'diag02' is now the max diagonal of the quad
    diag02 = VERDICT_MAX( diag02, diag13 );

    if( diag02 < VERDICT_DBL_MIN )
      metric_vals->stretch = VERDICT_DBL_MAX; 
    else
      metric_vals->stretch =  QUAD_STRETCH_FACTOR *
                              VERDICT_MIN(
                                VERDICT_MIN( lengths[0], lengths[1] ),
                                VERDICT_MIN( lengths[2], lengths[3] ) ) /
                              sqrt(diag02);
  }

  if(metrics_request_flag & (V_QUAD_CONDITION | V_QUAD_SHAPE | V_QUAD_SHAPE_AND_SIZE ) )
  {
    double lengths_squared[4];
    lengths_squared[0] = edges[0].length_squared();
    lengths_squared[1] = edges[1].length_squared();
    lengths_squared[2] = edges[2].length_squared();
    lengths_squared[3] = edges[3].length_squared();

    if( areas[0] < VERDICT_DBL_MIN ||
        areas[1] < VERDICT_DBL_MIN ||
        areas[2] < VERDICT_DBL_MIN ||
        areas[3] < VERDICT_DBL_MIN )
    {
      metric_vals->condition = VERDICT_DBL_MAX;
      metric_vals->shape= 0.0;
    }
    else
    {
      double max_condition = 0.0, condition;

      condition = (lengths_squared[0] + lengths_squared[3])/areas[0];
      max_condition = VERDICT_MAX( max_condition, condition ); 

      condition = (lengths_squared[1] + lengths_squared[0])/areas[1];
      max_condition = VERDICT_MAX( max_condition, condition ); 
     
      condition = (lengths_squared[2] + lengths_squared[1])/areas[2];
      max_condition = VERDICT_MAX( max_condition, condition ); 

      condition = (lengths_squared[3] + lengths_squared[2])/areas[3];
      max_condition = VERDICT_MAX( max_condition, condition ); 

      metric_vals->condition = 0.5*max_condition;
      metric_vals->shape =  2/max_condition;
    }
  } 

  if(metrics_request_flag & V_QUAD_AREA )
  {
    if( metric_vals->area > 0 ) 
      metric_vals->area = (double) VERDICT_MIN( metric_vals->area, VERDICT_DBL_MAX );
    metric_vals->area = (double) VERDICT_MAX( metric_vals->area, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_MAX_EDGE_RATIO )
  {
    if( metric_vals->max_edge_ratio > 0 ) 
      metric_vals->max_edge_ratio = (double) VERDICT_MIN( metric_vals->max_edge_ratio, VERDICT_DBL_MAX );
    metric_vals->max_edge_ratio = (double) VERDICT_MAX( metric_vals->max_edge_ratio, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_CONDITION )
  {
    if( metric_vals->condition > 0 ) 
      metric_vals->condition = (double) VERDICT_MIN( metric_vals->condition, VERDICT_DBL_MAX );
    metric_vals->condition = (double) VERDICT_MAX( metric_vals->condition, -VERDICT_DBL_MAX );
  }

  // calculate distortion
  if(metrics_request_flag & V_QUAD_DISTORTION)
  {
    metric_vals->distortion = v_quad_distortion(num_nodes, coordinates);

    if( metric_vals->distortion > 0 ) 
      metric_vals->distortion = (double) VERDICT_MIN( metric_vals->distortion, VERDICT_DBL_MAX );
    metric_vals->distortion = (double) VERDICT_MAX( metric_vals->distortion, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_JACOBIAN )
  {
    if( metric_vals->jacobian > 0 ) 
      metric_vals->jacobian = (double) VERDICT_MIN( metric_vals->jacobian, VERDICT_DBL_MAX );
    metric_vals->jacobian = (double) VERDICT_MAX( metric_vals->jacobian, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_MAXIMUM_ANGLE )
  {
    if( metric_vals->maximum_angle > 0 ) 
      metric_vals->maximum_angle = (double) VERDICT_MIN( metric_vals->maximum_angle, VERDICT_DBL_MAX );
    metric_vals->maximum_angle = (double) VERDICT_MAX( metric_vals->maximum_angle, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_MINIMUM_ANGLE )
  {
    if( metric_vals->minimum_angle > 0 ) 
      metric_vals->minimum_angle = (double) VERDICT_MIN( metric_vals->minimum_angle, VERDICT_DBL_MAX );
    metric_vals->minimum_angle = (double) VERDICT_MAX( metric_vals->minimum_angle, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_ODDY )
  {
    if( metric_vals->oddy > 0 ) 
      metric_vals->oddy = (double) VERDICT_MIN( metric_vals->oddy, VERDICT_DBL_MAX );
    metric_vals->oddy = (double) VERDICT_MAX( metric_vals->oddy, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_RELATIVE_SIZE_SQUARED )
  {
    if( metric_vals->relative_size_squared> 0 ) 
      metric_vals->relative_size_squared = (double) VERDICT_MIN( metric_vals->relative_size_squared, VERDICT_DBL_MAX );
    metric_vals->relative_size_squared = (double) VERDICT_MAX( metric_vals->relative_size_squared, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_SCALED_JACOBIAN )
  {
    if( metric_vals->scaled_jacobian> 0 ) 
      metric_vals->scaled_jacobian = (double) VERDICT_MIN( metric_vals->scaled_jacobian, VERDICT_DBL_MAX );
    metric_vals->scaled_jacobian = (double) VERDICT_MAX( metric_vals->scaled_jacobian, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_SHEAR )
  {
    if( metric_vals->shear > 0 ) 
      metric_vals->shear = (double) VERDICT_MIN( metric_vals->shear, VERDICT_DBL_MAX );
    metric_vals->shear = (double) VERDICT_MAX( metric_vals->shear, -VERDICT_DBL_MAX );
  }

  // calculate shear and size
  // reuse values from above
  if(metrics_request_flag & V_QUAD_SHEAR_AND_SIZE)
  {
    metric_vals->shear_and_size = metric_vals->shear * metric_vals->relative_size_squared;

    if( metric_vals->shear_and_size > 0 ) 
      metric_vals->shear_and_size = (double) VERDICT_MIN( metric_vals->shear_and_size, VERDICT_DBL_MAX );
    metric_vals->shear_and_size = (double) VERDICT_MAX( metric_vals->shear_and_size, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_SHAPE )
  {
    if( metric_vals->shape > 0 ) 
      metric_vals->shape = (double) VERDICT_MIN( metric_vals->shape, VERDICT_DBL_MAX );
    metric_vals->shape = (double) VERDICT_MAX( metric_vals->shape, -VERDICT_DBL_MAX );
  }

  // calculate shape and size
  // reuse values from above
  if(metrics_request_flag & V_QUAD_SHAPE_AND_SIZE)
  {
    metric_vals->shape_and_size = metric_vals->shape * metric_vals->relative_size_squared;

    if( metric_vals->shape_and_size > 0 ) 
      metric_vals->shape_and_size = (double) VERDICT_MIN( metric_vals->shape_and_size, VERDICT_DBL_MAX );
    metric_vals->shape_and_size = (double) VERDICT_MAX( metric_vals->shape_and_size, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_SKEW )
  {
    if( metric_vals->skew > 0 ) 
      metric_vals->skew = (double) VERDICT_MIN( metric_vals->skew, VERDICT_DBL_MAX );
    metric_vals->skew = (double) VERDICT_MAX( metric_vals->skew, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_STRETCH )
  {
    if( metric_vals->stretch > 0 ) 
      metric_vals->stretch = (double) VERDICT_MIN( metric_vals->stretch, VERDICT_DBL_MAX );
    metric_vals->stretch = (double) VERDICT_MAX( metric_vals->stretch, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_TAPER )
  {
    if( metric_vals->taper > 0 ) 
      metric_vals->taper = (double) VERDICT_MIN( metric_vals->taper, VERDICT_DBL_MAX );
    metric_vals->taper = (double) VERDICT_MAX( metric_vals->taper, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_QUAD_WARPAGE )
  {
    if( metric_vals->warpage > 0 ) 
      metric_vals->warpage = (double) VERDICT_MIN( metric_vals->warpage, VERDICT_DBL_MAX );
    metric_vals->warpage = (double) VERDICT_MAX( metric_vals->warpage, -VERDICT_DBL_MAX );
  }

}

