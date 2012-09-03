/*=========================================================================

  Module:    V_TriMetric.cpp

  Copyright (c) 2007 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 *
 * TriMetric.cpp contains quality calculations for Tris
 *
 * This file is part of VERDICT
 *
 */


#include "verdict.h"
#include "verdict_defines.hpp"
#include "V_GaussIntegration.hpp"
#include "VerdictVector.hpp"
#include <memory.h>
#include <stddef.h>

// the average area of a tri
static double verdict_tri_size = 0;
static ComputeNormal compute_normal = NULL;

/*! 
  get weights based on the average area of a set of
  tris
*/
static int v_tri_get_weight ( double &m11,
    double &m21,
    double &m12,
    double &m22 )
{
  static const double rootOf3 = sqrt(3.0);
  m11=1;
  m21=0;
  m12=0.5;
  m22=0.5*rootOf3;
  double scale = sqrt(2.0*verdict_tri_size/(m11*m22-m21*m12));
  
  m11 *= scale;
  m21 *= scale;
  m12 *= scale;
  m22 *= scale;
  
  return 1;
}


/*! sets the average area of a tri */
C_FUNC_DEF void v_set_tri_size( double size )
{
  verdict_tri_size = size;
}

C_FUNC_DEF void v_set_tri_normal_func( ComputeNormal func )
{
  compute_normal = func;
}

/*!
   the edge ratio of a triangle

   NB (P. Pebay 01/14/07): 
     Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
     minimum edge lengths

*/
C_FUNC_DEF double v_tri_edge_ratio( int /*num_nodes*/, double coordinates[][3] )
{

  // three vectors for each side 
  VerdictVector a( coordinates[1][0] - coordinates[0][0],
                   coordinates[1][1] - coordinates[0][1],
                   coordinates[1][2] - coordinates[0][2] );
  
  VerdictVector b( coordinates[2][0] - coordinates[1][0],
                   coordinates[2][1] - coordinates[1][1],
                   coordinates[2][2] - coordinates[1][2] );
  
  VerdictVector c( coordinates[0][0] - coordinates[2][0],
                   coordinates[0][1] - coordinates[2][1],
                   coordinates[0][2] - coordinates[2][2] );

  double a2 = a.length_squared();
  double b2 = b.length_squared();
  double c2 = c.length_squared();
 
  double m2, M2;
  if ( a2 < b2 )
    {
      if ( b2 < c2 )
        {
          m2 = a2;
          M2 = c2;
        }
      else // b2 <= a2
        {
          if ( a2 < c2 )
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
      if ( a2 < c2 )
        {
          m2 = b2;
          M2 = c2;
        }
      else // c2 <= a2
        {
          if ( b2 < c2 )
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

  if( m2 < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;
  else
  {
    double edge_ratio;
    edge_ratio = sqrt(M2 / m2);
    
    if( edge_ratio > 0 )
      return (double) VERDICT_MIN( edge_ratio, VERDICT_DBL_MAX );
    return (double) VERDICT_MAX( edge_ratio, -VERDICT_DBL_MAX );
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
C_FUNC_DEF double v_tri_aspect_ratio( int /*num_nodes*/, double coordinates[][3] )
{
  static const double normal_coeff = sqrt( 3. ) / 6.;

  // three vectors for each side 
  VerdictVector a( coordinates[1][0] - coordinates[0][0],
                   coordinates[1][1] - coordinates[0][1],
                   coordinates[1][2] - coordinates[0][2] );
  
  VerdictVector b( coordinates[2][0] - coordinates[1][0],
                   coordinates[2][1] - coordinates[1][1],
                   coordinates[2][2] - coordinates[1][2] );
  
  VerdictVector c( coordinates[0][0] - coordinates[2][0],
                   coordinates[0][1] - coordinates[2][1],
                   coordinates[0][2] - coordinates[2][2] );

  double a1 = a.length();
  double b1 = b.length();
  double c1 = c.length();
 
  double hm = a1 > b1 ? a1 : b1;
  hm = hm > c1 ? hm : c1;

  VerdictVector ab = a * b;
  double denominator = ab.length();

  if( denominator < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;
  else
  {
    double aspect_ratio;
    aspect_ratio = normal_coeff * hm * (a1 + b1 + c1) / denominator;
    
    if( aspect_ratio > 0 )
      return (double) VERDICT_MIN( aspect_ratio, VERDICT_DBL_MAX );
    return (double) VERDICT_MAX( aspect_ratio, -VERDICT_DBL_MAX );
  }

}

/*!
   the radius ratio of a triangle

   NB (P. Pebay 01/13/07): 
     CR / (2.0*IR) where CR is the circumradius and IR is the inradius

     The radius ratio is also known to VERDICT, for tetrahedral elements only,
     as the "aspect beta".
   
*/
C_FUNC_DEF double v_tri_radius_ratio( int /*num_nodes*/, double coordinates[][3] )
{

  // three vectors for each side 
  VerdictVector a( coordinates[1][0] - coordinates[0][0],
                   coordinates[1][1] - coordinates[0][1],
                   coordinates[1][2] - coordinates[0][2] );
  
  VerdictVector b( coordinates[2][0] - coordinates[1][0],
                   coordinates[2][1] - coordinates[1][1],
                   coordinates[2][2] - coordinates[1][2] );
  
  VerdictVector c( coordinates[0][0] - coordinates[2][0],
                   coordinates[0][1] - coordinates[2][1],
                   coordinates[0][2] - coordinates[2][2] );

  double a1 = a.length();
  double b1 = b.length();
  double c1 = c.length();
 
  VerdictVector ab = a * b;
  double denominator = ab.length_squared();

  if( denominator < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;

  double radius_ratio;
  radius_ratio = .25 * a1 * b1 * c1 * ( a1 + b1 + c1 ) / denominator;
  
  if( radius_ratio > 0 )
    return (double) VERDICT_MIN( radius_ratio, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( radius_ratio, -VERDICT_DBL_MAX );
}

/*!
   the Frobenius aspect of a tri

   srms^2/(2 * sqrt(3.0) * area)
   where srms^2 is sum of the lengths squared

   NB (P. Pebay 01/14/07): 
     this method was called "aspect ratio" in earlier incarnations of VERDICT
   
*/

C_FUNC_DEF double v_tri_aspect_frobenius( int /*num_nodes*/, double coordinates[][3] )
{
  static const double two_times_root_of_3 = 2*sqrt(3.0);

  // three vectors for each side 
  VerdictVector side1( coordinates[1][0] - coordinates[0][0],
                       coordinates[1][1] - coordinates[0][1],
                       coordinates[1][2] - coordinates[0][2] );
  
  VerdictVector side2( coordinates[2][0] - coordinates[1][0],
                       coordinates[2][1] - coordinates[1][1],
                       coordinates[2][2] - coordinates[1][2] );
  
  VerdictVector side3( coordinates[0][0] - coordinates[2][0],
                       coordinates[0][1] - coordinates[2][1],
                       coordinates[0][2] - coordinates[2][2] );
 
  //sum the lengths squared of each side
  double srms = (side1.length_squared() + side2.length_squared() 
      + side3.length_squared());
  
  // find two times the area of the triangle by cross product
  double areaX2 = ((side1 * (-side3)).length());

  if(areaX2 == 0.0)
    return (double)VERDICT_DBL_MAX;
 
  double aspect = (double)(srms / (two_times_root_of_3 * (areaX2)));
  if( aspect > 0 )
    return (double) VERDICT_MIN( aspect, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( aspect, -VERDICT_DBL_MAX );
}

/*!
  The area of a tri

  0.5 * jacobian at a node
*/
C_FUNC_DEF double v_tri_area( int /*num_nodes*/, double coordinates[][3] )
{
  // two vectors for two sides
  VerdictVector side1( coordinates[1][0] - coordinates[0][0],
                       coordinates[1][1] - coordinates[0][1],
                       coordinates[1][2] - coordinates[0][2] );
  
  VerdictVector side3( coordinates[2][0] - coordinates[0][0],
                       coordinates[2][1] - coordinates[0][1],
                       coordinates[2][2] - coordinates[0][2] );
 
  // the cross product of the two vectors representing two sides of the
  // triangle 
  VerdictVector tmp = side1 * side3;
  
  // return the magnitude of the vector divided by two
  double area = 0.5 * tmp.length();
  if( area > 0 )
    return (double) VERDICT_MIN( area, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( area, -VERDICT_DBL_MAX );
  
}


/*!
  The minimum angle of a tri

  The minimum angle of a tri is the minimum angle between 
  two adjacents sides out of all three corners of the triangle.
*/
C_FUNC_DEF double v_tri_minimum_angle( int /*num_nodes*/, double coordinates[][3] )
{

  // vectors for all the sides
  VerdictVector sides[4];
  sides[0].set(
      coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]
      );
  sides[1].set(
      coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2]
      );
  sides[2].set(
      coordinates[2][0] - coordinates[0][0],
      coordinates[2][1] - coordinates[0][1],
      coordinates[2][2] - coordinates[0][2]
      );

  // in case we need to find the interior angle
  // between sides 0 and 1
  sides[3] = -sides[1];

  // calculate the lengths squared of the sides
  double sides_lengths[3];
  sides_lengths[0] = sides[0].length_squared();
  sides_lengths[1] = sides[1].length_squared();
  sides_lengths[2] = sides[2].length_squared();

  if(sides_lengths[0] == 0.0 || sides_lengths[1] == 0.0 ||
     sides_lengths[2] == 0.0)
     return 0.0;
  
  // using the law of sines, we know that the minimum
  // angle is opposite of the shortest side

  // find the shortest side
  int short_side=0;
  if(sides_lengths[1] < sides_lengths[0])
    short_side = 1;
  if(sides_lengths[2] < sides_lengths[short_side])
    short_side = 2;

  // from the shortest side, calculate the angle of the 
  // opposite angle
  double min_angle;
  if(short_side == 0)
    {
    min_angle = sides[2].interior_angle(sides[1]);
    }
  else if(short_side == 1)
    {
    min_angle = sides[0].interior_angle(sides[2]);
    }
  else
    {
    min_angle = sides[0].interior_angle(sides[3]);
    }

  if( min_angle > 0 )
    return (double) VERDICT_MIN( min_angle, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_angle, -VERDICT_DBL_MAX );
  
}

/*!
  The maximum angle of a tri

  The maximum angle of a tri is the maximum angle between 
  two adjacents sides out of all three corners of the triangle.
*/
C_FUNC_DEF double v_tri_maximum_angle( int /*num_nodes*/, double coordinates[][3] )
{

  // vectors for all the sides
  VerdictVector sides[4];
  sides[0].set(
      coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]
      );
  sides[1].set(
      coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2]
      );
  sides[2].set(
      coordinates[2][0] - coordinates[0][0],
      coordinates[2][1] - coordinates[0][1],
      coordinates[2][2] - coordinates[0][2]
      );

  // in case we need to find the interior angle
  // between sides 0 and 1
  sides[3] = -sides[1];

  // calculate the lengths squared of the sides
  double sides_lengths[3];
  sides_lengths[0] = sides[0].length_squared();
  sides_lengths[1] = sides[1].length_squared();
  sides_lengths[2] = sides[2].length_squared();

  if(sides_lengths[0] == 0.0 || 
    sides_lengths[1] == 0.0 ||
    sides_lengths[2] == 0.0)
  {
    return 0.0;
  }      
  
  // using the law of sines, we know that the maximum
  // angle is opposite of the longest side

  // find the longest side
  int short_side=0;
  if(sides_lengths[1] > sides_lengths[0])
    short_side = 1;
  if(sides_lengths[2] > sides_lengths[short_side])
    short_side = 2;

  // from the longest side, calculate the angle of the 
  // opposite angle
  double max_angle;
  if(short_side == 0)
    {
    max_angle = sides[2].interior_angle(sides[1]);
    }
  else if(short_side == 1)
    {
    max_angle = sides[0].interior_angle(sides[2]);
    }
  else
    {
    max_angle = sides[0].interior_angle(sides[3]);
    }

  if( max_angle > 0 )
    return (double) VERDICT_MIN( max_angle, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( max_angle, -VERDICT_DBL_MAX );
  
}



/*!
  The condition of a tri

  Condition number of the jacobian matrix at any corner
*/
C_FUNC_DEF double v_tri_condition( int /*num_nodes*/, double coordinates[][3] )
{
  static const double rt3 = sqrt(3.0);
  
  VerdictVector v1(coordinates[1][0] - coordinates[0][0],
                   coordinates[1][1] - coordinates[0][1],
                   coordinates[1][2] - coordinates[0][2] );
  
  VerdictVector v2(coordinates[2][0] - coordinates[0][0],
                   coordinates[2][1] - coordinates[0][1],
                   coordinates[2][2] - coordinates[0][2] );
  
  VerdictVector tri_normal = v1 * v2;  
  double areax2= tri_normal.length();
  
  if (areax2 == 0.0 ) 
    return (double)VERDICT_DBL_MAX;

  double condition = (double)( ((v1%v1) + (v2%v2) - (v1%v2)) / (areax2*rt3) );

    //check for inverted if we have access to the normal
  if( compute_normal )
  {
    //center of tri
    double point[3], surf_normal[3];
    point[0] =  (coordinates[0][0] + coordinates[1][0] + coordinates[2][0]) / 3;
    point[1] =  (coordinates[0][1] + coordinates[1][1] + coordinates[2][1]) / 3;
    point[2] =  (coordinates[0][2] + coordinates[1][2] + coordinates[2][2]) / 3;

    //dot product
    compute_normal( point, surf_normal ); 
    if( (tri_normal.x()*surf_normal[0] + 
         tri_normal.y()*surf_normal[1] +
         tri_normal.z()*surf_normal[2] ) < 0 )
      return (double)VERDICT_DBL_MAX;
  }
  return (double)VERDICT_MIN( condition, VERDICT_DBL_MAX );
}

/*!
  The scaled jacobian of a tri

  minimum of the jacobian divided by the lengths of 2 edge vectors
*/
C_FUNC_DEF double v_tri_scaled_jacobian( int /*num_nodes*/, double coordinates[][3])
{
  static const double detw = 2./sqrt(3.0);
  VerdictVector first, second;
  double jacobian; 
  
  VerdictVector edge[3];
  edge[0].set(coordinates[1][0] - coordinates[0][0],
              coordinates[1][1] - coordinates[0][1],
              coordinates[1][2] - coordinates[0][2]);

  edge[1].set(coordinates[2][0] - coordinates[0][0],
              coordinates[2][1] - coordinates[0][1],
              coordinates[2][2] - coordinates[0][2]);

  edge[2].set(coordinates[2][0] - coordinates[1][0],
              coordinates[2][1] - coordinates[1][1],
              coordinates[2][2] - coordinates[1][2]);
  first = edge[1]-edge[0];
  second = edge[2]-edge[0];

  VerdictVector cross = first * second;
  jacobian = cross.length();

  double max_edge_length_product;
  max_edge_length_product = VERDICT_MAX( edge[0].length()*edge[1].length(),
                            VERDICT_MAX( edge[1].length()*edge[2].length(), 
                                         edge[0].length()*edge[2].length() ) ); 

  if( max_edge_length_product < VERDICT_DBL_MIN )
    return (double)0.0;

  jacobian *= detw;
  jacobian /= max_edge_length_product; 

  if( compute_normal )
  {
    //center of tri
    double point[3], surf_normal[3];
    point[0] =  (coordinates[0][0] + coordinates[1][0] + coordinates[2][0]) / 3;
    point[1] =  (coordinates[0][1] + coordinates[1][1] + coordinates[2][1]) / 3;
    point[2] =  (coordinates[0][2] + coordinates[1][2] + coordinates[2][2]) / 3;

    //dot product
    compute_normal( point, surf_normal ); 
    if( (cross.x()*surf_normal[0] + 
         cross.y()*surf_normal[1] +
         cross.z()*surf_normal[2] ) < 0 )
      jacobian *= -1; 
  }

  if( jacobian > 0 )
    return (double) VERDICT_MIN( jacobian, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( jacobian, -VERDICT_DBL_MAX );

}


/*!
  The shape of a tri

  2 / condition number of weighted jacobian matrix
*/
C_FUNC_DEF double v_tri_shape( int num_nodes, double coordinates[][3] )
{
  double condition = v_tri_condition( num_nodes, coordinates );

  double shape;
  if( condition <= VERDICT_DBL_MIN )
    shape = VERDICT_DBL_MAX;
  else
    shape = (1 / condition);

  if( shape > 0 )
    return (double) VERDICT_MIN( shape, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( shape, -VERDICT_DBL_MAX );
}

/*!
  The relative size of a tri

  Min(J,1/J) where J is the determinant of the weighted jacobian matrix.
*/
C_FUNC_DEF double v_tri_relative_size_squared( int /*num_nodes*/, double coordinates[][3] )
{
  double w11, w21, w12, w22;

  VerdictVector xxi, xet, tri_normal;
  
  v_tri_get_weight(w11,w21,w12,w22);

  double detw = v_determinant(w11,w21,w12,w22);

  if(detw == 0.0)
    return 0.0;

  xxi.set(coordinates[0][0] - coordinates[1][0],
    coordinates[0][1] - coordinates[1][1],
    coordinates[0][2] - coordinates[1][2]);

  xet.set(coordinates[0][0] - coordinates[2][0],
    coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  tri_normal = xxi * xet;

  double deta = tri_normal.length();
  if( deta == 0.0  || detw == 0.0 )
    return 0.0;
    
  double size = pow( deta/detw, 2 );
  
  double rel_size = VERDICT_MIN(size, 1.0/size );  

  if( rel_size > 0 )
    return (double) VERDICT_MIN( rel_size, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( rel_size, -VERDICT_DBL_MAX );
  
}

/*!
  The shape and size of a tri
  
  Product of the Shape and Relative Size
*/
C_FUNC_DEF double v_tri_shape_and_size( int num_nodes, double coordinates[][3] )
{
  double size, shape;  

  size = v_tri_relative_size_squared( num_nodes, coordinates );
  shape = v_tri_shape( num_nodes, coordinates );
  
  double shape_and_size = size * shape;

  if( shape_and_size > 0 )
    return (double) VERDICT_MIN( shape_and_size, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( shape_and_size, -VERDICT_DBL_MAX );

}


/*!
  The distortion of a tri

TODO:  make a short definition of the distortion and comment below
*/
C_FUNC_DEF double v_tri_distortion( int num_nodes, double coordinates[][3] )
{

   double distortion;
   int total_number_of_gauss_points=0;
   VerdictVector  aa, bb, cc,normal_at_point, xin;
   double element_area = 0.;

   aa.set(coordinates[1][0] - coordinates[0][0], 
    coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2] );
  
   bb.set(coordinates[2][0] - coordinates[0][0], 
    coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2] );
  

   VerdictVector tri_normal = aa * bb;
 
   int number_of_gauss_points=0;
   if (num_nodes ==3)
   {
      distortion = 1.0;
      return (double)distortion;
   }
   
   else if (num_nodes ==6)
   {
      total_number_of_gauss_points = 6;
      number_of_gauss_points = 6;
   }

   distortion = VERDICT_DBL_MAX;
   double shape_function[maxTotalNumberGaussPoints][maxNumberNodes];
   double dndy1[maxTotalNumberGaussPoints][maxNumberNodes];
   double dndy2[maxTotalNumberGaussPoints][maxNumberNodes];
   double weight[maxTotalNumberGaussPoints];

   //create an object of GaussIntegration
   int number_dims = 2;
   int is_tri = 1;
   GaussIntegration::initialize(number_of_gauss_points,num_nodes, number_dims, is_tri);
   GaussIntegration::calculate_shape_function_2d_tri();
   GaussIntegration::get_shape_func(shape_function[0], dndy1[0], dndy2[0], weight);

         // calculate element area
   int ife, ja;
   for (ife=0;ife<total_number_of_gauss_points; ife++)
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
         double jacobian = normal_at_point.length();
         element_area += weight[ife]*jacobian;
   }

   element_area *= 0.8660254;
   double dndy1_at_node[maxNumberNodes][maxNumberNodes];
   double dndy2_at_node[maxNumberNodes][maxNumberNodes];


   GaussIntegration::calculate_derivative_at_nodes_2d_tri( dndy1_at_node,  dndy2_at_node);

   VerdictVector normal_at_nodes[7];



   //evaluate normal at nodes and distortion values at nodes
   int  jai=0;
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
   double thickness, thickness_gauss;
   double distrt;
   //get_tri_thickness(tri, element_area, thickness );
     thickness = 0.001*sqrt(element_area);

   //set thickness gauss point location
   double zl = 0.5773502691896;
   if (flat_element) zl =0.0;

   int no_gauss_pts_z = (flat_element)? 1 : 2;
   double thickness_z;

   //loop on integration points
   int igz;
   for (ife=0;ife<total_number_of_gauss_points;ife++)
   {
      //loop on the thickness direction gauss points
      for (igz=0;igz<no_gauss_pts_z;igz++)
      {
  zl = -zl;
         thickness_z = zl*thickness/2.0;

         aa.set(0.0,0.0,0.0);
         bb.set(0.0,0.0,0.0);
         cc.set(0.0,0.0,0.0);

         for (ja=0;ja<num_nodes;ja++)
         {
            xin.set(coordinates[jai][0], coordinates[jai][1], coordinates[jai][2]);
            xin += thickness_z*normal_at_nodes[ja];
            aa  += dndy1[ife][ja]*xin;
            bb  += dndy2[ife][ja]*xin;
            thickness_gauss = shape_function[ife][ja]*thickness/2.0;
            cc  += thickness_gauss*normal_at_nodes[ja];
         }

         normal_at_point = aa*bb;
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
      double sign_jacobian = (tri_normal % normal_at_point) > 0? 1.:-1.;
      distrt = sign_jacobian  * (cc%normal_at_point);

      if (distrt < distortion) distortion = distrt;
   }     
   if (element_area*thickness !=0)
      distortion *=1./( element_area*thickness);
   else
      distortion *=1.;
   
  if( distortion > 0 )
    return (double) VERDICT_MIN( distortion, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( distortion, -VERDICT_DBL_MAX );
}


/*! 
  tri_quality for calculating multiple tri functions at once

  using this method is generally faster than using the individual 
  method multiple times.

*/
C_FUNC_DEF void v_tri_quality( int num_nodes, double coordinates[][3], 
    unsigned int metrics_request_flag, TriMetricVals *metric_vals ) 
{

  memset( metric_vals, 0, sizeof(TriMetricVals) );

  // for starts, lets set up some basic and common information

  /*  node numbers and side numbers used below

             2
             ++
            /  \ 
         2 /    \ 1
          /      \
         /        \
       0 ---------+ 1
             0
  */
  
  // vectors for each side
  VerdictVector sides[3];
  sides[0].set(
      coordinates[1][0] - coordinates[0][0],
      coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]
      );
  sides[1].set(
      coordinates[2][0] - coordinates[1][0],
      coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2]
      );
  sides[2].set(
      coordinates[2][0] - coordinates[0][0],
      coordinates[2][1] - coordinates[0][1],
      coordinates[2][2] - coordinates[0][2]
      );
  VerdictVector tri_normal = sides[0] * sides[2];
    //if we have access to normal information, check to see if the
    //element is inverted.  If we don't have the normal information
    //that we need for this, assume the element is not inverted.
    //This flag will be used for condition number, jacobian, shape,
    //and size and shape.
  bool is_inverted = false;
  if( compute_normal )
  {
      //center of tri
    double point[3], surf_normal[3];
    point[0] =  (coordinates[0][0] + coordinates[1][0] + coordinates[2][0]) / 3;
    point[1] =  (coordinates[0][1] + coordinates[1][1] + coordinates[2][1]) / 3;
    point[2] =  (coordinates[0][2] + coordinates[1][2] + coordinates[2][2]) / 3;
      //dot product
    compute_normal( point, surf_normal ); 
    if( (tri_normal.x()*surf_normal[0] + 
         tri_normal.y()*surf_normal[1] +
         tri_normal.z()*surf_normal[2] ) < 0 )
      is_inverted=true; 
  }
  
  // lengths squared of each side
  double sides_lengths_squared[3];
  sides_lengths_squared[0] = sides[0].length_squared();
  sides_lengths_squared[1] = sides[1].length_squared();
  sides_lengths_squared[2] = sides[2].length_squared();
 

  // if we are doing angle calcuations
  if( metrics_request_flag & (V_TRI_MINIMUM_ANGLE | V_TRI_MAXIMUM_ANGLE) )
  {
    // which is short and long side
    int short_side=0, long_side=0;

    if(sides_lengths_squared[1] < sides_lengths_squared[0])
      short_side = 1;
    if(sides_lengths_squared[2] < sides_lengths_squared[short_side])
      short_side = 2;
  
    if(sides_lengths_squared[1] > sides_lengths_squared[0])
      long_side = 1;
    if(sides_lengths_squared[2] > sides_lengths_squared[long_side])
      long_side = 2;


    // calculate the minimum angle of the tri
    if( metrics_request_flag & V_TRI_MINIMUM_ANGLE )
    {
      if(sides_lengths_squared[0] == 0.0 || 
        sides_lengths_squared[1] == 0.0 ||
        sides_lengths_squared[2] == 0.0)
      {
        metric_vals->minimum_angle = 0.0;
      }        
      else if(short_side == 0)
        metric_vals->minimum_angle = (double)sides[2].interior_angle(sides[1]);
      else if(short_side == 1)
        metric_vals->minimum_angle = (double)sides[0].interior_angle(sides[2]);
      else
        metric_vals->minimum_angle = (double)sides[0].interior_angle(-sides[1]);
    }
    
    // calculate the maximum angle of the tri
    if( metrics_request_flag & V_TRI_MAXIMUM_ANGLE )
    {
      if(sides_lengths_squared[0] == 0.0 || 
        sides_lengths_squared[1] == 0.0 ||
        sides_lengths_squared[2] == 0.0)
      {
        metric_vals->maximum_angle = 0.0;
      }        
      else if(long_side == 0)
        metric_vals->maximum_angle = (double)sides[2].interior_angle(sides[1]);
      else if(long_side == 1)
        metric_vals->maximum_angle = (double)sides[0].interior_angle(sides[2]);
      else
        metric_vals->maximum_angle = (double)sides[0].interior_angle(-sides[1]);
    }
  }


  // calculate the area of the tri
  // the following functions depend on area
  if( metrics_request_flag & (V_TRI_AREA | V_TRI_SCALED_JACOBIAN | 
    V_TRI_SHAPE | V_TRI_RELATIVE_SIZE_SQUARED | V_TRI_SHAPE_AND_SIZE ) )
  {
    metric_vals->area = (double)((sides[0] * sides[2]).length() * 0.5);
  }

  // calculate the aspect ratio
  if(metrics_request_flag & V_TRI_ASPECT_FROBENIUS)
  {
    // sum the lengths squared
    double srms = 
      sides_lengths_squared[0] +
      sides_lengths_squared[1] +
      sides_lengths_squared[2] ;

    // calculate once and reuse
    static const double twoTimesRootOf3 = 2*sqrt(3.0);

    double div = (twoTimesRootOf3 * 
      ( (sides[0] * sides[2]).length() ));

    if(div == 0.0)
      metric_vals->aspect_frobenius = (double)VERDICT_DBL_MAX;
    else
      metric_vals->aspect_frobenius = (double)(srms / div);
  }

  // calculate the radius ratio of the triangle
  if( metrics_request_flag & V_TRI_RADIUS_RATIO )
  {
  double a1 = sqrt( sides_lengths_squared[0] );
  double b1 = sqrt( sides_lengths_squared[1] );
  double c1 = sqrt( sides_lengths_squared[2] );

  VerdictVector ab = sides[0] * sides[1];

  metric_vals->radius_ratio = (double) .25 * a1 * b1 * c1 * ( a1 + b1 + c1 ) / ab.length_squared();
  }

  // calculate the scaled jacobian
  if(metrics_request_flag & V_TRI_SCALED_JACOBIAN)
  {
    // calculate once and reuse
    static const double twoOverRootOf3 = 2/sqrt(3.0);
    // use the area from above
    
    double tmp = tri_normal.length() * twoOverRootOf3;
      
    // now scale it by the lengths of the sides
    double min_scaled_jac = VERDICT_DBL_MAX;
    double temp_scaled_jac;
    for(int i=0; i<3; i++)
    {
      if(sides_lengths_squared[i%3] == 0.0 || sides_lengths_squared[(i+2)%3] == 0.0)
        temp_scaled_jac = 0.0;
      else
        temp_scaled_jac = tmp / sqrt(sides_lengths_squared[i%3]) / sqrt(sides_lengths_squared[(i+2)%3]);
      if( temp_scaled_jac < min_scaled_jac )
        min_scaled_jac = temp_scaled_jac;
    }
      //multiply by -1 if the normals are in opposite directions
    if( is_inverted )
    {
      min_scaled_jac *= -1; 
    }
    metric_vals->scaled_jacobian = (double)min_scaled_jac;

  }

  // calculate the condition number
  if(metrics_request_flag & V_TRI_CONDITION)
  {
    // calculate once and reuse
    static const double rootOf3 = sqrt(3.0);
      //if it is inverted, the condition number is considered to be infinity.
    if(is_inverted){
      metric_vals->condition = VERDICT_DBL_MAX;
    }
    else{
      double area2x = (sides[0] * sides[2]).length();
      if(area2x == 0.0 ) 
        metric_vals->condition = (double)(VERDICT_DBL_MAX);
      else
        metric_vals->condition = (double) ( (sides[0]%sides[0] +
                                                   sides[2]%sides[2] -
                                                   sides[0]%sides[2])  /
                                                  (area2x*rootOf3) );
    }
    
  }

  // calculate the shape
  if(metrics_request_flag & V_TRI_SHAPE || metrics_request_flag & V_TRI_SHAPE_AND_SIZE)
  {
      //if element is inverted, shape is zero.  We don't need to
      //calculate anything.
    if(is_inverted ){
      metric_vals->shape = 0.0;
    }
    else{//otherwise, we calculate the shape
        // calculate once and reuse
      static const double rootOf3 = sqrt(3.0);
        // reuse area from before
      double area2x = metric_vals->area * 2;
        // dot products
      double dots[3] = { 
        sides[0] % sides[0],
          sides[2] % sides[2],
          sides[0] % sides[2]
          };

        // add the dots
      double sum_dots = dots[0] + dots[1] - dots[2];

        // then the finale
      if( sum_dots == 0.0 ) 
        metric_vals->shape = 0.0;
      else
        metric_vals->shape = (double)(rootOf3 * area2x / sum_dots);
    }
    
  }

  // calculate relative size squared
  if(metrics_request_flag & V_TRI_RELATIVE_SIZE_SQUARED || metrics_request_flag & V_TRI_SHAPE_AND_SIZE)
  {
    // get weights
    double w11, w21, w12, w22;
    v_tri_get_weight(w11,w21,w12,w22);
    // get the determinant
    double detw = v_determinant(w11,w21,w12,w22);
    // use the area from above and divide with the determinant
    if( metric_vals->area == 0.0  || detw == 0.0 )
      metric_vals->relative_size_squared = 0.0;
    else
    {
      double size = metric_vals->area * 2.0 / detw;
      // square the size
      size *= size;
      // value ranges between 0 to 1
      metric_vals->relative_size_squared = (double)VERDICT_MIN(size, 1.0/size );
    }
  }

  // calculate shape and size
  if(metrics_request_flag & V_TRI_SHAPE_AND_SIZE)
  {
    metric_vals->shape_and_size = 
      metric_vals->relative_size_squared * metric_vals->shape;
  }

  // calculate distortion
  if(metrics_request_flag & V_TRI_DISTORTION)
    metric_vals->distortion = v_tri_distortion(num_nodes, coordinates);

  //take care of any over-flow problems
  if( metric_vals->aspect_frobenius > 0 )
    metric_vals->aspect_frobenius = (double) VERDICT_MIN( metric_vals->aspect_frobenius, VERDICT_DBL_MAX );\
  else
    metric_vals->aspect_frobenius = (double) VERDICT_MAX( metric_vals->aspect_frobenius, -VERDICT_DBL_MAX );

  if( metric_vals->area > 0 )
    metric_vals->area = (double) VERDICT_MIN( metric_vals->area, VERDICT_DBL_MAX );
  else
    metric_vals->area = (double) VERDICT_MAX( metric_vals->area, -VERDICT_DBL_MAX );

  if( metric_vals->minimum_angle > 0 )
    metric_vals->minimum_angle = (double) VERDICT_MIN( metric_vals->minimum_angle, VERDICT_DBL_MAX );
  else
    metric_vals->minimum_angle = (double) VERDICT_MAX( metric_vals->minimum_angle, -VERDICT_DBL_MAX );

  if( metric_vals->maximum_angle > 0 )
    metric_vals->maximum_angle = (double) VERDICT_MIN( metric_vals->maximum_angle, VERDICT_DBL_MAX );
  else
    metric_vals->maximum_angle = (double) VERDICT_MAX( metric_vals->maximum_angle , -VERDICT_DBL_MAX );

  if( metric_vals->condition > 0 )
    metric_vals->condition = (double) VERDICT_MIN( metric_vals->condition, VERDICT_DBL_MAX );
  else
    metric_vals->condition = (double) VERDICT_MAX( metric_vals->condition, -VERDICT_DBL_MAX );

  if( metric_vals->shape > 0 )
    metric_vals->shape = (double) VERDICT_MIN( metric_vals->shape, VERDICT_DBL_MAX );
  else
    metric_vals->shape = (double) VERDICT_MAX( metric_vals->shape, -VERDICT_DBL_MAX );

  if( metric_vals->radius_ratio > 0 )
    metric_vals->radius_ratio = (double) VERDICT_MIN( metric_vals->radius_ratio, VERDICT_DBL_MAX );\
  else
    metric_vals->radius_ratio = (double) VERDICT_MAX( metric_vals->radius_ratio, -VERDICT_DBL_MAX );

  if( metric_vals->scaled_jacobian > 0 )
    metric_vals->scaled_jacobian = (double) VERDICT_MIN( metric_vals->scaled_jacobian, VERDICT_DBL_MAX );
  else
    metric_vals->scaled_jacobian = (double) VERDICT_MAX( metric_vals->scaled_jacobian, -VERDICT_DBL_MAX );

  if( metric_vals->relative_size_squared > 0 )
    metric_vals->relative_size_squared = (double) VERDICT_MIN( metric_vals->relative_size_squared, VERDICT_DBL_MAX );
  else
    metric_vals->relative_size_squared = (double) VERDICT_MAX( metric_vals->relative_size_squared, -VERDICT_DBL_MAX );

  if( metric_vals->shape_and_size > 0 )
    metric_vals->shape_and_size = (double) VERDICT_MIN( metric_vals->shape_and_size, VERDICT_DBL_MAX );
  else
    metric_vals->shape_and_size = (double) VERDICT_MAX( metric_vals->shape_and_size, -VERDICT_DBL_MAX );

  if( metric_vals->distortion > 0 )
    metric_vals->distortion = (double) VERDICT_MIN( metric_vals->distortion, VERDICT_DBL_MAX );
  else
    metric_vals->distortion = (double) VERDICT_MAX( metric_vals->distortion, -VERDICT_DBL_MAX );
}
