/*=========================================================================

  Module:    V_GaussIntegration.hpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * GaussIntegration.hpp declaration of gauss integration functions
 *
 * This file is part of VERDICT
 *
 */

#ifndef GAUSS_INTEGRATION_HPP
#define GAUSS_INTEGRATION_HPP

#include "verdict.h"

namespace VERDICT_NAMESPACE
{
static constexpr int maxTotalNumberGaussPoints = 27;
static constexpr int maxNumberNodes = 20;
static constexpr int maxNumberGaussPoints = 3;
static constexpr int maxNumberGaussPointsTri = 6;
static constexpr int maxNumberGaussPointsTet = 4;

struct GaussIntegration
{
  VERDICT_HOST_DEVICE void get_signs_for_node_local_coord_hex(
    int node_id, double& sign_y1, double& sign_y2, double& sign_y3);
  //- to get the signs for  coordinates of hex nodes in the local computational space

  // constructors
  VERDICT_HOST_DEVICE void initialize(int n = 2, int m = 4, int dim = 2, int tri = 0);

  // manipulators
  VERDICT_HOST_DEVICE void get_gauss_pts_and_weight();
  //- get gauss point locations and weights

  VERDICT_HOST_DEVICE void get_tri_rule_pts_and_weight();
  //- get integration points and weights for triangular rules

  VERDICT_HOST_DEVICE void calculate_shape_function_2d_tri();
  //- calculate the shape functions and derivatives of shape functions
  //- at integration points for 2D triangular elements

  VERDICT_HOST_DEVICE void calculate_shape_function_2d_quad();
  //- calculate the shape functions and derivatives of shape functions
  //- at gaussian points for 2D quad elements

  VERDICT_HOST_DEVICE void get_shape_func(double shape_function[], double dndy1_at_gauss_pts[],
    double dndy2_at_gauss_ptsp[], double gauss_weight[]);
  //- get shape functions and the derivatives

  VERDICT_HOST_DEVICE void get_shape_func(double shape_function[], double dndy1_at_gauss_pts[],
    double dndy2_at_gauss_pts[], double dndy3_at_gauss_pts[], double gauss_weight[]);
  //- get shape functions and the derivatives for 3D elements

  VERDICT_HOST_DEVICE void calculate_derivative_at_nodes(
    double dndy1_at_nodes[][maxNumberNodes], double dndy2_at_nodes[][maxNumberNodes]);
  //- calculate shape function derivatives at nodes

  VERDICT_HOST_DEVICE void calculate_shape_function_3d_hex();
  //- calculate shape functions and derivatives of shape functions
  //- at gaussian points for 3D hex elements

  VERDICT_HOST_DEVICE void calculate_derivative_at_nodes_3d(double dndy1_at_nodes[][maxNumberNodes],
    double dndy2_at_nodes[][maxNumberNodes], double dndy3_at_nodes[][maxNumberNodes]);
  //- calculate shape function derivatives at nodes for hex elements

  VERDICT_HOST_DEVICE void calculate_derivative_at_nodes_2d_tri(
    double dndy1_at_nodes[][maxNumberNodes], double dndy2_at_nodes[][maxNumberNodes]);
  //- calculate shape function derivatives at nodes for triangular elements

  VERDICT_HOST_DEVICE void calculate_shape_function_3d_tet();
  //- calculate shape functions and derivatives of shape functions
  //- at integration points for 3D tet elements

  VERDICT_HOST_DEVICE void get_tet_rule_pts_and_weight();
  //- get integration points and weights for tetrhedron rules

  VERDICT_HOST_DEVICE void calculate_derivative_at_nodes_3d_tet(double dndy1_at_nodes[][maxNumberNodes],
    double dndy2_at_nodes[][maxNumberNodes], double dndy3_at_nodes[][maxNumberNodes]);
  //- calculate shape function derivatives at nodes for tetrahedron elements

  VERDICT_HOST_DEVICE void get_node_local_coord_tet(int node_id, double& y1, double& y2, double& y3, double& y4);
  //- get nodal volume coordinates for tetrahedron element

  int numberGaussPoints;
  int numberNodes;
  int numberDims;
  double gaussPointY[maxNumberGaussPoints];
  double gaussWeight[maxNumberGaussPoints];
  double shapeFunction[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy1GaussPts[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy2GaussPts[maxTotalNumberGaussPoints][maxNumberNodes];
  double dndy3GaussPts[maxTotalNumberGaussPoints][maxNumberNodes];
  double totalGaussWeight[maxTotalNumberGaussPoints];
  int totalNumberGaussPts;
  double y1Area[maxNumberGaussPointsTri];
  double y2Area[maxNumberGaussPointsTri];
  double y1Volume[maxNumberGaussPointsTet];
  double y2Volume[maxNumberGaussPointsTet];
  double y3Volume[maxNumberGaussPointsTet];
  double y4Volume[maxNumberGaussPointsTet];
};
} // namespace verdict

#endif
