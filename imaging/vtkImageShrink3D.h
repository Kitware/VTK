/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShrink3D.h
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
// .NAME vtkImageShrink3D - Subsamples an image.
// .SECTION Description
// vtkImageShrink3D shrinks an image by sub sampling on a 
// uniform grid (integer multiples).  This has become an obsolete
// filter, because spatial filters now have strides which will produce 
// the save result.


#ifndef __vtkImageShrink3D_h
#define __vtkImageShrink3D_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageShrink3D : public vtkImageFilter
{
public:
  vtkImageShrink3D();
  static vtkImageShrink3D *New() {return new vtkImageShrink3D;};
  const char *GetClassName() {return "vtkImageShrink3D";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the shrink factors
  vtkSetVector3Macro(ShrinkFactors,int);
  vtkGetVector3Macro(ShrinkFactors,int);

  // Description:
  // Set/Get the pixel to use as origin.
  vtkSetVector3Macro(Shift,int);
  vtkGetVector3Macro(Shift,int);

  // Description:
  // Choose Averaging or sub sampling
  vtkSetMacro(Averaging,int);
  vtkGetMacro(Averaging,int);
  vtkBooleanMacro(Averaging,int);
  
  
protected:
  int ShrinkFactors[3];
  int Shift[3];
  int Averaging;

  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
					vtkImageRegion *inRegion);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);  
};

#endif



