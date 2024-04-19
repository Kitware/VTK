/*=========================================================================

  Module:    VerdictVector.cpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * VerdictVector.cpp contains implementation of Vector operations
 *
 * This file is part of VERDICT
 *
 */

#include "VerdictVector.hpp"
#include "verdict.h"

#include <cmath>

namespace VERDICT_NAMESPACE
{

// scale the length of the vector to be the new_length
VerdictVector& VerdictVector::length(const double new_length)
{
  double len = this->length();
  xVal *= new_length / len;
  yVal *= new_length / len;
  zVal *= new_length / len;
  return *this;
}

double VerdictVector::interior_angle(const VerdictVector& otherVector)
{
  double cosAngle = 0., angleRad = 0., len1, len2 = 0.;

  if (((len1 = this->length()) > 0) && ((len2 = otherVector.length()) > 0))
  {
    cosAngle = VerdictVector::Dot(*this, otherVector) / (len1 * len2);
  }
  else
  {
    assert(len1 > 0);
    assert(len2 > 0);
  }

  if ((cosAngle > 1.0) && (cosAngle < 1.0001))
  {
    cosAngle = 1.0;
    angleRad = std::acos(cosAngle);
  }
  else if (cosAngle < -1.0 && cosAngle > -1.0001)
  {
    cosAngle = -1.0;
    angleRad = std::acos(cosAngle);
  }
  else if (cosAngle >= -1.0 && cosAngle <= 1.0)
  {
    angleRad = std::acos(cosAngle);
  }
  else
  {
    assert(cosAngle < 1.0001 && cosAngle > -1.0001);
  }

  return ((angleRad * 180.) / VERDICT_PI);
}

VerdictVector::VerdictVector(const double xyz[3])
  : xVal(xyz[0])
  , yVal(xyz[1])
  , zVal(xyz[2])
{
}

} // namespace verdict
