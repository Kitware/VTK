/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePadFilter.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
// .NAME vtkImagePadFilter - Pads an image to change its boundaries.
// .SECTION Description
// vtkImagePadFilter is a filter class that will change the boundaries of 
// an image.  Pixels out of the input bounds are set to PadValue.


#ifndef __vtkImagePadFilter_h
#define __vtkImagePadFilter_h


#include "vtkImageFilter.hh"

class vtkImagePadFilter : public vtkImageFilter
{
public:
  vtkImagePadFilter();
  char *GetClassName() {return "vtkImagePadFilter";};

  // Description:
  // Set/Get the PadValue of the filter
  vtkSetMacro(PadValue,float);
  vtkGetMacro(PadValue,float);
  // Description:
  // Set/Get the new boundaries of the image
  vtkSetVector3Macro(BoundaryOffset,int);
  vtkGetVector3Macro(BoundaryOffset,int);
  vtkSetVector3Macro(BoundarySize,int);
  vtkGetVector3Macro(BoundarySize,int);
  
  void GetBoundary(int *offset, int *size);

protected:
  float PadValue;
  int BoundaryOffset[3];
  int BoundarySize[3];
  
  void RequiredRegion(int *outOffset, int *outSize, 
		      int *inOffset, int *inSize);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void Pad(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void PadRegion(vtkImageRegion *region, int *offset, int *size);
};

#endif



