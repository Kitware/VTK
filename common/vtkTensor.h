/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkTensor - supporting class to enable assignment and referencing of tensors
// .SECTION Description
// vtkTensor is a floating point representation of an nxn tensor. vtkTensor 
// provides methods for assignment and reference of tensor components. It 
// does it in such a way as to minimize data copying.
//
// .SECTION Caveats
// vtkTensor performs its operations using pointer reference. You are 
// responsible for supplying data storage (if necessary) if local copies 
// of data are being made.

#ifndef __vtkTensor_h
#define __vtkTensor_h

#include "vtkObject.h"

class VTK_EXPORT vtkTensor : public vtkObject
{
public:
  static vtkTensor *New();
  vtkTypeMacro(vtkTensor,vtkObject);

  // Description:
  // Initialize tensor components to 0.0.
  void Initialize();

  // Description:
  // Get the tensor component (i,j).
  float GetComponent(int i, int j) {return this->T[i+3*j];};

  // Description:
  // Set the value of the tensor component (i,j).
  void SetComponent(int i, int j, float v) {if (i > 2 || j > 2) {vtkErrorMacro("trying to set tensor component i or j > 2: i = " << i << ", j = " << j); return;}; this->T[i+3*j] = v;};

  // Description:
  // Add to the value of the tensor component at location (i,j).
  void AddComponent(int i, int j, float v) { if (i > 2 || j > 2) {vtkErrorMacro("trying to add tensor component i or j > 2: i = " << i << ", j = " << j); return;}; this->T[i+3*j] += v;};

  // Description:
  // Return column vector from tensor. (Assumes 2D matrix form and 0-offset.)
  float *GetColumn(int j) { if (j > 2) {vtkErrorMacro("trying to get tensor column j > 2: j = " << j); return NULL;}; return this->T + 3*j;};

  // Description:
  // Deep copy of one tensor to another tensor.
  void DeepCopy(vtkTensor *t);

  // Description:
  // Provide float * type conversion.
  operator float*() {return this->T;};

  // Description:
  // Data member left public for efficiency.
  float *T;
  
protected: 
  vtkTensor();
  ~vtkTensor() {};
  vtkTensor(const vtkTensor&) {};
  void operator=(const vtkTensor&) {};

  float Storage[9];
};

inline void vtkTensor::Initialize()
{
  for (int j=0; j<3; j++)
    {
    for (int i=0; i<3; i++)
      {
      this->T[i+j*3] = 0.0;
      }
    }
}

inline void vtkTensor::DeepCopy(vtkTensor *t)
{
  for (int j=0; j < 3; j++)
    {
    for (int i=0; i < 3; i++)
      {
      this->T[i+3*j] = t->T[i+3*j];
      }
    }
}

#endif
