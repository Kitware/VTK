/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximalSuppression3d.h
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
// .NAME vtkImageNonMaximalSuppression3d - Thins Gradient images..
// .SECTION Description
// vtkImageNonMaximalSuppression3d Sets to zero any gradient
// that is not a peak.  If a pixel has a neighbor along the gradient
// that has larger magnitude, the smaller pixel is set to zero.
// The vector of the image is just passed along.  
// The input and output must be float.


#ifndef __vtkImageNonMaximalSuppression3d_h
#define __vtkImageNonMaximalSuppression3d_h


#include "vtkImageSpatialFilter.h"

class vtkImageNonMaximalSuppression3d : public vtkImageSpatialFilter
{
public:
  vtkImageNonMaximalSuppression3d();
  char *GetClassName() {return "vtkImageNonMaximalSuppression3d";};
  
  void SetAxes(int axis0, int axis1, int axis2);
  void InterceptCacheUpdate(vtkImageRegion *region);

protected:
  void ExecuteCenter4d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void Execute4d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);

};

#endif



