/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximumSuppression.h
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
// .NAME vtkImageNonMaximumSuppression - Thins Gradient images.
// .SECTION Description
// vtkImageNonMaximumSuppression Sets to zero any gradient
// that is not a peak.  If a pixel has a neighbor along the gradient
// that has larger magnitude, the smaller pixel is set to zero.
// The filter takes two inputs: a gradient magnitude and a gradient vector.
// Output is magnitude information and is always in floats.


#ifndef __vtkImageNonMaximumSuppression_h
#define __vtkImageNonMaximumSuppression_h


#include "vtkImageTwoInputFilter.h"

class VTK_EXPORT vtkImageNonMaximumSuppression : public vtkImageTwoInputFilter
{
public:
  vtkImageNonMaximumSuppression();
  static vtkImageNonMaximumSuppression *New() {return new vtkImageNonMaximumSuppression;};
  const char *GetClassName() {return "vtkImageNonMaximumSuppression";};
  
  // Description:
  // These method add VTK_IMAGE_COMPONENT_AXIS as the last axis.
  void SetFilteredAxes(int num, int *axes);
  vtkImageSetMacro(FilteredAxes, int);

  // Description:
  // Rename the inputs.
  void SetMagnitudeInput(vtkImageCache *input) {this->SetInput1(input);};
  void SetMagnitudeInput(vtkStructuredPoints *input) {this->SetInput1(input);};
  void SetVectorInput(vtkImageCache *input) {this->SetInput2(input);};
  void SetVectorInput(vtkStructuredPoints *input) {this->SetInput2(input);};
  
  // Description:
  // If "HandleBoundariesOn" then boundary pixels are duplicated
  // So central differences can get values.
  vtkSetMacro(HandleBoundaries, int);
  vtkGetMacro(HandleBoundaries, int);
  vtkBooleanMacro(HandleBoundaries, int);

  
protected:
  int HandleBoundaries;
  int Dimensionality;

  void ExecuteImageInformation(vtkImageCache *in1, vtkImageCache *in2,
			       vtkImageCache *out);
  void ComputeRequiredInputUpdateExtent(vtkImageCache *out, vtkImageCache *in1,
					vtkImageCache *in2);
  void Execute(vtkImageRegion *inRegion1, vtkImageRegion *inRegion2, 
	       vtkImageRegion *outRegion);

};

#endif



