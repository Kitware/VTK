/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkglBufferObject_h
#define vtkglBufferObject_h

#include "vtkRenderingOpenGL2Module.h"
#include "vtkStdString.h" // for std::string

namespace vtkgl {

/**
 * @brief OpenGL buffer object
 *
 * OpenGL buffer object to store index, geometry and/or attribute data on the
 * GPU.
 */

class VTKRENDERINGOPENGL2_EXPORT BufferObject
{
public:
  enum ObjectType {
    ArrayBuffer,
    ElementArrayBuffer
  };

  BufferObject(ObjectType type = ArrayBuffer);
  ~BufferObject();

  /** Get the type of the buffer object. */
  ObjectType GetType() const;

  /** Get the handle of the buffer object. */
  int GetHandle() const;

  /** Determine if the buffer object is ready to be used. */
  bool IsReady() const { return Dirty == false; }

  /**
   * Upload data to the buffer object. The BufferObject::type() must match
   * @a type or be uninitialized.
   *
   * The T type must have tightly packed values of T::value_type accessible by
   * reference via T::operator[]. Additionally, the standard size() and empty()
   * methods must be implemented. The std::vector class is an example of such a
   * supported containers.
   */
  template <class T>
  bool Upload(const T &array, ObjectType type);

  // non vector version
  template <class T>
  bool Upload(const T *array, size_t numElements, ObjectType type);

  /**
   * Bind the buffer object ready for rendering.
   * @note Only one ARRAY_BUFFER and one ELEMENT_ARRAY_BUFFER may be bound at
   * any time.
   */
  bool Bind();

  /**
   * Release the buffer. This should be done after rendering is complete.
   */
  bool Release();


  // Description:
  // Release any graphics resources that are being consumed by this class.
  void ReleaseGraphicsResources();

  /**
   * Return a string describing errors.
   */
  std::string GetError() const { return Error; }

private:
  bool UploadInternal(const void *buffer, size_t size, ObjectType objectType);

  struct Private;
  Private *d;

  bool  Dirty;
  std::string Error;
};

template <class T>
inline bool BufferObject::Upload(const T &array,
                                 BufferObject::ObjectType objectType)
{
  if (array.empty())
    {
    this->Error = "Refusing to upload empty array.";
    return false;
    }
  return this->UploadInternal(&array[0],
                              array.size() * sizeof(typename T::value_type),
                              objectType);
}

template <class T>
inline bool BufferObject::Upload(const T *array, size_t numElements,
                                 BufferObject::ObjectType objectType)
{
  if (!array)
    {
    this->Error = "Refusing to upload empty array.";
    return false;
    }
  return this->UploadInternal(array,
                              numElements * sizeof(T),
                              objectType);
}

}

#endif

// VTK-HeaderTest-Exclude: vtkglBufferObject.h
