/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSinusoidSource.h
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
// .NAME vtkImageSinusoidSource - Create an image with sinusoidal pixel values.
// .SECTION Description
// vtkImageSinusoidSource just produces images with pixel values determined 
// by a sinusoid.


#ifndef __vtkImageSinusoidSource_h
#define __vtkImageSinusoidSource_h

#include "vtkImageSource.h"

class VTK_IMAGING_EXPORT vtkImageSinusoidSource : public vtkImageSource
{
public:
  static vtkImageSinusoidSource *New();
  vtkTypeRevisionMacro(vtkImageSinusoidSource,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
                      int zMin, int zMax);
  
  // Description:
  // Set/Get the direction vector which determines the sinusoidal
  // orientation. The magnitude is ignored.
  void SetDirection(float,float,float);
  void SetDirection(float dir[3]);
  vtkGetVector3Macro(Direction, float);
  
  // Description:
  // Set/Get the period of the sinusoid in pixels.
  vtkSetMacro(Period, float);
  vtkGetMacro(Period, float);

  // Description:
  // Set/Get the phase: 0->2Pi.  0 => Cosine, pi/2 => Sine.
  vtkSetMacro(Phase, float);
  vtkGetMacro(Phase, float);

  // Description:
  // Set/Get the magnitude of the sinusoid.
  vtkSetMacro(Amplitude, float);
  vtkGetMacro(Amplitude, float);

protected:
  vtkImageSinusoidSource();
  ~vtkImageSinusoidSource() {};

  int WholeExtent[6];
  float Direction[3];
  float Period;
  float Phase;
  float Amplitude;

  virtual void ExecuteInformation();
  virtual void ExecuteData(vtkDataObject *data);
private:
  vtkImageSinusoidSource(const vtkImageSinusoidSource&);  // Not implemented.
  void operator=(const vtkImageSinusoidSource&);  // Not implemented.
};


#endif



