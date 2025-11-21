/*=========================================================================

  Module:    VerdictVector.hpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

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

#include <cassert>
#include <utility>
#include <math.h>

namespace VERDICT_NAMESPACE
{
class VerdictVector
{
public:
  //- Heading: Constructors and Destructor
  VERDICT_HOST_DEVICE constexpr VerdictVector(); //- Default constructor.

  VERDICT_HOST_DEVICE constexpr VerdictVector(const double x, const double y, const double z);
  //- Constructor: create vector from three components

  VERDICT_HOST_DEVICE constexpr VerdictVector(const double xyz[3]);
  //- Constructor: create vector from tuple

  VERDICT_HOST_DEVICE constexpr VerdictVector(const VerdictVector& tail, const VerdictVector& head);
  VERDICT_HOST_DEVICE constexpr VerdictVector(const double *tail, const double *head, int dimension);
  VERDICT_HOST_DEVICE constexpr VerdictVector(const double *tail, const double *head);
  //- Constructor for a VerdictVector starting at tail and pointing
  //- to head.

  template <typename ARG1, typename ARG2, typename ARG3> constexpr VerdictVector(ARG1, ARG2, ARG3) = delete;
  //- define this template to avoid ambiguity between the (double, double, double) and (double *, double *, int) constructors

  VERDICT_HOST_DEVICE constexpr VerdictVector(const VerdictVector& copy_from); //- Copy Constructor

  //- Heading: Set and Inquire Functions
  VERDICT_HOST_DEVICE constexpr void set(const double xv, const double yv, const double zv);
  //- Change vector components to {x}, {y}, and {z}

  VERDICT_HOST_DEVICE constexpr void set(const double xyz[3]);
  //- Change vector components to xyz[0], xyz[1], xyz[2]

  VERDICT_HOST_DEVICE constexpr void set(const VerdictVector& tail, const VerdictVector& head);
  //- Change vector to go from tail to head.

  VERDICT_HOST_DEVICE constexpr void set(const VerdictVector& to_copy);
  //- Same as operator=(const VerdictVector&)

  VERDICT_HOST_DEVICE constexpr double x() const; //- Return x component of vector

  VERDICT_HOST_DEVICE constexpr double y() const; //- Return y component of vector

  VERDICT_HOST_DEVICE constexpr double z() const; //- Return z component of vector

  VERDICT_HOST_DEVICE constexpr double operator[](int i) const;

  VERDICT_HOST_DEVICE constexpr void get_xyz(double& x, double& y, double& z); //- Get x, y, z components
  VERDICT_HOST_DEVICE constexpr void get_xyz(double xyz[3]);                   //- Get xyz tuple

  VERDICT_HOST_DEVICE constexpr void x(const double xv); //- Set x component of vector

  VERDICT_HOST_DEVICE constexpr void y(const double yv); //- Set y component of vector

  VERDICT_HOST_DEVICE constexpr void z(const double zv); //- Set z component of vector

  VERDICT_HOST_DEVICE double normalize();
  //- Normalize (set magnitude equal to 1) vector - return the magnitude

  VERDICT_HOST_DEVICE VerdictVector& length(const double new_length);
  //- Change length of vector to {new_length}. Can be used to move a
  //- location a specified distance from the origin in the current
  //- orientation.

  VERDICT_HOST_DEVICE double length() const;
  //- Calculate the length of the vector.
  //- Use {length_squared()} if only comparing lengths, not adding.

  VERDICT_HOST_DEVICE constexpr double length_squared() const;
  //- Calculate the squared length of the vector.
  //- Faster than {length()} since it eliminates the square root if
  //- only comparing other lengths.

  VERDICT_HOST_DEVICE double interior_angle(const VerdictVector& otherVector);
  //- Calculate the interior angle: acos((a%b)/(|a||b|))
  //- Returns angle in degrees.

  VERDICT_HOST_DEVICE constexpr void perpendicular_z();
  //- Transform this vector to a perpendicular one, leaving
  //- z-component alone. Rotates clockwise about the z-axis by pi/2.

  //- Heading: Operator Overloads  *****************************
  VERDICT_HOST_DEVICE constexpr VerdictVector& operator+=(const VerdictVector& vec);
  //- Compound Assignment: addition: {this = this + vec}

  VERDICT_HOST_DEVICE constexpr VerdictVector& operator-=(const VerdictVector& vec);
  //- Compound Assignment: subtraction: {this = this - vec}

  VERDICT_HOST_DEVICE constexpr VerdictVector& operator*=(const VerdictVector& vec);
  //- Compound Assignment: cross product: {this = this * vec},
  //- non-commutative

  VERDICT_HOST_DEVICE constexpr VerdictVector& operator*=(const double scalar);
  //- Compound Assignment: multiplication: {this = this * scalar}

  VERDICT_HOST_DEVICE constexpr VerdictVector& operator/=(const double scalar);
  //- Compound Assignment: division: {this = this / scalar}

  VERDICT_HOST_DEVICE constexpr VerdictVector operator-() const;
  //- unary negation.

  VERDICT_HOST_DEVICE friend VerdictVector operator~(const VerdictVector& vec);
  //- normalize. Returns a new vector which is a copy of {vec},
  //- scaled such that {|vec|=1}. Uses overloaded bitwise NOT operator.

  VERDICT_HOST_DEVICE friend constexpr VerdictVector operator+(const VerdictVector& v1, const VerdictVector& v2);
  //- vector addition

  VERDICT_HOST_DEVICE friend constexpr VerdictVector operator-(const VerdictVector& v1, const VerdictVector& v2);
  //- vector subtraction

  VERDICT_HOST_DEVICE friend constexpr VerdictVector operator*(const VerdictVector& v1, const VerdictVector& v2);
  //- vector cross product, non-commutative

  VERDICT_HOST_DEVICE friend constexpr VerdictVector operator*(const VerdictVector& v1, const double sclr);
  //- vector * scalar

  VERDICT_HOST_DEVICE friend constexpr VerdictVector operator*(const double sclr, const VerdictVector& v1);
  //- scalar * vector

  VERDICT_HOST_DEVICE friend constexpr double operator%(const VerdictVector& v1, const VerdictVector& v2);
  //- dot product

  VERDICT_HOST_DEVICE static constexpr double Dot(const VerdictVector& v1, const VerdictVector& v2);
  //- dot product

  VERDICT_HOST_DEVICE friend constexpr VerdictVector operator/(const VerdictVector& v1, const double sclr);
  //- vector / scalar

  VERDICT_HOST_DEVICE friend constexpr int operator==(const VerdictVector& v1, const VerdictVector& v2);
  //- Equality operator

  VERDICT_HOST_DEVICE friend constexpr int operator!=(const VerdictVector& v1, const VerdictVector& v2);
  //- Inequality operator

  VERDICT_HOST_DEVICE constexpr VerdictVector& operator=(const VerdictVector& from);

private:
  double Val[3];
};

constexpr double VerdictVector::operator[](int i) const
{
  return Val[i];
}

constexpr double VerdictVector::x() const
{
  return Val[0];
}
constexpr double VerdictVector::y() const
{
  return Val[1];
}
constexpr double VerdictVector::z() const
{
  return Val[2];
}
constexpr void VerdictVector::get_xyz(double xyz[3])
{
  xyz[0] = Val[0];
  xyz[1] = Val[1];
  xyz[2] = Val[2];
}
constexpr void VerdictVector::get_xyz(double& xv, double& yv, double& zv)
{
  xv = Val[0];
  yv = Val[1];
  zv = Val[2];
}
constexpr void VerdictVector::x(const double xv)
{
  Val[0] = xv;
}
constexpr void VerdictVector::y(const double yv)
{
  Val[1] = yv;
}
constexpr void VerdictVector::z(const double zv)
{
  Val[2] = zv;
}
constexpr VerdictVector& VerdictVector::operator+=(const VerdictVector& vector)
{
  Val[0] += vector.x();
  Val[1] += vector.y();
  Val[2] += vector.z();
  return *this;
}

constexpr VerdictVector& VerdictVector::operator-=(const VerdictVector& vector)
{
  Val[0] -= vector.x();
  Val[1] -= vector.y();
  Val[2] -= vector.z();
  return *this;
}

constexpr VerdictVector& VerdictVector::operator*=(const VerdictVector& vector)
{
  const double xcross = Val[1] * vector.z() - Val[2] * vector.y();
  const double ycross = Val[2] * vector.x() - Val[0] * vector.z();
  const double zcross = Val[0] * vector.y() - Val[1] * vector.x();
  Val[0] = xcross;
  Val[1] = ycross;
  Val[2] = zcross;
  return *this;
}

constexpr VerdictVector::VerdictVector(const VerdictVector& copy_from)
  : Val{ copy_from.Val[0], copy_from.Val[1], copy_from.Val[2] }
{
}

constexpr VerdictVector::VerdictVector()
  : Val{ 0,0,0 }
{
}

constexpr VerdictVector::VerdictVector(const double *tail, const double *head, int dimension)
  : Val{head[0] - tail[0], head[1] - tail[1], dimension == 2 ? 0.0 : head[2] - tail[2] }
{
}

constexpr VerdictVector::VerdictVector(const double *tail, const double *head)
  : Val{head[0] - tail[0], head[1] - tail[1], head[2] - tail[2]}
{
}

constexpr VerdictVector::VerdictVector(const VerdictVector& tail, const VerdictVector& head)
  : Val{ head.Val[0] - tail.Val[0], head.Val[1] - tail.Val[1], head.Val[2] - tail.Val[2] }
{
}

constexpr VerdictVector::VerdictVector(const double xIn, const double yIn, const double zIn)
  : Val{ xIn, yIn, zIn }
{
}

constexpr VerdictVector::VerdictVector(const double xyz[3])
  : Val{xyz[0], xyz[1], xyz[2]}
{
}

// This sets the vector to be perpendicular to it's current direction.
// NOTE:
//      This is a 2D function.  It only works in the XY Plane.
constexpr void VerdictVector::perpendicular_z()
{
  double temp = x();
  x(y());
  y(-temp);
}

constexpr void VerdictVector::set(const double xv, const double yv, const double zv)
{
  Val[0] = xv;
  Val[1] = yv;
  Val[2] = zv;
}

constexpr void VerdictVector::set(const double xyz[3])
{
  Val[0] = xyz[0];
  Val[1] = xyz[1];
  Val[2] = xyz[2];
}

constexpr void VerdictVector::set(const VerdictVector& tail, const VerdictVector& head)
{
  Val[0] = head.Val[0] - tail.Val[0];
  Val[1] = head.Val[1] - tail.Val[1];
  Val[2] = head.Val[2] - tail.Val[2];
}

constexpr VerdictVector& VerdictVector::operator=(const VerdictVector& from)
{
  Val[0] = from.Val[0];
  Val[1] = from.Val[1];
  Val[2] = from.Val[2];
  return *this;
}

constexpr void VerdictVector::set(const VerdictVector& to_copy)
{
  *this = to_copy;
}

// Scale all values by scalar.
constexpr VerdictVector& VerdictVector::operator*=(const double scalar)
{
  Val[0] *= scalar;
  Val[1] *= scalar;
  Val[2] *= scalar;
  return *this;
}

// Scales all values by 1/scalar
constexpr VerdictVector& VerdictVector::operator/=(const double scalar)
{
#ifndef __HIP_DEVICE_COMPILE__
  assert(scalar != 0);
#endif
  Val[0] /= scalar;
  Val[1] /= scalar;
  Val[2] /= scalar;
  return *this;
}

// Returns the normalized 'this'.
VERDICT_HOST_DEVICE inline VerdictVector operator~(const VerdictVector& vec)
{
  VerdictVector temp = vec;
  temp.normalize();
  return temp;
}

// Unary minus.  Negates all values in vector.
constexpr VerdictVector VerdictVector::operator-() const
{
  return VerdictVector(-Val[0], -Val[1], -Val[2]);
}

constexpr VerdictVector operator+(const VerdictVector& vector1, const VerdictVector& vector2)
{
  double xv = vector1.x() + vector2.x();
  double yv = vector1.y() + vector2.y();
  double zv = vector1.z() + vector2.z();
  return VerdictVector(xv, yv, zv);
  //  return VerdictVector(vector1) += vector2;
}

constexpr VerdictVector operator-(const VerdictVector& vector1, const VerdictVector& vector2)
{
  double xv = vector1.x() - vector2.x();
  double yv = vector1.y() - vector2.y();
  double zv = vector1.z() - vector2.z();
  return VerdictVector(xv, yv, zv);
  //  return VerdictVector(vector1) -= vector2;
}

// Cross products.
// vector1 cross vector2
constexpr VerdictVector operator*(const VerdictVector& vector1, const VerdictVector& vector2)
{
  return VerdictVector(vector1) *= vector2;
}

// Returns a scaled vector.
constexpr VerdictVector operator*(const VerdictVector& vector1, const double scalar)
{
  return VerdictVector(vector1) *= scalar;
}

// Returns a scaled vector
constexpr VerdictVector operator*(const double scalar, const VerdictVector& vector1)
{
  return VerdictVector(vector1) *= scalar;
}

// Returns a vector scaled by 1/scalar
constexpr VerdictVector operator/(const VerdictVector& vector1, const double scalar)
{
  return VerdictVector(vector1) /= scalar;
}

constexpr int operator==(const VerdictVector& v1, const VerdictVector& v2)
{
  return (v1.Val[0] == v2.Val[0] && v1.Val[1] == v2.Val[1] && v1.Val[2] == v2.Val[2]);
}

constexpr int operator!=(const VerdictVector& v1, const VerdictVector& v2)
{
  return (v1.Val[0] != v2.Val[0] || v1.Val[1] != v2.Val[1] || v1.Val[2] != v2.Val[2]);
}

constexpr double VerdictVector::length_squared() const
{
  return (Val[0] * Val[0] + Val[1] * Val[1] + Val[2] * Val[2]);
}

VERDICT_HOST_DEVICE inline double VerdictVector::length() const
{
  return (sqrt(Val[0] * Val[0] + Val[1] * Val[1] + Val[2] * Val[2]));
}

// Dot Product.
constexpr double operator%(const VerdictVector& vector1, const VerdictVector& vector2)
{
  return VerdictVector::Dot(vector1, vector2);
}
constexpr double VerdictVector::Dot(const VerdictVector& vector1, const VerdictVector& vector2)
{
  return (vector1.Val[0] * vector2.Val[0] + vector1.Val[1] * vector2.Val[1] + vector1.Val[2] * vector2.Val[2]);
}

struct ElemScale
{
  VerdictVector center;
  double scale;
};

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE constexpr ElemScale elem_scaling(int num_coords, const CoordsContainerType coordinates, int dimension = 3)
{
  VerdictVector min(VERDICT_DBL_MAX, VERDICT_DBL_MAX, dimension == 3 ? VERDICT_DBL_MAX : 0);
  VerdictVector max(-VERDICT_DBL_MAX, -VERDICT_DBL_MAX, dimension == 3 ? -VERDICT_DBL_MAX : 0);
  VerdictVector center(0.0, 0.0, 0.0);

  for (int i = 0; i < num_coords; i++)
  {
    if (coordinates[i][0] < min.x())
      min.x(coordinates[i][0]);
    if (coordinates[i][1] < min.y())
      min.y(coordinates[i][1]);
    if (coordinates[i][0] > max.x())
      max.x(coordinates[i][0]);
    if (coordinates[i][1] > max.y())
      max.y(coordinates[i][1]);
    if (dimension == 3)
    {
      if (coordinates[i][2] < min.z())
        min.z(coordinates[i][2]);
      if (coordinates[i][2] > max.z())
        max.z(coordinates[i][2]);
    }
    center += VerdictVector(coordinates[i][0], coordinates[i][1], dimension == 3 ? coordinates[i][2] : 0.0);
  }
  center /= (double)num_coords;

  double len = (max - min).length();
  if (len < VERDICT_DBL_MIN)
  {
    center = VerdictVector(0.0,0.0,0.0);
    len = 1.0;
  }
  return ElemScale{center, len};
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE constexpr double apply_elem_scaling_on_points(int num_coords, const CoordsContainerType coordinates, int num_vec, VerdictVector* v, int dimension = 3)
{
  auto char_size = elem_scaling(num_coords, coordinates, dimension);
  for (int i = 0; i < num_vec; i++)
  {
    v[i] -= char_size.center;
    v[i] /= char_size.scale;
  }
  return char_size.scale;
}

template <typename CoordsContainerType>
VERDICT_HOST_DEVICE constexpr double apply_elem_scaling_on_edges(int num_coords, const CoordsContainerType coordinates, int num_vec, VerdictVector* v, int dimension = 3)
{
  auto char_size = elem_scaling(num_coords, coordinates, dimension);
  for (int i = 0; i < num_vec; i++)
  {
    v[i] /= char_size.scale;
  }
  return char_size.scale;
}


} // namespace verdict

#endif
