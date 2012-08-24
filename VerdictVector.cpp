/*=========================================================================

  Module:    VerdictVector.cpp

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 *
 * VerdictVector.cpp contains implementation of Vector operations
 *
 * This file is part of VERDICT
 *
 */


#include "verdict.h"
#include <math.h>
#include "VerdictVector.hpp"
#include <float.h>

#if defined(__BORLANDC__)
#pragma warn -8004 /* "assigned a value that is never used" */
#endif

const double TWO_VERDICT_PI = 2.0 * VERDICT_PI;


VerdictVector &VerdictVector::length(const double new_length)
{
  double len = this->length();
  xVal *= new_length / len;
  yVal *= new_length / len;
  zVal *= new_length / len;
  return *this;
}


double VerdictVector::distance_between(const VerdictVector& test_vector)
{
  double xv = xVal - test_vector.x();
  double yv = yVal - test_vector.y();
  double zv = zVal - test_vector.z();
  
  return( sqrt( xv * xv + yv * yv + zv * zv ) );
}

/*
void VerdictVector::print_me()
{
  PRINT_INFO("X: %f\n",xVal);
  PRINT_INFO("Y: %f\n",yVal);
  PRINT_INFO("Z: %f\n",zVal);
  
}
*/

double VerdictVector::interior_angle(const VerdictVector &otherVector)
{
  double cosAngle=0., angleRad=0., len1, len2=0.;
  
  if (((len1 = this->length()) > 0) && ((len2 = otherVector.length()) > 0))
    cosAngle = (*this % otherVector)/(len1 * len2);
  else
  {
    assert(len1 > 0);
    assert(len2 > 0);
  }
  
  if ((cosAngle > 1.0) && (cosAngle < 1.0001))
  {
    cosAngle = 1.0;
    angleRad = acos(cosAngle);
  }
  else if (cosAngle < -1.0 && cosAngle > -1.0001)
  {
    cosAngle = -1.0;
    angleRad = acos(cosAngle);
  }
  else if (cosAngle >= -1.0 && cosAngle <= 1.0)
    angleRad = acos(cosAngle);
  else
  {
    assert(cosAngle < 1.0001 && cosAngle > -1.0001);
  }
  
  return( (angleRad * 180.) / VERDICT_PI );
}


// Interpolate between two vectors.
// Returns (1-param)*v1 + param*v2
VerdictVector v_interpolate(const double param, const VerdictVector &v1,
                        const VerdictVector &v2)
{
  VerdictVector temp = (1.0 - param) * v1;
  temp += param * v2;
  return temp;
}

void VerdictVector::xy_to_rtheta()
{
    //careful about overwriting
  double r_ = length();
  double theta_ = atan2( y(), x() );
  if (theta_ < 0.0) 
    theta_ += TWO_VERDICT_PI;
  
  r( r_ );
  theta( theta_ );
}

void VerdictVector::rtheta_to_xy()
{
    //careful about overwriting
  double x_ =  r() * cos( theta() );
  double y_ =  r() * sin( theta() );
  
  x( x_ );
  y( y_ );
}

void VerdictVector::rotate(double angle, double )
{
  xy_to_rtheta();
  theta() += angle;
  rtheta_to_xy();
}

void VerdictVector::blow_out(double gamma, double rmin)
{
    // if gamma == 1, then 
    // map on a circle : r'^2 = sqrt( 1 - (1-r)^2 )
    // if gamma ==0, then map back to itself
    // in between, linearly interpolate
  xy_to_rtheta();
//  r() = sqrt( (2. - r()) * r() ) * gamma  + r() * (1-gamma);
  assert(gamma > 0.0);
    // the following limits should really be roundoff-based
  if (r() > rmin*1.001 && r() < 1.001) {
    r() = rmin + pow(r(), gamma) * (1.0 - rmin);
  }
  rtheta_to_xy();
}

void VerdictVector::reflect_about_xaxis(double, double )
{
  yVal = -yVal;
}

void VerdictVector::scale_angle(double gamma, double )
{
  const double r_factor = 0.3;
  const double theta_factor = 0.6;
  
  xy_to_rtheta();
  
    // if neary 2pi, treat as zero
    // some near zero stuff strays due to roundoff
  if (theta() > TWO_VERDICT_PI - 0.02)
    theta() = 0;
    // the above screws up on big sheets - need to overhaul at the sheet level
  
  if ( gamma < 1 )
  {
      //squeeze together points of short radius so that
      //long chords won't cross them
    theta() += (VERDICT_PI-theta())*(1-gamma)*theta_factor*(1-r());
    
      //push away from center of circle, again so long chords won't cross
    r( (r_factor + r()) / (1 + r_factor) );
    
      //scale angle by gamma
    theta() *= gamma;
  }
  else
  {
      //scale angle by gamma, making sure points nearly 2pi are treated as zero
    double new_theta = theta() * gamma;
    if ( new_theta < 2.5 * VERDICT_PI || r() < 0.2) 
      theta( new_theta );
  }
  rtheta_to_xy();
}

double VerdictVector::vector_angle_quick(const VerdictVector& vec1, const VerdictVector& vec2)
{
  //- compute the angle between two vectors in the plane defined by this vector
  // build yAxis and xAxis such that xAxis is the projection of
  // vec1 onto the normal plane of this vector

  // NOTE: vec1 and vec2 are Vectors from the vertex of the angle along
  //       the two sides of the angle.
  //       The angle returned is the right-handed angle around this vector
  //       from vec1 to vec2.

  // NOTE: vector_angle_quick gives exactly the same answer as vector_angle below
  //       providing this vector is normalized.  It does so with two fewer
  //       cross-product evaluations and two fewer vector normalizations.
  //       This can be a substantial time savings if the function is called
  //       a significant number of times (e.g Hexer) ... (jrh 11/28/94)
  // NOTE: vector_angle() is much more robust. Do not use vector_angle_quick()
  //       unless you are very sure of the safety of your input vectors.

  VerdictVector ry = (*this) * vec1;
  VerdictVector rx = ry * (*this);

  double xv = vec2 % rx;
  double yv = vec2 % ry;

  double angle;
  assert( xv != 0.0 || yv != 0.0 );

  angle = atan2( yv, xv );

  if ( angle < 0.0 )
  {
    angle += TWO_VERDICT_PI;
  }
  return angle;
}

VerdictVector v_vector_rotate(
  const double angle, const VerdictVector &normalAxis, const VerdictVector &referenceAxis )
{
    // A new coordinate system is created with the xy plane corresponding
    // to the plane normal to the normal axis, and the x axis corresponding to
    // the projection of the reference axis onto the normal plane.  The normal
    // plane is the tangent plane at the root point.  A unit vector is
    // constructed along the local x axis and then rotated by the given
    // ccw angle to form the new point.  The new point, then is a unit
    // distance from the global origin in the tangent plane.
  
  double x, y;
  
    // project a unit distance from root along reference axis
  
  VerdictVector yAxis = normalAxis * referenceAxis;
  VerdictVector xAxis = yAxis * normalAxis;
  yAxis.normalize();
  xAxis.normalize();
  
  x = cos(angle);
  y = sin(angle);
  
  xAxis *= x;
  yAxis *= y;
  return VerdictVector(xAxis + yAxis);
}

double VerdictVector::vector_angle(const VerdictVector &vector1,
                                 const VerdictVector &vector2) const
{
    // This routine does not assume that any of the input vectors are of unit
    // length. This routine does not normalize the input vectors.
    // Special cases:
    //     If the normal vector is zero length:
    //         If a new one can be computed from vectors 1 & 2:
    //             the normal is replaced with the vector cross product
    //         else the two vectors are colinear and zero or 2PI is returned.
    //     If the normal is colinear with either (or both) vectors
    //         a new one is computed with the cross products
    //         (and checked again).
  
    // Check for zero length normal vector
  VerdictVector normal = *this;
  double normal_lensq = normal.length_squared();
  double len_tol = 0.0000001;
  if( normal_lensq <= len_tol )
  {
      // null normal - make it the normal to the plane defined by vector1
      // and vector2. If still null, the vectors are colinear so check
      // for zero or 180 angle.
    normal = vector1 * vector2;
    normal_lensq = normal.length_squared();
    if( normal_lensq <= len_tol )
    {
      double cosine = vector1 % vector2;
      if( cosine > 0.0 ) return 0.0;
      else               return VERDICT_PI;
    }
  }
  
    //Trap for normal vector colinear to one of the other vectors. If so,
    //use a normal defined by the two vectors.
  double dot_tol = 0.985;
  double dot = vector1 % normal;
  if( dot * dot >= vector1.length_squared() * normal_lensq * dot_tol )
  {
    normal = vector1 * vector2;
    normal_lensq = normal.length_squared();
    
      //Still problems if all three vectors were colinear
    if( normal_lensq <= len_tol )
    {
      double cosine = vector1 % vector2;
      if( cosine >= 0.0 ) return 0.0;
      else                return VERDICT_PI;
    }
  }
  else
  {
      //The normal and vector1 are not colinear, now check for vector2
    dot = vector2 % normal;
    if( dot * dot >= vector2.length_squared() * normal_lensq * dot_tol )
    {
      normal = vector1 * vector2;
    }
  }
  
    // Assume a plane such that the normal vector is the plane's normal.
    // Create yAxis perpendicular to both the normal and vector1. yAxis is
    // now in the plane. Create xAxis as the perpendicular to both yAxis and
    // the normal. xAxis is in the plane and is the projection of vector1
    // into the plane.
  
  normal.normalize();
  VerdictVector yAxis = normal;
  yAxis *= vector1;
  double yv = vector2 % yAxis;
    //  yAxis memory slot will now be used for xAxis
  yAxis *= normal;
  double xv = vector2 % yAxis;
  
  
    //  assert(x != 0.0 || y != 0.0);
  if( xv == 0.0 && yv == 0.0 )
  {
    return 0.0;
  }
  double angle = atan2( yv, xv );
  
  if (angle < 0.0)
  {
    angle += TWO_VERDICT_PI;
  }
  return angle;
}

bool VerdictVector::within_tolerance( const VerdictVector &vectorPtr2,
                                            double tolerance) const
{
  if (( fabs (this->x() - vectorPtr2.x()) < tolerance) &&
      ( fabs (this->y() - vectorPtr2.y()) < tolerance) &&
      ( fabs (this->z() - vectorPtr2.z()) < tolerance)
      )
  {
    return true;
  }
  
  return false;
}

void VerdictVector::orthogonal_vectors( VerdictVector &vector2, 
                                      VerdictVector &vector3 )
{
  double xv[3];
  unsigned short i=0;
  unsigned short imin=0;
  double rmin = 1.0E20;
  unsigned short iperm1[3];
  unsigned short iperm2[3];
  unsigned short cont_flag = 1;
  double vec1[3], vec2[3];
  double rmag;
  
    // Copy the input vector and normalize it
  VerdictVector vector1 = *this;
  vector1.normalize();
  
    // Initialize perm flags
  iperm1[0] = 1; iperm1[1] = 2; iperm1[2] = 0;
  iperm2[0] = 2; iperm2[1] = 0; iperm2[2] = 1;
  
    // Get into the array format we can work with
  vector1.get_xyz( vec1 );
  
  while (i<3 && cont_flag )
  {
    if (fabs(vec1[i]) < 1e-6)
    {
      vec2[i] = 1.0;
      vec2[iperm1[i]] = 0.0;
      vec2[iperm2[i]] = 0.0;
      cont_flag = 0;
    }
    
    if (fabs(vec1[i]) < rmin)
    {
      imin = i;
      rmin = fabs(vec1[i]);
    }
    ++i;
  }
  
  if (cont_flag)
  {
    xv[imin] = 1.0;
    xv[iperm1[imin]] = 0.0;
    xv[iperm2[imin]] = 0.0;
    
      // Determine cross product
    vec2[0] = vec1[1] * xv[2] - vec1[2] * xv[1];
    vec2[1] = vec1[2] * xv[0] - vec1[0] * xv[2];
    vec2[2] = vec1[0] * xv[1] - vec1[1] * xv[0];
    
      // Unitize
    rmag = sqrt(vec2[0]*vec2[0] + vec2[1]*vec2[1] + vec2[2]*vec2[2]);
    vec2[0] /= rmag;
    vec2[1] /= rmag;
    vec2[2] /= rmag;
  }
  
    // Copy 1st orthogonal vector into VerdictVector vector2
  vector2.set( vec2 );
  
    // Cross vectors to determine last orthogonal vector
  vector3 = vector1 * vector2;
}

//- Find next point from this point using a direction and distance
void VerdictVector::next_point( const VerdictVector &direction,
                              double distance, VerdictVector& out_point )
{
  VerdictVector my_direction = direction;
  my_direction.normalize();
  
    // Determine next point in space
  out_point.x( xVal + (distance * my_direction.x()) );     
  out_point.y( yVal + (distance * my_direction.y()) );     
  out_point.z( zVal + (distance * my_direction.z()) ); 
  
  return;
}

VerdictVector::VerdictVector(const double xyz[3]) 
  : xVal(xyz[0]), yVal(xyz[1]), zVal(xyz[2])
{}
