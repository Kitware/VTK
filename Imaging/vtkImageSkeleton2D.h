/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSkeleton2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkImageSkeleton2D - Skeleton of 2D images.
// .SECTION Description
// vtkImageSkeleton2D should leave only single pixel width lines
// of non-zero-valued pixels (values of 1 are not allowed).  
// It works by erosion on a 3x3 neighborhood with special rules.
// The number of iterations determines how far the filter can erode.
// There are three pruning levels:  
//  prune == 0 will leave traces on all angles...
//  prune == 1 will not leave traces on 135 degree angles, but will on 90.
//  prune == 2 does not leave traces on any angles leaving only closed loops.
// Prune defaults to zero. The output scalar type is the same as the input.



#ifndef __vtkImageSkeleton2D_h
#define __vtkImageSkeleton2D_h

#include "vtkImageIterateFilter.h"

class VTK_EXPORT vtkImageSkeleton2D : public vtkImageIterateFilter
{
public:
  static vtkImageSkeleton2D *New();
  vtkTypeMacro(vtkImageSkeleton2D,vtkImageIterateFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // When prune is on, only closed loops are left unchanged.
  vtkSetMacro(Prune,int);
  vtkGetMacro(Prune,int);
  vtkBooleanMacro(Prune,int);

  // Description:
  // Sets the number of cycles in the erosion.
  void SetNumberOfIterations(int num);
  
  virtual void IterativeExecuteData(vtkImageData *in, vtkImageData *out) 
    { this->MultiThread(in,out);};

protected:
  vtkImageSkeleton2D();
  ~vtkImageSkeleton2D() {};
  vtkImageSkeleton2D(const vtkImageSkeleton2D&);
  void operator=(const vtkImageSkeleton2D&);

  int Prune;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6] );
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int outExt[6], int id);
};

#endif



