/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageTwoInputFilter.h
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
// .NAME vtkImageTwoInputFilter - Generic superclass for filters that have
// two inputs.
// .SECTION Description
// vtkImageTwoInputFilter handles two inputs.  
// It is just a subclass of vtkImageMultipleInputFilter with some
// methods that are specific to two inputs.  Although the inputs are labeled
// input1 and input2, they are stored in an array indexed starting at 0.

#ifndef __vtkImageTwoInputFilter_h
#define __vtkImageTwoInputFilter_h

#include "vtkImageMultipleInputFilter.h"

class VTK_FILTERING_EXPORT vtkImageTwoInputFilter : public vtkImageMultipleInputFilter
{
public:
  static vtkImageTwoInputFilter *New();
  vtkTypeMacro(vtkImageTwoInputFilter,vtkImageMultipleInputFilter);
  
  // Description:
  // Set the Input1 of this filter. If a ScalarType has not been set,
  // then the ScalarType of the input is used.
  virtual void SetInput1(vtkImageData *input);
  
  // Description:
  // Set the Input2 of this filter. If a ScalarType has not been set,
  // then the ScalarType of the input is used.
  virtual void SetInput2(vtkImageData *input);
  
  // Description:
  // Get the inputs to this filter.
  vtkImageData *GetInput1() {return (vtkImageData *)this->Inputs[0];}
  vtkImageData *GetInput2() {return (vtkImageData *)this->Inputs[1];}
  
protected:
  vtkImageTwoInputFilter();
  ~vtkImageTwoInputFilter() {};

private:
  vtkImageTwoInputFilter(const vtkImageTwoInputFilter&);  // Not implemented.
  void operator=(const vtkImageTwoInputFilter&);  // Not implemented.
};

#endif








