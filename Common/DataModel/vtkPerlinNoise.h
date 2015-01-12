/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPerlinNoise.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#ifndef vtkPerlinNoise_h
#define vtkPerlinNoise_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class VTKCOMMONDATAMODEL_EXPORT vtkPerlinNoise : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkPerlinNoise,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Instantiate the class.
  static vtkPerlinNoise *New();

  // Description:
  // Evaluate PerlinNoise function.
  double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description:
  // Evaluate PerlinNoise gradient.  Currently, the method returns a 0
  // gradient.
  void EvaluateGradient(double x[3], double n[3]);

  // Description:
  // Set/get the frequency, or physical scale,  of the noise function
  // (higher is finer scale).  The frequency can be adjusted per axis, or
  // the same for all axes.
  vtkSetVector3Macro(Frequency,double);
  vtkGetVectorMacro(Frequency,double,3);

  // Description:
  // Set/get the phase of the noise function.  This parameter can be used to
  // shift the noise function within space (perhaps to avoid a beat with a
  // noise pattern at another scale).  Phase tends to repeat about every
  // unit, so a phase of 0.5 is a half-cycle shift.
  vtkSetVector3Macro(Phase,double);
  vtkGetVectorMacro(Phase,double,3);

  // Description:
  // Set/get the amplitude of the noise function. Amplitude can be negative.
  // The noise function varies randomly between -|Amplitude| and |Amplitude|.
  // Therefore the range of values is 2*|Amplitude| large.
  // The initial amplitude is 1.
  vtkSetMacro(Amplitude,double);
  vtkGetMacro(Amplitude,double);

protected:
  vtkPerlinNoise();
  ~vtkPerlinNoise() {}

  double Frequency[3];
  double Phase[3];
  double Amplitude;

private:
  vtkPerlinNoise(const vtkPerlinNoise&); // Not implemented
  void operator=(const vtkPerlinNoise&); // Not implemented
};

#endif
