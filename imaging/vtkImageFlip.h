/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFlip.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageFlip - 
// .SECTION Description
// vtkImageFlip will reflect the data over all of the filtered axes.
// The Origin and Spacing are not changed, min0 and max0 of extent are
// negated, and are swapped.  If PreserveImageExtent is "On", then the 
// image is shifted so that it has the same image extent.  
// The default preserves the extent of the input.

#ifndef __vtkImageFlip_h
#define __vtkImageFlip_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageFlip : public vtkImageFilter
{
public:
  vtkImageFlip();
  static vtkImageFlip *New() {return new vtkImageFlip;};
  const char *GetClassName() {return "vtkImageFlip";};

  // Description:
  // Specify which axes will be flipped.
  void SetFilteredAxes(int num, int *axes);
  vtkImageSetMacro(FilteredAxes, int);
  
  // Description:
  // If PreseveImageExtent is off, then extent of axis0 is simply
  // multiplied by -1.  If it is on, then the new image min (-imageMax0)
  // is shifted to old image min (imageMin0).
  vtkSetMacro(PreserveImageExtent, int);
  vtkGetMacro(PreserveImageExtent, int);
  vtkBooleanMacro(PreserveImageExtent, int);
  
protected:
  int PreserveImageExtent;
  
  void ExecuteImageInformation();
  void ComputeRequiredInputUpdateExtent();
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif



