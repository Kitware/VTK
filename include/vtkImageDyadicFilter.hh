/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDyadicFilter.hh
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
// .NAME vtkImageDyadicFilter - Generic filter has two inputs
// .SECTION Description
// vtkImageDyadicFilter is a filter class the main interface is the function
// RequestRegion which is passed a Region and returns a tile.


#ifndef __vtkImageDyadicFilter_h
#define __vtkImageDyadicFilter_h


#include "vtkImageCachedSource.hh"
#include "vtkImageRegion.hh"

class vtkImageDyadicFilter : public vtkImageCachedSource
{
public:
  vtkImageDyadicFilter();
  char *GetClassName() {return "vtkImageDyadicFilter";};

  void GenerateRegion(int *outOffset, int *outSize);    
  unsigned long int GetPipelineMTime();
  void GetBoundary(int *offset, int *size);

  virtual void SetInput1(vtkImageSource *input);
  virtual void SetInput2(vtkImageSource *input);
  // Description:
  // Get the first input to this filter.
  vtkGetObjectMacro(Input1,vtkImageSource);

  // Description:
  // Get the second input to this filter.
  vtkGetObjectMacro(Input2,vtkImageSource);

protected:
  vtkImageSource *Input1;  // the first input to the filter 
  vtkImageSource *Input2;  // the second input to the filter 

  vtkImageData *Input1Data; // cache to save in1Region while splitting for in2

  // Not working yet
  void GenerateRegionTiled(int *outOffset, int *outSize);
  virtual void SplitRegion(int *outOffset, int *outSize, int *pieceSize);
  int GetSplittingInput();
  void ClearInputs();
  vtkImageRegion *GetInput1Region(int *in1Offset, int *in1Size);
  vtkImageRegion *GetInput2Region(int *in2Offset, int *in2Size);


  // method specified by sub classes
  virtual void RequiredInput1Region(int *outOffset, int *outSize,
				 int *inOffset1, int *inSize1);
  virtual void RequiredInput2Region(int *outOffset, int *outSize,
				 int *inOffset2, int *inSize2);
  virtual void Execute(vtkImageRegion *inRegion1, vtkImageRegion *inRegion2, 
		       vtkImageRegion *outRegion);
};

#endif







