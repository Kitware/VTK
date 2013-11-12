/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradient.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageGradient - Computes the gradient vector.
// .SECTION Description
// vtkImageGradient computes the gradient vector of an image.  The
// vector results are stored as scalar components. The Dimensionality
// determines whether to perform a 2d or 3d gradient. The default is
// two dimensional XY gradient.  OutputScalarType is always
// double. Gradient is computed using central differences.

#ifndef __vtkImageGradient_h
#define __vtkImageGradient_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageGradient : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageGradient *New();
  vtkTypeMacro(vtkImageGradient,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Determines how the input is interpreted (set of 2d slices ...)
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);

  // Description:
  // Get/Set whether to handle boundaries.  If enabled, boundary
  // pixels are treated as duplicated so that central differencing
  // works for the boundary pixels.  If disabled, the output whole
  // extent of the image is reduced by one pixel.
  vtkSetMacro(HandleBoundaries, int);
  vtkGetMacro(HandleBoundaries, int);
  vtkBooleanMacro(HandleBoundaries, int);

protected:
  vtkImageGradient();
  ~vtkImageGradient() {}

  int HandleBoundaries;
  int Dimensionality;

  virtual int RequestInformation (vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);
  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  void ThreadedRequestData(vtkInformation*,
                           vtkInformationVector**,
                           vtkInformationVector*,
                           vtkImageData*** inData,
                           vtkImageData** outData,
                           int outExt[6],
                           int threadId);
private:
  vtkImageGradient(const vtkImageGradient&);  // Not implemented.
  void operator=(const vtkImageGradient&);  // Not implemented.
};

#endif



