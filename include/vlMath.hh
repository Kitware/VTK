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
//
// Class for performing common math operations (e.g., dot, cross products)
//
#ifndef __vlMath_hh
#define __vlMath_hh

#include <math.h>

class vlMath
{
public:
  vlMath() {};

  float Pi() {return 3.14159265358979;};

  float DegreesToRadians() {return 0.018977369;};

  float Dot(float x[3], float y[3]) 
    {return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];};

  void Cross(float x[3], float y[3], float z[3])
    {z[0] = x[1]*y[2] - x[2]*y[1]; 
     z[1] = x[2]*y[0] - x[0]*y[2];
     z[2] = x[0]*y[1] - x[1]*y[0];};

  float Norm(float x[3])
    {return sqrt(x[0]*x[0] + x[1]*x[1] + x[2]*x[2]);};

  void Normalize(float x[3])
    {float den; int i;
     if ( (den = this->Norm(x)) != 0.0 )
        for (i=0; i < 3; i++) x[i] /= den;
    }

  float Determinate3x3(float *c1, float *c2, float *c3)
    {return c1[0]*c2[1]*c3[2] + c2[0]*c3[1]*c1[2] + c3[0]*c1[1]*c2[2] -
     c1[0]*c3[1]*c2[2] - c2[0]*c1[1]*c3[2] - c3[0]*c2[1]*c1[2];};

  float Determinate2x2(float *c1, float *c2)
    {return c1[0]*c2[1] - c2[0]*c1[1];};

  float Distance2BetweenPoints(float *x, float *y)
    {return (x[0]-y[0])*(x[0]-y[0]) + (x[1]-y[1])*(x[1]-y[1]) +
            (x[2]-y[2])*(x[2]-y[2]);};

  void RandomSeed(long s);  
  float Random();  
  float Random(float min, float max) {return min+Random()*(max-min);};

protected:
  static long Seed;
};

#endif
