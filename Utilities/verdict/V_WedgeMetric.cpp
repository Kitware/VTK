/*=========================================================================

Module:    V_WedgeMetric.cpp

Copyright (c) 2006 Sandia Corporation.
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 *
 * WedgeMetric.cpp contains quality calculations for wedges
 *
 * This file is part of VERDICT
 *
 */


#include "verdict.h"
#include "VerdictVector.hpp"
#include <memory.h> 
#include "verdict_defines.hpp"
#include "V_GaussIntegration.hpp"
/*
   the wedge element


   5
   ^
   / \
   / | \
   / /2\ \
   6/_______\4
   | /   \ |
   |/_____\|
   3         1

 */



/*!

  calculate the volume of a wedge

  this is done by dividing the wedge into 3 tets
  and summing the volume of each tet

 */

C_FUNC_DEF double v_wedge_volume( int num_nodes, double coordinates[][3] )
{

  double volume = 0;
  VerdictVector side1, side2, side3;

  if ( num_nodes == 6 )
  {

    // divide the wedge into 3 tets and calculate each volume

    side1.set( coordinates[1][0] - coordinates[0][0],
        coordinates[1][1] - coordinates[0][1],
        coordinates[1][2] - coordinates[0][2]);

    side2.set( coordinates[2][0] - coordinates[0][0],
        coordinates[2][1] - coordinates[0][1],
        coordinates[2][2] - coordinates[0][2]);


    side3.set( coordinates[3][0] - coordinates[0][0],
        coordinates[3][1] - coordinates[0][1],
        coordinates[3][2] - coordinates[0][2]);

    volume = side3 % (side1 * side2)  / 6;

    side1.set( coordinates[4][0] - coordinates[1][0],
        coordinates[4][1] - coordinates[1][1],
        coordinates[4][2] - coordinates[1][2]);

    side2.set( coordinates[5][0] - coordinates[1][0],
        coordinates[5][1] - coordinates[1][1],
        coordinates[5][2] - coordinates[1][2]);


    side3.set( coordinates[3][0] - coordinates[1][0],
        coordinates[3][1] - coordinates[1][1],
        coordinates[3][2] - coordinates[1][2]);

    volume += side3 % (side1 * side2)  / 6;

    side1.set( coordinates[5][0] - coordinates[1][0],
        coordinates[5][1] - coordinates[1][1],
        coordinates[5][2] - coordinates[1][2]);

    side2.set( coordinates[2][0] - coordinates[1][0],
        coordinates[2][1] - coordinates[1][1],
        coordinates[2][2] - coordinates[1][2]);


    side3.set( coordinates[3][0] - coordinates[1][0],
        coordinates[3][1] - coordinates[1][1],
        coordinates[3][2] - coordinates[1][2]);

    volume += side3 % (side1 * side2)  / 6;

  }

  return (double)volume;

}



C_FUNC_DEF void v_wedge_quality( int num_nodes, double coordinates[][3], 
    unsigned int metrics_request_flag, WedgeMetricVals *metric_vals )
{
  memset( metric_vals, 0, sizeof(WedgeMetricVals) );

  if(metrics_request_flag & V_WEDGE_VOLUME)
    metric_vals->volume = v_wedge_volume(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_EDGE_RATIO)
    metric_vals->edge_ratio = v_wedge_edge_ratio(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_MAX_ASPECT_FROBENIUS)
    metric_vals->max_aspect_frobenius = v_wedge_max_aspect_frobenius(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_MEAN_ASPECT_FROBENIUS)
    metric_vals->mean_aspect_frobenius = v_wedge_mean_aspect_frobenius(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_JACOBIAN)
    metric_vals->jacobian = v_wedge_jacobian(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_SCALED_JACOBIAN)
    metric_vals->scaled_jacobian = v_wedge_scaled_jacobian(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_DISTORTION)
    metric_vals->distortion = v_wedge_distortion(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_MAX_STRETCH)
    metric_vals->max_stretch = v_wedge_max_stretch(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_SHAPE)
    metric_vals->shape = v_wedge_shape(num_nodes, coordinates);
  if(metrics_request_flag & V_WEDGE_CONDITION)
    metric_vals->condition = v_wedge_condition(num_nodes, coordinates);
}


/* Edge ratio
   The edge ratio quality metric is the ratio of the longest to shortest edge of
   a wedge.
   q = L_max / L_min

   Dimension : 1
   Acceptable range : --
   Normal range : [1,DBL_MAX]
   Full range : [1,DBL_MAX]
   q for right, unit wedge : 1
   Reference : -
   */
C_FUNC_DEF double v_wedge_edge_ratio( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector a,b,c,d,e,f,g,h,i;

  a.set( coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2] );

  b.set( coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2] );

  c.set( coordinates[0][0] - coordinates[2][0],
      coordinates[0][1] - coordinates[2][1],
      coordinates[0][2] - coordinates[2][2] );

  d.set( coordinates[4][0] - coordinates[3][0],
      coordinates[4][1] - coordinates[3][1],
      coordinates[4][2] - coordinates[3][2] );

  e.set( coordinates[5][0] - coordinates[4][0],
      coordinates[5][1] - coordinates[4][1],
      coordinates[5][2] - coordinates[4][2] );

  f.set( coordinates[3][0] - coordinates[5][0],
      coordinates[3][1] - coordinates[5][1],
      coordinates[3][2] - coordinates[5][2] );

  g.set( coordinates[3][0] - coordinates[0][0],
      coordinates[3][1] - coordinates[0][1],
      coordinates[3][2] - coordinates[0][2] );

  h.set( coordinates[4][0] - coordinates[1][0],
      coordinates[4][1] - coordinates[1][1],
      coordinates[4][2] - coordinates[1][2] );

  i.set( coordinates[5][0] - coordinates[2][0],
      coordinates[5][1] - coordinates[2][1],
      coordinates[5][2] - coordinates[2][2] );

  double a2 = a.length_squared();
  double b2 = b.length_squared();
  double c2 = c.length_squared();
  double d2 = d.length_squared();
  double e2 = e.length_squared();
  double f2 = f.length_squared();
  double g2 = g.length_squared();
  double h2 = h.length_squared();
  double i2 = i.length_squared();

  double max = a2, min = a2;

  if (max <= b2){max = b2;}
  if (b2 <= min){min = b2;}

  if (max <= c2){max = c2;}
  if (c2 <= min){min = c2;}

  if (max <= d2){max = d2;}
  if (d2 <= min){min = d2;}

  if (max <= e2){max = e2;}
  if (e2 <= min){min = e2;}

  if (max <= f2){max = f2;}
  if (f2 <= min){min = f2;}

  if (max <= g2){max = g2;}
  if (g2 <= min){min = g2;}

  if (max <= h2){max = h2;}
  if (h2 <= min){min = h2;}

  if (max <= i2){max = i2;}
  if (i2 <= min){min = i2;}

  double edge_ratio = sqrt( max / min );

  if( edge_ratio > 0 )
    return (double) VERDICT_MIN( edge_ratio, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( edge_ratio, -VERDICT_DBL_MAX );

}

/* For wedges, there is not a unique definition of the aspect Frobenius. Rather,
 * this metric uses the aspect Frobenius defined for tetrahedral (see section
 * 6.4) and is comparable in methodology to the maximum aspect Frobenius defined
 * for hexahedra (see section 7.7). This value is normalized for a unit wedge.

 q = max(F_0123, F_1204, F_2015, F_3540, F_4351, F_5432)

 This is also known as the wedge condition number.

 Dimension : 1
 Acceptable Range :
 Normal Range :
 Full Range :
 q for right, unit wedge : 1
 Reference : Adapted from section 7.7
 Verdict Function : v_wedge_max_aspect_frobenius or v_wedge_condition

 */

C_FUNC_DEF double v_wedge_max_aspect_frobenius( int /*num_nodes*/, double coordinates[][3] )
{
  double mini_tris[4][3];
  double aspect1 = 0, aspect2 = 0, aspect3 = 0, aspect4 = 0, aspect6 = 0;
  int i = 0;
  // Take first tetrahedron
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[0][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[1][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[2][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[3][i];}

  aspect1 = v_tet_aspect_frobenius(4,mini_tris);

  //Take second tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[1][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[2][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[0][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[4][i];}

  aspect2 = v_tet_aspect_frobenius(4,mini_tris);

  //3rd tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[2][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[0][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[1][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[5][i];}

  aspect3 = v_tet_aspect_frobenius(4,mini_tris);

  //4th tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[3][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[5][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[4][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[0][i];}

  aspect4 = v_tet_aspect_frobenius(4,mini_tris);

  //5th tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[4][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[3][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[5][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[1][i];}

  v_tet_aspect_frobenius(4,mini_tris);

  //6th tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[5][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[4][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[3][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[2][i];}

  aspect6 = v_tet_aspect_frobenius(4,mini_tris);

  double max_aspect = VERDICT_MAX(aspect1,aspect2);
  max_aspect = VERDICT_MAX(max_aspect,aspect3);
  max_aspect = VERDICT_MAX(max_aspect,aspect4);
  max_aspect = VERDICT_MAX(max_aspect,aspect6);
  max_aspect = max_aspect/1.16477;

  if ( max_aspect > 0 )
    return (double) VERDICT_MIN( max_aspect, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( max_aspect, -VERDICT_DBL_MAX );

}
/*
   For wedges, there is not a unique definition of the aspect Frobenius. Rather,
   this metric uses the aspect Frobenius defined for tetrahedra (see section
   6.4) and is comparable in methodology to the mean aspect Frobenius defined
   for hexahedra (see section 7.8). This value is normalized for a unit wedge.

   q = 1/6 * (F_0123 + F_1204 + F+2015 + F_3540 + F_4351 + F_5432)

   Dimension : 1
   Acceptable Range :
   Normal Range :
   Full Range :
   q for right, unit wedge : 1
   Reference : Adapted from section 7.8
   Verdict Function : v_wedge_mean_aspect_frobenius

   */

C_FUNC_DEF double v_wedge_mean_aspect_frobenius( int /*num_nodes*/, double coordinates[][3] )
{
  double mini_tris[4][3];
  double aspect1 = 0, aspect2 = 0, aspect3 = 0, aspect4 = 0, aspect5 = 0, aspect6 = 0;
  int i = 0;
  // Take first tetrahedron
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[0][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[1][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[2][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[3][i];}

  aspect1 = v_tet_aspect_frobenius(4,mini_tris);

  //Take second tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[1][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[2][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[0][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[4][i];}

  aspect2 = v_tet_aspect_frobenius(4,mini_tris);

  //3rd tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[2][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[0][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[1][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[5][i];}

  aspect3 = v_tet_aspect_frobenius(4,mini_tris);

  //4th tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[3][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[5][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[4][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[0][i];}

  aspect4 = v_tet_aspect_frobenius(4,mini_tris);

  //5th tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[4][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[3][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[5][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[1][i];}

  aspect5 = v_tet_aspect_frobenius(4,mini_tris);

  //6th tet
  for (i = 0; i < 3; i++){mini_tris[0][i] = coordinates[5][i];}
  for (i = 0; i < 3; i++){mini_tris[1][i] = coordinates[4][i];}
  for (i = 0; i < 3; i++){mini_tris[2][i] = coordinates[3][i];}
  for (i = 0; i < 3; i++){mini_tris[3][i] = coordinates[2][i];}

  aspect6 = v_tet_aspect_frobenius(4,mini_tris);

  double mean_aspect = (aspect1 + aspect2 + aspect3 + aspect4 + aspect5 + aspect6)/6;
  mean_aspect = mean_aspect/1.16477;

  if ( mean_aspect > 0 )
    return (double) VERDICT_MIN( mean_aspect, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( mean_aspect, -VERDICT_DBL_MAX );

}

/* This is the minimum determinant of the Jacobian matrix evaluated at each
 * corner of the element.

 q = min[((L_2 X L_0) * L_3)_k]
 where ((L_2 X L_0) * L_3)_k is the determinant of the Jacobian of the
 tetrahedron defined at the kth corner node, and L_2, L_0 and L_3 are the edges
 defined according to the standard for tetrahedral elements.

 Dimension : L^3
 Acceptable Range : [0,DBL_MAX]
 Normal Range : [0,DBL_MAX]
 Full Range : [-DBL_MAX,DBL_MAX]
 q for right, unit wedge : sqrt(3)/2
 Reference : Adapted from section 6.10
 Verdict Function : v_wedge_jacobian
 */

C_FUNC_DEF double v_wedge_jacobian( int /*num_nodes*/, double coordinates[][3] )
{
  double min_jacobian = 0, current_jacobian = 0;
  VerdictVector vec1,vec2,vec3;

  // Node 0
  vec1.set( coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2] );

  vec2.set( coordinates[3][0] - coordinates[0][0],
      coordinates[3][1] - coordinates[0][1],
      coordinates[3][2] - coordinates[0][2] );

  vec3.set( coordinates[2][0] - coordinates[0][0],
      coordinates[2][1] - coordinates[0][1],
      coordinates[2][2] - coordinates[0][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = current_jacobian;

  //node 1
  vec1.set( coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2] );

  vec2.set( coordinates[4][0] - coordinates[1][0],
      coordinates[4][1] - coordinates[1][1],
      coordinates[4][2] - coordinates[1][2] );

  vec3.set( coordinates[0][0] - coordinates[1][0],
      coordinates[0][1] - coordinates[1][1],
      coordinates[0][2] - coordinates[1][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian,min_jacobian);

  //node 2
  vec1.set( coordinates[0][0] - coordinates[2][0],
      coordinates[0][1] - coordinates[2][1],
      coordinates[0][2] - coordinates[2][2] );

  vec2.set( coordinates[5][0] - coordinates[2][0],
      coordinates[5][1] - coordinates[2][1],
      coordinates[5][2] - coordinates[2][2] );

  vec3.set( coordinates[1][0] - coordinates[2][0],
      coordinates[1][1] - coordinates[2][1],
      coordinates[1][2] - coordinates[2][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian,min_jacobian);

  //node 3
  vec1.set( coordinates[0][0] - coordinates[3][0],
      coordinates[0][1] - coordinates[3][1],
      coordinates[0][2] - coordinates[3][2] );

  vec2.set( coordinates[4][0] - coordinates[3][0],
      coordinates[4][1] - coordinates[3][1],
      coordinates[4][2] - coordinates[3][2] );

  vec3.set( coordinates[5][0] - coordinates[3][0],
      coordinates[5][1] - coordinates[3][1],
      coordinates[5][2] - coordinates[3][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian,min_jacobian);

  //node 4
  vec1.set( coordinates[1][0] - coordinates[4][0],
      coordinates[1][1] - coordinates[4][1],
      coordinates[1][2] - coordinates[4][2] );

  vec2.set( coordinates[5][0] - coordinates[4][0],
      coordinates[5][1] - coordinates[4][1],
      coordinates[5][2] - coordinates[4][2] );

  vec3.set( coordinates[3][0] - coordinates[4][0],
      coordinates[3][1] - coordinates[4][1],
      coordinates[3][2] - coordinates[4][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian,min_jacobian);

  //node 5
  vec1.set( coordinates[3][0] - coordinates[5][0],
      coordinates[3][1] - coordinates[5][1],
      coordinates[3][2] - coordinates[5][2] );

  vec2.set( coordinates[4][0] - coordinates[5][0],
      coordinates[4][1] - coordinates[5][1],
      coordinates[4][2] - coordinates[5][2] );

  vec3.set( coordinates[2][0] - coordinates[5][0],
      coordinates[2][1] - coordinates[5][1],
      coordinates[2][2] - coordinates[5][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian,min_jacobian);

  if ( min_jacobian > 0 )
    return (double) VERDICT_MIN( min_jacobian, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_jacobian, -VERDICT_DBL_MAX );

}

/* distortion is a measure of how well a particular wedge element maps to a
 * 'master' wedge with vertices:
 P0 - (0, 0, 0)
 P1 - (1, 0, 0)
 P2 - (1/2, sqrt(3)/2, 0)
 P3 - (0, 0, 1)
 P4 - (1, 0, 1)
 P2 - (1/2, sqrt(3)/2, 1)
 and volume (V_m).
 The behavior of the map is measured by sampling the determinant of the Jacobian
 at the vertices (k).  Thus the distortion is given by:
 q = ( min_k { det(J_k)} * V_m ) / V

 Dimension : 1
 Acceptable Range : [0.5,1]
 Normal Range : [0,1]
 Full Range : [-DBL_MAX,DBL_MAX]
 q for right, unit wedge : 1
 Reference : Adapted from section 7.3
 Verdict Function : v_wedge_distortion
 */

C_FUNC_DEF double v_wedge_distortion( int num_nodes, double coordinates[][3] )
{

  double jacobian = 0, distortion = 45, current_volume = 0, master_volume = 0.433013;

  jacobian = v_wedge_jacobian( num_nodes, coordinates );
  current_volume = v_wedge_volume( num_nodes, coordinates);
  distortion = jacobian*master_volume/current_volume/0.866025;

  if ( distortion > 0 )
    return (double) VERDICT_MIN( distortion, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( distortion, -VERDICT_DBL_MAX );

}

/*
   The stretch of a wedge element is here defined to be the maximum value of the
   stretch (S) of the three quadrilateral faces (see section 5.21):
   q = max[S_1043, S_1254, S_2035]

   Dimension : 1
   Acceptable Range :
   Normal Range :
   Full Range : [0,DBL_MAX]
   q for right, unit wedge : 1
   Reference : Adapted from section 5.21
   Verdict Function : v_wedge_max_stretch
   */

C_FUNC_DEF double v_wedge_max_stretch( int /*num_nodes*/, double coordinates[][3] )
{
  //This function finds the stretch of the 3 quadrilateral faces and returns the maximum value

  double stretch = 42, quad_face[4][3], stretch1 = 42, stretch2 = 42, stretch3 = 42;
  int i = 0;

  //first face
  for (i = 0; i < 3; i++){quad_face[0][i] = coordinates[0][i];}
  for (i = 0; i < 3; i++){quad_face[1][i] = coordinates[1][i];}
  for (i = 0; i < 3; i++){quad_face[2][i] = coordinates[4][i];}
  for (i = 0; i < 3; i++){quad_face[3][i] = coordinates[3][i];}
  stretch1 = v_quad_stretch(4,quad_face);

  //second face
  for (i = 0; i < 3; i++){quad_face[0][i] = coordinates[1][i];}
  for (i = 0; i < 3; i++){quad_face[1][i] = coordinates[2][i];}
  for (i = 0; i < 3; i++){quad_face[2][i] = coordinates[5][i];}
  for (i = 0; i < 3; i++){quad_face[3][i] = coordinates[4][i];}
  stretch2 = v_quad_stretch(4,quad_face);

  //third face
  for (i = 0; i < 3; i++){quad_face[0][i] = coordinates[2][i];}
  for (i = 0; i < 3; i++){quad_face[1][i] = coordinates[0][i];}
  for (i = 0; i < 3; i++){quad_face[2][i] = coordinates[3][i];}
  for (i = 0; i < 3; i++){quad_face[3][i] = coordinates[5][i];}
  stretch3 = v_quad_stretch(4,quad_face);

  stretch = VERDICT_MAX(stretch1,stretch2);
  stretch = VERDICT_MAX(stretch,stretch3);

  if ( stretch > 0 )
    return (double) VERDICT_MIN( stretch, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( stretch, -VERDICT_DBL_MAX );

}

/*
   This is the minimum determinant of the Jacobian matrix evaluated at each
   corner of the element, divided by the corresponding edge lengths and
   normalized to the unit wedge:
   q = min(  2 / sqrt(3) * ((L_2 X L_0) * L_3)_k / sqrt(mag(L_2) * mag(L_0) * mag(L_3)))
   where ((L_2 X L_0) * L_3)_k is the determinant of the Jacobian of the
   tetrahedron defined at the kth corner node, and L_2, L_0 and L_3 are the
   egdes defined according to the standard for tetrahedral elements.

   Dimension : 1
   Acceptable Range :
   Normal Range :
   Full Range : [?,DBL_MAX]
   q for right, unit wedge : 1
   Reference : Adapted from section 6.14 and 7.11
   Verdict Function : v_wedge_scaled_jacobian
   */

C_FUNC_DEF double v_wedge_scaled_jacobian( int /*num_nodes*/, double coordinates[][3] )
{
  double min_jacobian = 0, current_jacobian = 0,lengths = 42;
  VerdictVector vec1,vec2,vec3;

  // Node 0
  vec1.set( coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2] );

  vec2.set( coordinates[3][0] - coordinates[0][0],
      coordinates[3][1] - coordinates[0][1],
      coordinates[3][2] - coordinates[0][2] );

  vec3.set( coordinates[2][0] - coordinates[0][0],
      coordinates[2][1] - coordinates[0][1],
      coordinates[2][2] - coordinates[0][2] );

  lengths = sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = (vec2 % (vec1 * vec3));
  min_jacobian = current_jacobian/lengths;

  //node 1
  vec1.set( coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2] );

  vec2.set( coordinates[4][0] - coordinates[1][0],
      coordinates[4][1] - coordinates[1][1],
      coordinates[4][2] - coordinates[1][2] );

  vec3.set( coordinates[0][0] - coordinates[1][0],
      coordinates[0][1] - coordinates[1][1],
      coordinates[0][2] - coordinates[1][2] );

  lengths = sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian/lengths,min_jacobian);

  //node 2
  vec1.set( coordinates[0][0] - coordinates[2][0],
      coordinates[0][1] - coordinates[2][1],
      coordinates[0][2] - coordinates[2][2] );

  vec2.set( coordinates[5][0] - coordinates[2][0],
      coordinates[5][1] - coordinates[2][1],
      coordinates[5][2] - coordinates[2][2] );

  vec3.set( coordinates[1][0] - coordinates[2][0],
      coordinates[1][1] - coordinates[2][1],
      coordinates[1][2] - coordinates[2][2] );

  lengths = sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian/lengths,min_jacobian);

  //node 3
  vec1.set( coordinates[0][0] - coordinates[3][0],
      coordinates[0][1] - coordinates[3][1],
      coordinates[0][2] - coordinates[3][2] );

  vec2.set( coordinates[4][0] - coordinates[3][0],
      coordinates[4][1] - coordinates[3][1],
      coordinates[4][2] - coordinates[3][2] );

  vec3.set( coordinates[5][0] - coordinates[3][0],
      coordinates[5][1] - coordinates[3][1],
      coordinates[5][2] - coordinates[3][2] );

  lengths = sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian/lengths,min_jacobian);

  //node 4
  vec1.set( coordinates[1][0] - coordinates[4][0],
      coordinates[1][1] - coordinates[4][1],
      coordinates[1][2] - coordinates[4][2] );

  vec2.set( coordinates[5][0] - coordinates[4][0],
      coordinates[5][1] - coordinates[4][1],
      coordinates[5][2] - coordinates[4][2] );

  vec3.set( coordinates[3][0] - coordinates[4][0],
      coordinates[3][1] - coordinates[4][1],
      coordinates[3][2] - coordinates[4][2] );

  lengths = sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian/lengths,min_jacobian);

  //node 5
  vec1.set( coordinates[3][0] - coordinates[5][0],
      coordinates[3][1] - coordinates[5][1],
      coordinates[3][2] - coordinates[5][2] );

  vec2.set( coordinates[4][0] - coordinates[5][0],
      coordinates[4][1] - coordinates[5][1],
      coordinates[4][2] - coordinates[5][2] );

  vec3.set( coordinates[2][0] - coordinates[5][0],
      coordinates[2][1] - coordinates[5][1],
      coordinates[2][2] - coordinates[5][2] );

  lengths = sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = VERDICT_MIN(current_jacobian/lengths,min_jacobian);

  min_jacobian *= 2/sqrt(3.0);

  if ( min_jacobian > 0 )
    return (double) VERDICT_MIN( min_jacobian, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_jacobian, -VERDICT_DBL_MAX );

}

/*
   The shape metric is defined to be 3 divided by the minimum mean ratio of the
   Jacobian matrix evaluated at the element corners:
   q = 3 / min(i=0,1,...,6){ J_i ^ 2/3 / (mag(L_0) + mag(L_1) + mag(L_2) ) }
   where J_i is the Jacobian and L_0, L_1, L_2 are the sides of the tetrahedral
   formed at the ith corner.

   Dimension : 1
   Acceptable Range : [0.3,1]
   Normal Range : [0,1]
   Full Range : [0,1]
   q for right, unit wedge : 1
   Reference : Adapted from section 7.12
   Verdict Function : v_wedge_shape
   */

C_FUNC_DEF double v_wedge_shape( int /*num_nodes*/, double coordinates[][3] )
{
  double current_jacobian = 0, current_shape, norm_jacobi = 0;
  double min_shape = 1.0;
  static const double two_thirds = 2.0/3.0;
  VerdictVector vec1,vec2,vec3;

  // Node 0
  vec1.set( coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2] );

  vec2.set( coordinates[3][0] - coordinates[0][0],
      coordinates[3][1] - coordinates[0][1],
      coordinates[3][2] - coordinates[0][2] );

  vec3.set( coordinates[2][0] - coordinates[0][0],
      coordinates[2][1] - coordinates[0][1],
      coordinates[2][2] - coordinates[0][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  norm_jacobi = current_jacobian*2.0/sqrt(3.0);
  current_shape = 3*pow(norm_jacobi,two_thirds)/(vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
  min_shape = VERDICT_MIN(current_shape,min_shape);

  //node 1
  vec1.set( coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2] );

  vec2.set( coordinates[4][0] - coordinates[1][0],
      coordinates[4][1] - coordinates[1][1],
      coordinates[4][2] - coordinates[1][2] );

  vec3.set( coordinates[0][0] - coordinates[1][0],
      coordinates[0][1] - coordinates[1][1],
      coordinates[0][2] - coordinates[1][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  norm_jacobi = current_jacobian*2.0/sqrt(3.0);
  current_shape = 3*pow(norm_jacobi,two_thirds)/(vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
  min_shape = VERDICT_MIN(current_shape,min_shape);

  //node 2
  vec1.set( coordinates[0][0] - coordinates[2][0],
      coordinates[0][1] - coordinates[2][1],
      coordinates[0][2] - coordinates[2][2] );

  vec2.set( coordinates[5][0] - coordinates[2][0],
      coordinates[5][1] - coordinates[2][1],
      coordinates[5][2] - coordinates[2][2] );

  vec3.set( coordinates[1][0] - coordinates[2][0],
      coordinates[1][1] - coordinates[2][1],
      coordinates[1][2] - coordinates[2][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  norm_jacobi = current_jacobian*2.0/sqrt(3.0);
  current_shape = 3*pow(norm_jacobi,two_thirds)/(vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
  min_shape = VERDICT_MIN(current_shape,min_shape);

  //node 3
  vec1.set( coordinates[0][0] - coordinates[3][0],
      coordinates[0][1] - coordinates[3][1],
      coordinates[0][2] - coordinates[3][2] );

  vec2.set( coordinates[4][0] - coordinates[3][0],
      coordinates[4][1] - coordinates[3][1],
      coordinates[4][2] - coordinates[3][2] );

  vec3.set( coordinates[5][0] - coordinates[3][0],
      coordinates[5][1] - coordinates[3][1],
      coordinates[5][2] - coordinates[3][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  norm_jacobi = current_jacobian*2.0/sqrt(3.0);
  current_shape = 3*pow(norm_jacobi,two_thirds)/(vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
  min_shape = VERDICT_MIN(current_shape,min_shape);

  //node 4
  vec1.set( coordinates[1][0] - coordinates[4][0],
      coordinates[1][1] - coordinates[4][1],
      coordinates[1][2] - coordinates[4][2] );

  vec2.set( coordinates[5][0] - coordinates[4][0],
      coordinates[5][1] - coordinates[4][1],
      coordinates[5][2] - coordinates[4][2] );

  vec3.set( coordinates[3][0] - coordinates[4][0],
      coordinates[3][1] - coordinates[4][1],
      coordinates[3][2] - coordinates[4][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  norm_jacobi = current_jacobian*2.0/sqrt(3.0);
  current_shape = 3*pow(norm_jacobi,two_thirds)/(vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
  min_shape = VERDICT_MIN(current_shape,min_shape);

  //node 5
  vec1.set( coordinates[3][0] - coordinates[5][0],
      coordinates[3][1] - coordinates[5][1],
      coordinates[3][2] - coordinates[5][2] );

  vec2.set( coordinates[4][0] - coordinates[5][0],
      coordinates[4][1] - coordinates[5][1],
      coordinates[4][2] - coordinates[5][2] );

  vec3.set( coordinates[2][0] - coordinates[5][0],
      coordinates[2][1] - coordinates[5][1],
      coordinates[2][2] - coordinates[5][2] );

  current_jacobian = vec2 % (vec1 * vec3);
  norm_jacobi = current_jacobian*2.0/sqrt(3.0);
  current_shape = 3*pow(norm_jacobi,two_thirds)/(vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
  min_shape = VERDICT_MIN(current_shape,min_shape);

  if (min_shape < VERDICT_DBL_MIN)
    return 0;
  return min_shape;

}

/* For wedges, there is not a unique definition of the aspect Frobenius. Rather,
 * this metric uses the aspect Frobenius defined for tetrahedral (see section
 * 6.4) and is comparable in methodology to the maximum aspect Frobenius defined
 * for hexahedra (see section 7.7). This value is normalized for a unit wedge.

 q = max(F_0123, F_1204, F_2015, F_3540, F_4351, F_5432)

 This is also known as the wedge condition number.

 Dimension : 1
 Acceptable Range :
 Normal Range :
 Full Range :
 q for right, unit wedge : 1
 Reference : Adapted from section 7.7
 Verdict Function : v_wedge_max_aspect_frobenius or v_wedge_condition

 */
C_FUNC_DEF double v_wedge_condition( int /*num_nodes*/, double coordinates[][3] )
{
  return v_wedge_max_aspect_frobenius(6,coordinates);
}

