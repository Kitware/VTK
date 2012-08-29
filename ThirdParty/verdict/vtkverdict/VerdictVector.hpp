/*=========================================================================

  Module:    VerdictVector.hpp

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 *
 * VerdictVector.hpp contains declarations of vector operations
 *
 * This file is part of VERDICT
 *
 */

// .SECTION Thanks
// Prior to its inclusion within VTK, this code was developed by the CUBIT
// project at Sandia National Laboratories. 

#ifndef VERDICTVECTOR_HPP
#define VERDICTVECTOR_HPP

#include "verdict.h"
#include <assert.h>
#include <math.h>

class VerdictVector;
typedef void ( VerdictVector::*transform_function )( double gamma,
                                                   double gamma2);
// a pointer to some function that transforms the point,
// taking a double parameter.  e.g. blow_out, rotate, and scale_angle

class VerdictVector
{
public:
  
    //- Heading: Constructors and Destructor
  VerdictVector();  //- Default constructor.
  
  VerdictVector(const double x, const double y, const double z);
    //- Constructor: create vector from three components
  
  VerdictVector( const double xyz[3] );
    //- Constructor: create vector from tuple

  VerdictVector (const VerdictVector& tail, const VerdictVector& head);
    //- Constructor for a VerdictVector starting at tail and pointing
    //- to head.
  
  VerdictVector(const VerdictVector& copy_from);  //- Copy Constructor
  
    //- Heading: Set and Inquire Functions
  void set( const double xv, const double yv, const double zv );
    //- Change vector components to {x}, {y}, and {z}
  
  void set( const double xyz[3] );
    //- Change vector components to xyz[0], xyz[1], xyz[2]

  void set(const VerdictVector& tail, const VerdictVector& head);
    //- Change vector to go from tail to head.
  
  void set(const VerdictVector& to_copy);
    //- Same as operator=(const VerdictVector&)
  
  double x() const; //- Return x component of vector
  
  double y() const; //- Return y component of vector
  
  double z() const; //- Return z component of vector
  
  void get_xyz( double &x, double &y, double &z ); //- Get x, y, z components
  void get_xyz( double xyz[3] ); //- Get xyz tuple
  
  double &r(); //- Return r component of vector, if (r,theta) format
  
  double &theta();  //- Return theta component of vector, if (r,theta) format
  
  void x( const double xv ); //- Set x component of vector
  
  void y( const double yv ); //- Set y component of vector
  
  void z( const double zv ); //- Set z component of vector
  
  void r( const double xv ); //- Set r component of vector, if (r,theta) format
  
  void theta( const double yv ); //- Set theta component of vector, if (r,theta) format
  
  void xy_to_rtheta();
    //- convert from cartesian to polar coordinates, just 2d for now
    //- theta is in [0,2 PI)
  
  void rtheta_to_xy();
    //- convert from  polar to cartesian coordinates, just 2d for now
  
  void scale_angle(double gamma, double );
    //- tranform_function.
    //- transform  (x,y) to (r,theta) to (r,gamma*theta) to (x',y')
    //- plus some additional scaling so long chords won't cross short ones
  
  void blow_out(double gamma, double gamma2 = 0.0);
    //- transform_function
    //- blow points radially away from the origin, 
  
  void rotate(double angle, double );
    //- transform function.
    //- transform  (x,y) to (r,theta) to (r,theta+angle) to (x',y')
  
  void reflect_about_xaxis(double dummy, double );
    //- dummy argument to make it a transform function
  
  double normalize();
    //- Normalize (set magnitude equal to 1) vector - return the magnitude

  VerdictVector& length(const double new_length);
    //- Change length of vector to {new_length}. Can be used to move a
    //- location a specified distance from the origin in the current
    //- orientation.
  
  double length() const;
    //- Calculate the length of the vector.
    //- Use {length_squared()} if only comparing lengths, not adding.
  
  double distance_between(const VerdictVector& test_vector);
    //- Calculate the distance from the head of one vector
    //  to the head of the test_vector.
  
  double length_squared() const;
    //- Calculate the squared length of the vector.
    //- Faster than {length()} since it eliminates the square root if
    //- only comparing other lengths.
  
  double interior_angle(const VerdictVector &otherVector);
    //- Calculate the interior angle: acos((a%b)/(|a||b|))
    //- Returns angle in degrees.
  
  double vector_angle_quick(const VerdictVector& vec1, const VerdictVector& vec2);
    //- Calculate the interior angle between the projections of
    //- {vec1} and {vec2} onto the plane defined by the {this} vector.
    //- The angle returned is the right-handed angle around the {this}
    //- vector from {vec1} to {vec2}. Angle is in radians.
  
  double vector_angle( const VerdictVector &vector1,
                       const VerdictVector &vector2) const;
    //- Compute the angle between the projections of {vector1} and {vector2}
    //- onto the plane defined by *this. The angle is the
    //- right-hand angle, in radians, about *this from {vector1} to {vector2}.
  
  void perpendicular_z();
    //- Transform this vector to a perpendicular one, leaving
    //- z-component alone. Rotates clockwise about the z-axis by pi/2.

  void print_me();
    //- Prints out the coordinates of this vector.
  
  void orthogonal_vectors( VerdictVector &vector2, VerdictVector &vector3 );
    //- Finds 2 (arbitrary) vectors that are orthogonal to this one
  
  void next_point( const VerdictVector &direction, double distance, 
                   VerdictVector& out_point );
    //- Finds the next point in space based on *this* point (starting point), 
    //- a direction and the distance to extend in the direction. The
    //- direction vector need not be a unit vector.  The out_point can be
    //- "*this" (i.e., modify point in place).
  
  bool within_tolerance( const VerdictVector &vectorPtr2,
                                double tolerance) const;
    //- Compare two vectors to see if they are spatially equal.  They
    //- compare if x, y, and z are all within tolerance.
  
    //- Heading: Operator Overloads  *****************************
  VerdictVector&  operator+=(const VerdictVector &vec);
    //- Compound Assignment: addition: {this = this + vec}
  
  VerdictVector& operator-=(const VerdictVector &vec);
    //- Compound Assignment: subtraction: {this = this - vec}
  
  VerdictVector& operator*=(const VerdictVector &vec);
    //- Compound Assignment: cross product: {this = this * vec},
    //- non-commutative
  
  VerdictVector& operator*=(const double scalar);
    //- Compound Assignment: multiplication: {this = this * scalar}
  
  VerdictVector& operator/=(const double scalar);
    //- Compound Assignment: division: {this = this / scalar}
  
  VerdictVector operator-() const;
    //- unary negation.
  
  friend VerdictVector operator~(const VerdictVector &vec);
    //- normalize. Returns a new vector which is a copy of {vec},
    //- scaled such that {|vec|=1}. Uses overloaded bitwise NOT operator.
  
  friend VerdictVector operator+(const VerdictVector &v1, 
                               const VerdictVector &v2);
    //- vector addition
  
  friend VerdictVector operator-(const VerdictVector &v1, 
                               const VerdictVector &v2);
    //- vector subtraction
  
  friend VerdictVector operator*(const VerdictVector &v1, 
                               const VerdictVector &v2);
    //- vector cross product, non-commutative
  
  friend VerdictVector operator*(const VerdictVector &v1, const double sclr);
    //- vector * scalar
  
  friend VerdictVector operator*(const double sclr, const VerdictVector &v1);
    //- scalar * vector
  
  friend double operator%(const VerdictVector &v1, const VerdictVector &v2);
    //- dot product
  
  friend VerdictVector operator/(const VerdictVector &v1, const double sclr);
    //- vector / scalar
  
  friend int operator==(const VerdictVector &v1, const VerdictVector &v2);
    //- Equality operator
  
  friend int operator!=(const VerdictVector &v1, const VerdictVector &v2);
    //- Inequality operator
  
  friend VerdictVector v_interpolate(const double param, const VerdictVector &v1,
                                 const VerdictVector &v2);
    //- Interpolate between two vectors. Returns (1-param)*v1 + param*v2


  VerdictVector &operator=(const VerdictVector& from);
  
private:
  
  double xVal;  //- x component of vector.
  double yVal;  //- y component of vector.
  double zVal;  //- z component of vector.
};

VerdictVector v_vector_rotate(
  const double angle, const VerdictVector &normalAxis, const VerdictVector &referenceAxis );
  //- A new coordinate system is created with the xy plane corresponding
  //- to the plane normal to {normalAxis}, and the x axis corresponding to
  //- the projection of {referenceAxis} onto the normal plane.  The normal
  //- plane is the tangent plane at the root point.  A unit vector is
  //- constructed along the local x axis and then rotated by the given
  //- ccw angle to form the new point.  The new point, then is a unit
  //- distance from the global origin in the tangent plane.
  //- {angle} is in radians.

inline double VerdictVector::x() const
{ return xVal; }
inline double VerdictVector::y() const
{ return yVal; }
inline double VerdictVector::z() const
{ return zVal; }
inline void VerdictVector::get_xyz(double xyz[3])
{
  xyz[0] = xVal;
  xyz[1] = yVal;
  xyz[2] = zVal;
}
inline void VerdictVector::get_xyz( double& xv, double& yv, double& zv )
{
  xv = xVal; 
  yv = yVal; 
  zv = zVal;
}
inline double &VerdictVector::r()
{ return xVal; }
inline double &VerdictVector::theta()
{ return yVal; }
inline void VerdictVector::x( const double xv )
{ xVal = xv; }
inline void VerdictVector::y( const double yv )
{ yVal = yv; }
inline void VerdictVector::z( const double zv )
{ zVal = zv; }
inline void VerdictVector::r( const double xv )
{ xVal = xv; }
inline void VerdictVector::theta( const double yv )
{ yVal = yv; }
inline VerdictVector& VerdictVector::operator+=(const VerdictVector &vector)
{
  xVal += vector.x();
  yVal += vector.y();
  zVal += vector.z();
  return *this;
}

inline VerdictVector& VerdictVector::operator-=(const VerdictVector &vector)
{
  xVal -= vector.x();
  yVal -= vector.y();
  zVal -= vector.z();
  return *this;
}

inline VerdictVector& VerdictVector::operator*=(const VerdictVector &vector)
{
  double xcross, ycross, zcross;
  xcross = yVal * vector.z() - zVal * vector.y();
  ycross = zVal * vector.x() - xVal * vector.z();
  zcross = xVal * vector.y() - yVal * vector.x();
  xVal = xcross;
  yVal = ycross;
  zVal = zcross;
  return *this;
}

inline VerdictVector::VerdictVector(const VerdictVector& copy_from)
    : xVal(copy_from.xVal), yVal(copy_from.yVal), zVal(copy_from.zVal)
{}

inline VerdictVector::VerdictVector()
    : xVal(0), yVal(0), zVal(0)
{}

inline VerdictVector::VerdictVector (const VerdictVector& tail,
                                 const VerdictVector& head)
    : xVal(head.xVal - tail.xVal),
      yVal(head.yVal - tail.yVal),
      zVal(head.zVal - tail.zVal)
{}

inline VerdictVector::VerdictVector(const double xIn,
                                const double yIn,
                                const double zIn)
    : xVal(xIn), yVal(yIn), zVal(zIn)
{}

// This sets the vector to be perpendicular to it's current direction.
// NOTE:
//      This is a 2D function.  It only works in the XY Plane.
inline void VerdictVector::perpendicular_z()
{
  double temp = x();
  x( y() );
  y( -temp );
}

inline void VerdictVector::set( const double xv,
                                const double yv,
                                const double zv )
{
  xVal = xv;
  yVal = yv;
  zVal = zv;
}

inline void VerdictVector::set(const double xyz[3])
{
  xVal = xyz[0];
  yVal = xyz[1];
  zVal = xyz[2];
}

inline void VerdictVector::set(const VerdictVector& tail,
                             const VerdictVector& head)
{
  xVal = head.xVal - tail.xVal;
  yVal = head.yVal - tail.yVal;
  zVal = head.zVal - tail.zVal;
}

inline VerdictVector& VerdictVector::operator=(const VerdictVector &from)  
{
  xVal = from.xVal; 
  yVal = from.yVal; 
  zVal = from.zVal; 
  return *this;
}

inline void VerdictVector::set(const VerdictVector& to_copy)
{
  *this = to_copy;
}

// Scale all values by scalar.
inline VerdictVector& VerdictVector::operator*=(const double scalar)
{
  xVal *= scalar;
  yVal *= scalar;
  zVal *= scalar;
  return *this;
}

// Scales all values by 1/scalar
inline VerdictVector& VerdictVector::operator/=(const double scalar)
{
  assert (scalar != 0);
  xVal /= scalar;
  yVal /= scalar;
  zVal /= scalar;
  return *this;
}

// Returns the normalized 'this'.
inline VerdictVector operator~(const VerdictVector &vec)
{
  double mag = sqrt(vec.xVal*vec.xVal +
                    vec.yVal*vec.yVal +
                    vec.zVal*vec.zVal);
  
  VerdictVector temp = vec;
  if (mag != 0.0)
  {
    temp /= mag;
  }
  return temp;
}

// Unary minus.  Negates all values in vector.
inline VerdictVector VerdictVector::operator-() const
{
  return VerdictVector(-xVal, -yVal, -zVal);
}

inline VerdictVector operator+(const VerdictVector &vector1,
                      const VerdictVector &vector2)
{
  double xv = vector1.x() + vector2.x();
  double yv = vector1.y() + vector2.y();
  double zv = vector1.z() + vector2.z();
  return VerdictVector(xv,yv,zv);
//  return VerdictVector(vector1) += vector2;
}

inline VerdictVector operator-(const VerdictVector &vector1,
                      const VerdictVector &vector2)
{
  double xv = vector1.x() - vector2.x();
  double yv = vector1.y() - vector2.y();
  double zv = vector1.z() - vector2.z();
  return VerdictVector(xv,yv,zv);
//  return VerdictVector(vector1) -= vector2;
}

// Cross products.
// vector1 cross vector2
inline VerdictVector operator*(const VerdictVector &vector1,
                      const VerdictVector &vector2)
{
  return VerdictVector(vector1) *= vector2;
}

// Returns a scaled vector.
inline VerdictVector operator*(const VerdictVector &vector1,
                      const double scalar)
{
  return VerdictVector(vector1) *= scalar;
}

// Returns a scaled vector
inline VerdictVector operator*(const double scalar,
                             const VerdictVector &vector1)
{
  return VerdictVector(vector1) *= scalar;
}

// Returns a vector scaled by 1/scalar
inline VerdictVector operator/(const VerdictVector &vector1,
                             const double scalar)
{
  return VerdictVector(vector1) /= scalar;
}

inline int operator==(const VerdictVector &v1, const VerdictVector &v2)
{
  return (v1.xVal == v2.xVal && v1.yVal == v2.yVal && v1.zVal == v2.zVal);
}

inline int operator!=(const VerdictVector &v1, const VerdictVector &v2)
{
  return (v1.xVal != v2.xVal || v1.yVal != v2.yVal || v1.zVal != v2.zVal);
}

inline double VerdictVector::length_squared() const
{
  return( xVal*xVal + yVal*yVal + zVal*zVal );
}

inline double VerdictVector::length() const
{
  return( sqrt(xVal*xVal + yVal*yVal + zVal*zVal) );
}

inline double VerdictVector::normalize()
{
  double mag = length();
  if (mag != 0)
  {
    xVal = xVal / mag;
    yVal = yVal / mag;
    zVal = zVal / mag;
  }
  return mag;
}


// Dot Product.
inline double operator%(const VerdictVector &vector1,
                        const VerdictVector &vector2)
{
  return( vector1.x() * vector2.x() +
          vector1.y() * vector2.y() +
          vector1.z() * vector2.z() );
}

#endif
