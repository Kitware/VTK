/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.h
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
// .NAME vtkImageResample - Resamples an image to be larger or smaller.
// .SECTION Description
// This filter produces an output with different spacing (and extent)
// than the input.  Linear interpolation can be used to resample the data.
// The Output spacing can be set explicitly or relative to input spacing
// with the SetAxisMagnificationFactor method.


#ifndef __vtkImageResample_h
#define __vtkImageResample_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageResample : public vtkImageToImageFilter
{
public:
  static vtkImageResample *New();
  vtkTypeRevisionMacro(vtkImageResample,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set desired spacing.  
  // Zero is a reserved value indicating spacing has not been set.
  void SetAxisOutputSpacing(int axis, float spacing);
  
  // Description:
  // Set/Get Magnification factors.
  // Zero is a reserved value indicating values have not been computed.
  void SetAxisMagnificationFactor(int axis, float factor);
  float GetAxisMagnificationFactor(int axis);

  // Description:
  // Turn interpolation on and off (pixel replication is used when off).
  vtkSetMacro(Interpolate,int);
  vtkGetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);
  
  // Description:
  // Dimensionality is the number of axes which are considered during
  // execution. To process images dimensionality would be set to 2.
  // This has the same effect as setting the output spacing of the third
  // axis to 1.0
  vtkSetMacro(Dimensionality,int);
  vtkGetMacro(Dimensionality,int);

protected:
  vtkImageResample();
  ~vtkImageResample() {};

  float MagnificationFactors[3];
  float OutputSpacing[3];
  int Interpolate;
  int Dimensionality;
  
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageResample(const vtkImageResample&);  // Not implemented.
  void operator=(const vtkImageResample&);  // Not implemented.
};

#endif
