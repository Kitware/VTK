/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensors.h
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
// .NAME vtkTensors - represent and manipulate 3x3 tensors
// .SECTION Description
// vtkTensors represents 3x3 tensors. The data model for vtkTensors is an 
// array of 3x3 matrices accessible by (point or cell) id.

#ifndef __vtkTensors_h
#define __vtkTensors_h

#include "vtkAttributeData.h"
#include "vtkTensor.h"

class vtkIdList;
class vtkTensors;

class VTK_EXPORT vtkTensors : public vtkAttributeData
{
public:
  static vtkTensors *New(int dataType);
  static vtkTensors *New();

  vtkTypeMacro(vtkTensors,vtkAttributeData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  vtkAttributeData *MakeObject() 
    {return vtkTensors::New(this->GetDataType());};

  // Description:
  // Return number of tensors in array.
  vtkIdType GetNumberOfTensors() {return this->Data->GetNumberOfTuples();};

  // Description:
  // Return a pointer to a float tensor for a specific id.
  vtkTensor *GetTensor(vtkIdType id);

  // Description:
  // Specify the number of tensors for this object to hold. Does an
  // allocation as well as setting the MaxId ivar. Used in conjunction with
  // SetTensor() method for fast insertion.
  void SetNumberOfTensors(vtkIdType number);

  // Description:
  // Return the Tensor requested in the tensor passed.
  void GetTensor(vtkIdType id, vtkTensor *t);

  // Description:
  // Insert a Tensor into an object. No range checking performed (fast!).
  // Make sure you use SetNumberOfScalars() to allocate memory prior
  // to using SetTensor().
  void SetTensor(vtkIdType id, vtkTensor *t);

  // Description:
  // Insert a Tensor into object. Range checking performed and memory
  // allocated as necessary.
  void InsertTensor(vtkIdType id, vtkTensor *t);
  void InsertTensor(vtkIdType id, float t11, float t12, float t13, 
                    float t21, float t22, float t23, 
                    float t31, float t32, float t33);

  // Description:
  // Insert a Tensor at end of array and return its location (id) in the array.
  vtkIdType InsertNextTensor(vtkTensor *t);
  vtkIdType InsertNextTensor(float t11, float t12, float t13, 
                             float t21, float t22, float t23, 
                             float t31, float t32, float t33);

  // Description:
  // Given a list of pt ids, return an array of tensors.
  void GetTensors(vtkIdList *ptId, vtkTensors *fv);

protected:
  vtkTensors();
  ~vtkTensors();
  vtkTensors(const vtkTensors&) {};
  void operator=(const vtkTensors&) {};

  vtkTensor *T;

};


inline vtkTensor *vtkTensors::GetTensor(vtkIdType id)
{
  this->T->T = this->Data->GetTuple(id);
  return this->T;
}

inline void vtkTensors::SetNumberOfTensors(vtkIdType number)
{
  this->Data->SetNumberOfComponents(9);
  this->Data->SetNumberOfTuples(number);
}

// These include files are placed here so that if Tensors.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif

