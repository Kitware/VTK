/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSpatialFilter.h
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
// .NAME vtkImageSpatialFilter - Filters that operate on pixel neighborhoods.
// .SECTION Description
// vtkImageSpatialFilter is a super class for filters that operate on an
// input neighborhood for each output pixel. It handles even sized
// neighborhoods, but their can be a half pixel shift associated with
// processing.  This superclass has some logic for handling boundaries.  It
// can split regions into boundary and non-boundary pieces and call different
// execute methods.


#ifndef __vtkImageSpatialFilter_h
#define __vtkImageSpatialFilter_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageSpatialFilter : public vtkImageToImageFilter
{
public:
  static vtkImageSpatialFilter *New();
  vtkTypeMacro(vtkImageSpatialFilter,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the Kernel size.
  int *GetKernelSize() {return this->KernelSize;}
  
  // Description:
  // Get the Kernel middle.
  int *GetKernelMiddle() {return this->KernelMiddle;}

protected:
  vtkImageSpatialFilter();
  ~vtkImageSpatialFilter() {};
  vtkImageSpatialFilter(const vtkImageSpatialFilter&);
  void operator=(const vtkImageSpatialFilter&);

  int   KernelSize[3];
  int   KernelMiddle[3];      // Index of kernel origin
  int   Strides[3];      // Shrink factor
  int   HandleBoundaries;     // Output shrinks if boundaries aren't handled

  // Called by the superclass
  void ExecuteInformation();
  // Override this method if you have to.
  virtual void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);

  void ComputeOutputWholeExtent(int extent[6], int handleBoundaries);
  void ComputeInputUpdateExtent(int extent[6], int wholeExtent[6]);

};

#endif










