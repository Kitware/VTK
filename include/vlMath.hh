/*=========================================================================

  Program:   Visualization Library
  Module:    vlMath.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlMath - performs common math operations
// .SECTION Description
// vlMath is provides methods to perform common math operations. These 
// include providing constants such as Pi, conversion from degrees to 
// radians, vector operations such as dot and cross products and vector 
// norm, matrix determinates for 2x2, 3x3, and 4x4 matrices, and random 
// number generation.

#ifndef __vlMath_hh
#define __vlMath_hh

#include <math.h>

class vlMath
{
public:
  vlMath() {};
  ~vlMath() {};

  float Pi() {return 3.14159265358979;};
  float DegreesToRadians() {return 0.017453292;};

  float Dot(float x[3], float y[3]);
  void Cross(float x[3], float y[3], float z[3]);
  float Norm(float x[3]);
  void Normalize(float x[3]);

  float Determinate2x2(float *c1, float *c2);
  double Determinate2x2(double a, double b, double c, double d);
  float Determinate3x3(float *c1, float *c2, float *c3);
  double Determinate3x3(double a1, double a2, double a3, 
                        double b1, double b2, double b3, 
                        double c1, double c2, double c3);
  float Distance2BetweenPoints(float *x, float *y);

  void RandomSeed(long s);  
  float Random();  
  float Random(float min, float max);

protected:
  static long Seed;
};

// Description:
// Dot product of two 3-vectors.
inline float vlMath::Dot(float x[3], float y[3]) 
{
  return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];
}

// Description:
// Cross product of two 3-vectors. Result vector in z[3].
inline void vlMath::Cross(float x[3], float y[3], float z[3])
{
  z[0] = x[1]*y[2] - x[2]*y[1]; 
  z[1] = x[2]*y[0] - x[0]*y[2];
  z[2] = x[0]*y[1] - x[1]*y[0];
}

// Description:
// Compute the norm of 3-vector.
inline float vlMath::Norm(float x[3])
{
  return sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);
}

// Description:
// Normalize (in place) a 3-vector.
inline void vlMath::Normalize(float x[3])
{
  float den; 
  if ( (den = this->Norm(x)) != 0.0 ) for (int i=0; i < 3; i++) x[i] /= den;
}

// Description:
// Compute determinate of 2x2 matrix. Two columns of matrix are input.
inline float vlMath::Determinate2x2(float *c1, float *c2)
{
  return c1[0]*c2[1] - c2[0]*c1[1];
}

// Description:
// Calculate the determinent of a 2x2 matrix: | a b |
//                                            | c d |
inline double vlMath::Determinate2x2(double a, double b, double c, double d)
{
  return (a * d - b * c);
}

// Description:
// Compute determinate of 3x3 matrix. Three columns of matrix are input.
inline float vlMath::Determinate3x3(float *c1, float *c2, float *c3)
{
  return c1[0]*c2[1]*c3[2] + c2[0]*c3[1]*c1[2] + c3[0]*c1[1]*c2[2] -
         c1[0]*c3[1]*c2[2] - c2[0]*c1[1]*c3[2] - c3[0]*c2[1]*c1[2];
}

// Description:
// Calculate the determinent of a 3x3 matrix in the form:
//     | a1,  b1,  c1 |
//     | a2,  b2,  c2 |
//     | a3,  b3,  c3 |
inline double vlMath::Determinate3x3(double a1, double a2, double a3, 
                                     double b1, double b2, double b3, 
                                     double c1, double c2, double c3)
{
    return   a1 * this->Determinate2x2( b2, b3, c2, c3 )
           - b1 * this->Determinate2x2( a2, a3, c2, c3 )
           + c1 * this->Determinate2x2( a2, a3, b2, b3 );
}

// Description:
// Compute distance squared between two points.
inline float vlMath::Distance2BetweenPoints(float *x, float *y)
{
  return (x[0]-y[0])*(x[0]-y[0]) + (x[1]-y[1])*(x[1]-y[1]) +
         (x[2]-y[2])*(x[2]-y[2]);
}

// Description:
// Generate random number between (min,max)
inline float vlMath::Random(float min, float max)
{
  return min + this->Random()*(max-min);
}

#endif
