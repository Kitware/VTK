/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDilateErode3D.h
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
// .NAME vtkImageDilateErode3D - Dilates one value and erodes another.
// .SECTION Description
// vtkImageDilateErode3D will dilate one value and erode another.
// It uses an elliptical foot print, and only erodes/dilates on the
// boundary of the two values.  The filter is restricted to the 
// X, Y, and Z axes for now.  It can degenerate to a 2 or 1 dimensional
// filter by setting the kernel size to 1 for a specific axis.


#ifndef __vtkImageDilateErode3D_h
#define __vtkImageDilateErode3D_h


#include "vtkImageSpatialFilter.h"

class vtkImageEllipsoidSource;

class VTK_IMAGING_EXPORT vtkImageDilateErode3D : public vtkImageSpatialFilter
{
public:
  // Description:
  // Construct an instance of vtkImageDilateErode3D filter.
  // By default zero values are dilated.
  static vtkImageDilateErode3D *New();
  vtkTypeMacro(vtkImageDilateErode3D,vtkImageSpatialFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This method sets the size of the neighborhood.  It also sets the 
  // default middle of the neighborhood and computes the elliptical foot print.
  void SetKernelSize(int size0, int size1, int size2);

  
  // Description:
  // Set/Get the Dilate and Erode values to be used by this filter.
  vtkSetMacro(DilateValue, float);
  vtkGetMacro(DilateValue, float);
  vtkSetMacro(ErodeValue, float);
  vtkGetMacro(ErodeValue, float);

protected:
  vtkImageDilateErode3D();
  ~vtkImageDilateErode3D();
  vtkImageDilateErode3D(const vtkImageDilateErode3D&);
  void operator=(const vtkImageDilateErode3D&);

  vtkImageEllipsoidSource *Ellipse;
  float DilateValue;
  float ErodeValue;
    
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
		       int extent[6], int id);
};

#endif

