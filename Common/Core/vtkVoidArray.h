/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoidArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVoidArray - dynamic, self-adjusting array of void* pointers
// .SECTION Description
// vtkVoidArray is an array of pointers to void. It provides methods
// for insertion and retrieval of these pointers values, and will
// automatically resize itself to hold new data.

#ifndef __vtkVoidArray_h
#define __vtkVoidArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

class VTKCOMMONCORE_EXPORT vtkVoidArray : public vtkObject
{
public:
  // Description:
  // Initialize with empty array.
  static vtkVoidArray *New();

  vtkTypeMacro(vtkVoidArray,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allocate memory for this array. Delete old storage only if necessary.
  // Note that the parameter ext is no longer used.
  int Allocate(vtkIdType sz, vtkIdType ext=1000);

  // Description:
  // Release storage and reset array to initial state.
  void Initialize();

  // Description:
  // Return the type of data.
  int GetDataType() {return VTK_VOID;}

  // Description:
  // Return the size of the data contained in the array.
  int GetDataTypeSize() { return sizeof(void*); }

  // Description:
  // Set the number of void* pointers held in the array.
  void SetNumberOfPointers(vtkIdType number)
    {this->Allocate(number); this->NumberOfPointers = number;}

  // Description:
  // Get the number of void* pointers held in the array.
  vtkIdType GetNumberOfPointers()
    {return this->NumberOfPointers;}

  // Description:
  // Get the void* pointer at the ith location.
  void* GetVoidPointer(vtkIdType id)
    {return this->Array[id];}

  // Description:
  // Set the void* pointer value at the ith location in the array.
  void SetVoidPointer(vtkIdType id, void* ptr)
    {this->Array[id] = ptr;}

  // Description:
  // Insert (memory allocation performed) the void* into the ith location
  // in the array.
  void InsertVoidPointer(vtkIdType i, void* ptr);

  // Description:
  // Insert (memory allocation performed) the void* pointer at the
  // end of the array.
  vtkIdType InsertNextVoidPointer(void* tuple);

  // Description:
  // Reuse already allocated data; make the container look like it is
  // empty.
  void Reset()
    {this->NumberOfPointers = 0;}

  // Description:
  // Resize the array to just fit the inserted memory. Reclaims extra memory.
  void Squeeze()
    {this->ResizeAndExtend (this->NumberOfPointers);}

  // Description:
  // Get the address of a particular data index. Performs no checks
  // to verify that the memory has been allocated etc.
  void** GetPointer(vtkIdType id) {return this->Array + id;}

  // Description:
  // Get the address of a particular data index. Make sure data is allocated
  // for the number of items requested. Set NumberOfPointers according to
  // the number of data values requested.
  void** WritePointer(vtkIdType id, vtkIdType number);

  // Description:
  // Deep copy of another void array.
  void DeepCopy(vtkVoidArray *va);

protected:
  vtkVoidArray();
  ~vtkVoidArray();

  vtkIdType NumberOfPointers;
  vtkIdType Size;
  void**    Array;  // pointer to data

  void** ResizeAndExtend(vtkIdType sz);  // function to resize data

private:
  vtkVoidArray(const vtkVoidArray&);  // Not implemented.
  void operator=(const vtkVoidArray&);  // Not implemented.
};


#endif
