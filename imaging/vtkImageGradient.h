/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGradient.h
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
// .NAME vtkImageGradient - Computes the gradient vector.
// .SECTION Description
// vtkImageGradient computes the gradient
// vector of an image.  The vector results are placed along the
// component axis.  Setting the FilteredAxes determines whether the gradient
// computed on 1D lines, 2D images, 3D volumes or higher dimensional 
// images.  The default is two dimensional XY images.  OutputScalarType
// is always float.  Gradient is computed using central differences.



#ifndef __vtkImageGradient_h
#define __vtkImageGradient_h


#include "vtkImageSpatialFilter.h"

class VTK_EXPORT vtkImageGradient : public vtkImageFilter
{
public:
  vtkImageGradient();
  static vtkImageGradient *New() {return new vtkImageGradient;};
  const char *GetClassName() {return "vtkImageGradient";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Which axes should be considered when computing the gradient.
  void SetFilteredAxes(int num, int *axes);
  vtkImageSetMacro(FilteredAxes,int);
  
  // Description:
  // If "HandleBoundariesOn" then boundary pixels are duplicated
  // So central differences can get values.
  vtkSetMacro(HandleBoundaries, int);
  vtkGetMacro(HandleBoundaries, int);
  vtkBooleanMacro(HandleBoundaries, int);

protected:
  int HandleBoundaries;
  int Dimensionality;
  
  void ExecuteImageInformation();
  void ComputeRequiredInputUpdateExtent();
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);

};

#endif



