/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerlinNoise.h
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
// .NAME vtkPerlinNoise - an implicit function that implements Perlin noise
// .SECTION Description
// vtkPerlinNoise computes a Perlin noise field as an implicit function.
// vtkPerlinNoise is a concrete implementation of vtkImplicitFunction.
// Perlin noise, originally described by Ken Perlin, is a non-periodic and
// continuous noise function useful for modeling real-world objects.
//
// The amplitude and frequency of the noise pattern are adjustable.  This
// implementation of Perlin noise is derived closely from Greg Ward's version
// in Graphics Gems II.

// .SECTION See Also
// vtkImplicitFunction

#ifndef __vtkPerlinNoise_h
#define __vtkPerlinNoise_h

#include "vtkImplicitFunction.h"

class VTK_FILTERING_EXPORT vtkPerlinNoise : public vtkImplicitFunction
{
public:
  vtkTypeRevisionMacro(vtkPerlinNoise,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Instantiate the class.
  static vtkPerlinNoise *New();

  // Description:
  // Evaluate PerlinNoise function.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description:
  // Evaluate PerlinNoise gradient.  Currently, the method returns a 0 
  // gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Set/get the frequency, or physical scale,  of the noise function 
  // (higher is finer scale).  The frequency can be adjusted per axis, or
  // the same for all axes.
  vtkSetVector3Macro(Frequency,float);
  vtkGetVectorMacro(Frequency,float,3);

  // Description: 
  // Set/get the phase of the noise function.  This parameter can be used to
  // shift the noise function within space (perhaps to avoid a beat with a
  // noise pattern at another scale).  Phase tends to repeat about every
  // unit, so a phase of 0.5 is a half-cycle shift.
  vtkSetVector3Macro(Phase,float);
  vtkGetVectorMacro(Phase,float,3);

  // Description:
  // Set/get the amplitude of the noise function.  By default, the amplitude
  // is 1.
  vtkSetMacro(Amplitude,float);
  vtkGetMacro(Amplitude,float);

protected:
  vtkPerlinNoise();
  ~vtkPerlinNoise() {}

  float Frequency[3];
  float Phase[3];
  float Amplitude;

private:
  vtkPerlinNoise(const vtkPerlinNoise&); //purposely not implemented
  void operator=(const vtkPerlinNoise&); //purposely not implemented
};

#endif
