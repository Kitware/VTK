/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnify.h
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
// .NAME vtkImageMagnify - Magnifies an image by integer values
// .SECTION Description
// vtkImageMagnify maps each pixel of the input onto a nxmx... region
// of the output.  Location (0,0,...) remains in the same place.
// The filter is decomposed into many filters, one for each axis.


#ifndef __vtkImageMagnify_h
#define __vtkImageMagnify_h


#include "vtkImageDecomposedFilter.h"
#include "vtkImageMagnify1D.h"
#include "vtkImageSetGet.h"

class VTK_EXPORT vtkImageMagnify : public vtkImageDecomposedFilter
{
public:
  vtkImageMagnify();
  static vtkImageMagnify *New() {return new vtkImageMagnify;};
  char *GetClassName() {return "vtkImageMagnify";};

  // Description:
  // Set/Get Magnification factors
  void SetMagnificationFactors(int num, int *factors);
  vtkImageSetMacro(MagnificationFactors,int);
  void GetMagnificationFactors(int num, int *factors);
  vtkImageGetMacro(MagnificationFactors,int);
  int *GetMagnificationFactors() {return this->MagnificationFactors;};  
  
  // Description:
  // Turn interpolation on and off (pixel replication)
  void SetInterpolate(int interpolate);
  int GetInterpolate();
  vtkBooleanMacro(Interpolate,int);
  
  // Description:
  // Determines how many sub filters are created.
  void SetDimensionality(int num);
  
protected:
  int MagnificationFactors[VTK_IMAGE_DIMENSIONS];
  int Interpolate;
};

#endif



