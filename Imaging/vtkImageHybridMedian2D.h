/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHybridMedian2D.h
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
// .NAME vtkImageHybridMedian2D - Median filter that preserves lines and 
// corners.
// .SECTION Description
// vtkImageHybridMedian2D is a median filter that preserves thin lines and
// corners.  It operates on a 5x5 pixel neighborhood.  It computes two values
// initially: the median of the + neighbors and the median of the x
// neighbors.  It the computes the median of these two values plus the center
// pixel.  This result of this second median is the output pixel value.



#ifndef __vtkImageHybridMedian2D_h
#define __vtkImageHybridMedian2D_h


#include "vtkImageSpatialFilter.h"

class VTK_EXPORT vtkImageHybridMedian2D : public vtkImageSpatialFilter
{
public:
  static vtkImageHybridMedian2D *New();
  vtkTypeMacro(vtkImageHybridMedian2D,vtkImageSpatialFilter);

protected:
  vtkImageHybridMedian2D();
  ~vtkImageHybridMedian2D() {};
  vtkImageHybridMedian2D(const vtkImageHybridMedian2D&);
  void operator=(const vtkImageHybridMedian2D&);

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int outExt[6], int id);
  float ComputeMedian(float *array, int size);
};

#endif



