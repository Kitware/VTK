/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLowerThresholdFilter.h
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
// .NAME vtkImageLowerThresholdFilter - puts a lower threshold on pixels
// .SECTION Description
// vtkImageLowerThresholdFilter is a pixel filter class that implements a 
// nonlinear lower threshold.  If a pixel is below Threshold, it is replaces
// with Replace.


#ifndef __vtkImageLowerThresholdFilter_h
#define __vtkImageLowerThresholdFilter_h


#include "vtkImageFilter.hh"

class vtkImageLowerThresholdFilter : public vtkImageFilter
{
public:
  char *GetClassName() {return "vtkImageLowerThresholdFilter";};
  vtkImageLowerThresholdFilter();

  // Description:
  // Set/Get the Threshold
  vtkSetMacro(Threshold,float);
  vtkGetMacro(Threshold,float);

  // Description:
  // Set/Get the Replace Value;
  vtkSetMacro(Replace,float);
  vtkGetMacro(Replace,float);

protected:
  float Threshold;
  float Replace;

  void RequiredRegion(int *outOffset, int *outSize, 
		      int *inOffset, int *inSize);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif



