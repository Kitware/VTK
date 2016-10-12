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
/**
 * @class   vtkBuffer
 * @brief   internal storage class used by vtkSOADataArrayTemplate,
 * vtkAOSDataArrayTemplate, and others.
 *
 * vtkBuffer makes it easier to keep data pointers in vtkDataArray subclasses.
 * This is an internal class and not intended for direct use expect when writing
 * new types of vtkDataArray subclasses.
*/

#ifndef vtkBuffer_h
#define vtkBuffer_h

#include "vtkObject.h"
#include "vtkObjectFactory.h" // New() implementation

template <class ScalarTypeT>
class vtkBuffer : public vtkObject
{
public:
  vtkTemplateTypeMacro(vtkBuffer<ScalarTypeT>, vtkObject)
  typedef ScalarTypeT ScalarType;
  enum
  {
    VTK_DATA_ARRAY_FREE,
    VTK_DATA_ARRAY_DELETE
  };

  static vtkBuffer<ScalarTypeT>* New();

  /**
   * Access the buffer as a scalar pointer.
   */
  inline ScalarType* GetBuffer() { return this->Pointer; }
  inline const ScalarType* GetBuffer() const { return this->Pointer; }

  /**
   * Set the memory buffer that this vtkBuffer object will manage. @a array
   * is a pointer to the buffer data and @a size is the size of the bufffer (in
   * number of elements). If @a save is true, the buffer will not be freed when
   * this vtkBuffer object is deleted or resize -- otherwise, @a deleteMethod
   * specifies how the buffer will be freed.
   */
  void SetBuffer(ScalarType* array, vtkIdType size, bool save=false,
                 int deleteMethod=VTK_DATA_ARRAY_FREE);

  /**
   * Return the number of elements the current buffer can hold.
   */
  inline vtkIdType GetSize() const { return this->Size; }

  /**
   * Allocate a new buffer that holds @a size elements. Old data is not saved.
   */
  bool Allocate(vtkIdType size);

  /**
   * Allocate a new buffer that holds @a newsize elements. Old data is
   * preserved.
   */
  bool Reallocate(vtkIdType newsize);

protected:
  vtkBuffer()
    : Pointer(NULL),
      Size(0),
      Save(false),
      DeleteMethod(VTK_DATA_ARRAY_FREE)
  {
  }

  ~vtkBuffer() VTK_OVERRIDE
  {
    this->SetBuffer(NULL, 0);
  }

  ScalarType *Pointer;
  vtkIdType Size;
  bool Save;
  int DeleteMethod;

private:
  vtkBuffer(const vtkBuffer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBuffer&) VTK_DELETE_FUNCTION;
};

template <class ScalarT>
inline vtkBuffer<ScalarT> *vtkBuffer<ScalarT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkBuffer<ScalarT>)
}

//------------------------------------------------------------------------------
template <typename ScalarT>
void vtkBuffer<ScalarT>::SetBuffer(
    typename vtkBuffer<ScalarT>::ScalarType *array,
    vtkIdType size, bool save, int deleteMethod)
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

//------------------------------------------------------------------------------
template <typename ScalarT>
bool vtkBuffer<ScalarT>::Allocate(vtkIdType size)
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

//------------------------------------------------------------------------------
template <typename ScalarT>
bool vtkBuffer<ScalarT>::Reallocate(vtkIdType newsize)
{
  if (newsize == 0) { return this->Allocate(0); }

  if (this->Pointer &&
      (this->Save || this->DeleteMethod == VTK_DATA_ARRAY_DELETE))
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

#endif
// VTK-HeaderTest-Exclude: vtkBuffer.h
