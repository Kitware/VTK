/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage2dGradientFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImage2dGradientFilter - magnitude and phase of gradient.
// .SECTION Description
// vtkImage2dGradientFilter computes a gradient of 2d image using central
// differences.  The output is always float and has three components.
// The magnitude is returned in component 0 and the direction returned
// in components 1 and 2 as a normalized vector.


#ifndef __vtkImage2dGradientFilter_h
#define __vtkImage2dGradientFilter_h


#include "vtkImageSpatialFilter.h"

class vtkImage2dGradientFilter : public vtkImageSpatialFilter
{
public:
  vtkImage2dGradientFilter();
  char *GetClassName() {return "vtkImage2dGradientFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetAxes2d(int axis0, int axis1);
  void InterceptCacheUpdate(vtkImageRegion *region);
  
  
protected:

  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void Execute3d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);

};

#endif



