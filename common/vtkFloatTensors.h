/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatTensors.h
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
// .NAME vtkFloatTensors - floating point representation of tensor data
// .SECTION Description
// vtkFloatTensors is an (obsolete) concrete implementation of vtkTensors. 
// Tensor values are represented using float values.

#ifndef __vtkFloatTensors_h
#define __vtkFloatTensors_h

#include "vtkTensors.h"
#include "vtkFloatArray.h"

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_EXPORT vtkFloatTensors : public vtkTensors
{
public:
  static vtkFloatTensors *New();
  
  // Description:
  // Set the data type for this object.
  void SetDataType(int dataType);
  
  // Description:
  // Set the data for this object. Only accepts VTK_FLOAT type.
  void SetData(vtkDataArray *);

  // Description:
  // Get pointer to array of data starting at data position "id".
  float *GetPointer(const int id);

  // Description:
  // Get pointer to data array. Useful for direct writes of data. MaxId is
  // bumped by number (and memory allocated if necessary). Id is the
  // location you wish to write into; number is the number of tensors to
  // write.
  float *WritePointer(const int id, const int number);

protected:
  vtkFloatTensors() {};
  ~vtkFloatTensors() {};
  vtkFloatTensors(const vtkFloatTensors&) {};
  void operator=(const vtkFloatTensors&) {};
  
private:
  // hide the vtkTensors' New() method
  static vtkFloatTensors *New(int) { return vtkFloatTensors::New();};
};


inline float *vtkFloatTensors::GetPointer(const int id)
{
  return ((vtkFloatArray *)this->Data)->GetPointer(9*id);
} 

inline float *vtkFloatTensors::WritePointer(const int id, const int number)
{
  return ((vtkFloatArray *)this->Data)->WritePointer(9*id,9*number);
}

inline void vtkFloatTensors::SetData(vtkDataArray *data)
{
  if ( data->GetDataType() != VTK_FLOAT )
    {
    vtkErrorMacro(<<"Float tensors only accepts float data type");
    return;
    }
  vtkTensors::SetData(data);
}

inline void vtkFloatTensors::SetDataType(int type)
{
  if ( type != VTK_FLOAT )
    {
    vtkErrorMacro(<<"Float tensors only accepts float data type");
    return;
    }

  vtkTensors::SetDataType(type);
}
#endif

#endif
