/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample1D.h
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
// .NAME vtkImageResample1D - Resamples an image using linear interpolation.
// .SECTION Description
// vtkImageResample1D 


#ifndef __vtkImageResample1D_h
#define __vtkImageResample1D_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageResample1D : public vtkImageFilter
{
public:
  vtkImageResample1D();
  static vtkImageResample1D *New() {return new vtkImageResample1D;};
  char *GetClassName() {return "vtkImageResample1D";};

  // Description:
  // Set/Get the magnification resulting from resampling.
  // Zero is a special value indicating the value has not been computed.
  void SetMagnificationFactor(float factor);
  float GetMagnificationFactor();

  // Description:
  // Set the desired Spacing of the output.
  // Zero is a special value indicating the value has not been set.
  void SetOutputSpacing(float spacing);
  
protected:
  float MagnificationFactor;
  float OutputSpacing;
  
  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
					vtkImageRegion *inRegion);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);  
};

#endif



