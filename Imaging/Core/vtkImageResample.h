/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageResample - Resamples an image to be larger or smaller.
// .SECTION Description
// This filter produces an output with different spacing (and extent)
// than the input.  Linear interpolation can be used to resample the data.
// The Output spacing can be set explicitly or relative to input spacing
// with the SetAxisMagnificationFactor method.


#ifndef __vtkImageResample_h
#define __vtkImageResample_h


#include "vtkImageReslice.h"

class VTK_IMAGING_EXPORT vtkImageResample : public vtkImageReslice
{
public:
  static vtkImageResample *New();
  vtkTypeMacro(vtkImageResample,vtkImageReslice);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set desired spacing.  
  // Zero is a reserved value indicating spacing has not been set.
  void SetAxisOutputSpacing(int axis, double spacing);
  
  // Description:
  // Set/Get Magnification factors.
  // Zero is a reserved value indicating values have not been computed.
  void SetAxisMagnificationFactor(int axis, double factor);
  double GetAxisMagnificationFactor(int axis, vtkInformation *inInfo=0);
  
  // Description:
  // Dimensionality is the number of axes which are considered during
  // execution. To process images dimensionality would be set to 2.
  // This has the same effect as setting the magnification of the third
  // axis to 1.0
  vtkSetMacro(Dimensionality,int);
  vtkGetMacro(Dimensionality,int);

protected:
  vtkImageResample();
  ~vtkImageResample() {};

  double MagnificationFactors[3];
  double OutputSpacing[3];
  int Dimensionality;
  
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkImageResample(const vtkImageResample&);  // Not implemented.
  void operator=(const vtkImageResample&);  // Not implemented.
};

#endif
