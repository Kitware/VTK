/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
  // Description:
  // Construct object with an initial data array of type dataType (by default
  // dataType is VTK_FLOAT.
  vtkAttributeData(int dataType=VTK_FLOAT);

  ~vtkAttributeData();
  const char *GetClassName() {return "vtkAttributeData";};
  void PrintSelf(ostream& os, vtkIndent indent);
  virtual int Allocate(const int sz, const int ext=1000);
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
  virtual void DeepCopy(vtkAttributeData& ad);
  virtual void ShallowCopy(vtkAttributeData& ad);

protected:
  vtkDataArray *Data;  // Array which represents data

};


#endif
