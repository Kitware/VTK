/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageStencil.h
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
// .NAME vtkImageToImageStencil - clip an image with a mask image
// .SECTION Description
// vtkImageToImageStencil will convert a vtkImageData into an stencil
// that can be used with vtkImageStecil or other vtk classes that apply
// a stencil to an image.
// .SECTION see also
// vtkImageStencil vtkImplicitFunctionToImageStencil vtkPolyDataToImageStencil

#ifndef __vtkImageToImageStencil_h
#define __vtkImageToImageStencil_h


#include "vtkImageStencilSource.h"
#include "vtkImageData.h"

class VTK_IMAGING_EXPORT vtkImageToImageStencil : public vtkImageStencilSource
{
public:
  static vtkImageToImageStencil *New();
  vtkTypeRevisionMacro(vtkImageToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the image data to convert into a stencil.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();

  // Description:
  // The values greater than or equal to the value match.
  void ThresholdByUpper(float thresh);
  
  // Description:
  // The values less than or equal to the value match.
  void ThresholdByLower(float thresh);
  
  // Description:
  // The values in a range (inclusive) match
  void ThresholdBetween(float lower, float upper);
  
  // Description:
  // Get the Upper and Lower thresholds.
  vtkSetMacro(UpperThreshold, float);
  vtkGetMacro(UpperThreshold, float);
  vtkSetMacro(LowerThreshold, float);
  vtkGetMacro(LowerThreshold, float);

protected:
  vtkImageToImageStencil();
  ~vtkImageToImageStencil();

  void ThreadedExecute(vtkImageStencilData *output,
                       int extent[6], int threadId);

  float UpperThreshold;
  float LowerThreshold;
  float Threshold;
private:
  vtkImageToImageStencil(const vtkImageToImageStencil&);  // Not implemented.
  void operator=(const vtkImageToImageStencil&);  // Not implemented.
};

#endif
