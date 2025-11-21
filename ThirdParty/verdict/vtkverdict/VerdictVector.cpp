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

#include <math.h>

namespace VERDICT_NAMESPACE
{

// scale the length of the vector to be the new_length
VERDICT_HOST_DEVICE VerdictVector& VerdictVector::length(const double new_length)
{
  double len = this->length();
  Val[0] *= new_length / len;
  Val[1] *= new_length / len;
  Val[2] *= new_length / len;
  return *this;
}

VERDICT_HOST_DEVICE double VerdictVector::interior_angle(const VerdictVector& otherVector)
{
  double cosAngle = 0., angleRad = 0., len1, len2 = 0.;

  if (((len1 = this->length()) > 0) && ((len2 = otherVector.length()) > 0))
  {
    cosAngle = VerdictVector::Dot(*this, otherVector) / (len1 * len2);
  }
  else
  {
#ifndef __HIP_DEVICE_COMPILE__
    assert(len1 > 0);
    assert(len2 > 0);
#endif
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
  {
    angleRad = acos(cosAngle);
  }
  else
  {
#ifndef __HIP_DEVICE_COMPILE__
    assert(cosAngle < 1.0001 && cosAngle > -1.0001);
#endif
  }

  return ((angleRad * 180.) / VERDICT_PI);
}

VERDICT_HOST_DEVICE double VerdictVector::normalize()
{
  double mag = length();
  if (mag > VERDICT_DBL_MIN*2.0)
  {
    Val[0] = Val[0] / mag;
    Val[1] = Val[1] / mag;
    Val[2] = Val[2] / mag;
    return mag;
  }
  Val[0] = 0.0;
  Val[1] = 0.0;
  Val[2] = 0.0;
  return 0;
}


} // namespace verdict
