
/*
 *
 * TetMetric.cpp contains quality calculations for Tets
 *
 * Copyright (C) 2003 Sandia National Laboratories <cubit@sandia.gov>
 *
 * This file is part of VERDICT
 *
 * This copy of VERDICT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * VERDICT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#define VERDICT_EXPORTS

#include "verdict.h"
#include "verdict_defines.hpp"
#include "VerdictVector.hpp"
#include "V_GaussIntegration.hpp"
#include <memory.h>

//! the average volume of a tet
VERDICT_REAL verdict_tet_size = 0;

/*! 
  set the average volume of a tet
*/
C_FUNC_DEF void v_set_tet_size( VERDICT_REAL size )
{
  verdict_tet_size = size;
}

/*!
  get the weights based on the average size
  of a tet
*/
int get_weight ( VerdictVector &w1,
                 VerdictVector &w2,
                 VerdictVector &w3 )
{
  static const double rt3 = sqrt(3.0);
  static const double root_of_2 = sqrt(2.0);
  
  w1.set(1,0,0);
  w2.set(0.5, 0.5*rt3, 0 );
  w3.set(0.5, rt3/6.0, root_of_2/rt3); 

  double scale = pow( 6.*verdict_tet_size/determinant(w1,w2,w3),0.3333333333333);   

  w1 *= scale;
  w2 *= scale;
  w3 *= scale;
  
  return 1;
}

/*!
  the scaled jacobian of a tet

  minimum of the jacobian divided by the lengths of 3 edge vectors

*/
C_FUNC_DEF VERDICT_REAL v_tet_scaled_jacobian( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{

  VerdictVector side0, side1, side2, side3, side4, side5;

  side0.set( coordinates[1][0] - coordinates[0][0],
             coordinates[1][1] - coordinates[0][1],
             coordinates[1][2] - coordinates[0][2] );
  
  side1.set( coordinates[2][0] - coordinates[1][0],
             coordinates[2][1] - coordinates[1][1],
             coordinates[2][2] - coordinates[1][2] );
  
  side2.set( coordinates[0][0] - coordinates[2][0],
             coordinates[0][1] - coordinates[2][1],
             coordinates[0][2] - coordinates[2][2] );

  side3.set( coordinates[3][0] - coordinates[0][0],
             coordinates[3][1] - coordinates[0][1],
             coordinates[3][2] - coordinates[0][2] );
  
  side4.set( coordinates[3][0] - coordinates[1][0],
             coordinates[3][1] - coordinates[1][1],
             coordinates[3][2] - coordinates[1][2] );
  
  side5.set( coordinates[3][0] - coordinates[2][0],
             coordinates[3][1] - coordinates[2][1],
             coordinates[3][2] - coordinates[2][2] );
  
  double jacobi;

  jacobi = side3 % ( side2 * side0 );

  // products of lengths squared of each edge attached to a node.
  double length_squared[4] = {
    side0.length_squared() * side2.length_squared() * side3.length_squared(),
    side0.length_squared() * side1.length_squared() * side4.length_squared(),
    side1.length_squared() * side2.length_squared() * side5.length_squared(),
    side3.length_squared() * side4.length_squared() * side5.length_squared()
  };
  int which_node = 0;
  if(length_squared[1] > length_squared[which_node])
    which_node = 1;
  if(length_squared[2] > length_squared[which_node])
    which_node = 2;
  if(length_squared[3] > length_squared[which_node])
    which_node = 3;
  
  double length_product = sqrt( length_squared[which_node] );
  if(length_product < fabs(jacobi))
    length_product = fabs(jacobi);

  if( length_product < VERDICT_DBL_MIN )
    return (VERDICT_REAL) VERDICT_DBL_MAX; 

  static const double root_of_2 = sqrt(2.0);

  return (VERDICT_REAL)(root_of_2 * jacobi / length_product);

}

/*!
  the radius ratio of a tet

  CR / (3.0*IR) where CR is the circumsphere radius and IR is the inscribed sphere radius
*/
C_FUNC_DEF VERDICT_REAL v_tet_radius_ratio( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{

  //Determine side vectors
  VerdictVector side[6];

  side[0].set( coordinates[1][0] - coordinates[0][0],
               coordinates[1][1] - coordinates[0][1],
               coordinates[1][2] - coordinates[0][2] );
  
  side[1].set( coordinates[2][0] - coordinates[1][0],
               coordinates[2][1] - coordinates[1][1],
               coordinates[2][2] - coordinates[1][2] );
  
  side[2].set( coordinates[0][0] - coordinates[2][0],
               coordinates[0][1] - coordinates[2][1],
               coordinates[0][2] - coordinates[2][2] );

  side[3].set( coordinates[3][0] - coordinates[0][0],
               coordinates[3][1] - coordinates[0][1],
               coordinates[3][2] - coordinates[0][2] );
  
  side[4].set( coordinates[3][0] - coordinates[1][0],
               coordinates[3][1] - coordinates[1][1],
               coordinates[3][2] - coordinates[1][2] );
  
  side[5].set( coordinates[3][0] - coordinates[2][0],
               coordinates[3][1] - coordinates[2][1],
               coordinates[3][2] - coordinates[2][2] );

  VerdictVector numerator = side[3].length_squared() * ( side[2] * side[0]) +
                            side[2].length_squared() * ( side[3] * side[0]) +
                            side[0].length_squared() * ( side[3] * side[2]);

  double area_sum;
  area_sum = ((side[2] * side[0]).length() + 
              (side[3] * side[0]).length() +
              (side[4] * side[1]).length() + 
              (side[3] * side[2]).length() ) * 0.5;
  
  double volume = v_tet_volume(4, coordinates);
  
  if( volume < VERDICT_DBL_MIN ) 
    return (VERDICT_REAL)VERDICT_DBL_MAX;
  else
  {
    double radius_ratio;
    radius_ratio = numerator.length() * area_sum / (108*volume*volume); 
    
    if( radius_ratio > 0 )
      return (VERDICT_REAL) VERDICT_MIN( radius_ratio, VERDICT_DBL_MAX );
    return (VERDICT_REAL) VERDICT_MAX( radius_ratio, -VERDICT_DBL_MAX );
  }

}

/*!
  the radius ratio of a tet, a.k.a. "aspect beta"
  NB (P.Pebay 11/28/06): 
     this method is maintained for backwards compatibility only.
     It will become deprecated at some point.

  CR / (3.0*IR) where CR is the circumsphere radius and IR is the inscribed sphere radius
*/
C_FUNC_DEF VERDICT_REAL v_tet_aspect_beta( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{

  return v_tet_radius_ratio(4, coordinates);
  
}

/*!
  the aspect gamma of a tet

  srms^3 / (8.479670*V) where srms = sqrt(sum(Si^2)/6), where Si is the edge length
*/
C_FUNC_DEF VERDICT_REAL v_tet_aspect_gamma( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{

  //Determine side vectors
  VerdictVector side0, side1, side2, side3, side4, side5;

  side0.set( coordinates[1][0] - coordinates[0][0],
             coordinates[1][1] - coordinates[0][1], 
             coordinates[1][2] - coordinates[0][2] ); 

  side1.set( coordinates[2][0] - coordinates[1][0],
             coordinates[2][1] - coordinates[1][1],
             coordinates[2][2] - coordinates[1][2] );
  
  side2.set( coordinates[0][0] - coordinates[2][0],
             coordinates[0][1] - coordinates[2][1],
             coordinates[0][2] - coordinates[2][2] );

  side3.set( coordinates[3][0] - coordinates[0][0],
             coordinates[3][1] - coordinates[0][1],
             coordinates[3][2] - coordinates[0][2] );
  
  side4.set( coordinates[3][0] - coordinates[1][0],
             coordinates[3][1] - coordinates[1][1],
             coordinates[3][2] - coordinates[1][2] );
  
  side5.set( coordinates[3][0] - coordinates[2][0],
             coordinates[3][1] - coordinates[2][1],
             coordinates[3][2] - coordinates[2][2] );
  

  double volume = fabs( v_tet_volume(4, coordinates) );

  if( volume  < VERDICT_DBL_MIN )
    return (VERDICT_REAL)VERDICT_DBL_MAX;
  else
  {
    double srms = sqrt((side0.length_squared() + side1.length_squared() +
                        side2.length_squared() + side3.length_squared() +
                        side4.length_squared() + side5.length_squared()) / 6.0 );

    double aspect_ratio_gamma = pow(srms, 3) / (8.47967 * volume );  
    return (VERDICT_REAL)aspect_ratio_gamma;
  }
}

/*!
  the volume of a tet

  1/6 * jacobian at a corner node
*/
C_FUNC_DEF VERDICT_REAL v_tet_volume( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{

  //Determine side vectors
  VerdictVector side0, side2, side3;

  side0.set( coordinates[1][0] - coordinates[0][0],
             coordinates[1][1] - coordinates[0][1],
             coordinates[1][2] - coordinates[0][2] );
  
  side2.set( coordinates[0][0] - coordinates[2][0],
             coordinates[0][1] - coordinates[2][1],
             coordinates[0][2] - coordinates[2][2] );
  
  side3.set( coordinates[3][0] - coordinates[0][0],
             coordinates[3][1] - coordinates[0][1],
             coordinates[3][2] - coordinates[0][2] );

  return  (VERDICT_REAL)((side3 % (side2 * side0)) / 6.0);

}

/*!
  the condition of a tet

  condition number of the jacobian matrix at any corner
*/
C_FUNC_DEF VERDICT_REAL v_tet_condition( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{

  double condition, term1, term2, det;
  double rt3 = sqrt(3.0);
  double rt6 = sqrt(6.0);

  VerdictVector side0, side2, side3;
  
  side0.set(coordinates[1][0] - coordinates[0][0],
            coordinates[1][1] - coordinates[0][1],
            coordinates[1][2] - coordinates[0][2]);

  side2.set(coordinates[0][0] - coordinates[2][0],
            coordinates[0][1] - coordinates[2][1],
            coordinates[0][2] - coordinates[2][2]);

  side3.set(coordinates[3][0] - coordinates[0][0],
            coordinates[3][1] - coordinates[0][1],
            coordinates[3][2] - coordinates[0][2]);

  VerdictVector c_1, c_2, c_3; 

  c_1 = side0;
  c_2 = (-2*side2-side0)/rt3;
  c_3 = (3*side3+side2-side0)/rt6;

  term1 = c_1 % c_1 + c_2 % c_2 + c_3 % c_3;
  term2 = ( c_1 * c_2 ) % ( c_1 * c_2 ) + 
          ( c_2 * c_3 ) % ( c_2 * c_3 ) + 
          ( c_1 * c_3 ) % ( c_1 * c_3 );
  det = c_1 % ( c_2 * c_3 );
  
  if ( det <= VERDICT_DBL_MIN )
    return VERDICT_DBL_MAX;
  else 
    condition = sqrt( term1 * term2 ) /(3.0* det);
  
  return (VERDICT_REAL)condition;
}


/*!
  the jacobian of a tet

  TODO
*/
C_FUNC_DEF VERDICT_REAL v_tet_jacobian( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{
  VerdictVector side0, side2, side3;

  side0.set( coordinates[1][0] - coordinates[0][0],
             coordinates[1][1] - coordinates[0][1],
             coordinates[1][2] - coordinates[0][2] );
  
  side2.set( coordinates[0][0] - coordinates[2][0],
             coordinates[0][1] - coordinates[2][1],
             coordinates[0][2] - coordinates[2][2] );

  side3.set( coordinates[3][0] - coordinates[0][0],
             coordinates[3][1] - coordinates[0][1],
             coordinates[3][2] - coordinates[0][2] );
  

  return (VERDICT_REAL)(side3 % (side2 * side0));

}


/*!
  the shape of a tet

  3/ condition number of weighted jacobian matrix
*/
C_FUNC_DEF VERDICT_REAL v_tet_shape( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{

   static const double two_thirds = 2.0/3.0;
   static const double root_of_2 = sqrt(2.0);

   VerdictVector edge0, edge2, edge3;

  edge0.set(coordinates[1][0] - coordinates[0][0],
            coordinates[1][1] - coordinates[0][1],
            coordinates[1][2] - coordinates[0][2]);

  edge2.set(coordinates[0][0] - coordinates[2][0],
            coordinates[0][1] - coordinates[2][1],
            coordinates[0][2] - coordinates[2][2]);

  edge3.set(coordinates[3][0] - coordinates[0][0],
            coordinates[3][1] - coordinates[0][1],
            coordinates[3][2] - coordinates[0][2]);

  double jacobian = edge3 % (edge2 * edge0);
  if(jacobian < VERDICT_DBL_MIN){
    return (VERDICT_REAL)0.0;
  }
  double num = 3 * pow( root_of_2 * jacobian, two_thirds );
  double den = 1.5*(edge0%edge0  + edge2%edge2  + edge3%edge3)-
                   (edge0%-edge2 + -edge2%edge3 + edge3%edge0);

  if ( den < VERDICT_DBL_MIN ) 
    return (VERDICT_REAL)0.0;
    
  return (VERDICT_REAL)VERDICT_MAX( num/den, 0 );
}



/*!
  the relative size of a tet

  Min(J,1/J), where J is the determinant of the weighted Jacobian matrix
*/
C_FUNC_DEF VERDICT_REAL v_tet_relative_size_squared( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{
  double size;
  VerdictVector w1, w2, w3;
  get_weight(w1,w2,w3);
  double avg_volume = (w1 % (w2 *w3))/6.0;
  
  double volume = v_tet_volume(4, coordinates);

  if( avg_volume < VERDICT_DBL_MIN )
    return 0.0;
  else
  {
    size = volume/avg_volume;
    if( size <= VERDICT_DBL_MIN )
      return 0.0;
    if ( size > 1 ) 
      size = (double)(1)/size;
  }
  return (VERDICT_REAL)(size*size);
}


/*!
  the shape and size of a tet

  Product of the shape and relative size
*/
C_FUNC_DEF VERDICT_REAL v_tet_shape_and_size( int num_nodes, VERDICT_REAL coordinates[][3] )
{
  
  double shape, size;
  shape = v_tet_shape( num_nodes, coordinates );
  size = v_tet_relative_size_squared (num_nodes, coordinates );  
  
  return (VERDICT_REAL)(shape * size);

}



/*!
  the distortion of a tet
*/
C_FUNC_DEF VERDICT_REAL v_tet_distortion( int num_nodes, VERDICT_REAL coordinates[][3] )
{

   double distortion = VERDICT_DBL_MAX;
   int   number_of_gauss_points=0;
   if (num_nodes ==4)
      // for linear tet, the distortion is always 1 because
      // straight edge tets are the target shape for tet
      return 1.0;

   else if (num_nodes ==10)
      //use four integration points for quadratic tet
      number_of_gauss_points = 4;

   int number_dims = 3;
   int total_number_of_gauss_points = number_of_gauss_points;
   // use is_tri=1 to indicate this is for tet in 3D
   int is_tri =1;

   double shape_function[maxTotalNumberGaussPoints][maxNumberNodes];
   double dndy1[maxTotalNumberGaussPoints][maxNumberNodes];
   double dndy2[maxTotalNumberGaussPoints][maxNumberNodes];
   double dndy3[maxTotalNumberGaussPoints][maxNumberNodes];
   double weight[maxTotalNumberGaussPoints];

   // create an object of GaussIntegration for tet
   GaussIntegration::initialize(number_of_gauss_points,num_nodes, number_dims, is_tri);
   GaussIntegration::calculate_shape_function_3d_tet();
   GaussIntegration::get_shape_func(shape_function[0], dndy1[0], dndy2[0], dndy3[0],weight);

   // vector xxi is the derivative vector of coordinates w.r.t local xi coordinate in the
   // computation space
   // vector xet is the derivative vector of coordinates w.r.t local et coordinate in the
   // computation space
   // vector xze is the derivative vector of coordinates w.r.t local ze coordinate in the
   // computation space
   VerdictVector xxi, xet, xze, xin;

   double jacobian, minimum_jacobian;
   double element_volume =0.0;
   minimum_jacobian = VERDICT_DBL_MAX;

   // calculate element volume
   int ife, ja;
   for (ife=0;ife<total_number_of_gauss_points; ife++)
   {
      xxi.set(0.0,0.0,0.0);
      xet.set(0.0,0.0,0.0);
      xze.set(0.0,0.0,0.0);

      for (ja=0;ja<num_nodes;ja++)
      {
         xin.set(coordinates[ja][0], coordinates[ja][1], coordinates[ja][2]);
         xxi += dndy1[ife][ja]*xin;
         xet += dndy2[ife][ja]*xin;
         xze += dndy3[ife][ja]*xin;
      }

      // determinant
      jacobian = xxi % ( xet * xze );
      if (minimum_jacobian > jacobian)
         minimum_jacobian = jacobian;

      element_volume += weight[ife]*jacobian;
      }//element_volume is 6 times the actual volume

   // loop through all nodes
   double dndy1_at_node[maxNumberNodes][maxNumberNodes];
   double dndy2_at_node[maxNumberNodes][maxNumberNodes];
   double dndy3_at_node[maxNumberNodes][maxNumberNodes];


   GaussIntegration::calculate_derivative_at_nodes_3d_tet( dndy1_at_node,
                                                           dndy2_at_node,
                                                           dndy3_at_node);
   int node_id;
   for (node_id=0;node_id<num_nodes; node_id++)
   {
      xxi.set(0.0,0.0,0.0);
      xet.set(0.0,0.0,0.0);
      xze.set(0.0,0.0,0.0);

      for (ja=0;ja<num_nodes;ja++)
      {
         xin.set(coordinates[ja][0], coordinates[ja][1], coordinates[ja][2]);
         xxi += dndy1_at_node[node_id][ja]*xin;
         xet += dndy2_at_node[node_id][ja]*xin;
         xze += dndy3_at_node[node_id][ja]*xin;
      }

      jacobian = xxi % ( xet * xze );
      if (minimum_jacobian > jacobian)
         minimum_jacobian = jacobian;

      }
   distortion = minimum_jacobian/element_volume;

   return (VERDICT_REAL)distortion;
}


/*!
  the quality metrics of a tet
*/
C_FUNC_DEF void v_tet_quality( int num_nodes, VERDICT_REAL coordinates[][3], 
    unsigned int metrics_request_flag, TetMetricVals *metric_vals )
{

  memset( metric_vals, 0, sizeof(TetMetricVals) );

  /*
  
    node numbers and edge numbers below


    
             3 
             +            edge 0 is node 0 to 1
            +|+           edge 1 is node 1 to 2
          3/ | \5         edge 2 is node 0 to 2
          / 4|  \         edge 3 is node 0 to 3
        0 - -|- + 2       edge 4 is node 1 to 3
          \  |  +         edge 5 is node 2 to 3
          0\ | /1
            +|/           edge 2 is behind edge 4
             1 

             
  */

  // lets start with making the vectors
  VerdictVector edges[6];
  edges[0].set( coordinates[1][0] - coordinates[0][0],
                coordinates[1][1] - coordinates[0][1],
                coordinates[1][2] - coordinates[0][2] );

  edges[1].set( coordinates[2][0] - coordinates[1][0],
                coordinates[2][1] - coordinates[1][1],
                coordinates[2][2] - coordinates[1][2] );

  edges[2].set( coordinates[0][0] - coordinates[2][0],
                coordinates[0][1] - coordinates[2][1],
                coordinates[0][2] - coordinates[2][2] );

  edges[3].set( coordinates[3][0] - coordinates[0][0],
                coordinates[3][1] - coordinates[0][1],
                coordinates[3][2] - coordinates[0][2] );

  edges[4].set( coordinates[3][0] - coordinates[1][0],
                coordinates[3][1] - coordinates[1][1],
                coordinates[3][2] - coordinates[1][2] );

  edges[5].set( coordinates[3][0] - coordinates[2][0],
                coordinates[3][1] - coordinates[2][1],
                coordinates[3][2] - coordinates[2][2] );

  // common numbers
  static const double root_of_2 = sqrt(2.0);
 
  // calculate the jacobian 
  static const int do_jacobian = V_TET_JACOBIAN | V_TET_VOLUME | 
    V_TET_ASPECT_BETA | V_TET_ASPECT_GAMMA | V_TET_SHAPE | 
    V_TET_RELATIVE_SIZE_SQUARED | V_TET_SHAPE_AND_SIZE | 
    V_TET_SCALED_JACOBIAN | V_TET_CONDITION;
  if(metrics_request_flag & do_jacobian )
  {
    metric_vals->jacobian = (VERDICT_REAL)(edges[3] % (edges[2] * edges[0]));
  }
 
  // calculate the volume 
  if(metrics_request_flag & V_TET_VOLUME)
  {
    metric_vals->volume = (VERDICT_REAL)(metric_vals->jacobian / 6.0);
  }
  
  // calculate aspect ratio
  if(metrics_request_flag & V_TET_ASPECT_BETA)
  {
    double surface_area = ((edges[2] * edges[0]).length() + 
                           (edges[3] * edges[0]).length() +
                           (edges[4] * edges[1]).length() + 
                           (edges[3] * edges[2]).length() ) * 0.5;

    VerdictVector numerator = edges[3].length_squared() * ( edges[2] * edges[0] ) +
                              edges[2].length_squared() * ( edges[3] * edges[0] ) +
                              edges[0].length_squared() * ( edges[3] * edges[2] );

    double volume = metric_vals->jacobian / 6.0;

    if(volume < VERDICT_DBL_MIN )
      metric_vals->aspect_beta = (VERDICT_REAL)(VERDICT_DBL_MAX);
    else
      metric_vals->aspect_beta = 
        (VERDICT_REAL)( numerator.length() * surface_area/ (108*volume*volume) );
  }

  // calculate the aspect gamma 
  if(metrics_request_flag & V_TET_ASPECT_GAMMA)
  {
    double volume = fabs( metric_vals->jacobian / 6.0 );
    if( fabs( volume ) < VERDICT_DBL_MIN ) 
      metric_vals->aspect_gamma = VERDICT_DBL_MAX;
    else
    {
      double srms = sqrt((
            edges[0].length_squared() + edges[1].length_squared() +
            edges[2].length_squared() + edges[3].length_squared() +
            edges[4].length_squared() + edges[5].length_squared()
            ) / 6.0 );

      // cube the srms
      srms *= (srms * srms);
      metric_vals->aspect_gamma = (VERDICT_REAL)( srms / (8.47967 * volume ));
    }
  }

  // calculate the shape of the tet
  if(metrics_request_flag & ( V_TET_SHAPE | V_TET_SHAPE_AND_SIZE ) )
  {
      //if the jacobian is non-positive, the shape is 0
    if(metric_vals->jacobian < VERDICT_DBL_MIN){
      metric_vals->shape = (VERDICT_REAL)0.0;
    }
    else{
      static const double two_thirds = 2.0/3.0;
      double num = 3.0 * pow(root_of_2 * metric_vals->jacobian, two_thirds);
      double den = 1.5 *
        (edges[0] % edges[0]  + edges[2] % edges[2]  + edges[3] % edges[3]) -
        (edges[0] % -edges[2] + -edges[2] % edges[3] + edges[3] % edges[0]);

      if( den < VERDICT_DBL_MIN )
        metric_vals->shape = (VERDICT_REAL)0.0;
      else
        metric_vals->shape = (VERDICT_REAL)VERDICT_MAX( num/den, 0 );
    }
    
  }
  
  // calculate the relative size of the tet
  if(metrics_request_flag & (V_TET_RELATIVE_SIZE_SQUARED | V_TET_SHAPE_AND_SIZE ))
  {
    VerdictVector w1, w2, w3;
    get_weight(w1,w2,w3);
    double avg_vol = (w1 % (w2 *w3))/6;
    
    if( avg_vol < VERDICT_DBL_MIN )
      metric_vals->relative_size_squared = 0.0; 
    else
    {
      double tmp = metric_vals->jacobian / (6*avg_vol);
      if( tmp < VERDICT_DBL_MIN )
        metric_vals->relative_size_squared = 0.0; 
      else
      {
        tmp *= tmp;
        metric_vals->relative_size_squared = (VERDICT_REAL)VERDICT_MIN(tmp, 1/tmp);
      }
    }
  }
  
  // calculate the shape and size
  if(metrics_request_flag & V_TET_SHAPE_AND_SIZE)
  {
    metric_vals->shape_and_size = (VERDICT_REAL)(metric_vals->shape * metric_vals->relative_size_squared);
  }
  
  // calculate the scaled jacobian
  if(metrics_request_flag & V_TET_SCALED_JACOBIAN)
  {
    //find out which node the normalized jacobian can be calculated at
    //and it will be the smaller than at other nodes
    double length_squared[4] = {
      edges[0].length_squared() * edges[2].length_squared() * edges[3].length_squared(),
      edges[0].length_squared() * edges[1].length_squared() * edges[4].length_squared(),
      edges[1].length_squared() * edges[2].length_squared() * edges[5].length_squared(),
      edges[3].length_squared() * edges[4].length_squared() * edges[5].length_squared()
    };
    
    int which_node = 0;
    if(length_squared[1] > length_squared[which_node])
      which_node = 1;
    if(length_squared[2] > length_squared[which_node])
      which_node = 2;
    if(length_squared[3] > length_squared[which_node])
      which_node = 3;

    // find the scaled jacobian at this node
    double length_product = sqrt( length_squared[which_node] );
    if(length_product < fabs(metric_vals->jacobian))
      length_product = fabs(metric_vals->jacobian);

    if( length_product < VERDICT_DBL_MIN )
      metric_vals->scaled_jacobian = (VERDICT_REAL) VERDICT_DBL_MAX; 
    else
      metric_vals->scaled_jacobian = 
        (VERDICT_REAL)(root_of_2 * metric_vals->jacobian / length_product);
  }
  
  // calculate the condition number
  if(metrics_request_flag & V_TET_CONDITION)
  {
    static const double root_of_3 = sqrt(3.0);
    static const double root_of_6 = sqrt(6.0);

    VerdictVector c_1, c_2, c_3; 
    c_1 = edges[0];
    c_2 = (-2*edges[2] - edges[0])/root_of_3;
    c_3 = (3*edges[3] + edges[2] - edges[0])/root_of_6;

    double term1 =  c_1 % c_1 + c_2 % c_2 + c_3 % c_3;
    double term2 = ( c_1 * c_2 ) % ( c_1 * c_2 ) + 
                   ( c_2 * c_3 ) % ( c_2 * c_3 ) + 
                   ( c_3 * c_1 ) % ( c_3 * c_1 );

    double det = c_1 % ( c_2 * c_3 );

    if(det <= VERDICT_DBL_MIN)
      metric_vals->condition = (VERDICT_REAL)VERDICT_DBL_MAX; 
    else
      metric_vals->condition = (VERDICT_REAL)(sqrt(term1 * term2) / (3.0*det)); 
  }
    
  // calculate the distortion
  if(metrics_request_flag & V_TET_DISTORTION)
  {
    metric_vals->distortion = v_tet_distortion(num_nodes, coordinates);
  }

  //check for overflow
  if(metrics_request_flag & V_TET_ASPECT_BETA )
  {
    if( metric_vals->aspect_beta > 0 ) 
      metric_vals->aspect_beta = (VERDICT_REAL) VERDICT_MIN( metric_vals->aspect_beta, VERDICT_DBL_MAX );
    metric_vals->aspect_beta = (VERDICT_REAL) VERDICT_MAX( metric_vals->aspect_beta, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_ASPECT_GAMMA)
  {
    if( metric_vals->aspect_gamma > 0 ) 
      metric_vals->aspect_gamma = (VERDICT_REAL) VERDICT_MIN( metric_vals->aspect_gamma, VERDICT_DBL_MAX );
    metric_vals->aspect_gamma = (VERDICT_REAL) VERDICT_MAX( metric_vals->aspect_gamma, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_VOLUME)
  {
    if( metric_vals->volume > 0 ) 
      metric_vals->volume = (VERDICT_REAL) VERDICT_MIN( metric_vals->volume, VERDICT_DBL_MAX );
    metric_vals->volume = (VERDICT_REAL) VERDICT_MAX( metric_vals->volume, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_CONDITION)
  {
    if( metric_vals->condition > 0 ) 
      metric_vals->condition = (VERDICT_REAL) VERDICT_MIN( metric_vals->condition, VERDICT_DBL_MAX );
    metric_vals->condition = (VERDICT_REAL) VERDICT_MAX( metric_vals->condition, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_JACOBIAN)
  {
    if( metric_vals->jacobian > 0 ) 
      metric_vals->jacobian = (VERDICT_REAL) VERDICT_MIN( metric_vals->jacobian, VERDICT_DBL_MAX );
    metric_vals->jacobian = (VERDICT_REAL) VERDICT_MAX( metric_vals->jacobian, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_SCALED_JACOBIAN)
  {
    if( metric_vals->scaled_jacobian > 0 ) 
      metric_vals->scaled_jacobian = (VERDICT_REAL) VERDICT_MIN( metric_vals->scaled_jacobian, VERDICT_DBL_MAX );
    metric_vals->scaled_jacobian = (VERDICT_REAL) VERDICT_MAX( metric_vals->scaled_jacobian, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_SHAPE)
  {
    if( metric_vals->shape > 0 ) 
      metric_vals->shape = (VERDICT_REAL) VERDICT_MIN( metric_vals->shape, VERDICT_DBL_MAX );
    metric_vals->shape = (VERDICT_REAL) VERDICT_MAX( metric_vals->shape, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_RELATIVE_SIZE_SQUARED)
  {
    if( metric_vals->relative_size_squared > 0 ) 
      metric_vals->relative_size_squared = (VERDICT_REAL) VERDICT_MIN( metric_vals->relative_size_squared, VERDICT_DBL_MAX );
    metric_vals->relative_size_squared = (VERDICT_REAL) VERDICT_MAX( metric_vals->relative_size_squared, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_SHAPE_AND_SIZE)
  {
    if( metric_vals->shape_and_size > 0 ) 
      metric_vals->shape_and_size = (VERDICT_REAL) VERDICT_MIN( metric_vals->shape_and_size, VERDICT_DBL_MAX );
    metric_vals->shape_and_size = (VERDICT_REAL) VERDICT_MAX( metric_vals->shape_and_size, -VERDICT_DBL_MAX );
  }

  if(metrics_request_flag & V_TET_DISTORTION)
  {
    if( metric_vals->distortion > 0 ) 
      metric_vals->distortion = (VERDICT_REAL) VERDICT_MIN( metric_vals->distortion, VERDICT_DBL_MAX );
    metric_vals->distortion = (VERDICT_REAL) VERDICT_MAX( metric_vals->distortion, -VERDICT_DBL_MAX );
  }


}


