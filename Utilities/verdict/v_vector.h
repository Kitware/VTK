
/*
 *
 * v_vector.h contains simple vector operations
 *
 * Copyright (C) 2003 Sandia National Laboratories <cubit@sandia.gov>
 *
 * This file is part of VERDICT
 *
 * This copy of VERDICT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * VERDICT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



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


