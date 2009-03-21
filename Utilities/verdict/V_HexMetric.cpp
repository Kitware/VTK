/*=========================================================================

  Module:    V_HexMetric.cpp

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 *
 * HexMetric.cpp contains quality calculations for hexes
 *
 * This file is part of VERDICT
 *
 */

#define VERDICT_EXPORTS

#include "verdict.h"
#include "VerdictVector.hpp"
#include "V_GaussIntegration.hpp"
#include "verdict_defines.hpp"
#include <memory.h>

#if defined(__BORLANDC__)
#pragma warn -8004 /* "assigned a value that is never used" */
#endif

//! the average volume of a hex
static double v_hex_size = 0;

//! weights based on the average size of a hex
static int v_hex_get_weight( VerdictVector &v1, 
    VerdictVector &v2,
    VerdictVector &v3 )
{

  if ( v_hex_size == 0)
    return 0;

  v1.set(1,0,0);
  v2.set(0,1,0);
  v3.set(0,0,1);

  double scale = pow(v_hex_size/ (v1 % (v2 * v3 )), 0.33333333333); 
  v1 *= scale;
  v2 *= scale;
  v3 *= scale;

  return 1;
}

//! returns the average volume of a hex
C_FUNC_DEF void v_set_hex_size( double size )
{
  v_hex_size = size;
}

#define make_hex_nodes(coord, pos)                      \
  for (int mhcii = 0; mhcii < 8; mhcii++ )              \
  {                                                     \
    pos[mhcii].set( coord[mhcii][0],                    \
        coord[mhcii][1],                                \
        coord[mhcii][2] );                              \
  }


#define make_edge_length_squares(edges, lengths)        \
{                                                       \
  for(int melii=0; melii<12; melii++)                   \
    lengths[melii] = edges[melii].length_squared();     \
}
  

//! make VerdictVectors from coordinates
static void v_make_hex_edges( double coordinates[][3], VerdictVector edges[12] )
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
  edges[4].set(
      coordinates[5][0] - coordinates[4][0],
      coordinates[5][1] - coordinates[4][1],
      coordinates[5][2] - coordinates[4][2]
      );
  edges[5].set(
      coordinates[6][0] - coordinates[5][0],
      coordinates[6][1] - coordinates[5][1],
      coordinates[6][2] - coordinates[5][2]
      );
  edges[6].set(
      coordinates[7][0] - coordinates[6][0],
      coordinates[7][1] - coordinates[6][1],
      coordinates[7][2] - coordinates[6][2]
      );
  edges[7].set(
      coordinates[4][0] - coordinates[7][0],
      coordinates[4][1] - coordinates[7][1],
      coordinates[4][2] - coordinates[7][2]
      );
  edges[8].set(
      coordinates[4][0] - coordinates[0][0],
      coordinates[4][1] - coordinates[0][1],
      coordinates[4][2] - coordinates[0][2]
      );
  edges[9].set(
      coordinates[5][0] - coordinates[1][0],
      coordinates[5][1] - coordinates[1][1],
      coordinates[5][2] - coordinates[1][2]
      );
  edges[10].set(
      coordinates[6][0] - coordinates[2][0],
      coordinates[6][1] - coordinates[2][1],
      coordinates[6][2] - coordinates[2][2]
      );
  edges[11].set(
      coordinates[7][0] - coordinates[3][0],
      coordinates[7][1] - coordinates[3][1],
      coordinates[7][2] - coordinates[3][2]
      );
}

#if 0 /* Not currently used and not exposed in verdict.h */
/*!
  localizes hex coordinates
*/
static void v_localize_hex_coordinates(double coordinates[][3], VerdictVector position[8] )
{

  int ii;
  for ( ii = 0; ii < 8; ii++ )
  {
    position[ii].set( coordinates[ii][0],
        coordinates[ii][1],
        coordinates[ii][2] );
  }
  
  // ... Make centroid of element the center of coordinate system
  VerdictVector point_1256 = position[1];
  point_1256 += position[2];
  point_1256 += position[5];
  point_1256 += position[6];

  VerdictVector point_0374 = position[0];
  point_0374 += position[3];
  point_0374 += position[7];
  point_0374 += position[4];

  VerdictVector centroid = point_1256;
  centroid += point_0374;
  centroid /= 8.0;

  int i;
  for ( i = 0; i < 8; i++)
    position[i] -= centroid;

  // ... Rotate element such that center of side 1-2 and 0-3 define X axis
  double DX = point_1256.x() - point_0374.x();
  double DY = point_1256.y() - point_0374.y();
  double DZ = point_1256.z() - point_0374.z();

  double AMAGX = sqrt(DX*DX + DZ*DZ);
  double AMAGY = sqrt(DX*DX + DY*DY + DZ*DZ);
  double FMAGX = AMAGX == 0.0 ? 1.0 : 0.0;
  double FMAGY = AMAGY == 0.0 ? 1.0 : 0.0;

  double CZ = DX / (AMAGX + FMAGX) + FMAGX;
  double SZ = DZ / (AMAGX + FMAGX);
  double CY = AMAGX / (AMAGY + FMAGY) + FMAGY;
  double SY = DY / (AMAGY + FMAGY);

  double temp;
 
  for (i = 0; i < 8; i++) 
  {
    temp =  CY * CZ * position[i].x() + CY * SZ * position[i].z() +
      SY * position[i].y();
    position[i].y( -SY * CZ * position[i].x() - SY * SZ * position[i].z() +
        CY * position[i].y());
    position[i].z( -SZ * position[i].x() + CZ * position[i].z());
    position[i].x(temp);
  }


  // ... Now, rotate about Y
  VerdictVector delta = -position[0];
  delta -= position[1];
  delta += position[2];
  delta += position[3];
  delta -= position[4];
  delta -= position[5];
  delta += position[6];
  delta += position[7];

  DY = delta.y();
  DZ = delta.z();

  AMAGY = sqrt(DY*DY + DZ*DZ);
  FMAGY = AMAGY == 0.0 ? 1.0 : 0.0;

  double CX = DY / (AMAGY + FMAGY) + FMAGY;
  double SX = DZ / (AMAGY + FMAGY);
  
  for (i = 0; i < 8; i++) 
    {
    temp =  CX * position[i].y() + SX * position[i].z();
    position[i].z(-SX * position[i].y() + CX * position[i].z());
    position[i].y(temp);
    }
}

static double v_safe_ratio3( const double numerator, 
    const double denominator,
    const double max_ratio )
{
    // this filter is essential for good running time in practice
  double return_value;

  const double filter_n = max_ratio * 1.0e-16;
  const double filter_d = 1.0e-16;
  if ( fabs( numerator ) <= filter_n && fabs( denominator ) >= filter_d )
    {
    return_value = numerator / denominator;
    }
  else
    {
    return_value = fabs(numerator) / max_ratio >= fabs(denominator) ?
      ( (numerator >= 0.0 && denominator >= 0.0) ||
        (numerator < 0.0 && denominator < 0.0) ?
        max_ratio : -max_ratio )
      : numerator / denominator;
    }

  return return_value;
}
#endif /* Not currently used and not exposed in verdict.h */

static double v_safe_ratio( const double numerator, 
    const double denominator )
{

  double return_value;
  const double filter_n = VERDICT_DBL_MAX; 
  const double filter_d = VERDICT_DBL_MIN;
  if ( fabs( numerator ) <= filter_n && fabs( denominator ) >= filter_d )
    {
    return_value = numerator / denominator;
    }
  else
    {
    return_value = VERDICT_DBL_MAX;
    }

  return return_value;
  
}



static double v_condition_comp(
  const VerdictVector &xxi, const VerdictVector &xet, const VerdictVector &xze )
{
  double det =  xxi % (xet * xze);
  
  if ( det <= VERDICT_DBL_MIN) { return VERDICT_DBL_MAX; }
  
  
  double term1 = xxi % xxi + xet % xet + xze % xze;
  double term2 = (( xxi*xet ) % ( xxi*xet )) + (( xet*xze ) % ( xet*xze )) + (( xze*xxi ) % ( xze*xxi ));
  
  return sqrt( term1 * term2 ) / det;
}



static double v_oddy_comp( const VerdictVector &xxi, 
    const VerdictVector &xet, 
    const VerdictVector &xze )
{
  static const double third=1.0/3.0;
  
  double g11, g12, g13, g22, g23, g33, rt_g;
  
  g11 = xxi % xxi;
  g12 = xxi % xet;
  g13 = xxi % xze;
  g22 = xet % xet;
  g23 = xet % xze;
  g33 = xze % xze;
  rt_g = xxi % (xet * xze);
  
  double oddy_metric;
  if ( rt_g > VERDICT_DBL_MIN ) 
  {
    double norm_G_squared = g11*g11 + 2.0*g12*g12 + 2.0*g13*g13 + g22*g22 + 2.0*g23*g23 +g33*g33;
    
    double norm_J_squared = g11 + g22 + g33;
    
    oddy_metric = ( norm_G_squared - third*norm_J_squared*norm_J_squared ) / pow( rt_g, 4.*third );
    
  }
  
  else
    oddy_metric = VERDICT_DBL_MAX;
  
  return oddy_metric;
  
}


//! calcualates edge lengths of a hex
static double v_hex_edge_length(int max_min, double coordinates[][3])
{
  double temp[3], edge[12];
  int i;
  
  //lengths^2 of edges
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[1][i] - coordinates[0][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[0] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[2][i] - coordinates[1][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[1] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[3][i] - coordinates[2][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[2] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[0][i] - coordinates[3][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[3] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[5][i] - coordinates[4][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[4] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[6][i] - coordinates[5][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[5] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[7][i] - coordinates[6][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[6] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[4][i] - coordinates[7][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[7] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[4][i] - coordinates[0][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[8] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[5][i] - coordinates[1][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[9] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[6][i] - coordinates[2][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[10] = sqrt( temp[0] + temp[1] + temp[2] );
  
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[7][i] - coordinates[3][i];
    temp[i] = temp[i] * temp[i];
  }
  edge[11] = sqrt( temp[0] + temp[1] + temp[2] );
  
  double _edge = edge[0];
  
  if ( max_min == 0)
  {
    for( i = 1; i<12; i++) 
      _edge = VERDICT_MIN( _edge, edge[i] ); 
    return _edge;
  }  
  else
  {
    for( i = 1; i<12; i++) 
      _edge = VERDICT_MAX( _edge, edge[i] );
    return _edge;
  }
  
}


static double v_diag_length(int max_min, double coordinates[][3]) 
{
  double temp[3], diag[4];
  int i;
  
  //lengths^2  f diag nals
  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[6][i] - coordinates[0][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[0] = sqrt( temp[0] + temp[1] + temp[2] );

  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[4][i] - coordinates[2][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[1] = sqrt( temp[0] + temp[1] + temp[2] );

  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[7][i] - coordinates[1][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[2] = sqrt( temp[0] + temp[1] + temp[2] );

  for (i = 0; i < 3; i++ )
  {
    temp[i] = coordinates[5][i] - coordinates[3][i];
    temp[i] = temp[i] * temp[i];
  }
  diag[3] = sqrt( temp[0] + temp[1] + temp[2] );

  double diagonal = diag[0];
  if ( max_min == 0 )  //Return min diagonal
  { 
    for( i = 1; i<4; i++)
      diagonal = VERDICT_MIN( diagonal, diag[i] );
    return diagonal;
  }
  else          //Return max diagonal
  {
    for( i = 1; i<4; i++)
      diagonal = VERDICT_MAX( diagonal, diag[i] );
    return diagonal;  
  }

}

//! calculates efg values
static VerdictVector v_calc_hex_efg( int efg_index, VerdictVector coordinates[8])
{
  
  VerdictVector efg;
  
  switch(efg_index) {
    
    case 1:
      efg =  coordinates[1];
      efg += coordinates[2];
      efg += coordinates[5];
      efg += coordinates[6];
      efg -= coordinates[0];
      efg -= coordinates[3];
      efg -= coordinates[4];
      efg -= coordinates[7];
      break;
      
    case 2:
      efg =  coordinates[2];
      efg += coordinates[3];
      efg += coordinates[6];
      efg += coordinates[7];
      efg -= coordinates[0];
      efg -= coordinates[1];
      efg -= coordinates[4];
      efg -= coordinates[5];
      break;
      
    case 3:
      efg =  coordinates[4];
      efg += coordinates[5];
      efg += coordinates[6];
      efg += coordinates[7];
      efg -= coordinates[0];
      efg -= coordinates[1];
      efg -= coordinates[2];
      efg -= coordinates[3];
      break;

    case 12:
      efg =  coordinates[0];
      efg += coordinates[2];
      efg += coordinates[4];
      efg += coordinates[6];
      efg -= coordinates[1];
      efg -= coordinates[3];
      efg -= coordinates[5];
      efg -= coordinates[7];
      break;
      
    case 13:
      efg =  coordinates[0];
      efg += coordinates[3];
      efg += coordinates[5];
      efg += coordinates[6];
      efg -= coordinates[1];
      efg -= coordinates[2];
      efg -= coordinates[4];
      efg -= coordinates[7];
      break;

   case 23:
      efg =  coordinates[0];
      efg += coordinates[1];
      efg += coordinates[6];
      efg += coordinates[7];
      efg -= coordinates[2];
      efg -= coordinates[3];
      efg -= coordinates[4];
      efg -= coordinates[5];
      break;

   case 123:
      efg =  coordinates[0];
      efg += coordinates[2];
      efg += coordinates[5];
      efg += coordinates[7];
      efg -= coordinates[1];
      efg -= coordinates[5];
      efg -= coordinates[6];
      efg -= coordinates[2];
      break;
      
   default:
      efg.set(0,0,0);
      
  }
  
  return efg;
}

/*!
   the edge ratio of a hex

   NB (P. Pebay 01/23/07): 
     Hmax / Hmin where Hmax and Hmin are respectively the maximum and the
     minimum edge lengths
*/
C_FUNC_DEF double v_hex_edge_ratio (int /*num_nodes*/, double coordinates[][3])
{

  VerdictVector edges[12];
  v_make_hex_edges(coordinates, edges);
  
  double a2 = edges[0].length_squared();
  double b2 = edges[1].length_squared();
  double c2 = edges[2].length_squared();
  double d2 = edges[3].length_squared();
  double e2 = edges[4].length_squared();
  double f2 = edges[5].length_squared();
  double g2 = edges[6].length_squared();
  double h2 = edges[7].length_squared();
  double i2 = edges[8].length_squared();
  double j2 = edges[9].length_squared();
  double k2 = edges[10].length_squared();
  double l2 = edges[11].length_squared();

  double mab,mcd,mef,Mab,Mcd,Mef;
  double mgh,mij,mkl,Mgh,Mij,Mkl;

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
  if ( e2 < f2 )
    {
      mef = e2;
      Mef = f2;
    }
  else // f2 <= e2
    {
      mef = f2;
      Mef = e2;
    }
  if ( g2 < h2 )
    {
      mgh = g2;
      Mgh = h2;
    }
  else // h2 <= g2
    {
      mgh = h2;
      Mgh = g2;
    }
  if ( i2 < j2 )
    {
      mij = i2;
      Mij = j2;
    }
  else // j2 <= i2
    {
      mij = j2;
      Mij = i2;
    }
  if ( k2 < l2 )
    {
      mkl = k2;
      Mkl = l2;
    }
  else // l2 <= k2
    {
      mkl = l2;
      Mkl = k2;
    }

  double m2;
  m2 = mab < mcd ? mab : mcd;
  m2 = m2  < mef ? m2  : mef;
  m2 = m2  < mgh ? m2  : mgh;
  m2 = m2  < mij ? m2  : mij;
  m2 = m2  < mkl ? m2  : mkl;

  if ( m2 < VERDICT_DBL_MIN ) 
    return (double)VERDICT_DBL_MAX;

  double M2;
  M2 = Mab > Mcd ? Mab : Mcd;
  M2 = M2  > Mef ? M2  : Mef;
  M2 = M2  > Mgh ? M2  : Mgh;
  M2 = M2  > Mij ? M2  : Mij;
  M2 = M2  > Mkl ? M2  : Mkl;
  m2 = m2  < mef ? m2  : mef;

  M2 = Mab > Mcd ? Mab : Mcd;
  M2 = M2  > Mef ? M2  : Mef;

  double edge_ratio = sqrt( M2 / m2 );
  
  if ( edge_ratio > 0 )
    return (double) VERDICT_MIN( edge_ratio, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( edge_ratio, -VERDICT_DBL_MAX );
}

/*!
  max edge ratio of a hex

  Maximum edge length ratio at hex center
*/
C_FUNC_DEF double v_hex_max_edge_ratio (int /*num_nodes*/, double coordinates[][3])
{
  double aspect;
  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );
  
  double aspect_12, aspect_13, aspect_23;
  
  VerdictVector efg1 = v_calc_hex_efg( 1, node_pos);
  VerdictVector efg2 = v_calc_hex_efg( 2, node_pos);
  VerdictVector efg3 = v_calc_hex_efg( 3, node_pos);

  double mag_efg1 = efg1.length();
  double mag_efg2 = efg2.length();
  double mag_efg3 = efg3.length();
  
  aspect_12 = v_safe_ratio( VERDICT_MAX( mag_efg1, mag_efg2 ) , VERDICT_MIN( mag_efg1, mag_efg2 ) );
  aspect_13 = v_safe_ratio( VERDICT_MAX( mag_efg1, mag_efg3 ) , VERDICT_MIN( mag_efg1, mag_efg3 ) );
  aspect_23 = v_safe_ratio( VERDICT_MAX( mag_efg2, mag_efg3 ) , VERDICT_MIN( mag_efg2, mag_efg3 ) );

  aspect = VERDICT_MAX( aspect_12, VERDICT_MAX( aspect_13, aspect_23 ) );

  if ( aspect > 0 )
    return (double) VERDICT_MIN( aspect, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( aspect, -VERDICT_DBL_MAX );
 
}

/*!
  skew of a hex

  Maximum ||cosA|| where A is the angle between edges at hex center.
*/
C_FUNC_DEF double v_hex_skew( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );
  
  double skew_1, skew_2, skew_3;
  
  VerdictVector efg1 = v_calc_hex_efg( 1, node_pos);
  VerdictVector efg2 = v_calc_hex_efg( 2, node_pos);
  VerdictVector efg3 = v_calc_hex_efg( 3, node_pos);
 
  if ( efg1.normalize() <= VERDICT_DBL_MIN )
    return VERDICT_DBL_MAX;
  if ( efg2.normalize() <= VERDICT_DBL_MIN )
    return VERDICT_DBL_MAX;
  if ( efg3.normalize() <= VERDICT_DBL_MIN )
    return VERDICT_DBL_MAX;

  skew_1 = fabs(efg1 % efg2);
  skew_2 = fabs(efg1 % efg3);
  skew_3 = fabs(efg2 % efg3);

  double skew = (VERDICT_MAX( skew_1, VERDICT_MAX( skew_2, skew_3 ) ));

  if ( skew > 0 )
    return (double) VERDICT_MIN( skew, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( skew, -VERDICT_DBL_MAX );
}

/*!
  taper of a hex

  Maximum ratio of lengths derived from opposite edges.
*/
C_FUNC_DEF double v_hex_taper( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );
  
  VerdictVector efg1 = v_calc_hex_efg( 1, node_pos);
  VerdictVector efg2 = v_calc_hex_efg( 2, node_pos);
  VerdictVector efg3 = v_calc_hex_efg( 3, node_pos);

  VerdictVector efg12 = v_calc_hex_efg( 12, node_pos);
  VerdictVector efg13 = v_calc_hex_efg( 13, node_pos);
  VerdictVector efg23 = v_calc_hex_efg( 23, node_pos);

  double taper_1 = fabs( v_safe_ratio( efg12.length(), VERDICT_MIN( efg1.length(), efg2.length())));
  double taper_2 = fabs( v_safe_ratio( efg13.length(), VERDICT_MIN( efg1.length(), efg3.length())));
  double taper_3 = fabs( v_safe_ratio( efg23.length(), VERDICT_MIN( efg2.length(), efg3.length())));

  double taper = (double)VERDICT_MAX(taper_1, VERDICT_MAX(taper_2, taper_3));  

  if ( taper > 0 )
    return (double) VERDICT_MIN( taper, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( taper, -VERDICT_DBL_MAX );
}

/*!
  volume of a hex

  Jacobian at hex center
*/
C_FUNC_DEF double v_hex_volume( int /*num_nodes*/, double coordinates[][3] )
{
  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );
  
  VerdictVector efg1 = v_calc_hex_efg( 1, node_pos);
  VerdictVector efg2 = v_calc_hex_efg( 2, node_pos);
  VerdictVector efg3 = v_calc_hex_efg( 3, node_pos);

  double volume;
  volume = (double) (efg1 % (efg2 * efg3))/64.0;

  if ( volume > 0 )
    return (double) VERDICT_MIN( volume, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( volume, -VERDICT_DBL_MAX );
}

/*!
  stretch of a hex

  sqrt(3) * minimum edge length / maximum diagonal length
*/
C_FUNC_DEF double v_hex_stretch( int /*num_nodes*/, double coordinates[][3] )
{
  static const double HEX_STRETCH_SCALE_FACTOR = sqrt(3.0);
  
  double min_edge = v_hex_edge_length( 0, coordinates );
  double max_diag = v_diag_length( 1, coordinates );  
  
  double stretch = HEX_STRETCH_SCALE_FACTOR * v_safe_ratio(min_edge, max_diag);

  if ( stretch > 0 )
    return (double) VERDICT_MIN( stretch, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( stretch, -VERDICT_DBL_MAX );
}

/*!
  diagonal ratio of a hex
  
  Minimum diagonal length / maximum diagonal length
*/
C_FUNC_DEF double v_hex_diagonal( int /*num_nodes*/, double coordinates[][3] )
{
  
  double min_diag = v_diag_length( 0, coordinates ); 
  double max_diag = v_diag_length( 1, coordinates );
  
  double diagonal = v_safe_ratio( min_diag, max_diag);

  if ( diagonal > 0 )
    return (double) VERDICT_MIN( diagonal, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( diagonal, -VERDICT_DBL_MAX );
}

#define SQR(x) ((x) * (x))

/*!
  dimension of a hex

  Pronto-specific characteristic length for stable time step calculation. 
  Char_length = Volume / 2 grad Volume
*/
C_FUNC_DEF double v_hex_dimension( int /*num_nodes*/, double coordinates[][3] )
{
  
  double gradop[9][4];

  double x1 = coordinates[0][0];
  double x2 = coordinates[1][0];
  double x3 = coordinates[2][0];
  double x4 = coordinates[3][0];
  double x5 = coordinates[4][0];
  double x6 = coordinates[5][0];
  double x7 = coordinates[6][0];
  double x8 = coordinates[7][0];

  double y1 = coordinates[0][1];
  double y2 = coordinates[1][1];
  double y3 = coordinates[2][1];
  double y4 = coordinates[3][1];
  double y5 = coordinates[4][1];
  double y6 = coordinates[5][1];
  double y7 = coordinates[6][1];
  double y8 = coordinates[7][1];

  double z1 = coordinates[0][2];
  double z2 = coordinates[1][2];
  double z3 = coordinates[2][2];
  double z4 = coordinates[3][2];
  double z5 = coordinates[4][2];
  double z6 = coordinates[5][2];
  double z7 = coordinates[6][2];
  double z8 = coordinates[7][2];

  double z24 = z2 - z4;
  double z52 = z5 - z2;
  double z45 = z4 - z5;
  gradop[1][1] = ( y2*(z6-z3-z45) + y3*z24 + y4*(z3-z8-z52)
      + y5*(z8-z6-z24) + y6*z52 + y8*z45 ) / 12.0;
         
  double z31 = z3 - z1;
  double z63 = z6 - z3;
  double z16 = z1 - z6;
  gradop[2][1] = ( y3*(z7-z4-z16) + y4*z31 + y1*(z4-z5-z63)
      + y6*(z5-z7-z31) + y7*z63 + y5*z16 ) / 12.0;

  double z42 = z4 - z2;
  double z74 = z7 - z4;
  double z27 = z2 - z7;
  gradop[3][1] = ( y4*(z8-z1-z27) + y1*z42 + y2*(z1-z6-z74)
      + y7*(z6-z8-z42) + y8*z74 + y6*z27 ) / 12.0;
             
  double z13 = z1 - z3;
  double z81 = z8 - z1;
  double z38 = z3 - z8;
  gradop[4][1] = ( y1*(z5-z2-z38) + y2*z13 + y3*(z2-z7-z81)
      + y8*(z7-z5-z13) + y5*z81 + y7*z38 ) / 12.0;
   
  double z86 = z8 - z6;
  double z18 = z1 - z8;
  double z61 = z6 - z1;
  gradop[5][1] = ( y8*(z4-z7-z61) + y7*z86 + y6*(z7-z2-z18)
      + y1*(z2-z4-z86) + y4*z18 + y2*z61 ) / 12.0;
     
  double z57 = z5 - z7;
  double z25 = z2 - z5;
  double z72 = z7 - z2;
  gradop[6][1] = ( y5*(z1-z8-z72) + y8*z57 + y7*(z8-z3-z25)
      + y2*(z3-z1-z57) + y1*z25 + y3*z72 ) / 12.0;
       
  double z68 = z6 - z8;
  double z36 = z3 - z6;
  double z83 = z8 - z3;
  gradop[7][1] = ( y6*(z2-z5-z83) + y5*z68 + y8*(z5-z4-z36)
      + y3*(z4-z2-z68) + y2*z36 + y4*z83 ) / 12.0;
         
  double z75 = z7 - z5;
  double z47 = z4 - z7;
  double z54 = z5 - z4;
  gradop[8][1] = ( y7*(z3-z6-z54) + y6*z75 + y5*(z6-z1-z47)
      + y4*(z1-z3-z75) + y3*z47 + y1*z54 ) / 12.0;
           
  double x24 = x2 - x4;
  double x52 = x5 - x2;
  double x45 = x4 - x5;
  gradop[1][2] = ( z2*(x6-x3-x45) + z3*x24 + z4*(x3-x8-x52)
      + z5*(x8-x6-x24) + z6*x52 + z8*x45 ) / 12.0;

  double x31 = x3 - x1;
  double x63 = x6 - x3;
  double x16 = x1 - x6;
  gradop[2][2] = ( z3*(x7-x4-x16) + z4*x31 + z1*(x4-x5-x63)
      + z6*(x5-x7-x31) + z7*x63 + z5*x16 ) / 12.0;

  double x42 = x4 - x2;
  double x74 = x7 - x4;
  double x27 = x2 - x7;
  gradop[3][2] = ( z4*(x8-x1-x27) + z1*x42 + z2*(x1-x6-x74)
      + z7*(x6-x8-x42) + z8*x74 + z6*x27 ) / 12.0;

  double x13 = x1 - x3;
  double x81 = x8 - x1;
  double x38 = x3 - x8;
  gradop[4][2] = ( z1*(x5-x2-x38) + z2*x13 + z3*(x2-x7-x81)
      + z8*(x7-x5-x13) + z5*x81 + z7*x38 ) / 12.0;

  double x86 = x8 - x6;
  double x18 = x1 - x8;
  double x61 = x6 - x1;
  gradop[5][2] = ( z8*(x4-x7-x61) + z7*x86 + z6*(x7-x2-x18)
      + z1*(x2-x4-x86) + z4*x18 + z2*x61 ) / 12.0;
                     
  double x57 = x5 - x7;
  double x25 = x2 - x5;
  double x72 = x7 - x2;
  gradop[6][2] = ( z5*(x1-x8-x72) + z8*x57 + z7*(x8-x3-x25)
      + z2*(x3-x1-x57) + z1*x25 + z3*x72 ) / 12.0;

  double x68 = x6 - x8;
  double x36 = x3 - x6;
  double x83 = x8 - x3;
  gradop[7][2] = ( z6*(x2-x5-x83) + z5*x68 + z8*(x5-x4-x36)
      + z3*(x4-x2-x68) + z2*x36 + z4*x83 ) / 12.0;

  double x75 = x7 - x5;
  double x47 = x4 - x7;
  double x54 = x5 - x4;
  gradop[8][2] = ( z7*(x3-x6-x54) + z6*x75 + z5*(x6-x1-x47)
      + z4*(x1-x3-x75) + z3*x47 + z1*x54 ) / 12.0;
           
  double y24 = y2 - y4;
  double y52 = y5 - y2;
  double y45 = y4 - y5;
  gradop[1][3] = ( x2*(y6-y3-y45) + x3*y24 + x4*(y3-y8-y52)
      + x5*(y8-y6-y24) + x6*y52 + x8*y45 ) / 12.0;
             
  double y31 = y3 - y1;
  double y63 = y6 - y3;
  double y16 = y1 - y6;
  gradop[2][3] = ( x3*(y7-y4-y16) + x4*y31 + x1*(y4-y5-y63)
      + x6*(y5-y7-y31) + x7*y63 + x5*y16 ) / 12.0;
               
  double y42 = y4 - y2;
  double y74 = y7 - y4;
  double y27 = y2 - y7;
  gradop[3][3] = ( x4*(y8-y1-y27) + x1*y42 + x2*(y1-y6-y74)
      + x7*(y6-y8-y42) + x8*y74 + x6*y27 ) / 12.0;
                 
  double y13 = y1 - y3;
  double y81 = y8 - y1;
  double y38 = y3 - y8;
  gradop[4][3] = ( x1*(y5-y2-y38) + x2*y13 + x3*(y2-y7-y81)
      + x8*(y7-y5-y13) + x5*y81 + x7*y38 ) / 12.0;
                   
  double y86 = y8 - y6;
  double y18 = y1 - y8;
  double y61 = y6 - y1;
  gradop[5][3] = ( x8*(y4-y7-y61) + x7*y86 + x6*(y7-y2-y18)
      + x1*(y2-y4-y86) + x4*y18 + x2*y61 ) / 12.0;
                     
  double y57 = y5 - y7;
  double y25 = y2 - y5;
  double y72 = y7 - y2;
  gradop[6][3] = ( x5*(y1-y8-y72) + x8*y57 + x7*(y8-y3-y25)
      + x2*(y3-y1-y57) + x1*y25 + x3*y72 ) / 12.0;
                       
  double y68 = y6 - y8;
  double y36 = y3 - y6;
  double y83 = y8 - y3;
  gradop[7][3] = ( x6*(y2-y5-y83) + x5*y68 + x8*(y5-y4-y36)
      + x3*(y4-y2-y68) + x2*y36 + x4*y83 ) / 12.0;
                         
  double y75 = y7 - y5;
  double y47 = y4 - y7;
  double y54 = y5 - y4;
  gradop[8][3] = ( x7*(y3-y6-y54) + x6*y75 + x5*(y6-y1-y47)
      + x4*(y1-y3-y75) + x3*y47 + x1*y54 ) / 12.0;

  //     calculate element volume and characteristic element aspect ratio
  //     (used in time step and hourglass control) - 



  double volume =  coordinates[0][0] * gradop[1][1]
    + coordinates[1][0] * gradop[2][1]
    + coordinates[2][0] * gradop[3][1]
    + coordinates[3][0] * gradop[4][1]
    + coordinates[4][0] * gradop[5][1]
    + coordinates[5][0] * gradop[6][1]
    + coordinates[6][0] * gradop[7][1]
    + coordinates[7][0] * gradop[8][1];
  double aspect = .5*SQR(volume) /
    ( SQR(gradop[1][1]) + SQR(gradop[2][1])
      + SQR(gradop[3][1]) + SQR(gradop[4][1])
      + SQR(gradop[5][1]) + SQR(gradop[6][1])
      + SQR(gradop[7][1]) + SQR(gradop[8][1])
      + SQR(gradop[1][2]) + SQR(gradop[2][2])
      + SQR(gradop[3][2]) + SQR(gradop[4][2])
      + SQR(gradop[5][2]) + SQR(gradop[6][2])
      + SQR(gradop[7][2]) + SQR(gradop[8][2])
      + SQR(gradop[1][3]) + SQR(gradop[2][3])
      + SQR(gradop[3][3]) + SQR(gradop[4][3])
      + SQR(gradop[5][3]) + SQR(gradop[6][3])
      + SQR(gradop[7][3]) + SQR(gradop[8][3]) );
     
  return (double)sqrt(aspect);
  
}

/*!
  oddy of a hex

  General distortion measure based on left Cauchy-Green Tensor
*/
C_FUNC_DEF double v_hex_oddy( int /*num_nodes*/, double coordinates[][3] )
{
  
  double oddy = 0.0, current_oddy;
  VerdictVector xxi, xet, xze;
  
  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );
  
  xxi = v_calc_hex_efg(1, node_pos);
  xet = v_calc_hex_efg(2, node_pos);
  xze = v_calc_hex_efg(3, node_pos);
  
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  xxi.set(coordinates[1][0] - coordinates[0][0],
          coordinates[1][1] - coordinates[0][1],
          coordinates[1][2] - coordinates[0][2] );
  
  xet.set(coordinates[3][0] - coordinates[0][0],
          coordinates[3][1] - coordinates[0][1],
          coordinates[3][2] - coordinates[0][2] );
  
  xze.set(coordinates[4][0] - coordinates[0][0],
          coordinates[4][1] - coordinates[0][1],
          coordinates[4][2] - coordinates[0][2] );
  
  
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  xxi.set(coordinates[2][0] - coordinates[1][0],
          coordinates[2][1] - coordinates[1][1],
          coordinates[2][2] - coordinates[1][2] );
  
  xet.set(coordinates[0][0] - coordinates[1][0],
          coordinates[0][1] - coordinates[1][1],
          coordinates[0][2] - coordinates[1][2] );
  
  xze.set(coordinates[5][0] - coordinates[1][0],
          coordinates[5][1] - coordinates[1][1],
          coordinates[5][2] - coordinates[1][2] );
  
  
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  xxi.set(coordinates[3][0] - coordinates[2][0],
          coordinates[3][1] - coordinates[2][1],
          coordinates[3][2] - coordinates[2][2] );
  
  xet.set(coordinates[1][0] - coordinates[2][0],
          coordinates[1][1] - coordinates[2][1],
          coordinates[1][2] - coordinates[2][2] );
  
  xze.set(coordinates[6][0] - coordinates[2][0],
          coordinates[6][1] - coordinates[2][1],
          coordinates[6][2] - coordinates[2][2] );
  
  
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  xxi.set(coordinates[0][0] - coordinates[3][0],
          coordinates[0][1] - coordinates[3][1],
          coordinates[0][2] - coordinates[3][2] );
  
  xet.set(coordinates[2][0] - coordinates[3][0],
          coordinates[2][1] - coordinates[3][1],
          coordinates[2][2] - coordinates[3][2] );
  
  xze.set(coordinates[7][0] - coordinates[3][0],
          coordinates[7][1] - coordinates[3][1],
          coordinates[7][2] - coordinates[3][2] );
 
 
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  xxi.set(coordinates[7][0] - coordinates[4][0],
          coordinates[7][1] - coordinates[4][1],
          coordinates[7][2] - coordinates[4][2] );
  
  xet.set(coordinates[5][0] - coordinates[4][0],
          coordinates[5][1] - coordinates[4][1],
          coordinates[5][2] - coordinates[4][2] );
  
  xze.set(coordinates[0][0] - coordinates[4][0],
          coordinates[0][1] - coordinates[4][1],
          coordinates[0][2] - coordinates[4][2] );
  
  
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  xxi.set(coordinates[4][0] - coordinates[5][0],
          coordinates[4][1] - coordinates[5][1],
          coordinates[4][2] - coordinates[5][2] );
  
  xet.set(coordinates[6][0] - coordinates[5][0],
          coordinates[6][1] - coordinates[5][1],
          coordinates[6][2] - coordinates[5][2] );
  
  xze.set(coordinates[1][0] - coordinates[5][0],
          coordinates[1][1] - coordinates[5][1],
          coordinates[1][2] - coordinates[5][2] );
  
  
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  xxi.set(coordinates[5][0] - coordinates[6][0],
          coordinates[5][1] - coordinates[6][1],
          coordinates[5][2] - coordinates[6][2] );
  
  xet.set(coordinates[7][0] - coordinates[6][0],
          coordinates[7][1] - coordinates[6][1],
          coordinates[7][2] - coordinates[6][2] );
  
  xze.set(coordinates[2][0] - coordinates[6][0],
          coordinates[2][1] - coordinates[6][1],
          coordinates[2][2] - coordinates[6][2] );
  
  
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  xxi.set(coordinates[6][0] - coordinates[7][0],
          coordinates[6][1] - coordinates[7][1],
          coordinates[6][2] - coordinates[7][2] );
  
  xet.set(coordinates[4][0] - coordinates[7][0],
          coordinates[4][1] - coordinates[7][1],
          coordinates[4][2] - coordinates[7][2] );
  
  xze.set(coordinates[3][0] - coordinates[7][0],
          coordinates[3][1] - coordinates[7][1],
          coordinates[3][2] - coordinates[7][2] );
  
  
  current_oddy = v_oddy_comp( xxi, xet, xze);
  if ( current_oddy > oddy ) { oddy = current_oddy; }
  
  if ( oddy > 0 )
    return (double) VERDICT_MIN( oddy, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( oddy, -VERDICT_DBL_MAX );

}

/*!
   the average Frobenius aspect of a hex

   NB (P. Pebay 01/20/07): 
     this function is calculated by averaging the 8 Frobenius aspects at
     each corner of the hex, when the reference corner is right isosceles.
*/
C_FUNC_DEF double v_hex_med_aspect_frobenius( int /*num_nodes*/, double coordinates[][3] )
{

  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );

  VerdictVector xxi, xet, xze;

  // J(0,0,0):

  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  double med_aspect_frobenius = v_condition_comp( xxi, xet, xze );

  // J(1,0,0):
  
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  med_aspect_frobenius += v_condition_comp( xxi, xet, xze );

  // J(1,1,0):
  
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  med_aspect_frobenius += v_condition_comp( xxi, xet, xze );

  // J(0,1,0):
  
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  med_aspect_frobenius += v_condition_comp( xxi, xet, xze );

  // J(0,0,1):

  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  med_aspect_frobenius += v_condition_comp( xxi, xet, xze );

  // J(1,0,1):

  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  med_aspect_frobenius += v_condition_comp( xxi, xet, xze );

  // J(1,1,1):

  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  med_aspect_frobenius += v_condition_comp( xxi, xet, xze );

  // J(1,1,1):

  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  med_aspect_frobenius += v_condition_comp( xxi, xet, xze );
  med_aspect_frobenius /= 24.;

  if ( med_aspect_frobenius > 0 )
    return (double) VERDICT_MIN( med_aspect_frobenius, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( med_aspect_frobenius, -VERDICT_DBL_MAX );

}

/*!
  maximum Frobenius condition number of a hex

  Maximum Frobenius condition number of the Jacobian matrix at 8 corners
   NB (P. Pebay 01/25/07): 
     this function is calculated by taking the maximum of the 8 Frobenius aspects at
     each corner of the hex, when the reference corner is right isosceles.
*/
C_FUNC_DEF double v_hex_max_aspect_frobenius( int /*num_nodes*/, double coordinates[][3] )
{

  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );

  VerdictVector xxi, xet, xze;

  // J(0,0,0):

  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  double condition = v_condition_comp( xxi, xet, xze );

  // J(1,0,0):
  
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  double current_condition = v_condition_comp( xxi, xet, xze );
  if ( current_condition > condition ) { condition = current_condition; }

  // J(1,1,0):
  
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  current_condition = v_condition_comp( xxi, xet, xze );
  if ( current_condition > condition ) { condition = current_condition; }

  // J(0,1,0):
  
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  current_condition = v_condition_comp( xxi, xet, xze );
  if ( current_condition > condition ) { condition = current_condition; }

  // J(0,0,1):

  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  current_condition = v_condition_comp( xxi, xet, xze );
  if ( current_condition > condition ) { condition = current_condition; }

  // J(1,0,1):

  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  current_condition = v_condition_comp( xxi, xet, xze );
  if ( current_condition > condition ) { condition = current_condition; }

  // J(1,1,1):

  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  current_condition = v_condition_comp( xxi, xet, xze );
  if ( current_condition > condition ) { condition = current_condition; }

  // J(1,1,1):

  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  current_condition = v_condition_comp( xxi, xet, xze );
  if ( current_condition > condition ) { condition = current_condition; }

  condition /= 3.;

  if ( condition > 0 )
    return (double) VERDICT_MIN( condition, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( condition, -VERDICT_DBL_MAX );

}

/*!
  The maximum Frobenius condition of a hex, a.k.a. condition
  NB (P. Pebay 01/25/07): 
     this method is maintained for backwards compatibility only.
     It will become deprecated at some point.

*/
C_FUNC_DEF double v_hex_condition( int /*num_nodes*/, double coordinates[][3] )
{

  return v_hex_max_aspect_frobenius(8, coordinates);
}

/*!
  jacobian of a hex

  Minimum pointwise volume of local map at 8 corners & center of hex
*/
C_FUNC_DEF double v_hex_jacobian( int /*num_nodes*/, double coordinates[][3] )
{
  
  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );

  double jacobian = VERDICT_DBL_MAX;
  double current_jacobian; 
  VerdictVector xxi, xet, xze;

  xxi = v_calc_hex_efg(1, node_pos );
  xet = v_calc_hex_efg(2, node_pos );
  xze = v_calc_hex_efg(3, node_pos );


  current_jacobian = xxi % (xet * xze) / 64.0;
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  // J(0,0,0):

  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  current_jacobian = xxi % (xet * xze);
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  // J(1,0,0):
  
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  current_jacobian = xxi % (xet * xze);
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  // J(1,1,0):
  
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  current_jacobian = xxi % (xet * xze);
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  // J(0,1,0):
  
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  current_jacobian = xxi % (xet * xze);
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  // J(0,0,1):

  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  current_jacobian = xxi % (xet * xze);
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  // J(1,0,1):

  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  current_jacobian = xxi % (xet * xze);
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  // J(1,1,1):

  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  current_jacobian = xxi % (xet * xze);
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  // J(0,1,1):

  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  current_jacobian = xxi % (xet * xze);
  if ( current_jacobian < jacobian ) { jacobian = current_jacobian; }

  if ( jacobian > 0 )
    return (double) VERDICT_MIN( jacobian, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( jacobian, -VERDICT_DBL_MAX );
}

/*!
  scaled jacobian of a hex

  Minimum Jacobian divided by the lengths of the 3 edge vectors
*/
C_FUNC_DEF double v_hex_scaled_jacobian( int /*num_nodes*/, double coordinates[][3] )
{

  double jacobi, min_norm_jac = VERDICT_DBL_MAX;  
  double min_jacobi = VERDICT_DBL_MAX;
  double temp_norm_jac, lengths;
  double len1_sq, len2_sq, len3_sq; 
  VerdictVector xxi, xet, xze;

  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );

  xxi = v_calc_hex_efg(1, node_pos );
  xet = v_calc_hex_efg(2, node_pos );
  xze = v_calc_hex_efg(3, node_pos );

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi) { min_jacobi = jacobi; }

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else  
    temp_norm_jac = jacobi;

  // J(0,0,0):

  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi ) { min_jacobi = jacobi; }

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else  
    temp_norm_jac = jacobi;


  // J(1,0,0):
  
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi ) { min_jacobi = jacobi; }
  
  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else  
    temp_norm_jac = jacobi;

  // J(1,1,0):
  
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi ) { min_jacobi = jacobi; }
  
  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else  
    temp_norm_jac = jacobi;

  // J(0,1,0):
  
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi ) { min_jacobi = jacobi; }
  
  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else  
    temp_norm_jac = jacobi;

  // J(0,0,1):

  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi ) { min_jacobi = jacobi; }
  
  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else 
    temp_norm_jac = jacobi;

  // J(1,0,1):

  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi ) { min_jacobi = jacobi; }
  
  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else
    temp_norm_jac = jacobi;

  // J(1,1,1):

  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi ) { min_jacobi = jacobi; }
  
  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else
    temp_norm_jac = jacobi;

  // J(0,1,1):

  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  jacobi = xxi % ( xet * xze );
  if ( jacobi < min_jacobi ) { min_jacobi = jacobi; }
  
  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return (double) VERDICT_DBL_MAX;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  temp_norm_jac = jacobi / lengths;
  if ( temp_norm_jac < min_norm_jac)
    min_norm_jac = temp_norm_jac;  
  else 
    temp_norm_jac = jacobi;

  if ( min_norm_jac> 0 )
    return (double) VERDICT_MIN( min_norm_jac, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_norm_jac, -VERDICT_DBL_MAX );
}

/*!
  shear of a hex
  
  3/Condition number of Jacobian Skew matrix
*/
C_FUNC_DEF double v_hex_shear( int /*num_nodes*/, double coordinates[][3] )
{

  double shear;
  double min_shear = 1.0; 
  VerdictVector xxi, xet, xze;
  double det, len1_sq, len2_sq, len3_sq, lengths;


  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );

  // J(0,0,0):

  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return 0;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  det = xxi % (xet * xze);
  if ( det < VERDICT_DBL_MIN ) { return 0; }

  shear = det / lengths;
  min_shear = VERDICT_MIN( shear, min_shear );


  // J(1,0,0):
  
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return 0;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  det = xxi % (xet * xze);
  if ( det < VERDICT_DBL_MIN ) { return 0; }

  shear = det / lengths; 
  min_shear = VERDICT_MIN( shear, min_shear );


  // J(1,1,0):
  
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return 0;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  det = xxi % (xet * xze);
  if ( det < VERDICT_DBL_MIN ) { return 0; }

  shear = det / lengths; 
  min_shear = VERDICT_MIN( shear, min_shear );


  // J(0,1,0):
  
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return 0;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  det = xxi % (xet * xze);
  if ( det < VERDICT_DBL_MIN ) { return 0; }

  shear = det / lengths; 
  min_shear = VERDICT_MIN( shear, min_shear );


  // J(0,0,1):

  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return 0;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  det = xxi % (xet * xze);
  if ( det < VERDICT_DBL_MIN ) { return 0; }

  shear = det / lengths; 
  min_shear = VERDICT_MIN( shear, min_shear );


  // J(1,0,1):

  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return 0;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  det = xxi % (xet * xze);
  if ( det < VERDICT_DBL_MIN ) { return 0; }

  shear = det / lengths; 
  min_shear = VERDICT_MIN( shear, min_shear );


  // J(1,1,1):

  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return 0;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  det = xxi % (xet * xze);
  if ( det < VERDICT_DBL_MIN ) { return 0; }

  shear = det / lengths; 
  min_shear = VERDICT_MIN( shear, min_shear );


  // J(0,1,1):

  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  len1_sq = xxi.length_squared();
  len2_sq = xet.length_squared();
  len3_sq = xze.length_squared();

  if ( len1_sq <= VERDICT_DBL_MIN || len2_sq <= VERDICT_DBL_MIN ||
      len3_sq <= VERDICT_DBL_MIN)
    return 0;

  lengths = sqrt( len1_sq * len2_sq * len3_sq );
  det = xxi % (xet * xze);
  if ( det < VERDICT_DBL_MIN ) { return 0; }

  shear = det / lengths; 
  min_shear = VERDICT_MIN( shear, min_shear );

  if ( min_shear <= VERDICT_DBL_MIN )
    min_shear = 0;
  
  if ( min_shear > 0 )
    return (double) VERDICT_MIN( min_shear, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_shear, -VERDICT_DBL_MAX );
}

/*!
  shape of a hex

  3/Condition number of weighted Jacobian matrix
*/
C_FUNC_DEF double v_hex_shape( int /*num_nodes*/, double coordinates[][3] )
{


  double det, shape;
  double min_shape = 1.0; 
  static const double two_thirds = 2.0/3.0;

  VerdictVector xxi, xet, xze;

  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );


  // J(0,0,0):

  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];

  det = xxi % (xet * xze);
  if ( det > VERDICT_DBL_MIN ) 
    shape = 3 * pow( det, two_thirds) / (xxi%xxi + xet%xet + xze%xze);
  else
    return 0;
  
  if ( shape < min_shape ) { min_shape = shape; }
  

  // J(1,0,0):
  
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  det = xxi % (xet * xze);
  if ( det > VERDICT_DBL_MIN ) 
    shape = 3 * pow( det, two_thirds) / (xxi%xxi + xet%xet + xze%xze);
  else
    return 0;
  
  if ( shape < min_shape ) { min_shape = shape; }
  

  // J(1,1,0):
  
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  det = xxi % (xet * xze);
  if ( det > VERDICT_DBL_MIN ) 
    shape = 3 * pow( det, two_thirds) / (xxi%xxi + xet%xet + xze%xze);
  else
    return 0;
  
  if ( shape < min_shape ) { min_shape = shape; }
  

  // J(0,1,0):
  
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  det = xxi % (xet * xze);
  if ( det > VERDICT_DBL_MIN ) 
    shape = 3 * pow( det, two_thirds) / (xxi%xxi + xet%xet + xze%xze);
  else
    return 0;
  
  if ( shape < min_shape ) { min_shape = shape; }
  

  // J(0,0,1):

  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  det = xxi % (xet * xze);
  if ( det > VERDICT_DBL_MIN ) 
    shape = 3 * pow( det, two_thirds) / (xxi%xxi + xet%xet + xze%xze);
  else
    return 0;
  
  if ( shape < min_shape ) { min_shape = shape; }
  

  // J(1,0,1):

  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  det = xxi % (xet * xze);
  if ( det > VERDICT_DBL_MIN ) 
    shape = 3 * pow( det, two_thirds) / (xxi%xxi + xet%xet + xze%xze);
  else
    return 0;
  
  if ( shape < min_shape ) { min_shape = shape; }
  

  // J(1,1,1):

  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  det = xxi % (xet * xze);
  if ( det > VERDICT_DBL_MIN ) 
    shape = 3 * pow( det, two_thirds) / (xxi%xxi + xet%xet + xze%xze);
  else
    return 0;
  
  if ( shape < min_shape ) { min_shape = shape; }
  

  // J(1,1,1):

  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  det = xxi % (xet * xze);
  if ( det > VERDICT_DBL_MIN ) 
    shape = 3 * pow( det, two_thirds) / (xxi%xxi + xet%xet + xze%xze);
  else
    return 0;
  
  if ( shape < min_shape ) { min_shape = shape; }

  if ( min_shape <= VERDICT_DBL_MIN )
    min_shape = 0;

  if ( min_shape > 0 )
    return (double) VERDICT_MIN( min_shape, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( min_shape, -VERDICT_DBL_MAX );
}

/*!
  relative size of a hex

  Min( J, 1/J ), where J is determinant of weighted Jacobian matrix
*/
C_FUNC_DEF double v_hex_relative_size_squared( int /*num_nodes*/, double coordinates[][3] )
{
  double size = 0;
  double tau; 

  VerdictVector xxi, xet, xze;
  double det, det_sum = 0;

  v_hex_get_weight( xxi, xet, xze );
 
  //This is the average relative size 
  double detw = xxi % (xet * xze);

  if ( detw < VERDICT_DBL_MIN ) 
    return 0; 

  VerdictVector node_pos[8];
  make_hex_nodes ( coordinates, node_pos );

  // J(0,0,0):

  xxi = node_pos[1] - node_pos[0];
  xet = node_pos[3] - node_pos[0];
  xze = node_pos[4] - node_pos[0];
  
  det = xxi % (xet * xze);
  det_sum += det;  


  // J(1,0,0):
  
  xxi = node_pos[2] - node_pos[1];
  xet = node_pos[0] - node_pos[1];
  xze = node_pos[5] - node_pos[1];

  det = xxi % (xet * xze);
  det_sum += det;  


  // J(0,1,0):
  
  xxi = node_pos[3] - node_pos[2];
  xet = node_pos[1] - node_pos[2];
  xze = node_pos[6] - node_pos[2];

  det = xxi % (xet * xze);
  det_sum += det;  

  // J(1,1,0):
  
  xxi = node_pos[0] - node_pos[3];
  xet = node_pos[2] - node_pos[3];
  xze = node_pos[7] - node_pos[3];

  det = xxi % (xet * xze);
  det_sum += det;  


  // J(0,1,0):

  xxi = node_pos[7] - node_pos[4];
  xet = node_pos[5] - node_pos[4];
  xze = node_pos[0] - node_pos[4];

  det = xxi % (xet * xze);
  det_sum += det;  


  // J(1,0,1):

  xxi = node_pos[4] - node_pos[5];
  xet = node_pos[6] - node_pos[5];
  xze = node_pos[1] - node_pos[5];

  det = xxi % (xet * xze);
  det_sum += det;  


  // J(1,1,1):

  xxi = node_pos[5] - node_pos[6];
  xet = node_pos[7] - node_pos[6];
  xze = node_pos[2] - node_pos[6];

  det = xxi % (xet * xze);
  det_sum += det;  


  // J(1,1,1):

  xxi = node_pos[6] - node_pos[7];
  xet = node_pos[4] - node_pos[7];
  xze = node_pos[3] - node_pos[7];

  det = xxi % (xet * xze);
  det_sum += det;  


  if ( det_sum > VERDICT_DBL_MIN )
  {
    tau = det_sum / ( 8*detw);

    tau = VERDICT_MIN( tau, 1.0/tau); 

    size = tau*tau; 
  }

  if ( size > 0 )
    return (double) VERDICT_MIN( size, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( size, -VERDICT_DBL_MAX );
}

/*!
  shape and size of a hex

  Product of Shape and Relative Size
*/
C_FUNC_DEF double v_hex_shape_and_size( int num_nodes, double coordinates[][3] )
{
  double size = v_hex_relative_size_squared( num_nodes, coordinates );
  double shape = v_hex_shape( num_nodes, coordinates );

  double shape_size = size * shape;

  if ( shape_size > 0 )
    return (double) VERDICT_MIN( shape_size, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( shape_size, -VERDICT_DBL_MAX );

}



/*!
  shear and size of a hex

  Product of Shear and Relative Size
*/
C_FUNC_DEF double v_hex_shear_and_size( int num_nodes, double coordinates[][3] )
{
  double size = v_hex_relative_size_squared( num_nodes, coordinates );
  double shear = v_hex_shear( num_nodes, coordinates );

  double shear_size = shear * size; 

  if ( shear_size > 0 )
    return (double) VERDICT_MIN( shear_size, VERDICT_DBL_MAX );
  return (double) VERDICT_MAX( shear_size, -VERDICT_DBL_MAX );

}

/*!
  distortion of a hex
*/
C_FUNC_DEF double v_hex_distortion( int num_nodes, double coordinates[][3] )
{

   //use 2x2 gauss points for linear hex and 3x3 for 2nd order hex
   int number_of_gauss_points=0;
   if (num_nodes ==8)
      //2x2 quadrature rule
      number_of_gauss_points = 2;
   else if (num_nodes ==20)
      //3x3 quadrature rule
      number_of_gauss_points = 3;

   int number_dimension = 3;
   int total_number_of_gauss_points = number_of_gauss_points
      *number_of_gauss_points*number_of_gauss_points;
   double distortion = VERDICT_DBL_MAX;

   // maxTotalNumberGaussPoints =27, maxNumberNodes = 20
   // they are defined in GaussIntegration.hpp
   // This is used to make these arrays static.
   // I tried dynamically allocated arrays but the new and delete
   // was very expensive

   double shape_function[maxTotalNumberGaussPoints][maxNumberNodes];
   double dndy1[maxTotalNumberGaussPoints][maxNumberNodes];
   double dndy2[maxTotalNumberGaussPoints][maxNumberNodes];
   double dndy3[maxTotalNumberGaussPoints][maxNumberNodes];
   double weight[maxTotalNumberGaussPoints];


   //create an object of GaussIntegration
   GaussIntegration::initialize(number_of_gauss_points,num_nodes,number_dimension );
   GaussIntegration::calculate_shape_function_3d_hex();
   GaussIntegration::get_shape_func(shape_function[0], dndy1[0], dndy2[0], dndy3[0],weight);


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

      jacobian = xxi % (xet * xze);
      if (minimum_jacobian > jacobian)
         minimum_jacobian = jacobian;

      element_volume += weight[ife]*jacobian;
      }

   // loop through all nodes
   double dndy1_at_node[maxNumberNodes][maxNumberNodes];
   double dndy2_at_node[maxNumberNodes][maxNumberNodes];
   double dndy3_at_node[maxNumberNodes][maxNumberNodes];

   GaussIntegration::calculate_derivative_at_nodes_3d( dndy1_at_node, dndy2_at_node, dndy3_at_node);
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

      jacobian = xxi % (xet * xze);
      if (minimum_jacobian > jacobian)
         minimum_jacobian = jacobian;

      }
   distortion = minimum_jacobian/element_volume*8.;
   return (double)distortion;
}





/*
C_FUNC_DEF double hex_jac_normjac_oddy_cond( int choices[], 
                      double coordinates[][3],
                      double answers[4]  )
{

  //Define variables
  int i; 
  
  double xxi[3], xet[3], xze[3];
  double norm_jacobian = 0.0, current_norm_jac = 0.0; 
        double jacobian = 0.0, current_jacobian = 0.0;
  double oddy = 0.0, current_oddy = 0.0;  
  double condition = 0.0, current_condition = 0.0;


        for( i=0; i<3; i++)
          xxi[i] = v_calc_hex_efg( 2, i, coordinates );
        for( i=0; i<3; i++)
          xet[i] = v_calc_hex_efg( 3, i, coordinates );
        for( i=0; i<3; i++)
          xze[i] = v_calc_hex_efg( 6, i, coordinates );

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze  );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 

    current_jacobian /= 64.0;
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }


  for( i=0; i<3; i++)
  {
    xxi[i] = coordinates[1][i] - coordinates[0][i];
    xet[i] = coordinates[3][i] - coordinates[0][i];
    xze[i] = coordinates[4][i] - coordinates[0][i];
  } 

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze  );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  
  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[1][i] - coordinates[0][i];
          xet[i] = coordinates[2][i] - coordinates[1][i];
          xze[i] = coordinates[5][i] - coordinates[1][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi,  xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }


  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[2][i] - coordinates[3][i];
          xet[i] = coordinates[3][i] - coordinates[0][i];
          xze[i] = coordinates[7][i] - coordinates[3][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  
  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[5][i] - coordinates[4][i];
          xet[i] = coordinates[7][i] - coordinates[4][i];
          xze[i] = coordinates[4][i] - coordinates[0][i];
  }  


  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  
  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[2][i] - coordinates[3][i];
          xet[i] = coordinates[2][i] - coordinates[1][i];
          xze[i] = coordinates[6][i] - coordinates[2][i];
  }  

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }


  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[5][i] - coordinates[4][i];
          xet[i] = coordinates[6][i] - coordinates[5][i];
          xze[i] = coordinates[5][i] - coordinates[1][i];
  }


  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

    
  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[6][i] - coordinates[7][i];
          xet[i] = coordinates[7][i] - coordinates[4][i];
          xze[i] = coordinates[7][i] - coordinates[3][i];
  }


  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }
  }

  
  for( i=0; i<3; i++)
  {
          xxi[i] = coordinates[6][i] - coordinates[7][i];
          xet[i] = coordinates[6][i] - coordinates[5][i];
          xze[i] = coordinates[6][i] - coordinates[2][i];
  }

  // norm jacobian and jacobian
  if ( choices[0] || choices[1] )
  {
    current_jacobian = determinant( xxi, xet, xze );
    current_norm_jac = normalize_jacobian( current_jacobian,
            xxi, xet, xze );
    
    if (current_norm_jac < norm_jacobian) { norm_jacobian = current_norm_jac; } 
    if (current_jacobian < jacobian) { jacobian = current_jacobian; }
  }

  // oddy 
  if ( choices[2] )
  {
    current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
  }

  // condition
  if ( choices[3] )
  {
    current_condition = v_condition_comp( xxi, xet, xze );
    if ( current_condition > condition ) { condition = current_condition; }

    condition /= 3.0;
  }

  
  answers[0] = jacobian;
  answers[1] = norm_jacobian;
  answers[2] = oddy;
  answers[3] = condition;

  return 1.0;

}
*/

/*!
  multiple quality functions of a hex
*/
C_FUNC_DEF void v_hex_quality( int num_nodes, double coordinates[][3], 
  unsigned int metrics_request_flag, HexMetricVals *metric_vals )
{ 
  memset( metric_vals, 0, sizeof(HexMetricVals) );

  // max edge ratio, skew, taper, and volume
  if (metrics_request_flag & (V_HEX_MAX_EDGE_RATIO | V_HEX_SKEW | V_HEX_TAPER ) ) 
  {
    VerdictVector node_pos[8];
    make_hex_nodes ( coordinates, node_pos );
   
    VerdictVector efg1, efg2, efg3;
    efg1 = v_calc_hex_efg( 1, node_pos);
    efg2 = v_calc_hex_efg( 2, node_pos);
    efg3 = v_calc_hex_efg( 3, node_pos);

    if (metrics_request_flag & V_HEX_MAX_EDGE_RATIO)
    {
      double max_edge_ratio_12, max_edge_ratio_13, max_edge_ratio_23;

      double mag_efg1 = efg1.length();
      double mag_efg2 = efg2.length();
      double mag_efg3 = efg3.length();
      
      max_edge_ratio_12 = v_safe_ratio( VERDICT_MAX( mag_efg1, mag_efg2 ) , VERDICT_MIN( mag_efg1, mag_efg2 ) );
      max_edge_ratio_13 = v_safe_ratio( VERDICT_MAX( mag_efg1, mag_efg3 ) , VERDICT_MIN( mag_efg1, mag_efg3 ) );
      max_edge_ratio_23 = v_safe_ratio( VERDICT_MAX( mag_efg2, mag_efg3 ) , VERDICT_MIN( mag_efg2, mag_efg3 ) );

      metric_vals->max_edge_ratio = (double)VERDICT_MAX( max_edge_ratio_12, VERDICT_MAX( max_edge_ratio_13, max_edge_ratio_23 ) );
    }
    
    if (metrics_request_flag & V_HEX_SKEW)
    {
      
      VerdictVector vec1 = efg1;
      VerdictVector vec2 = efg2;
      VerdictVector vec3 = efg3;

      if ( vec1.normalize() <= VERDICT_DBL_MIN ||
          vec2.normalize() <= VERDICT_DBL_MIN ||
          vec3.normalize() <= VERDICT_DBL_MIN  )
      {
        metric_vals->skew = (double)VERDICT_DBL_MAX;
      }
      else
      {
        double skewx = fabs(vec1 % vec2);
        double skewy = fabs(vec1 % vec3);
        double skewz = fabs(vec2 % vec3);

        metric_vals->skew = (double)(VERDICT_MAX( skewx, VERDICT_MAX( skewy, skewz ) ));
      }
    }
  
    if (metrics_request_flag & V_HEX_TAPER)
    {
      VerdictVector efg12 = v_calc_hex_efg( 12, node_pos);
      VerdictVector efg13 = v_calc_hex_efg( 13, node_pos);
      VerdictVector efg23 = v_calc_hex_efg( 23, node_pos);

      double taperx = fabs( v_safe_ratio( efg12.length(), VERDICT_MIN( efg1.length(), efg2.length())));
      double tapery = fabs( v_safe_ratio( efg13.length(), VERDICT_MIN( efg1.length(), efg3.length())));
      double taperz = fabs( v_safe_ratio( efg23.length(), VERDICT_MIN( efg2.length(), efg3.length())));
      
      metric_vals->taper = (double)VERDICT_MAX(taperx, VERDICT_MAX(tapery, taperz));  
    }
  }
  
  if (metrics_request_flag & V_HEX_VOLUME)
  {
    metric_vals->volume = v_hex_volume(8, coordinates ); 
  }

  if (metrics_request_flag & ( V_HEX_JACOBIAN | 
                              V_HEX_SCALED_JACOBIAN | 
                              V_HEX_MED_ASPECT_FROBENIUS |
                              V_HEX_MAX_ASPECT_FROBENIUS |
                              V_HEX_SHEAR | 
                              V_HEX_SHAPE | 
                              V_HEX_RELATIVE_SIZE_SQUARED |
                              V_HEX_SHAPE_AND_SIZE |
                              V_HEX_SHEAR_AND_SIZE |
                              V_HEX_STRETCH ))
  {

    static const double two_thirds = 2.0/3.0;
    VerdictVector edges[12];
    // the length squares
    double length_squared[12];
    // make vectors from coordinates
    v_make_hex_edges(coordinates, edges);

    // calculate the length squares if we need to
    if (metrics_request_flag & ( V_HEX_JACOBIAN | V_HEX_SHEAR | V_HEX_SCALED_JACOBIAN | V_HEX_SHAPE | 
          V_HEX_SHAPE_AND_SIZE | V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHEAR_AND_SIZE | V_HEX_STRETCH))
      make_edge_length_squares(edges, length_squared);

    double jacobian = VERDICT_DBL_MAX, scaled_jacobian = VERDICT_DBL_MAX;
    double med_aspect_frobenius = 0.;
    double max_aspect_frobenius = 0.;
    double shear=1.0, shape=1.0, oddy = 0.0;
    double current_jacobian, current_scaled_jacobian, current_condition, current_shape,
      detw=0, det_sum=0, current_oddy;
    VerdictBoolean rel_size_error = VERDICT_FALSE;
    
    VerdictVector xxi, xet, xze;

    // get weights if we need based on average size of a hex
    if (metrics_request_flag & (V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      v_hex_get_weight(xxi, xet, xze);
      detw = xxi % (xet * xze);
      if (detw < VERDICT_DBL_MIN)
        rel_size_error = VERDICT_TRUE;
    }

    xxi = edges[0] - edges[2] + edges[4] - edges[6];
    xet = edges[1] - edges[3] + edges[5] - edges[7];
    xze = edges[8] + edges[9] + edges[10] + edges[11];

    current_jacobian = xxi % (xet * xze) / 64.0;
    if ( current_jacobian < jacobian ) 
      jacobian = current_jacobian;


    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      current_jacobian *= 64.0;
      current_scaled_jacobian = current_jacobian / 
        sqrt(xxi.length_squared() * xet.length_squared() * xze.length_squared());
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }

    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( xxi, xet, xze );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }
    

    // J(0,0,0)
    current_jacobian = edges[0] % (-edges[3] * edges[8]);
    if ( current_jacobian < jacobian )
      jacobian = current_jacobian;

    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      det_sum += current_jacobian;
    }
    
    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      if (length_squared[0] <= VERDICT_DBL_MIN || length_squared[3] <= VERDICT_DBL_MIN
         || length_squared[8] <= VERDICT_DBL_MIN)
      {
        current_scaled_jacobian = VERDICT_DBL_MAX;
      }
      else
      {
        current_scaled_jacobian = current_jacobian / 
          sqrt(length_squared[0] * length_squared[3] * length_squared[8]);
      }
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }

    if ( metrics_request_flag & ( V_HEX_MAX_ASPECT_FROBENIUS | V_HEX_MED_ASPECT_FROBENIUS ) )
    {
      current_condition = v_condition_comp( edges[0], -edges[3], edges[8] );
      // First computation of the current_condition: no need to += nor to check if > max
      if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS ) { med_aspect_frobenius = current_condition; }
      if ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS ) { max_aspect_frobenius = current_condition; }
    }

    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( edges[0], -edges[3], edges[8] );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }

    if (metrics_request_flag & ( V_HEX_SHAPE | V_HEX_SHAPE_AND_SIZE ))
    {
      if (current_jacobian > VERDICT_DBL_MIN)
        current_shape = 3 * pow( current_jacobian, two_thirds) / 
          (length_squared[0] + length_squared[3] + length_squared[8]);
      else
        current_shape = 0;

      if (current_shape < shape) { shape = current_shape; }

    }

    // J(1,0,0)
    current_jacobian = edges[1] % (-edges[0] * edges[9]);
    if ( current_jacobian < jacobian )
      jacobian = current_jacobian;
    
    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      det_sum += current_jacobian;
    }
    
    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      if (length_squared[1] <= VERDICT_DBL_MIN || length_squared[0] <= VERDICT_DBL_MIN
         || length_squared[9] <= VERDICT_DBL_MIN)
      {
        current_scaled_jacobian = VERDICT_DBL_MAX;
      }
      else
      {
        current_scaled_jacobian = current_jacobian / 
          sqrt(length_squared[1] * length_squared[0] * length_squared[9]);
      }
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }

    if ( metrics_request_flag & ( V_HEX_MAX_ASPECT_FROBENIUS | V_HEX_MED_ASPECT_FROBENIUS ) )
    {
      current_condition = v_condition_comp( edges[1], -edges[0], edges[9] );
      if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS ) { med_aspect_frobenius += current_condition; }
      if ( ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS ) && ( current_condition > max_aspect_frobenius ) ) { max_aspect_frobenius = current_condition; }
    }

    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( edges[1], -edges[0], edges[9] );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }
    
    if (metrics_request_flag & ( V_HEX_SHAPE | V_HEX_SHAPE_AND_SIZE ))
    {
      if (current_jacobian > VERDICT_DBL_MIN)
        current_shape = 3 * pow( current_jacobian, two_thirds) / 
          (length_squared[1] + length_squared[0] + length_squared[9]);
      else
        current_shape = 0;

      if (current_shape < shape) { shape = current_shape; }

    }

    // J(1,1,0)
    current_jacobian = edges[2] % (-edges[1] * edges[10]);
    if ( current_jacobian < jacobian )
      jacobian = current_jacobian;
    
    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      det_sum += current_jacobian;
    }
    
    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      if (length_squared[2] <= VERDICT_DBL_MIN || length_squared[1] <= VERDICT_DBL_MIN
         || length_squared[10] <= VERDICT_DBL_MIN)
      {
        current_scaled_jacobian = VERDICT_DBL_MAX;
      }
      else
      {
        current_scaled_jacobian = current_jacobian / 
          sqrt(length_squared[2] * length_squared[1] * length_squared[10]);
      }
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }
    
    if ( metrics_request_flag & ( V_HEX_MAX_ASPECT_FROBENIUS | V_HEX_MED_ASPECT_FROBENIUS ) )
    {
      current_condition = v_condition_comp( edges[2], -edges[1], edges[10] );
      if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS ) { med_aspect_frobenius += current_condition; }
      if ( ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS ) && ( current_condition > max_aspect_frobenius ) ) { max_aspect_frobenius = current_condition; }
    }
    
    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( edges[2], -edges[1], edges[10] );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }

    if (metrics_request_flag & ( V_HEX_SHAPE | V_HEX_SHAPE_AND_SIZE ))
    {
      if (current_jacobian > VERDICT_DBL_MIN)
        current_shape = 3 * pow( current_jacobian, two_thirds) / 
          (length_squared[2] + length_squared[1] + length_squared[10]);
      else
        current_shape = 0;

      if (current_shape < shape) { shape = current_shape; }

    }


    // J(0,1,0)
    current_jacobian = edges[3] % (-edges[2] * edges[11]);
    if ( current_jacobian < jacobian )
      jacobian = current_jacobian;
    
    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      det_sum += current_jacobian;
    }
    
    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      if (length_squared[3] <= VERDICT_DBL_MIN || length_squared[2] <= VERDICT_DBL_MIN
         || length_squared[11] <= VERDICT_DBL_MIN)
      {
        current_scaled_jacobian = VERDICT_DBL_MAX;
      }
      else
      {
        current_scaled_jacobian = current_jacobian / 
          sqrt(length_squared[3] * length_squared[2] * length_squared[11]);
      }
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }

    if ( metrics_request_flag & ( V_HEX_MAX_ASPECT_FROBENIUS | V_HEX_MED_ASPECT_FROBENIUS ) )
    {
      current_condition = v_condition_comp( edges[3], -edges[2], edges[11] );
      if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS ) { med_aspect_frobenius += current_condition; }
      if ( ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS ) && ( current_condition > max_aspect_frobenius ) ) { max_aspect_frobenius = current_condition; }
    }
    
    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( edges[3], -edges[2], edges[11] );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }

    if (metrics_request_flag & ( V_HEX_SHAPE | V_HEX_SHAPE_AND_SIZE ))
    {
      if (current_jacobian > VERDICT_DBL_MIN)
        current_shape = 3 * pow( current_jacobian, two_thirds) / 
          (length_squared[3] + length_squared[2] + length_squared[11]);
      else
        current_shape = 0;

      if (current_shape < shape) { shape = current_shape; }

    }

    // J(0,0,1)
    current_jacobian = edges[4] % (-edges[8] * -edges[7]);
    if ( current_jacobian < jacobian )
      jacobian = current_jacobian;
    
    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      det_sum += current_jacobian;
    }
    
    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      if (length_squared[4] <= VERDICT_DBL_MIN || length_squared[8] <= VERDICT_DBL_MIN
         || length_squared[7] <= VERDICT_DBL_MIN)
      {
        current_scaled_jacobian = VERDICT_DBL_MAX;
      }
      else
      {
        current_scaled_jacobian = current_jacobian / 
          sqrt(length_squared[4] * length_squared[8] * length_squared[7]);
      }
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }

    if ( metrics_request_flag & ( V_HEX_MAX_ASPECT_FROBENIUS | V_HEX_MED_ASPECT_FROBENIUS ) )
    {
      current_condition = v_condition_comp( edges[4], -edges[8], -edges[7] );
      if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS ) { med_aspect_frobenius += current_condition; }
      if ( ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS ) && ( current_condition > max_aspect_frobenius ) ) { max_aspect_frobenius = current_condition; }
    }
    
    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( edges[4], -edges[8], -edges[7] );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }

    if (metrics_request_flag & ( V_HEX_SHAPE | V_HEX_SHAPE_AND_SIZE ))
    {
      if (current_jacobian > VERDICT_DBL_MIN)
        current_shape = 3 * pow( current_jacobian, two_thirds) / 
          (length_squared[4] + length_squared[8] + length_squared[7]);
      else
        current_shape = 0;

      if (current_shape < shape) { shape = current_shape; }

    }

    // J(1,0,1)
    current_jacobian = -edges[4] % (edges[5] * -edges[9]);
    if ( current_jacobian < jacobian )
      jacobian = current_jacobian;
    
    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      det_sum += current_jacobian;
    }
    
    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      if (length_squared[4] <= VERDICT_DBL_MIN || length_squared[5] <= VERDICT_DBL_MIN
         || length_squared[9] <= VERDICT_DBL_MIN)
      {
        current_scaled_jacobian = VERDICT_DBL_MAX;
      }
      else
      {
        current_scaled_jacobian = current_jacobian / 
          sqrt(length_squared[4] * length_squared[5] * length_squared[9]);
      }
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }
    
    if ( metrics_request_flag & ( V_HEX_MAX_ASPECT_FROBENIUS | V_HEX_MED_ASPECT_FROBENIUS ) )
    {
      current_condition = v_condition_comp( -edges[4], edges[5], -edges[9] );
      if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS ) { med_aspect_frobenius += current_condition; }
      if ( ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS ) && ( current_condition > max_aspect_frobenius ) ) { max_aspect_frobenius = current_condition; }
    }
    
    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( -edges[4], edges[5], -edges[9] );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }

    if (metrics_request_flag & ( V_HEX_SHAPE | V_HEX_SHAPE_AND_SIZE ))
    {
      if (current_jacobian > VERDICT_DBL_MIN)
        current_shape = 3 * pow( current_jacobian, two_thirds) / 
          (length_squared[4] + length_squared[5] + length_squared[9]);
      else
        current_shape = 0;

      if (current_shape < shape) { shape = current_shape; }

    }

    // J(1,1,1)
    current_jacobian = -edges[5] % (edges[6] * -edges[10]);
    if ( current_jacobian < jacobian )
      jacobian = current_jacobian;
    
    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      det_sum += current_jacobian;
    }
    
    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      if (length_squared[5] <= VERDICT_DBL_MIN || length_squared[6] <= VERDICT_DBL_MIN
         || length_squared[10] <= VERDICT_DBL_MIN)
      {
        current_scaled_jacobian = VERDICT_DBL_MAX;
      }
      else
      {
        current_scaled_jacobian = current_jacobian / 
          sqrt(length_squared[5] * length_squared[6] * length_squared[10]);
      }
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }
    
    if ( metrics_request_flag & ( V_HEX_MAX_ASPECT_FROBENIUS | V_HEX_MED_ASPECT_FROBENIUS ) )
    {
      current_condition = v_condition_comp( -edges[5], edges[6], -edges[10] );
      if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS ) { med_aspect_frobenius += current_condition; }
      if ( ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS ) && ( current_condition > max_aspect_frobenius ) ) { max_aspect_frobenius = current_condition; }
    }
    
    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( -edges[5], edges[6], -edges[10] );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }

    if (metrics_request_flag & ( V_HEX_SHAPE | V_HEX_SHAPE_AND_SIZE ))
    {
      if (current_jacobian > VERDICT_DBL_MIN)
        current_shape = 3 * pow( current_jacobian, two_thirds) / 
          (length_squared[5] + length_squared[6] + length_squared[10]);
      else
        current_shape = 0;

      if (current_shape < shape) { shape = current_shape; }

    }

    // J(0,1,1)
    current_jacobian = -edges[6] % (edges[7] * -edges[11]);
    if ( current_jacobian < jacobian )
      jacobian = current_jacobian;
    
    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      det_sum += current_jacobian;
    }
    
    if (metrics_request_flag & ( V_HEX_SCALED_JACOBIAN | V_HEX_SHEAR | V_HEX_SHEAR_AND_SIZE ))
    {
      if (length_squared[6] <= VERDICT_DBL_MIN || length_squared[7] <= VERDICT_DBL_MIN
         || length_squared[11] <= VERDICT_DBL_MIN)
      {
        current_scaled_jacobian = VERDICT_DBL_MAX;
      }
      else
      {
        current_scaled_jacobian = current_jacobian / 
          sqrt(length_squared[6] * length_squared[7] * length_squared[11]);
      }
      if (current_scaled_jacobian < scaled_jacobian)
        shear = scaled_jacobian = current_scaled_jacobian;
    }
    
    if ( metrics_request_flag & ( V_HEX_MAX_ASPECT_FROBENIUS | V_HEX_MED_ASPECT_FROBENIUS ) )
    {
      current_condition = v_condition_comp( -edges[6], edges[7], -edges[11] );
      if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS ) { med_aspect_frobenius += current_condition; }
      if ( ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS ) && ( current_condition > max_aspect_frobenius ) ) { max_aspect_frobenius = current_condition; }
    }
    
    if (metrics_request_flag & V_HEX_ODDY)
    {
      current_oddy = v_oddy_comp( -edges[6], edges[7], -edges[11] );
      if ( current_oddy > oddy ) { oddy = current_oddy; }
    }

    if (metrics_request_flag & ( V_HEX_SHAPE | V_HEX_SHAPE_AND_SIZE ))
    {
      if (current_jacobian > VERDICT_DBL_MIN)
        current_shape = 3 * pow( current_jacobian, two_thirds) / 
          (length_squared[6] + length_squared[7] + length_squared[11]);
      else
        current_shape = 0;

      if (current_shape < shape) { shape = current_shape; }

    }

    if (metrics_request_flag & ( V_HEX_RELATIVE_SIZE_SQUARED | V_HEX_SHAPE_AND_SIZE | V_HEX_SHEAR_AND_SIZE ))
    {
      if (det_sum > VERDICT_DBL_MIN && rel_size_error != VERDICT_TRUE)
      {
        double tau = det_sum / ( 8 * detw );
        metric_vals->relative_size_squared = (double)VERDICT_MIN( tau*tau, 1.0/tau/tau);
      }
      else
        metric_vals->relative_size_squared = 0.0;
    }

    // set values from above calculations
    if (metrics_request_flag & V_HEX_JACOBIAN)
      metric_vals->jacobian = (double)jacobian;

    if (metrics_request_flag & V_HEX_SCALED_JACOBIAN)
      metric_vals->scaled_jacobian = (double)scaled_jacobian;

    if ( metrics_request_flag & V_HEX_MED_ASPECT_FROBENIUS )
      metric_vals->med_aspect_frobenius = (double)( med_aspect_frobenius / 24. );

    if ( metrics_request_flag & V_HEX_MAX_ASPECT_FROBENIUS )
      metric_vals->condition = metric_vals->max_aspect_frobenius = (double)( max_aspect_frobenius / 3. );

    if (metrics_request_flag & V_HEX_SHEAR)
    {
      if ( shear < VERDICT_DBL_MIN )  //shear has range 0 to +1
        shear = 0;
      metric_vals->shear = (double)shear;
    }

    if (metrics_request_flag & V_HEX_SHAPE)
      metric_vals->shape = (double)shape; 
  
    if (metrics_request_flag & V_HEX_SHAPE_AND_SIZE)
      metric_vals->shape_and_size = (double)(shape * metric_vals->relative_size_squared);
  
    if (metrics_request_flag & V_HEX_SHEAR_AND_SIZE)
      metric_vals->shear_and_size = (double)(shear * metric_vals->relative_size_squared);
  
    if (metrics_request_flag & V_HEX_ODDY)
      metric_vals->oddy = (double)oddy;

    if (metrics_request_flag & V_HEX_STRETCH)
    {
      static const double HEX_STRETCH_SCALE_FACTOR = sqrt(3.0);
      double min_edge=length_squared[0];
      for(int j=1; j<12; j++)
        min_edge = VERDICT_MIN(min_edge, length_squared[j]);

      double max_diag = v_diag_length(1, coordinates);
        
      metric_vals->stretch = (double)(HEX_STRETCH_SCALE_FACTOR * ( v_safe_ratio( sqrt(min_edge), max_diag) ));
    }
  }


  if (metrics_request_flag & V_HEX_DIAGONAL)
    metric_vals->diagonal = v_hex_diagonal(num_nodes, coordinates);
  if (metrics_request_flag & V_HEX_DIMENSION)
    metric_vals->dimension = v_hex_dimension(num_nodes, coordinates);
  if (metrics_request_flag & V_HEX_DISTORTION)
                metric_vals->distortion = v_hex_distortion(num_nodes, coordinates);

  
  //take care of any overflow problems
  //max_edge_ratio
  if ( metric_vals->max_edge_ratio > 0 )
    metric_vals->max_edge_ratio = (double) VERDICT_MIN( metric_vals->max_edge_ratio, VERDICT_DBL_MAX );
  else
    metric_vals->max_edge_ratio = (double) VERDICT_MAX( metric_vals->max_edge_ratio, -VERDICT_DBL_MAX );

  //skew
  if ( metric_vals->skew > 0 )
    metric_vals->skew = (double) VERDICT_MIN( metric_vals->skew, VERDICT_DBL_MAX );
  else
    metric_vals->skew = (double) VERDICT_MAX( metric_vals->skew, -VERDICT_DBL_MAX );

  //taper
  if ( metric_vals->taper > 0 )
    metric_vals->taper = (double) VERDICT_MIN( metric_vals->taper, VERDICT_DBL_MAX );
  else
    metric_vals->taper = (double) VERDICT_MAX( metric_vals->taper, -VERDICT_DBL_MAX );

  //volume
  if ( metric_vals->volume > 0 )
    metric_vals->volume = (double) VERDICT_MIN( metric_vals->volume, VERDICT_DBL_MAX );
  else
    metric_vals->volume = (double) VERDICT_MAX( metric_vals->volume, -VERDICT_DBL_MAX );

  //stretch
  if ( metric_vals->stretch > 0 )
    metric_vals->stretch = (double) VERDICT_MIN( metric_vals->stretch, VERDICT_DBL_MAX );
  else
    metric_vals->stretch = (double) VERDICT_MAX( metric_vals->stretch, -VERDICT_DBL_MAX );

  //diagonal
  if ( metric_vals->diagonal > 0 )
    metric_vals->diagonal = (double) VERDICT_MIN( metric_vals->diagonal, VERDICT_DBL_MAX );
  else
    metric_vals->diagonal = (double) VERDICT_MAX( metric_vals->diagonal, -VERDICT_DBL_MAX );

  //dimension
  if ( metric_vals->dimension > 0 )
    metric_vals->dimension = (double) VERDICT_MIN( metric_vals->dimension, VERDICT_DBL_MAX );
  else
    metric_vals->dimension = (double) VERDICT_MAX( metric_vals->dimension, -VERDICT_DBL_MAX );

  //oddy
  if ( metric_vals->oddy > 0 )
    metric_vals->oddy = (double) VERDICT_MIN( metric_vals->oddy, VERDICT_DBL_MAX );
  else
    metric_vals->oddy = (double) VERDICT_MAX( metric_vals->oddy, -VERDICT_DBL_MAX );

  //condition
  if ( metric_vals->condition > 0 )
    metric_vals->condition = (double) VERDICT_MIN( metric_vals->condition, VERDICT_DBL_MAX );
  else
    metric_vals->condition = (double) VERDICT_MAX( metric_vals->condition, -VERDICT_DBL_MAX );

  //jacobian
  if ( metric_vals->jacobian > 0 )
    metric_vals->jacobian = (double) VERDICT_MIN( metric_vals->jacobian, VERDICT_DBL_MAX );
  else
    metric_vals->jacobian = (double) VERDICT_MAX( metric_vals->jacobian, -VERDICT_DBL_MAX );

  //scaled_jacobian
  if ( metric_vals->scaled_jacobian > 0 )
    metric_vals->scaled_jacobian = (double) VERDICT_MIN( metric_vals->scaled_jacobian, VERDICT_DBL_MAX );
  else
    metric_vals->scaled_jacobian = (double) VERDICT_MAX( metric_vals->scaled_jacobian, -VERDICT_DBL_MAX );

  //shear
  if ( metric_vals->shear > 0 )
    metric_vals->shear = (double) VERDICT_MIN( metric_vals->shear, VERDICT_DBL_MAX );
  else
    metric_vals->shear = (double) VERDICT_MAX( metric_vals->shear, -VERDICT_DBL_MAX );

  //shape
  if ( metric_vals->shape > 0 )
    metric_vals->shape = (double) VERDICT_MIN( metric_vals->shape, VERDICT_DBL_MAX );
  else
    metric_vals->shape = (double) VERDICT_MAX( metric_vals->shape, -VERDICT_DBL_MAX );

  //relative_size_squared
  if ( metric_vals->relative_size_squared > 0 )
    metric_vals->relative_size_squared = (double) VERDICT_MIN( metric_vals->relative_size_squared, VERDICT_DBL_MAX );
  else
    metric_vals->relative_size_squared = (double) VERDICT_MAX( metric_vals->relative_size_squared, -VERDICT_DBL_MAX );

  //shape_and_size
  if ( metric_vals->shape_and_size > 0 )
    metric_vals->shape_and_size = (double) VERDICT_MIN( metric_vals->shape_and_size, VERDICT_DBL_MAX );
  else
    metric_vals->shape_and_size = (double) VERDICT_MAX( metric_vals->shape_and_size, -VERDICT_DBL_MAX );

  //shear_and_size
  if ( metric_vals->shear_and_size > 0 ) metric_vals->shear_and_size = (double) VERDICT_MIN( metric_vals->shear_and_size, VERDICT_DBL_MAX );
  else
    metric_vals->shear_and_size = (double) VERDICT_MAX( metric_vals->shear_and_size, -VERDICT_DBL_MAX );
  
  //distortion
  if ( metric_vals->distortion > 0 )
    metric_vals->distortion = (double) VERDICT_MIN( metric_vals->distortion, VERDICT_DBL_MAX );
  else
    metric_vals->distortion = (double) VERDICT_MAX( metric_vals->distortion, -VERDICT_DBL_MAX );

}



