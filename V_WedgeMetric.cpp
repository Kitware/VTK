/*=========================================================================

  Module:    V_WedgeMetric.cpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * WedgeMetric.cpp contains quality calculations for wedges
 *
 * This file is part of VERDICT
 *
 */

#include "VerdictVector.hpp"
#include "verdict.h"

#include <algorithm>
#include <cmath> // for std::isnan

namespace VERDICT_NAMESPACE
{
extern double tri_equiangle_skew(int num_nodes, const double coordinates[][3]);
extern double quad_equiangle_skew(int num_nodes, const double coordinates[][3]);

static const double one_third = 1.0 / 3.0;
static const double two_thirds = 2.0 / 3.0;

// local methods
void make_wedge_faces(const double coordinates[][3], double tri1[][3], double tri2[][3],
  double quad1[][3], double quad2[][3], double quad3[][3]);

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

static const double WEDGE21_node_local_coord[21][3] = { { 0, 0, -1 }, { 1.0, 0, -1 },
  { 0, 1.0, -1 }, { 0, 0, 1.0 }, { 1.0, 0, 1.0 }, { 0, 1.0, 1.0 }, { 0.5, 0, -1 }, { 0.5, 0.5, -1 },
  { 0, 0.5, -1 }, { 0.0, 0.0, 0 }, { 1.0, 0, 0 }, { 0, 1.0, 0 }, { 0.5, 0, 1.0 }, { 0.5, 0.5, 1.0 },
  { 0, 0.5, 1.0 }, { one_third, one_third, 0 }, { one_third, one_third, -1 },
  { one_third, one_third, 1.0 }, { 0.5, 0.5, 0 }, { 0, 0.5, 0 }, { 0.5, 0, 0 } };

static void WEDGE21_gradients_of_the_shape_functions_for_RST(
  const double rst[3], double dhdr[21], double dhds[21], double dhdt[21])
{
  double RSM = 1.0 - rst[0] - rst[1];
  double RR = rst[0] * rst[0];
  double RS = rst[0] * rst[1];
  double SS = rst[1] * rst[1];
  double TP = 1.0 + rst[2];
  double TM = 1.0 - rst[2];
  double T2P = 1.0 + 2.0 * rst[2];
  double T2M = 1.0 - 2.0 * rst[2];

  dhdr[0] = -0.5 * rst[2] * TM * (4.0 * rst[0] + 7.0 * rst[1] - 3.0 - 6.0 * RS - 3.0 * SS);
  dhds[0] = -0.5 * rst[2] * TM * (7.0 * rst[0] + 4.0 * rst[1] - 3.0 - 6.0 * RS - 3.0 * RR);
  dhdt[0] = -0.5 * T2M * RSM * (1.0 - 2.0 * (rst[0] + rst[1]) + 3.0 * RS);

  dhdr[1] = -0.5 * rst[2] * TM * (4.0 * rst[0] - 1.0 + 3.0 * rst[1] - 6.0 * RS - 3.0 * SS);
  dhds[1] = -0.5 * rst[2] * TM * (3.0 * rst[0] - 6.0 * RS - 3.0 * RR);
  dhdt[1] = -0.5 * T2M * (rst[0] - 2.0 * (RSM * rst[0] + RS) + 3.0 * RSM * RS);

  dhdr[2] = -0.5 * rst[2] * TM * (3.0 * rst[1] - 6.0 * RS - 3.0 * SS);
  dhds[2] = -0.5 * rst[2] * TM * (4.0 * rst[1] - 1.0 + 3.0 * rst[0] - 6.0 * RS - 3.0 * RR);
  dhdt[2] = -0.5 * T2M * (rst[1] - 2.0 * (RSM * rst[1] + RS) + 3.0 * RSM * RS);

  dhdr[3] = 0.5 * rst[2] * TP * (4.0 * rst[0] + 7.0 * rst[1] - 3.0 - 6.0 * RS - 3.0 * SS);
  dhds[3] = 0.5 * rst[2] * TP * (7.0 * rst[0] + 4.0 * rst[1] - 3.0 - 6.0 * RS - 3.0 * RR);
  dhdt[3] = 0.5 * T2P * RSM * (1.0 - 2.0 * (rst[0] + rst[1]) + 3.0 * RS);

  dhdr[4] = 0.5 * rst[2] * TP * (4.0 * rst[0] - 1.0 + 3.0 * rst[1] - 6.0 * RS - 3.0 * SS);
  dhds[4] = 0.5 * rst[2] * TP * (3.0 * rst[0] - 6.0 * RS - 3.0 * RR);
  dhdt[4] = 0.5 * T2P * (rst[0] - 2.0 * (RSM * rst[0] + RS) + 3.0 * RSM * RS);

  dhdr[5] = 0.5 * rst[2] * TP * (3.0 * rst[1] - 6.0 * RS - 3.0 * SS);
  dhds[5] = 0.5 * rst[2] * TP * (4.0 * rst[1] - 1.0 + 3.0 * rst[0] - 6.0 * RS - 3.0 * RR);
  dhdt[5] = 0.5 * T2P * (rst[1] - 2.0 * (RSM * rst[1] + RS) + 3.0 * RSM * RS);

  dhdr[6] = -0.5 * rst[2] * TM * (4.0 - 8.0 * rst[0] - 16.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[6] = -0.5 * rst[2] * TM * (-16.0 * rst[0] + 12.0 * RR + 24.0 * RS);
  dhdt[6] = -0.5 * T2M * RSM * (4.0 * rst[0] - 12.0 * RS);

  dhdr[7] = -0.5 * rst[2] * TM * (-8.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[7] = -0.5 * rst[2] * TM * (-8.0 * rst[0] + 12.0 * RR + 24.0 * RS);
  dhdt[7] = -0.5 * T2M * (4.0 * RS - 12.0 * RSM * RS);

  dhdr[8] = -0.5 * rst[2] * TM * (-16.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[8] = -0.5 * rst[2] * TM * (4.0 - 16.0 * rst[0] - 8.0 * rst[1] + 12.0 * RR + 24.0 * RS);
  dhdt[8] = -0.5 * T2M * RSM * (4.0 * rst[1] - 12.0 * RS);

  dhdr[12] = 0.5 * rst[2] * TP * (4.0 - 8.0 * rst[0] - 16.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[12] = 0.5 * rst[2] * TP * (-16.0 * rst[0] + 12.0 * RR + 24.0 * RS);
  dhdt[12] = 0.5 * T2P * RSM * (4.0 * rst[0] - 12.0 * RS);

  dhdr[13] = 0.5 * rst[2] * TP * (-8.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[13] = 0.5 * rst[2] * TP * (-8.0 * rst[0] + 12.0 * RR + 24.0 * RS);
  dhdt[13] = 0.5 * T2P * (4.0 * RS - 12.0 * RSM * RS);

  dhdr[14] = 0.5 * rst[2] * TP * (-16.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[14] = 0.5 * rst[2] * TP * (4.0 - 16.0 * rst[0] - 8.0 * rst[1] + 12.0 * RR + 24.0 * RS);
  dhdt[14] = 0.5 * T2P * RSM * (4.0 * rst[1] - 12.0 * RS);

  dhdr[9] = TP * TM * (4.0 * rst[0] + 7.0 * rst[1] - 3.0 - 6.0 * RS - 3.0 * SS);
  dhds[9] = TP * TM * (7.0 * rst[0] + 4.0 * rst[1] - 3.0 - 6.0 * RS - 3.0 * RR);
  dhdt[9] = -2.0 * rst[2] * RSM * (1.0 - 2.0 * (rst[0] + rst[1]) + 3.0 * RS);

  dhdr[10] = TP * TM * (4.0 * rst[0] - 1.0 + 3.0 * rst[1] - 6.0 * RS - 3.0 * SS);
  dhds[10] = TP * TM * (3.0 * rst[0] - 6.0 * RS - 3.0 * RR);
  dhdt[10] = -2.0 * rst[2] * (rst[0] - 2.0 * (RSM * rst[0] + RS) + 3.0 * RSM * RS);

  dhdr[11] = TP * TM * (3.0 * rst[1] - 6.0 * RS - 3.0 * SS);
  dhds[11] = TP * TM * (4.0 * rst[1] - 1.0 + 3.0 * rst[0] - 6.0 * RS - 3.0 * RR);
  dhdt[11] = -2.0 * rst[2] * (rst[1] - 2.0 * (RSM * rst[1] + RS) + 3.0 * RSM * RS);

  dhdr[16] = -0.5 * 27.0 * rst[2] * TM * (rst[1] - 2.0 * RS - SS);
  dhds[16] = -0.5 * 27.0 * rst[2] * TM * (rst[0] - RR - 2.0 * RS);
  dhdt[16] = -0.5 * 27.0 * T2M * RSM * RS;

  dhdr[17] = 0.5 * 27.0 * rst[2] * TP * (rst[1] - 2.0 * RS - SS);
  dhds[17] = 0.5 * 27.0 * rst[2] * TP * (rst[0] - RR - 2.0 * RS);
  dhdt[17] = 0.5 * 27.0 * T2P * RSM * RS;

  dhdr[20] = TP * TM * (4.0 - 8.0 * rst[0] - 16.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[20] = TP * TM * (-16.0 * rst[0] + 12.0 * RR + 24.0 * RS);
  dhdt[20] = -2.0 * rst[2] * RSM * (4.0 * rst[0] - 12.0 * RS);

  dhdr[18] = TP * TM * (-8.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[18] = TP * TM * (-8.0 * rst[0] + 12.0 * RR + 24.0 * RS);
  dhdt[18] = -2.0 * rst[2] * (4.0 * RS - 12.0 * RSM * RS);

  dhdr[19] = TP * TM * (-16.0 * rst[1] + 24.0 * RS + 12.0 * SS);
  dhds[19] = TP * TM * (4.0 - 16.0 * rst[0] - 8.0 * rst[1] + 12.0 * RR + 24.0 * RS);
  dhdt[19] = -2.0 * rst[2] * RSM * (4.0 * rst[1] - 12.0 * RS);

  dhdr[15] = 27.0 * TM * TP * (rst[1] - 2.0 * RS - SS);
  dhds[15] = 27.0 * TM * TP * (rst[0] - RR - 2.0 * RS);
  dhdt[15] = -2.0 * 27.0 * rst[2] * RSM * RS;
}

double wedge_equiangle_skew(int /*num_nodes*/, const double coordinates[][3])
{
  double tri1[3][3];
  double tri2[3][3];
  double quad1[4][3];
  double quad2[4][3];
  double quad3[4][3];

  make_wedge_faces(coordinates, tri1, tri2, quad1, quad2, quad3);

  double tri1_skew = tri_equiangle_skew(3, tri1);
  double tri2_skew = tri_equiangle_skew(3, tri2);
  double quad1_skew = quad_equiangle_skew(4, quad1);
  double quad2_skew = quad_equiangle_skew(4, quad2);
  double quad3_skew = quad_equiangle_skew(4, quad3);

  double max_skew = tri1_skew;
  max_skew = max_skew > tri2_skew ? max_skew : tri2_skew;
  max_skew = max_skew > quad1_skew ? max_skew : quad1_skew;
  max_skew = max_skew > quad2_skew ? max_skew : quad2_skew;
  max_skew = max_skew > quad3_skew ? max_skew : quad3_skew;

  return max_skew;
}

/*!
  calculate the volume of a wedge

  this is done by dividing the wedge into 11 tets
  and summing the volume of each tet

 */
double wedge_volume(int /*num_nodes*/, const double coordinates[][3])
{
  // We need to divide the wedge into 11 tets.
  // This is a better solution than 3 tets or 3 hexes because
  // if the wedge is twisted then the 3 quads will be twisted.
  // This presents a problem when you have multiple wedges next to
  // each other.  A hex or tet representation of a wedge may vary
  // from one wedge to another.  This means that if wedge A splits
  // a quad one way, wedge B may split the matching quad the other direction.
  // this will produce an error in the total volume calculation across
  // multiple wedges.  Placing a center point on each quad and dividing the
  // wedge into 11 tets avoids this problem because each wedge will
  // split the quads the same way.  This eliminates error in the total
  // volume calculation across multiple wedges.

  double center_coords[3][3];
  // calculate the center of the quads
  center_coords[0][0] =
    (coordinates[0][0] + coordinates[1][0] + coordinates[3][0] + coordinates[4][0]) / 4;
  center_coords[0][1] =
    (coordinates[0][1] + coordinates[1][1] + coordinates[3][1] + coordinates[4][1]) / 4;
  center_coords[0][2] =
    (coordinates[0][2] + coordinates[1][2] + coordinates[3][2] + coordinates[4][2]) / 4;

  center_coords[1][0] =
    (coordinates[1][0] + coordinates[2][0] + coordinates[4][0] + coordinates[5][0]) / 4;
  center_coords[1][1] =
    (coordinates[1][1] + coordinates[2][1] + coordinates[4][1] + coordinates[5][1]) / 4;
  center_coords[1][2] =
    (coordinates[1][2] + coordinates[2][2] + coordinates[4][2] + coordinates[5][2]) / 4;

  center_coords[2][0] =
    (coordinates[2][0] + coordinates[0][0] + coordinates[3][0] + coordinates[5][0]) / 4;
  center_coords[2][1] =
    (coordinates[2][1] + coordinates[0][1] + coordinates[3][1] + coordinates[5][1]) / 4;
  center_coords[2][2] =
    (coordinates[2][2] + coordinates[0][2] + coordinates[3][2] + coordinates[5][2]) / 4;

  // create the tets.
  double tet_coords[11][4][3];
  tet_coords[0][0][0] = coordinates[0][0];
  tet_coords[0][0][1] = coordinates[0][1];
  tet_coords[0][0][2] = coordinates[0][2];
  tet_coords[0][1][0] = coordinates[3][0];
  tet_coords[0][1][1] = coordinates[3][1];
  tet_coords[0][1][2] = coordinates[3][2];
  tet_coords[0][2][0] = center_coords[0][0];
  tet_coords[0][2][1] = center_coords[0][1];
  tet_coords[0][2][2] = center_coords[0][2];
  tet_coords[0][3][0] = center_coords[2][0];
  tet_coords[0][3][1] = center_coords[2][1];
  tet_coords[0][3][2] = center_coords[2][2];

  tet_coords[1][0][0] = coordinates[1][0];
  tet_coords[1][0][1] = coordinates[1][1];
  tet_coords[1][0][2] = coordinates[1][2];
  tet_coords[1][1][0] = coordinates[4][0];
  tet_coords[1][1][1] = coordinates[4][1];
  tet_coords[1][1][2] = coordinates[4][2];
  tet_coords[1][2][0] = center_coords[1][0];
  tet_coords[1][2][1] = center_coords[1][1];
  tet_coords[1][2][2] = center_coords[1][2];
  tet_coords[1][3][0] = center_coords[0][0];
  tet_coords[1][3][1] = center_coords[0][1];
  tet_coords[1][3][2] = center_coords[0][2];

  tet_coords[2][0][0] = coordinates[2][0];
  tet_coords[2][0][1] = coordinates[2][1];
  tet_coords[2][0][2] = coordinates[2][2];
  tet_coords[2][1][0] = coordinates[5][0];
  tet_coords[2][1][1] = coordinates[5][1];
  tet_coords[2][1][2] = coordinates[5][2];
  tet_coords[2][2][0] = center_coords[2][0];
  tet_coords[2][2][1] = center_coords[2][1];
  tet_coords[2][2][2] = center_coords[2][2];
  tet_coords[2][3][0] = center_coords[1][0];
  tet_coords[2][3][1] = center_coords[1][1];
  tet_coords[2][3][2] = center_coords[1][2];

  tet_coords[3][0][0] = center_coords[0][0];
  tet_coords[3][0][1] = center_coords[0][1];
  tet_coords[3][0][2] = center_coords[0][2];
  tet_coords[3][1][0] = center_coords[2][0];
  tet_coords[3][1][1] = center_coords[2][1];
  tet_coords[3][1][2] = center_coords[2][2];
  tet_coords[3][2][0] = center_coords[1][0];
  tet_coords[3][2][1] = center_coords[1][1];
  tet_coords[3][2][2] = center_coords[1][2];
  tet_coords[3][3][0] = coordinates[0][0];
  tet_coords[3][3][1] = coordinates[0][1];
  tet_coords[3][3][2] = coordinates[0][2];

  tet_coords[4][0][0] = coordinates[1][0];
  tet_coords[4][0][1] = coordinates[1][1];
  tet_coords[4][0][2] = coordinates[1][2];
  tet_coords[4][1][0] = center_coords[0][0];
  tet_coords[4][1][1] = center_coords[0][1];
  tet_coords[4][1][2] = center_coords[0][2];
  tet_coords[4][2][0] = center_coords[1][0];
  tet_coords[4][2][1] = center_coords[1][1];
  tet_coords[4][2][2] = center_coords[1][2];
  tet_coords[4][3][0] = coordinates[0][0];
  tet_coords[4][3][1] = coordinates[0][1];
  tet_coords[4][3][2] = coordinates[0][2];

  tet_coords[5][0][0] = coordinates[2][0];
  tet_coords[5][0][1] = coordinates[2][1];
  tet_coords[5][0][2] = coordinates[2][2];
  tet_coords[5][1][0] = coordinates[1][0];
  tet_coords[5][1][1] = coordinates[1][1];
  tet_coords[5][1][2] = coordinates[1][2];
  tet_coords[5][2][0] = center_coords[1][0];
  tet_coords[5][2][1] = center_coords[1][1];
  tet_coords[5][2][2] = center_coords[1][2];
  tet_coords[5][3][0] = coordinates[0][0];
  tet_coords[5][3][1] = coordinates[0][1];
  tet_coords[5][3][2] = coordinates[0][2];

  tet_coords[6][0][0] = coordinates[2][0];
  tet_coords[6][0][1] = coordinates[2][1];
  tet_coords[6][0][2] = coordinates[2][2];
  tet_coords[6][1][0] = center_coords[1][0];
  tet_coords[6][1][1] = center_coords[1][1];
  tet_coords[6][1][2] = center_coords[1][2];
  tet_coords[6][2][0] = center_coords[2][0];
  tet_coords[6][2][1] = center_coords[2][1];
  tet_coords[6][2][2] = center_coords[2][2];
  tet_coords[6][3][0] = coordinates[0][0];
  tet_coords[6][3][1] = coordinates[0][1];
  tet_coords[6][3][2] = coordinates[0][2];

  tet_coords[7][0][0] = center_coords[0][0];
  tet_coords[7][0][1] = center_coords[0][1];
  tet_coords[7][0][2] = center_coords[0][2];
  tet_coords[7][1][0] = center_coords[1][0];
  tet_coords[7][1][1] = center_coords[1][1];
  tet_coords[7][1][2] = center_coords[1][2];
  tet_coords[7][2][0] = center_coords[2][0];
  tet_coords[7][2][1] = center_coords[2][1];
  tet_coords[7][2][2] = center_coords[2][2];
  tet_coords[7][3][0] = coordinates[3][0];
  tet_coords[7][3][1] = coordinates[3][1];
  tet_coords[7][3][2] = coordinates[3][2];

  tet_coords[8][0][0] = coordinates[5][0];
  tet_coords[8][0][1] = coordinates[5][1];
  tet_coords[8][0][2] = coordinates[5][2];
  tet_coords[8][1][0] = center_coords[2][0];
  tet_coords[8][1][1] = center_coords[2][1];
  tet_coords[8][1][2] = center_coords[2][2];
  tet_coords[8][2][0] = center_coords[1][0];
  tet_coords[8][2][1] = center_coords[1][1];
  tet_coords[8][2][2] = center_coords[1][2];
  tet_coords[8][3][0] = coordinates[3][0];
  tet_coords[8][3][1] = coordinates[3][1];
  tet_coords[8][3][2] = coordinates[3][2];

  tet_coords[9][0][0] = coordinates[4][0];
  tet_coords[9][0][1] = coordinates[4][1];
  tet_coords[9][0][2] = coordinates[4][2];
  tet_coords[9][1][0] = coordinates[5][0];
  tet_coords[9][1][1] = coordinates[5][1];
  tet_coords[9][1][2] = coordinates[5][2];
  tet_coords[9][2][0] = center_coords[1][0];
  tet_coords[9][2][1] = center_coords[1][1];
  tet_coords[9][2][2] = center_coords[1][2];
  tet_coords[9][3][0] = coordinates[3][0];
  tet_coords[9][3][1] = coordinates[3][1];
  tet_coords[9][3][2] = coordinates[3][2];

  tet_coords[10][0][0] = coordinates[4][0];
  tet_coords[10][0][1] = coordinates[4][1];
  tet_coords[10][0][2] = coordinates[4][2];
  tet_coords[10][1][0] = center_coords[1][0];
  tet_coords[10][1][1] = center_coords[1][1];
  tet_coords[10][1][2] = center_coords[1][2];
  tet_coords[10][2][0] = center_coords[0][0];
  tet_coords[10][2][1] = center_coords[0][1];
  tet_coords[10][2][2] = center_coords[0][2];
  tet_coords[10][3][0] = coordinates[3][0];
  tet_coords[10][3][1] = coordinates[3][1];
  tet_coords[10][3][2] = coordinates[3][2];

  double volume = 0.0;
  for (int t = 0; t < 11; t++)
  {
    volume += tet_volume(4, tet_coords[t]);
  }

  return (double)volume;
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
double wedge_edge_ratio(int /*num_nodes*/, const double coordinates[][3])
{
  VerdictVector a, b, c, d, e, f, g, h, i;

  a.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  b.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  c.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  d.set(coordinates[4][0] - coordinates[3][0], coordinates[4][1] - coordinates[3][1],
    coordinates[4][2] - coordinates[3][2]);

  e.set(coordinates[5][0] - coordinates[4][0], coordinates[5][1] - coordinates[4][1],
    coordinates[5][2] - coordinates[4][2]);

  f.set(coordinates[3][0] - coordinates[5][0], coordinates[3][1] - coordinates[5][1],
    coordinates[3][2] - coordinates[5][2]);

  g.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  h.set(coordinates[4][0] - coordinates[1][0], coordinates[4][1] - coordinates[1][1],
    coordinates[4][2] - coordinates[1][2]);

  i.set(coordinates[5][0] - coordinates[2][0], coordinates[5][1] - coordinates[2][1],
    coordinates[5][2] - coordinates[2][2]);

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

  if (max <= b2)
  {
    max = b2;
  }
  if (b2 <= min)
  {
    min = b2;
  }

  if (max <= c2)
  {
    max = c2;
  }
  if (c2 <= min)
  {
    min = c2;
  }

  if (max <= d2)
  {
    max = d2;
  }
  if (d2 <= min)
  {
    min = d2;
  }

  if (max <= e2)
  {
    max = e2;
  }
  if (e2 <= min)
  {
    min = e2;
  }

  if (max <= f2)
  {
    max = f2;
  }
  if (f2 <= min)
  {
    min = f2;
  }

  if (max <= g2)
  {
    max = g2;
  }
  if (g2 <= min)
  {
    min = g2;
  }

  if (max <= h2)
  {
    max = h2;
  }
  if (h2 <= min)
  {
    min = h2;
  }

  if (max <= i2)
  {
    max = i2;
  }
  if (i2 <= min)
  {
    min = i2;
  }

  double edge_ratio = std::sqrt(max / min);

  if (std::isnan(edge_ratio))
  {
    return VERDICT_DBL_MAX;
  }
  if (edge_ratio < 1.)
  {
    return 1.;
  }
  return (double)std::min(edge_ratio, VERDICT_DBL_MAX);
}

static void aspects(int num_nodes, const double coordinates[][3], double& aspect1, double& aspect2,
  double& aspect3, double& aspect4, double& aspect5, double& aspect6)
{
  if (num_nodes < 6)
  {
    aspect1 = 0;
    aspect2 = 0;
    aspect3 = 0;
    aspect4 = 0;
    aspect5 = 0;
    aspect6 = 0;
    return;
  }

  double mini_tris[4][3];
  int i = 0;
  // Take first tetrahedron
  for (i = 0; i < 3; i++)
  {
    mini_tris[0][i] = coordinates[0][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[1][i] = coordinates[1][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[2][i] = coordinates[2][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[3][i] = coordinates[3][i];
  }

  aspect1 = tet_aspect_frobenius(4, mini_tris);

  // Take second tet
  for (i = 0; i < 3; i++)
  {
    mini_tris[0][i] = coordinates[1][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[1][i] = coordinates[2][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[2][i] = coordinates[0][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[3][i] = coordinates[4][i];
  }

  aspect2 = tet_aspect_frobenius(4, mini_tris);

  // 3rd tet
  for (i = 0; i < 3; i++)
  {
    mini_tris[0][i] = coordinates[2][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[1][i] = coordinates[0][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[2][i] = coordinates[1][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[3][i] = coordinates[5][i];
  }

  aspect3 = tet_aspect_frobenius(4, mini_tris);

  // 4th tet
  for (i = 0; i < 3; i++)
  {
    mini_tris[0][i] = coordinates[3][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[1][i] = coordinates[5][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[2][i] = coordinates[4][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[3][i] = coordinates[0][i];
  }

  aspect4 = tet_aspect_frobenius(4, mini_tris);

  // 5th tet
  for (i = 0; i < 3; i++)
  {
    mini_tris[0][i] = coordinates[4][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[1][i] = coordinates[3][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[2][i] = coordinates[5][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[3][i] = coordinates[1][i];
  }

  aspect5 = tet_aspect_frobenius(4, mini_tris);

  // 6th tet
  for (i = 0; i < 3; i++)
  {
    mini_tris[0][i] = coordinates[5][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[1][i] = coordinates[4][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[2][i] = coordinates[3][i];
  }
  for (i = 0; i < 3; i++)
  {
    mini_tris[3][i] = coordinates[2][i];
  }

  aspect6 = tet_aspect_frobenius(4, mini_tris);
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
 Verdict Function : wedge_max_aspect_frobenius or wedge_condition
 */
double wedge_max_aspect_frobenius(int num_nodes, const double coordinates[][3])
{
  double aspect1, aspect2, aspect3, aspect4, aspect5, aspect6;
  aspects(num_nodes, coordinates, aspect1, aspect2, aspect3, aspect4, aspect5, aspect6);

  double max_aspect = std::max({ aspect1, aspect2, aspect3, aspect4, aspect5, aspect6 });

  if (max_aspect >= VERDICT_DBL_MAX)
  {
    return VERDICT_DBL_MAX;
  }
  max_aspect /= 1.16477;
  return std::max(max_aspect, 1.);
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
   Verdict Function : wedge_mean_aspect_frobenius
   */
double wedge_mean_aspect_frobenius(int num_nodes, const double coordinates[][3])
{
  double aspect1, aspect2, aspect3, aspect4, aspect5, aspect6;
  aspects(num_nodes, coordinates, aspect1, aspect2, aspect3, aspect4, aspect5, aspect6);

  double mean_aspect = (aspect1 + aspect2 + aspect3 + aspect4 + aspect5 + aspect6);
  if (mean_aspect >= VERDICT_DBL_MAX)
  {
    return VERDICT_DBL_MAX;
  }

  mean_aspect /= (6. * 1.16477);
  return std::max(mean_aspect, 1.);
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
 Verdict Function : wedge_jacobian
 */
double wedge_jacobian(int num_nodes, const double coordinates[][3])
{
  if (num_nodes == 21)
  {
    double dhdr[21];
    double dhds[21];
    double dhdt[21];
    double min_determinant = VERDICT_DBL_MAX;

    for (int i = 0; i < 15; i++)
    {
      WEDGE21_gradients_of_the_shape_functions_for_RST(
        WEDGE21_node_local_coord[i], dhdr, dhds, dhdt);
      double jacobian[3][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };

      for (int j = 0; j < 21; j++)
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
      double det =
        (VerdictVector(jacobian[0]) * VerdictVector(jacobian[1])) % VerdictVector(jacobian[2]);
      min_determinant = std::min(det, min_determinant);
    }
    return min_determinant;
  }
  else
  {
    double min_jacobian = 0, current_jacobian = 0;
    VerdictVector vec1, vec2, vec3;

    // Node 0
    vec1.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
      coordinates[1][2] - coordinates[0][2]);

    vec2.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
      coordinates[3][2] - coordinates[0][2]);

    vec3.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
      coordinates[2][2] - coordinates[0][2]);

    current_jacobian = vec2 % (vec1 * vec3);
    min_jacobian = current_jacobian;

    // node 1
    vec1.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
      coordinates[2][2] - coordinates[1][2]);

    vec2.set(coordinates[4][0] - coordinates[1][0], coordinates[4][1] - coordinates[1][1],
      coordinates[4][2] - coordinates[1][2]);

    vec3.set(coordinates[0][0] - coordinates[1][0], coordinates[0][1] - coordinates[1][1],
      coordinates[0][2] - coordinates[1][2]);

    current_jacobian = vec2 % (vec1 * vec3);
    min_jacobian = std::min(current_jacobian, min_jacobian);

    // node 2
    vec1.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
      coordinates[0][2] - coordinates[2][2]);

    vec2.set(coordinates[5][0] - coordinates[2][0], coordinates[5][1] - coordinates[2][1],
      coordinates[5][2] - coordinates[2][2]);

    vec3.set(coordinates[1][0] - coordinates[2][0], coordinates[1][1] - coordinates[2][1],
      coordinates[1][2] - coordinates[2][2]);

    current_jacobian = vec2 % (vec1 * vec3);
    min_jacobian = std::min(current_jacobian, min_jacobian);

    // node 3
    vec1.set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
      coordinates[0][2] - coordinates[3][2]);

    vec2.set(coordinates[4][0] - coordinates[3][0], coordinates[4][1] - coordinates[3][1],
      coordinates[4][2] - coordinates[3][2]);

    vec3.set(coordinates[5][0] - coordinates[3][0], coordinates[5][1] - coordinates[3][1],
      coordinates[5][2] - coordinates[3][2]);

    current_jacobian = vec2 % (vec1 * vec3);
    min_jacobian = std::min(current_jacobian, min_jacobian);

    // node 4
    vec1.set(coordinates[1][0] - coordinates[4][0], coordinates[1][1] - coordinates[4][1],
      coordinates[1][2] - coordinates[4][2]);

    vec2.set(coordinates[5][0] - coordinates[4][0], coordinates[5][1] - coordinates[4][1],
      coordinates[5][2] - coordinates[4][2]);

    vec3.set(coordinates[3][0] - coordinates[4][0], coordinates[3][1] - coordinates[4][1],
      coordinates[3][2] - coordinates[4][2]);

    current_jacobian = vec2 % (vec1 * vec3);
    min_jacobian = std::min(current_jacobian, min_jacobian);

    // node 5
    vec1.set(coordinates[3][0] - coordinates[5][0], coordinates[3][1] - coordinates[5][1],
      coordinates[3][2] - coordinates[5][2]);

    vec2.set(coordinates[4][0] - coordinates[5][0], coordinates[4][1] - coordinates[5][1],
      coordinates[4][2] - coordinates[5][2]);

    vec3.set(coordinates[2][0] - coordinates[5][0], coordinates[2][1] - coordinates[5][1],
      coordinates[2][2] - coordinates[5][2]);

    current_jacobian = vec2 % (vec1 * vec3);
    min_jacobian = std::min(current_jacobian, min_jacobian);

    if (min_jacobian > 0)
    {
      return (double)std::min(min_jacobian, VERDICT_DBL_MAX);
    }
    return (double)std::max(min_jacobian, -VERDICT_DBL_MAX);
  }
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
 Verdict Function : wedge_distortion
 */
double wedge_distortion(int num_nodes, const double coordinates[][3])
{
  double jacobian = wedge_jacobian(num_nodes, coordinates);
  double master_volume = 0.433013;
  double current_volume = wedge_volume(num_nodes, coordinates);
  double distortion = VERDICT_DBL_MAX;
  if (std::abs(current_volume) > 0.0)
    distortion = jacobian * master_volume / current_volume / 0.866025;

  if (std::isnan(distortion))
  {
    return VERDICT_DBL_MAX;
  }
  if (distortion >= VERDICT_DBL_MAX)
  {
    return VERDICT_DBL_MAX;
  }
  if (distortion <= -VERDICT_DBL_MAX)
  {
    return -VERDICT_DBL_MAX;
  }
  return distortion;
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
   Verdict Function : wedge_max_stretch
   */
double wedge_max_stretch(int /*num_nodes*/, const double coordinates[][3])
{
  // This function finds the stretch of the 3 quadrilateral faces and returns the maximum value

  double stretch = 42, quad_face[4][3], stretch1 = 42, stretch2 = 42, stretch3 = 42;
  int i = 0;

  // first face
  for (i = 0; i < 3; i++)
  {
    quad_face[0][i] = coordinates[0][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[1][i] = coordinates[1][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[2][i] = coordinates[4][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[3][i] = coordinates[3][i];
  }
  stretch1 = quad_stretch(4, quad_face);

  // second face
  for (i = 0; i < 3; i++)
  {
    quad_face[0][i] = coordinates[1][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[1][i] = coordinates[2][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[2][i] = coordinates[5][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[3][i] = coordinates[4][i];
  }
  stretch2 = quad_stretch(4, quad_face);

  // third face
  for (i = 0; i < 3; i++)
  {
    quad_face[0][i] = coordinates[2][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[1][i] = coordinates[0][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[2][i] = coordinates[3][i];
  }
  for (i = 0; i < 3; i++)
  {
    quad_face[3][i] = coordinates[5][i];
  }
  stretch3 = quad_stretch(4, quad_face);

  stretch = std::max({ stretch1, stretch2, stretch3 });

  if (stretch > 0)
  {
    return (double)std::min(stretch, VERDICT_DBL_MAX);
  }
  return (double)std::max(stretch, -VERDICT_DBL_MAX);
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
   Verdict Function : wedge_scaled_jacobian
   */
double wedge_scaled_jacobian(int /*num_nodes*/, const double coordinates[][3])
{
  double min_jacobian = 0, current_jacobian = 0, lengths = 42;
  VerdictVector vec1, vec2, vec3;

  // Node 0
  vec1.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  vec2.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  vec3.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  lengths = std::sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = (vec2 % (vec1 * vec3));
  min_jacobian = current_jacobian / lengths;

  // node 1
  vec1.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  vec2.set(coordinates[4][0] - coordinates[1][0], coordinates[4][1] - coordinates[1][1],
    coordinates[4][2] - coordinates[1][2]);

  vec3.set(coordinates[0][0] - coordinates[1][0], coordinates[0][1] - coordinates[1][1],
    coordinates[0][2] - coordinates[1][2]);

  lengths = std::sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = std::min(current_jacobian / lengths, min_jacobian);

  // node 2
  vec1.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  vec2.set(coordinates[5][0] - coordinates[2][0], coordinates[5][1] - coordinates[2][1],
    coordinates[5][2] - coordinates[2][2]);

  vec3.set(coordinates[1][0] - coordinates[2][0], coordinates[1][1] - coordinates[2][1],
    coordinates[1][2] - coordinates[2][2]);

  lengths = std::sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = std::min(current_jacobian / lengths, min_jacobian);

  // node 3
  vec1.set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);

  vec2.set(coordinates[4][0] - coordinates[3][0], coordinates[4][1] - coordinates[3][1],
    coordinates[4][2] - coordinates[3][2]);

  vec3.set(coordinates[5][0] - coordinates[3][0], coordinates[5][1] - coordinates[3][1],
    coordinates[5][2] - coordinates[3][2]);

  lengths = std::sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = std::min(current_jacobian / lengths, min_jacobian);

  // node 4
  vec1.set(coordinates[1][0] - coordinates[4][0], coordinates[1][1] - coordinates[4][1],
    coordinates[1][2] - coordinates[4][2]);

  vec2.set(coordinates[5][0] - coordinates[4][0], coordinates[5][1] - coordinates[4][1],
    coordinates[5][2] - coordinates[4][2]);

  vec3.set(coordinates[3][0] - coordinates[4][0], coordinates[3][1] - coordinates[4][1],
    coordinates[3][2] - coordinates[4][2]);

  lengths = std::sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = std::min(current_jacobian / lengths, min_jacobian);

  // node 5
  vec1.set(coordinates[3][0] - coordinates[5][0], coordinates[3][1] - coordinates[5][1],
    coordinates[3][2] - coordinates[5][2]);

  vec2.set(coordinates[4][0] - coordinates[5][0], coordinates[4][1] - coordinates[5][1],
    coordinates[4][2] - coordinates[5][2]);

  vec3.set(coordinates[2][0] - coordinates[5][0], coordinates[2][1] - coordinates[5][1],
    coordinates[2][2] - coordinates[5][2]);

  lengths = std::sqrt(vec1.length_squared() * vec2.length_squared() * vec3.length_squared());

  current_jacobian = vec2 % (vec1 * vec3);
  min_jacobian = std::min(current_jacobian / lengths, min_jacobian);

  min_jacobian *= 2 / std::sqrt(3.0);

  if (min_jacobian > 0)
  {
    return (double)std::min(min_jacobian, VERDICT_DBL_MAX);
  }
  return (double)std::max(min_jacobian, -VERDICT_DBL_MAX);
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
   Verdict Function : wedge_shape
   */
double wedge_shape(int /*num_nodes*/, const double coordinates[][3])
{
  double current_jacobian = 0, current_shape, norm_jacobi = 0;
  double min_shape = 1.0;
  VerdictVector vec1, vec2, vec3;

  // Node 0
  vec1.set(coordinates[1][0] - coordinates[0][0], coordinates[1][1] - coordinates[0][1],
    coordinates[1][2] - coordinates[0][2]);

  vec2.set(coordinates[3][0] - coordinates[0][0], coordinates[3][1] - coordinates[0][1],
    coordinates[3][2] - coordinates[0][2]);

  vec3.set(coordinates[2][0] - coordinates[0][0], coordinates[2][1] - coordinates[0][1],
    coordinates[2][2] - coordinates[0][2]);

  current_jacobian = vec2 % (vec1 * vec3);
  if (current_jacobian > VERDICT_DBL_MIN)
  {
    norm_jacobi = current_jacobian * 2.0 / std::sqrt(3.0);
    current_shape = 3 * std::pow(norm_jacobi, two_thirds) /
      (vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
    min_shape = std::min(current_shape, min_shape);
  }
  else
  {
    return 0;
  }

  // node 1
  vec1.set(coordinates[2][0] - coordinates[1][0], coordinates[2][1] - coordinates[1][1],
    coordinates[2][2] - coordinates[1][2]);

  vec2.set(coordinates[4][0] - coordinates[1][0], coordinates[4][1] - coordinates[1][1],
    coordinates[4][2] - coordinates[1][2]);

  vec3.set(coordinates[0][0] - coordinates[1][0], coordinates[0][1] - coordinates[1][1],
    coordinates[0][2] - coordinates[1][2]);

  current_jacobian = vec2 % (vec1 * vec3);
  if (current_jacobian > VERDICT_DBL_MIN)
  {
    norm_jacobi = current_jacobian * 2.0 / std::sqrt(3.0);
    current_shape = 3 * std::pow(norm_jacobi, two_thirds) /
      (vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
    min_shape = std::min(current_shape, min_shape);
  }
  else
  {
    return 0;
  }

  // node 2
  vec1.set(coordinates[0][0] - coordinates[2][0], coordinates[0][1] - coordinates[2][1],
    coordinates[0][2] - coordinates[2][2]);

  vec2.set(coordinates[5][0] - coordinates[2][0], coordinates[5][1] - coordinates[2][1],
    coordinates[5][2] - coordinates[2][2]);

  vec3.set(coordinates[1][0] - coordinates[2][0], coordinates[1][1] - coordinates[2][1],
    coordinates[1][2] - coordinates[2][2]);

  current_jacobian = vec2 % (vec1 * vec3);
  if (current_jacobian > VERDICT_DBL_MIN)
  {
    norm_jacobi = current_jacobian * 2.0 / std::sqrt(3.0);
    current_shape = 3 * std::pow(norm_jacobi, two_thirds) /
      (vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
    min_shape = std::min(current_shape, min_shape);
  }
  else
  {
    return 0;
  }

  // node 3
  vec1.set(coordinates[0][0] - coordinates[3][0], coordinates[0][1] - coordinates[3][1],
    coordinates[0][2] - coordinates[3][2]);

  vec2.set(coordinates[4][0] - coordinates[3][0], coordinates[4][1] - coordinates[3][1],
    coordinates[4][2] - coordinates[3][2]);

  vec3.set(coordinates[5][0] - coordinates[3][0], coordinates[5][1] - coordinates[3][1],
    coordinates[5][2] - coordinates[3][2]);

  current_jacobian = vec2 % (vec1 * vec3);
  if (current_jacobian > VERDICT_DBL_MIN)
  {
    norm_jacobi = current_jacobian * 2.0 / std::sqrt(3.0);
    current_shape = 3 * std::pow(norm_jacobi, two_thirds) /
      (vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
    min_shape = std::min(current_shape, min_shape);
  }
  else
  {
    return 0;
  }

  // node 4
  vec1.set(coordinates[1][0] - coordinates[4][0], coordinates[1][1] - coordinates[4][1],
    coordinates[1][2] - coordinates[4][2]);

  vec2.set(coordinates[5][0] - coordinates[4][0], coordinates[5][1] - coordinates[4][1],
    coordinates[5][2] - coordinates[4][2]);

  vec3.set(coordinates[3][0] - coordinates[4][0], coordinates[3][1] - coordinates[4][1],
    coordinates[3][2] - coordinates[4][2]);

  current_jacobian = vec2 % (vec1 * vec3);
  if (current_jacobian > VERDICT_DBL_MIN)
  {
    norm_jacobi = current_jacobian * 2.0 / std::sqrt(3.0);
    current_shape = 3 * std::pow(norm_jacobi, two_thirds) /
      (vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
    min_shape = std::min(current_shape, min_shape);
  }
  else
  {
    return 0;
  }

  // node 5
  vec1.set(coordinates[3][0] - coordinates[5][0], coordinates[3][1] - coordinates[5][1],
    coordinates[3][2] - coordinates[5][2]);

  vec2.set(coordinates[4][0] - coordinates[5][0], coordinates[4][1] - coordinates[5][1],
    coordinates[4][2] - coordinates[5][2]);

  vec3.set(coordinates[2][0] - coordinates[5][0], coordinates[2][1] - coordinates[5][1],
    coordinates[2][2] - coordinates[5][2]);

  current_jacobian = vec2 % (vec1 * vec3);
  if (current_jacobian > VERDICT_DBL_MIN)
  {
    norm_jacobi = current_jacobian * 2.0 / std::sqrt(3.0);
    current_shape = 3 * std::pow(norm_jacobi, two_thirds) /
      (vec1.length_squared() + vec2.length_squared() + vec3.length_squared());
    min_shape = std::min(current_shape, min_shape);
  }
  else
  {
    return 0;
  }

  if (min_shape < VERDICT_DBL_MIN)
  {
    return 0;
  }
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
 Verdict Function : wedge_max_aspect_frobenius or wedge_condition
 */
double wedge_condition(int /*num_nodes*/, const double coordinates[][3])
{
  return wedge_max_aspect_frobenius(6, coordinates);
}

void make_wedge_faces(const double coordinates[][3], double tri1[][3], double tri2[][3],
  double quad1[][3], double quad2[][3], double quad3[][3])
{
  // tri1
  tri1[0][0] = coordinates[0][0];
  tri1[0][1] = coordinates[0][1];
  tri1[0][2] = coordinates[0][2];

  tri1[1][0] = coordinates[1][0];
  tri1[1][1] = coordinates[1][1];
  tri1[1][2] = coordinates[1][2];

  tri1[2][0] = coordinates[2][0];
  tri1[2][1] = coordinates[2][1];
  tri1[2][2] = coordinates[2][2];

  // tri2
  tri2[0][0] = coordinates[3][0];
  tri2[0][1] = coordinates[3][1];
  tri2[0][2] = coordinates[3][2];

  tri2[1][0] = coordinates[4][0];
  tri2[1][1] = coordinates[4][1];
  tri2[1][2] = coordinates[4][2];

  tri2[2][0] = coordinates[5][0];
  tri2[2][1] = coordinates[5][1];
  tri2[2][2] = coordinates[5][2];

  // quad1
  quad1[0][0] = coordinates[0][0];
  quad1[0][1] = coordinates[0][1];
  quad1[0][2] = coordinates[0][2];

  quad1[1][0] = coordinates[1][0];
  quad1[1][1] = coordinates[1][1];
  quad1[1][2] = coordinates[1][2];

  quad1[2][0] = coordinates[4][0];
  quad1[2][1] = coordinates[4][1];
  quad1[2][2] = coordinates[4][2];

  quad1[3][0] = coordinates[3][0];
  quad1[3][1] = coordinates[3][1];
  quad1[3][2] = coordinates[3][2];

  // quad2
  quad2[0][0] = coordinates[1][0];
  quad2[0][1] = coordinates[1][1];
  quad2[0][2] = coordinates[1][2];

  quad2[1][0] = coordinates[2][0];
  quad2[1][1] = coordinates[2][1];
  quad2[1][2] = coordinates[2][2];

  quad2[2][0] = coordinates[5][0];
  quad2[2][1] = coordinates[5][1];
  quad2[2][2] = coordinates[5][2];

  quad2[3][0] = coordinates[4][0];
  quad2[3][1] = coordinates[4][1];
  quad2[3][2] = coordinates[4][2];

  // quad3
  quad3[0][0] = coordinates[2][0];
  quad3[0][1] = coordinates[2][1];
  quad3[0][2] = coordinates[2][2];

  quad3[1][0] = coordinates[0][0];
  quad3[1][1] = coordinates[0][1];
  quad3[1][2] = coordinates[0][2];

  quad3[2][0] = coordinates[3][0];
  quad3[2][1] = coordinates[3][1];
  quad3[2][2] = coordinates[3][2];

  quad3[3][0] = coordinates[5][0];
  quad3[3][1] = coordinates[5][1];
  quad3[3][2] = coordinates[5][2];
}
} // namespace verdict
