/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDotProduct.h
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
// .NAME vtkImageDotProduct - Dot product of two vector images.
// .SECTION Description
// vtkImageDotProduct interpretes one axis of the input images as
// vectors and takes the dot product vector by vector.  The first axis
// is the vector axis and defaults to VTK_IMAGE_COMPONENT_AXIS.
// The output collapses the vector axis to the extent (0,0).


#ifndef __vtkImageDotProduct_h
#define __vtkImageDotProduct_h



#include "vtkImageTwoInputFilter.h"

class VTK_EXPORT vtkImageDotProduct : public vtkImageTwoInputFilter
{
public:
  vtkImageDotProduct();
  static vtkImageDotProduct *New() {return new vtkImageDotProduct;};
  char *GetClassName() {return "vtkImageDotProduct";};

protected:
  void ComputeOutputImageInformation(vtkImageRegion *inRegion1,
				     vtkImageRegion *inRegion2,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
					vtkImageRegion *inRegion1,
					vtkImageRegion *inRegion2);
  void Execute(vtkImageRegion *inRegion1, 
	       vtkImageRegion *inRegion2, 
	       vtkImageRegion *outRegion);
};

#endif



