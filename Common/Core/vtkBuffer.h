/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBuffer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBuffer - internal storage class used by vtkSOADataArrayTemplate,
// vtkAOSDataArrayTemplate, and others.
// .SECTION Description
// vtkBuffer makes it easier to keep data pointers in vtkDataArray subclasses.
// This is an internal class and not intended for direct use expect when writing
// new types of vtkDataArray subclasses.

#ifndef vtkBuffer_h
#define vtkBuffer_h

#include "vtkObject.h"

template <class ScalarTypeT>
class vtkBuffer
{
public:
  typedef ScalarTypeT ScalarType;
  enum DeleteMethod
    {
    VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE
    };

  vtkBuffer()
    : Pointer(NULL),
    Size(0),
    Save(false),
    DeleteMethod(VTK_DATA_ARRAY_FREE)
    {
    }

  ~vtkBuffer()
    {
    }

  inline ScalarType* GetBuffer() const
    { return this->Pointer; }
  void SetBuffer(ScalarType* array, vtkIdType size, bool save=false,
                 int deleteMethod=VTK_DATA_ARRAY_FREE)
    {
    if (this->Pointer != array)
      {
      if (!this->Save)
        {
        if (this->DeleteMethod == VTK_DATA_ARRAY_FREE)
          {
          free(this->Pointer);
          }
        else
          {
          delete [] this->Pointer;
          }
        }
      this->Pointer = array;
      }
    this->Size = size;
    this->Save = save;
    this->DeleteMethod = deleteMethod;
    }

  inline vtkIdType GetSize() const { return this->Size; }

  bool Allocate(vtkIdType size)
    {
    // release old memory.
    this->SetBuffer(NULL, 0);
    if (size > 0)
      {
      ScalarType* newArray =
          static_cast<ScalarType*>(malloc(size * sizeof(ScalarType)));
      if (newArray)
        {
        this->SetBuffer(newArray, size, false, VTK_DATA_ARRAY_FREE);
        return true;
        }
      return false;
      }
    return true; // size == 0
    }

  bool Reallocate(vtkIdType newsize)
    {
    if (newsize == 0) { return this->Allocate(0); }

    // OS X's realloc does not free memory if the new block is smaller.  This
    // is a very serious problem and causes huge amount of memory to be
    // wasted. Do not use realloc on the Mac.
    bool dontUseRealloc=false;
#if defined __APPLE__
    dontUseRealloc=true;
#endif

    if (this->Pointer &&
      (this->Save || this->DeleteMethod == VTK_DATA_ARRAY_DELETE ||
       dontUseRealloc))
      {
      ScalarType* newArray =
          static_cast<ScalarType*>(malloc(newsize * sizeof(ScalarType)));
      if (!newArray)
        {
        return false;
        }
      std::copy(this->Pointer, this->Pointer + std::min(this->Size, newsize),
                newArray);
      // now save the new array and release the old one too.
      this->SetBuffer(newArray, newsize, false, VTK_DATA_ARRAY_FREE);
      }
    else
      {
      // Try to reallocate with minimal memory usage and possibly avoid
      // copying.
      ScalarType* newArray = static_cast<ScalarType*>(
            realloc(this->Pointer, newsize * sizeof(ScalarType)));
      if (!newArray)
        {
        return false;
        }
      this->Pointer = newArray;
      this->Size = newsize;
      }
    return true;
    }

protected:
  ScalarType *Pointer;
  vtkIdType Size;
  bool Save;
  int DeleteMethod;
};

#endif
// VTK-HeaderTest-Exclude: vtkBuffer.h
