/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerlinNoise.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPerlinNoise.h"
#include "vtkObjectFactory.h"
#include <math.h>

#ifdef vtkCxxRevisionMacro
vtkCxxRevisionMacro(vtkPerlinNoise, "1.1");
#endif

// These functions are from Greg Ward's recursive implementation in 
// Graphics Gems II.  I've kept the names the same for instructional
// purposes, and only changed things where optimizations could be made.

static float hermite(float p0, float p1, 
                      float r0, float r1, float t) 
{
  float tt = t*t;

  return (p0*((2.0*t - 3.0)*tt + 1.0) + 
          p1*(-2.0*t + 3.0)*tt +
          r0*((t-2.0)*t+1.0)*t +
          r1*(t-1.0)*tt);
}

// assumes 32 bit ints, but so it seems does VTK
static float frand(int s)
{
  s = (s<<13) ^ s;
  s = (s*(s*s*15731 + 789221)+1376312589)&VTK_INT_MAX;

  return 1.0 - float(s)/(VTK_INT_MAX/2 + 1);
}

static void rand3abcd(int x, int y, int z, float outv[4]) 
{
  outv[0] = frand(67*x + 59*y + 71*z);
  outv[1] = frand(73*x + 79*y + 83*z);
  outv[2] = frand(89*x + 97*y + 101*z);
  outv[3] = frand(103*x + 107*y + 109*z);
}

static void interpolate(float f[4], int i, int n, 
                        int xlim[3][2], float xarg[2])
{
  float f0[4], f1[4];
  
  if (n == 0) 
    {
    rand3abcd(xlim[0][i&1], xlim[1][(i>>1) & 1], xlim[2][i>>2], f);
    return;
    }
  n--;
  interpolate(f0, i, n, xlim, xarg);
  interpolate(f1, i | (1<<n), n, xlim, xarg);
  
  f[0] = (1.0 - xarg[n])*f0[0] + xarg[n]*f1[0];
  f[1] = (1.0 - xarg[n])*f0[1] + xarg[n]*f1[1];
  f[2] = (1.0 - xarg[n])*f0[2] + xarg[n]*f1[2];
  f[3] = hermite(f0[3], f1[3], f0[n], f1[n], xarg[n]);
}


static void perlinNoise(float x[3], float noise[4])
{
  float xarg[3];
  int xlim[3][2];

  xlim[0][0] = int(floor(x[0]));
  xlim[1][0] = int(floor(x[1]));
  xlim[2][0] = int(floor(x[2]));
  
  xlim[0][1] = xlim[0][0] + 1;
  xlim[1][1] = xlim[1][0] + 1;
  xlim[2][1] = xlim[2][0] + 1;

  xarg[0] = x[0] - xlim[0][0];
  xarg[1] = x[1] - xlim[1][0];
  xarg[2] = x[2] - xlim[2][0];

  interpolate(noise, 0, 3, xlim, xarg);
}


//----------------------------------------------------------------------------
vtkPerlinNoise* vtkPerlinNoise::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPerlinNoise");
  if (ret)
    {
    return (vtkPerlinNoise*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPerlinNoise;
}

vtkPerlinNoise::vtkPerlinNoise()
{
  this->Frequency[0] = 1.0;
  this->Frequency[1] = 1.0;
  this->Frequency[2] = 1.0;

  this->Phase[0] = 0.0;
  this->Phase[1] = 0.0;
  this->Phase[2] = 0.0;

  this->Amplitude = 1.0;
}

float vtkPerlinNoise::EvaluateFunction(float x[3])
{
  float xd[3];
  float noise[4];

  xd[0] = x[0]*this->Frequency[0] - this->Phase[0]*2.0;
  xd[1] = x[1]*this->Frequency[1] - this->Phase[1]*2.0;
  xd[2] = x[2]*this->Frequency[2] - this->Phase[2]*2.0;
  perlinNoise(xd, noise);
  return noise[3]*this->Amplitude;
}

// Evaluate PerlinNoise gradient.
void vtkPerlinNoise::EvaluateGradient(float x[3], float n[3])
{
  // contrary to the paper, the vector computed as a byproduct of
  // the Perlin Noise computation isn't a gradient;  it's a tangent.
  // Doing this right will take some work.
  n[0] = 0.0;
  n[1] = 0.0;
  n[2] = 0.0;
}

void vtkPerlinNoise::PrintSelf(ostream& os, vtkIndent indent)
{
#if VTK_MAJOR_VERSION < 4
  vtkImplicitFunction::PrintSelf(os,indent);
#else
  this->Superclass::PrintSelf(os,indent);
#endif

  os << indent << "Amplitude: " << this->Amplitude << "\n";
  os << indent << "Frequency: (" 
     << this->Frequency[0] << ", "
     << this->Frequency[1] << ", "
     << this->Frequency[2] << ")\n";

  os << indent << "Phase: (" 
     << this->Phase[0] << ", "
     << this->Phase[1] << ", "
     << this->Phase[2] << ")\n";
}
