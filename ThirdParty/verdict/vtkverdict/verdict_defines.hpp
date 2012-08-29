/*=========================================================================

  Module:    verdict_defines.hpp

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 * verdict_defines.cpp contains common definitions
 *
 * This file is part of VERDICT
 *
 */

// .SECTION Thanks
// Prior to its inclusion within VTK, this code was developed by the CUBIT
// project at Sandia National Laboratories. 

#ifndef VERDICT_DEFINES
#define VERDICT_DEFINES

#include <math.h>
#include "v_vector.h"
#include "VerdictVector.hpp"

enum VerdictBoolean { VERDICT_FALSE = 0, VERDICT_TRUE = 1} ;


#define VERDICT_MIN(a,b)  ( (a) < (b) ? (a) : (b) )
#define VERDICT_MAX(a,b)  ( (a) > (b) ? (a) : (b) )

inline double v_determinant(double a,
    double b,
    double c,
    double d)
{ 
  return ((a)*(d) - (b)*(c));
}

inline double v_determinant( VerdictVector v1,
    VerdictVector v2,
    VerdictVector v3 )
{
  return v1 % (v2 * v3);
}

#define jacobian_matrix(a,b,c,d,e,f,g) \
	double jac_mat_tmp; \
	jac_mat_tmp = sqrt(a); \
	if(jac_mat_tmp == 0) { d = 0; e = 0; f = 0; g = 0; } \
	else { d = jac_mat_tmp; e = 0; f = b/jac_mat_tmp; g = c/jac_mat_tmp; }


// this assumes that detmw != 0
#define form_t(m11,m21,m12,m22,mw11,mw21,mw12,mw22,detmw,xm11,xm21,xm12,xm22) \
	xm11= (m11*mw22-m12*mw21)/detmw; \
	xm21= (m21*mw22-m22*mw21)/detmw; \
	xm12= (m12*mw11-m11*mw12)/detmw; \
	xm22= (m22*mw11-m21*mw12)/detmw;


static double v_sqrt_2 = sqrt(2.0);

inline double normalize_jacobian( double jacobi,
    VerdictVector& v1,
    VerdictVector& v2,
    VerdictVector& v3,
    int tet_flag = 0 )
{
  double return_value = 0.0;

  if ( jacobi != 0.0 )
  {
    
    double l1, l2, l3, length_product;
    // Note: there may be numerical problems if one is a lot shorter
    // than the others this way. But scaling each vector before the
    // triple product would involve 3 square roots instead of just
    // one.
    l1 = v1.length_squared();
    l2 = v2.length_squared();
    l3 = v3.length_squared();
    length_product = sqrt( l1 * l2 * l3 );
    
    // if some numerical scaling problem, or just plain roundoff,
    // then push back into range [-1,1].
    if ( length_product < fabs(jacobi) )
    {
      length_product = fabs(jacobi);
    }
    
    if( tet_flag == 1)
      return_value = v_sqrt_2 * jacobi / length_product;
    else
      return_value = jacobi / length_product;
    
  }
  return return_value;
  
}


inline double  norm_squared( double m11,
    double m21,
    double m12,
    double m22 )
{
  return m11*m11+m21*m21+m12*m12+m22*m22;
}

#define metric_matrix(m11,m21,m12,m22,gm11,gm12,gm22) \
	gm11 = m11*m11+m21*m21; \
	gm12 = m11*m12+m21*m22; \
	gm22 = m12*m12+m22*m22;


inline int skew_matrix(double gm11, double gm12, double gm22, double det, double &qm11, double &qm21, double &qm12, double &qm22 )
{
  double tmp = sqrt(gm11*gm22);
  if( tmp == 0 ) {return false;}
  
  qm11=1;
  qm21=0;
  qm12=gm12/tmp;
  qm22=det/tmp;
  return true;
}


inline void inverse(VerdictVector x1,
    VerdictVector x2,
    VerdictVector x3,
    VerdictVector& u1,
    VerdictVector& u2,
    VerdictVector& u3 )
{ 
  double  detx = v_determinant(x1, x2, x3);
  VerdictVector rx1, rx2, rx3;
  
  rx1.set(x1.x(), x2.x(), x3.x());
  rx2.set(x1.y(), x2.y(), x3.y());
  rx3.set(x1.z(), x2.z(), x3.z());
  
  u1 = rx2 * rx3;
  u2 = rx3 * rx1;
  u3 = rx1 * rx2;
  
  u1 /= detx;
  u2 /= detx;
  u3 /= detx;
} 

/*
inline void form_T(double a1[3],
  double a2[3],
  double a3[3],
  double w1[3],
  double w2[3],
  double w3[3],
  double t1[3],
  double t2[3],
  double t3[3] )
{

  double x1[3], x2[3], x3[3];
  double ra1[3], ra2[3], ra3[3];
  
  x1[0] = a1[0];
  x1[1] = a2[0];
  x1[2] = a3[0];
  
  x2[0] = a1[1];
  x2[1] = a2[1];
  x2[2] = a3[1];
  
  x3[0] = a1[2];
  x3[1] = a2[2];
  x3[2] = a3[2];
  
  inverse(w1,w2,w3,x1,x2,x3);
  
  t1[0] = dot_product(ra1, x1);
  t1[1] = dot_product(ra1, x2);
  t1[2] = dot_product(ra1, x3);

  t2[0] = dot_product(ra2, x1);
  t2[0] = dot_product(ra2, x2);
  t2[0] = dot_product(ra2, x3);
  
  t3[0] = dot_product(ra3, x1);
  t3[0] = dot_product(ra3, x2);
  t3[0] = dot_product(ra3, x3);

}
*/

inline void form_Q( const VerdictVector& v1,
    const VerdictVector& v2,
    const VerdictVector& v3,
    VerdictVector& q1,
    VerdictVector& q2,
    VerdictVector& q3 )
{
  
  double g11, g12, g13, g22, g23, g33;
  
  g11 = v1 % v1;
  g12 = v1 % v2;
  g13 = v1 % v3;
  g22 = v2 % v2;
  g23 = v2 % v3;
  g33 = v3 % v3;
  
  double rtg11 = sqrt(g11);
  double rtg22 = sqrt(g22);
  double rtg33 = sqrt(g33);
  VerdictVector temp1;
  
  temp1 = v1 * v2;
  
  double cross = sqrt( temp1 % temp1 );
  
  double q11,q21,q31;
  double q12,q22,q32;
  double q13,q23,q33;
  
  q11=1;
  q21=0;
  q31=0;
  
  q12 = g12 / rtg11 / rtg22;
  q22 = cross / rtg11 / rtg22;
  q32 = 0;
  
  q13 = g13 / rtg11 / rtg33;
  q23 = ( g11*g23-g12*g13 )/ rtg11 / rtg33 / cross;
  temp1 = v2 * v3;
  q33 = ( v1 % temp1  ) / rtg33 / cross;
  
  q1.set( q11, q21, q31 );
  q2.set( q12, q22, q32 );
  q3.set( q13, q23, q33 );
  
}

inline void product( VerdictVector& a1,
    VerdictVector& a2,
    VerdictVector& a3,
    VerdictVector& b1,
    VerdictVector& b2,
    VerdictVector& b3,
    VerdictVector& c1,
    VerdictVector& c2,
    VerdictVector& c3 )
{
  
  VerdictVector x1, x2, x3;
  
  x1.set( a1.x(), a2.x(), a3.x() );
  x2.set( a1.y(), a2.y(), a3.y() );
  x3.set( a1.z(), a2.z(), a3.z() );
  
  c1.set( x1 % b1, x2 % b1, x3 % b1 );
  c2.set( x1 % b2, x2 % b2, x3 % b2 );
  c3.set( x1 % b3, x2 % b3, x3 % b3 );
}


inline double norm_squared( VerdictVector& x1,
    VerdictVector& x2,
    VerdictVector& x3 )

{
  return  (x1 % x1) + (x2 % x2) + (x3 % x3);
}


inline double skew_x( VerdictVector& q1,
    VerdictVector& q2,
    VerdictVector& q3,
    VerdictVector& qw1,
    VerdictVector& qw2,
    VerdictVector& qw3 )
{
  double normsq1, normsq2, kappa;
  VerdictVector u1, u2, u3;
  VerdictVector x1, x2, x3;
  
  inverse(qw1,qw2,qw3,u1,u2,u3);
  product(q1,q2,q3,u1,u2,u3,x1,x2,x3);
  inverse(x1,x2,x3,u1,u2,u3);
  normsq1 = norm_squared(x1,x2,x3);
  normsq2 = norm_squared(u1,u2,u3);
  kappa = sqrt( normsq1 * normsq2 );
  
  double skew = 0;
  if ( kappa > VERDICT_DBL_MIN )
    skew = 3/kappa;
  
  return skew;
}

#endif
