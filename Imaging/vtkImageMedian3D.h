/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMedian3D.h
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
// .NAME vtkImageMedian3D - Median Filter
// .SECTION Description
// vtkImageMedian3D a Median filter that replaces each pixel with the 
// median value from a rectangular neighborhood around that pixel.
// Neighborhoods can be no more than 3 dimensional.  Setting one
// axis of the neighborhood kernelSize to 1 changes the filter
// into a 2D median.  


#ifndef __vtkImageMedian3D_h
#define __vtkImageMedian3D_h


#include "vtkImageSpatialFilter.h"

class VTK_EXPORT vtkImageMedian3D : public vtkImageSpatialFilter
{
public:
  static vtkImageMedian3D *New();
  vtkTypeMacro(vtkImageMedian3D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method sets the size of the neighborhood.  It also sets the 
  // default middle of the neighborhood 
  void SetKernelSize(int size0, int size1, int size2);

  // Description:
  // Return the number of elements in the median mask
  vtkGetMacro(NumberOfElements,int);
  
protected:
  vtkImageMedian3D();
  ~vtkImageMedian3D() {};
  vtkImageMedian3D(const vtkImageMedian3D&);
  void operator=(const vtkImageMedian3D&);

  int NumberOfElements;

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int extent[6], int id);

};

#endif



