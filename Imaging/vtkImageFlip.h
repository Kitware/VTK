/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.h
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
// .NAME vtkImageFlip - This flips an axis of an image. Right becomes left ...
// .SECTION Description
// vtkImageFlip will reflect the data along the filtered axis.
// If PreserveImageExtent is "On", then the 
// image is shifted so that it has the same image extent, and the origin is
// shifted appropriately. When PreserveImageExtent is "off",
// The Origin  is not changed, min and max of extent (of filtered axis) are
// negated, and are swapped. The default preserves the extent of the input.

#ifndef __vtkImageFlip_h
#define __vtkImageFlip_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageFlip : public vtkImageToImageFilter
{
public:
  static vtkImageFlip *New();

  vtkTypeRevisionMacro(vtkImageFlip,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify which axes will be flipped.
  vtkSetMacro(FilteredAxis, int);
  vtkGetMacro(FilteredAxis, int);

  // Description:
  // Specify which axes will be flipped.
  // For compatibility with old scripts
  void SetFilteredAxes(int axis) {this->SetFilteredAxis(axis);}
  
  // Description:
  // If PreseveImageExtent is off, then extent of axis0 is simply
  // multiplied by -1.  If it is on, then the new image min (-imageMax0)
  // is shifted to old image min (imageMin0).
  vtkSetMacro(PreserveImageExtent, int);
  vtkGetMacro(PreserveImageExtent, int);
  vtkBooleanMacro(PreserveImageExtent, int);
  
protected:
  vtkImageFlip();
  ~vtkImageFlip() {};

  int FilteredAxis;
  int PreserveImageExtent;
  
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int id);
private:
  vtkImageFlip(const vtkImageFlip&);  // Not implemented.
  void operator=(const vtkImageFlip&);  // Not implemented.
};

#endif



