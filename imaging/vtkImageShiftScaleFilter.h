/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShiftScaleFilter.h
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
// .NAME vtkImageShiftScaleFilter - Upper threshold on pixel values
// .SECTION Description
// vtkImageShiftScaleFilter is a pixel filter class that implements a 
// nonlinear upper threshold.  If a pixel is above Threshold, it is replaced
// with Replace.


#ifndef __vtkImageShiftScaleFilter_h
#define __vtkImageShiftScaleFilter_h


#include "vtkImageFilter.h"

class vtkImageShiftScaleFilter : public vtkImageFilter
{
public:
  vtkImageShiftScaleFilter();
  char *GetClassName() {return "vtkImageShiftScaleFilter";};

  // Description:
  // Set/Get the Shift
  vtkSetMacro(Shift,float);
  vtkGetMacro(Shift,float);

  // Description:
  // Set/Get the scale Value;
  vtkSetMacro(Scale,float);
  vtkGetMacro(Scale,float);

protected:
  float Shift;
  float Scale;

  void Execute2d(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif



