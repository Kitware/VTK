/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.h
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
// .NAME vtkImageAppendComponents - Collects components from two inputs into
// one output.
// .SECTION Description
// vtkImageAppendComponents takes the components from two inputs and merges
// them into one output. If Input1 has M components, and Input2 has N 
// components, the output will have M+N components with input1
// components coming first.


#ifndef __vtkImageAppendComponents_h
#define __vtkImageAppendComponents_h


#include "vtkImageMultipleInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageAppendComponents : public vtkImageMultipleInputFilter
{
public:
  static vtkImageAppendComponents *New();
  vtkTypeMacro(vtkImageAppendComponents,vtkImageMultipleInputFilter);

#ifndef VTK_REMOVE_LEGACY_CODE
  // Description:
  // Do not use these: They are for legacy compatibility back when this was a
  // two input filter.
  virtual void SetInput1(vtkImageData *input)
    {VTK_LEGACY_METHOD(SetInput,"3.2"); this->SetInput(0, input);}
  virtual void SetInput2(vtkImageData *input)
    {VTK_LEGACY_METHOD(SetInput,"3.2"); this->SetInput(1, input);}
#endif
  
protected:
  vtkImageAppendComponents() {};
  ~vtkImageAppendComponents() {};
  
  void ExecuteInformation(vtkImageData **inputs, vtkImageData *output);
  void ExecuteInformation(){this->vtkImageMultipleInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
		       int extent[6], int id);
private:
  vtkImageAppendComponents(const vtkImageAppendComponents&);  // Not implemented.
  void operator=(const vtkImageAppendComponents&);  // Not implemented.
};

#endif




