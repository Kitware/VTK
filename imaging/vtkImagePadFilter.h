/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.h
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
// .NAME vtkImagePadFilter - Super class for filters that fill in extra pixels.
// .SECTION Description
// vtkImagePadFilter Changes the image extent of an image.  If the image
// extent is larger than the input image extent, the extra pixels are
// filled by an alogorithm detemined by the subclass.
// The image extent of the output has to be specified.


#ifndef __vtkImagePadFilter_h
#define __vtkImagePadFilter_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImagePadFilter : public vtkImageFilter
{
public:
  vtkImagePadFilter();
  static vtkImagePadFilter *New() {return new vtkImagePadFilter;};
  char *GetClassName() {return "vtkImagePadFilter";};

  // Description:
  // The image extent of the output has to be set explicitely.
  void SetOutputImageExtent(int num, int *extent);
  vtkImageSetExtentMacro(OutputImageExtent);
  void GetOutputImageExtent(int num, int *extent);
  vtkImageGetExtentMacro(OutputImageExtent);
  
  // Description:
  // This affects the OutputImage Extent
  void SetAxes(int num, int *axes);
  vtkImageSetMacro(Axes,int);
  
protected:
  int OutputImageExtent[VTK_IMAGE_EXTENT_DIMENSIONS];

  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
					vtkImageRegion *inRegion);

void ChangeExtentCoordinateSystem(int *extentIn, int *axesIn,
				  int *extentOut, int *axesOut);
};

#endif



