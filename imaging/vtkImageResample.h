/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.h
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
// .NAME vtkImageResample - Resamples an image using linear interpolation.
// .SECTION Description
// vtkImageResample breaks the resampling of volumes into 1D operations.
// The Magnification can be specified, or the desired OutputSpacing.


#ifndef __vtkImageResample_h
#define __vtkImageResample_h


#include "vtkImageDecomposedFilter.h"
#include "vtkImageResample1D.h"
#include "vtkImageSetGet.h"

class VTK_EXPORT vtkImageResample : public vtkImageDecomposedFilter
{
public:
  vtkImageResample();
  static vtkImageResample *New() {return new vtkImageResample;};
  const char *GetClassName() {return "vtkImageResample";};

  // Description:
  // Set/Get Magnification factors.
  // Zero is a reserved value indicating values have not been computed.
  void SetMagnificationFactors(int num, float *factors);
  vtkImageSetMacro(MagnificationFactors,float);
  void GetMagnificationFactors(int num, float *factors);
  vtkImageGetMacro(MagnificationFactors,float);
  float *GetMagnificationFactors() {return this->MagnificationFactors;};  
  
  // Description:
  // Set desired spacing.  
  // Zero is a reserved value indicating spacing has not been set.
  void SetOutputSpacing(int num, float *spacing);
  vtkImageSetMacro(OutputSpacing,float);
  
  // Description:
  // Determines how many sub filters are created.
  void SetDimensionality(int num);
  
protected:
  float MagnificationFactors[VTK_IMAGE_DIMENSIONS];
  float OutputSpacing[VTK_IMAGE_DIMENSIONS];
};

#endif



