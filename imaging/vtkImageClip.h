/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageClip.h
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
// .NAME vtkImageClip - Reduces the image extent of the input.
// .SECTION Description
// vtkImageClip  will make an image smaller.  The output must have
// an image extent which is the subset of the input.  Data is
// not copied in this filter.  If Automatic is On, then the new image extent
// is automatically computed to remove uniform pixel values (white space).
// Note that even small variations (ie OutputScalarType VTK_FLOAT)
// will stop the clipping.  All the data must be parsed when the 
// OutputImageExtent is recomputed.


#ifndef __vtkImageClip_h
#define __vtkImageClip_h

#include "vtkImageInPlaceFilter.h"

class VTK_EXPORT vtkImageClip : public vtkImageInPlaceFilter
{
public:
  vtkImageClip();
  char *GetClassName() {return "vtkImageClip";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get whether to use automatic clipping.
  vtkSetMacro(Automatic,int);
  vtkGetMacro(Automatic,int);
  vtkBooleanMacro(Automatic,int);
  
  // Description:
  // The new image extent.  You should turn automatic clipping off
  // if you want to set the OutputImageExtent.
  void SetOutputImageExtent(int dim, int *extent);
  vtkImageSetExtentMacro(OutputImageExtent);
  void GetOutputImageExtent(int dim, int *extent);
  vtkImageGetExtentMacro(OutputImageExtent);
  
  // Description:
  // Sets the output image extent to be the input image extent.
  void ResetOutputImageExtent();

protected:
  // Time when OutputImageExtent was computed.
  vtkTimeStamp CTime;
  int Automatic;
  int Initialized; // Set the OutputImageExtent for the first time.
  int OutputImageExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void ComputeOutputImageInformation(vtkImageRegion *inRegion, 
				     vtkImageRegion *outRegion);
  void ComputeOutputImageExtent();
};



#endif



