/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSubsample3D.h
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
// .NAME vtkImageSubsample3D - Subsamples an image with an interger stride.
// .SECTION Description
// vtkImageSubsample3D shrinks an image by sub sampling on a 
// uniform grid (integer multiples).  The shrink factors indicate
// the strides taken along the filtered axes.  If the ShrinkFactor
// for an axis is 1, then the spacing along that axis is not changed.
// vtkImageGaussian and vtkImageMean can also Subsample, and
// vtkImageResample can change the spacing in non integer multiples
// of the original spacing.


#ifndef __vtkImageSubsample3D_h
#define __vtkImageSubsample3D_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageSubsample3D : public vtkImageFilter
{
public:
  vtkImageSubsample3D();
  static vtkImageSubsample3D *New() {return new vtkImageSubsample3D;};
  const char *GetClassName() {return "vtkImageSubsample3D";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the shrink factors of the FilteredAxes
  vtkSetVector3Macro(ShrinkFactors,int);
  vtkGetVector3Macro(ShrinkFactors,int);

  // Description:
  // Set/Get the pixel to use as origin.
  vtkSetVector3Macro(Shift,int);
  vtkGetVector3Macro(Shift,int);

  // Description:
  // Specify which axes will contribute to the gradient.
  // This also determines the NumberOfScalarComponents in the output.
  void SetFilteredAxes(int axis0, int axis1, int axis2);
  vtkGetVector3Macro(FilteredAxes, int);
  
protected:
  int ShrinkFactors[3];
  int Shift[3];
  int Averaging;

  void ExecuteImageInformation(vtkImageCache *in, vtkImageCache *out);
  void ComputeRequiredInputUpdateExtent(vtkImageCache *out,
					vtkImageCache *in);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);  
};

#endif



