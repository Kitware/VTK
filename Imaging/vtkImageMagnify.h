/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnify.h
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
// .NAME vtkImageMagnify - magnify an image by an integer value
// .SECTION Description
// vtkImageMagnify maps each pixel of the input onto a nxmx... region
// of the output.  Location (0,0,...) remains in the same place. The
// magnification occurs via pixel replication, or if Interpolate is on,
// by bilinear interpolation.

#ifndef __vtkImageMagnify_h
#define __vtkImageMagnify_h

#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageMagnify : public vtkImageToImageFilter
{
public:
  static vtkImageMagnify *New();
  vtkTypeRevisionMacro(vtkImageMagnify,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the integer magnification factors in the i-j-k directions.
  vtkSetVector3Macro(MagnificationFactors,int);
  vtkGetVector3Macro(MagnificationFactors,int);
  
  // Description:
  // Turn interpolation on and off (pixel replication is used when off).
  vtkSetMacro(Interpolate,int);
  vtkGetMacro(Interpolate,int);
  vtkBooleanMacro(Interpolate,int);
  

protected:
  vtkImageMagnify();
  ~vtkImageMagnify() {};

  int MagnificationFactors[3];
  int Interpolate;
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
private:
  vtkImageMagnify(const vtkImageMagnify&);  // Not implemented.
  void operator=(const vtkImageMagnify&);  // Not implemented.
};

#endif




