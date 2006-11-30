
/*
 *
 * GaussIntegration.hpp declaration of gauss integration functions
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
 *
 */


#ifndef GAUSS_INTEGRATION_HPP
#define GAUSS_INTEGRATION_HPP


#include "verdict.h"


#define maxTotalNumberGaussPoints 27
#define maxNumberNodes 20
#define maxNumberGaussPoints 3
#define maxNumberGaussPointsTri 6
#define maxNumberGaussPointsTet 4


namespace GaussIntegration  
{


   void get_signs_for_node_local_coord_hex(int node_id, double &sign_y1, 
                                           double &sign_y2, double &sign_y3);
   //- to get the signs for  coordinates of hex nodes in the local computational space 

   //constructors
   void initialize(int n=2, int m=4, int dim=2, int tri=0);
    
   //manipulators
   void get_gauss_pts_and_weight();
   //- get gauss point locations and weights

   void get_tri_rule_pts_and_weight();
   //- get integration points and weights for triangular rules

   void calculate_shape_function_2d_tri();
   //- calculate the shape functions and derivatives of shape functions 
   //- at integration points for 2D triangular elements

   void calculate_shape_function_2d_quad();
   //- calculate the shape functions and derivatives of shape functions 
   //- at gaussian points for 2D quad elements

   void get_shape_func(double shape_function[], double dndy1_at_gauss_pts[], double dndy2_at_gauss_ptsp[], double gauss_weight[]);
   //- get shape functions and the derivatives

   void get_shape_func(double shape_function[], double dndy1_at_gauss_pts[], 
                       double dndy2_at_gauss_pts[], double dndy3_at_gauss_pts[],
                       double gauss_weight[]); 
   //- get shape functions and the derivatives for 3D elements

   void calculate_derivative_at_nodes(double dndy1_at_nodes[][maxNumberNodes],
                                      double dndy2_at_nodes[][maxNumberNodes]);
   //- calculate shape function derivatives at nodes

   void calculate_shape_function_3d_hex();
   //- calculate shape functions and derivatives of shape functions 
   //- at gaussian points for 3D hex elements

   void calculate_derivative_at_nodes_3d(double dndy1_at_nodes[][maxNumberNodes],
                                         double dndy2_at_nodes[][maxNumberNodes],
                                         double dndy3_at_nodes[][maxNumberNodes]);
   //- calculate shape function derivatives at nodes for hex elements

   void calculate_derivative_at_nodes_2d_tri(double dndy1_at_nodes[][maxNumberNodes],
                                             double dndy2_at_nodes[][maxNumberNodes]);
   //- calculate shape function derivatives at nodes for triangular elements

   void calculate_shape_function_3d_tet();
   //- calculate shape functions and derivatives of shape functions 
   //- at integration points for 3D tet elements

   void get_tet_rule_pts_and_weight();
   //- get integration points and weights for tetrhedron rules

   void calculate_derivative_at_nodes_3d_tet(double dndy1_at_nodes[][maxNumberNodes], 
                                             double dndy2_at_nodes[][maxNumberNodes],
                                             double dndy3_at_nodes[][maxNumberNodes]);
   //- calculate shape function derivatives at nodes for tetrahedron elements
   
   void get_node_local_coord_tet(int node_id, double &y1, double &y2, 
                                                double &y3, double &y4);
   //- get nodal volume coordinates for tetrahedron element
};

#endif 
