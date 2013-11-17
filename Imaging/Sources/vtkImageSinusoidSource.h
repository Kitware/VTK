/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSinusoidSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#include "vtkImagingSourcesModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKIMAGINGSOURCES_EXPORT vtkImageSinusoidSource : public vtkImageAlgorithm
{
public:
  static vtkImageSinusoidSource *New();
  vtkTypeMacro(vtkImageSinusoidSource,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the extent of the whole output image.
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax,
                      int zMin, int zMax);

  // Description:
  // Set/Get the direction vector which determines the sinusoidal
  // orientation. The magnitude is ignored.
  void SetDirection(double,double,double);
  void SetDirection(double dir[3]);
  vtkGetVector3Macro(Direction, double);

  // Description:
  // Set/Get the period of the sinusoid in pixels.
  vtkSetMacro(Period, double);
  vtkGetMacro(Period, double);

  // Description:
  // Set/Get the phase: 0->2Pi.  0 => Cosine, pi/2 => Sine.
  vtkSetMacro(Phase, double);
  vtkGetMacro(Phase, double);

  // Description:
  // Set/Get the magnitude of the sinusoid.
  vtkSetMacro(Amplitude, double);
  vtkGetMacro(Amplitude, double);

protected:
  vtkImageSinusoidSource();
  ~vtkImageSinusoidSource() {}

  int WholeExtent[6];
  double Direction[3];
  double Period;
  double Phase;
  double Amplitude;

  virtual int RequestInformation (vtkInformation *, vtkInformationVector**, vtkInformationVector *);
  virtual void ExecuteDataWithInformation(vtkDataObject *data, vtkInformation* outInfo);
private:
  vtkImageSinusoidSource(const vtkImageSinusoidSource&);  // Not implemented.
  void operator=(const vtkImageSinusoidSource&);  // Not implemented.
};


#endif



