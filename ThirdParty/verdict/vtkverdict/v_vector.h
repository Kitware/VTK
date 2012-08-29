/*=========================================================================

  Module:    v_vector.h

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 *
 * v_vector.h contains simple vector operations
 *
 * This file is part of VERDICT
 *
 */

// .SECTION Thanks
// Prior to its inclusion within VTK, this code was developed by the CUBIT
// project at Sandia National Laboratories. 

#ifndef VERDICT_VECTOR
#define VERDICT_VECTOR


#include "verdict.h"
#include <math.h>
#include <assert.h>


// computes the dot product of 3d vectors
//double dot_product( double vec1[], double vec2[] );

// computes the cross product
//double *cross_product( double vec1[], double vec2[], double answer[] = 0);

// computes the interior angle between 2 vectors in degrees
//double interior_angle ( double vec1[], double vec2[] );

// computes the length of a vector
//double length ( double vec[] );

//double length_squared (double vec[] );


inline double dot_product( double vec1[], double vec2[] )
{

  double answer =  vec1[0] * vec2[0] +
     vec1[1] * vec2[1] +
     vec1[2] * vec2[2];
  return answer;
}
inline void normalize( double vec[] )
{
  double x = sqrt( vec[0]*vec[0] +
             vec[1]*vec[1] +
             vec[2]*vec[2] );

  vec[0] /= x;
  vec[1] /= x;
  vec[2] /= x;

}


inline double * cross_product( double vec1[], double vec2[], double answer[] )
{
  answer[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
  answer[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
  answer[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
  return answer;
}

inline double length ( double vec[] )
{
  return (sqrt ( vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] ));
}

inline double length_squared (double vec[] )
{
  return (vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] );
}


inline double interior_angle( double vec1[], double vec2[] )
{
  double len1, len2, cosAngle, angleRad;

  if (  ((len1 = length(vec1)) > 0 ) && ((len2 = length(vec2)) > 0 ) )
  {
    cosAngle = dot_product(vec1, vec2) / (len1 * len2);
  }
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

#endif


