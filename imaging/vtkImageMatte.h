/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMatte.h
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
// .NAME vtkImageMatte - Adds a border to an image.
// .SECTION Description
// vtkImageMatte adds a border to an image.  The border
// can have different widths for each axis.  
// The input is used directly with no
// copying of data (unless absolutely necessary).


#ifndef __vtkImageMatte_h
#define __vtkImageMatte_h

#include "vtkImageInPlaceFilter.h"

class VTK_EXPORT vtkImageMatte : public vtkImageInPlaceFilter
{
public:
  vtkImageMatte();
  vtkImageMatte *New() {return new vtkImageMatte;};
  char *GetClassName() {return "vtkImageMatte";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the border that defines the matte.
  void SetBorderWidths(int num, int *widths);
  vtkImageSetMacro(BorderWidths, int);
  void GetBorderWidths(int num, int *widths);
  vtkImageGetMacro(BorderWidths, int);

  // Description:
  // Set/Get the value to use as a border.
  vtkSetMacro(BorderValue,float);
  vtkGetMacro(BorderValue,float);

protected:
  int BorderWidths[VTK_IMAGE_DIMENSIONS];
  float BorderValue;
  
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void FillRegion(vtkImageRegion *region);
};



#endif



