/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeData.h
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
// .NAME vtkAttributeData - abstract class defines API for attribute data
// .SECTION Description
// vtkAttributeData is an abstract class that defines an API and methods
// to support attribute data such as scalars, vectors, tensors, etc. The
// class works by managing an underlying data array. This data array can 
// be explicitly set or alternatively, created by the object. You can
// control the type of the underlying data, if necessary.

// .SECTION See Also
// vtkPoints vtkScalars vtkVectors vtkNormals vtkTCoords vtkTensors vtkFieldData

#ifndef __vtkAttributeData_h
#define __vtkAttributeData_h

#include "vtkDataArray.h"

class VTK_EXPORT vtkAttributeData : public vtkObject 
{
public:
  vtkTypeMacro(vtkAttributeData,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate initial memory size.
  virtual int Allocate(const int sz, const int ext=1000);
  
  // Description:
  // Return object to instantiated state.
  virtual void Initialize();

  // Description:
  // Virtual constructor creates object of same type as this object.
  virtual vtkAttributeData *MakeObject() = 0;

  // Description:
  // Set/Get the underlying data array. This function must be implemented
  // in a concrete subclass to check for consistency. (The tuple size must
  // match the type of data. For example, 3-tuple data array can be assigned to
  // a vector, normal, or points object, but not a tensor object, which has a 
  // tuple dimension of 9. Scalars, on the other hand, can have tuple dimension
  //  from 1-4, depending on the type of scalar.)
  virtual void SetData(vtkDataArray *);
  vtkDataArray *GetData() {return this->Data;};

  // Description:
  // Return the underlying data type. An integer indicating data type is 
  // returned as specified in vtkSetGet.h.
  virtual int GetDataType();

  // Description:
  // Specify the underlying data type of the object.
  virtual void SetDataType(int dataType);
  void SetDataTypeToBit() {this->SetDataType(VTK_BIT);};
  void SetDataTypeToChar() {this->SetDataType(VTK_CHAR);};
  void SetDataTypeToUnsignedChar() {this->SetDataType(VTK_UNSIGNED_CHAR);};
  void SetDataTypeToShort() {this->SetDataType(VTK_SHORT);};
  void SetDataTypeToUnsignedShort() {this->SetDataType(VTK_UNSIGNED_SHORT);};
  void SetDataTypeToInt() {this->SetDataType(VTK_INT);};
  void SetDataTypeToUnsignedInt() {this->SetDataType(VTK_UNSIGNED_INT);};
  void SetDataTypeToLong() {this->SetDataType(VTK_LONG);};
  void SetDataTypeToUnsignedLong() {this->SetDataType(VTK_UNSIGNED_LONG);};
  void SetDataTypeToFloat() {this->SetDataType(VTK_FLOAT);};
  void SetDataTypeToDouble() {this->SetDataType(VTK_DOUBLE);};

  // Description:
  // Return a void pointer. For image pipeline interface and other 
  // special pointer manipulation.
  void *GetVoidPointer(const int id) {return this->Data->GetVoidPointer(id);};

  // Description:
  // Reclaim any extra memory.
  virtual void Squeeze() {this->Data->Squeeze();};

  // Description:
  // Make object look empty but do not delete memory.  
  virtual void Reset() {this->Data->Reset();};

  // Description:
  // Different ways to copy data. Shallow copy does reference count (i.e.,
  // assigns pointers and updates reference count); deep copy runs through
  // entire data array assigning values.
  virtual void DeepCopy(vtkAttributeData *ad);
  virtual void ShallowCopy(vtkAttributeData *ad);

  // Description:
  // Return the memory in kilobytes consumed by this attribute data. 
  // Used to support streaming and reading/writing data. The value 
  // returned is guaranteed to be greater than or equal to the 
  // memory required to actually represent the data represented 
  // by this object. The information returned is valid only after
  // the pipeline has been updated.
  unsigned long GetActualMemorySize();

protected:
  // Construct object with an initial data array of type dataType (by default
  // dataType is VTK_FLOAT.
  vtkAttributeData();
  vtkAttributeData(int dataType);
  ~vtkAttributeData();
  vtkAttributeData(const vtkAttributeData&) {};
  void operator=(const vtkAttributeData&) {};

  vtkDataArray *Data;  // Array which represents data

};


#endif
