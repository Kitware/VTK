/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkglBufferObject_h
#define __vtkglBufferObject_h

#include "vtkRenderingOpenGL2Module.h"
#include <string> // For member variables.

namespace vtkgl {

/**
 * @brief OpenGL buffer object
 *
 * OpenGL buffer object to store geometry/attribute data on the GPU.
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
  ObjectType type() const;

  /** Get the handle of the buffer object. */
  int handle() const;

  /** Determine if the buffer object is ready to be used. */
  bool ready() const { return m_dirty == false; }

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
  bool upload(const T &array, ObjectType type);

  /**
   * Bind the buffer object ready for rendering.
   * @note Only one ARRAY_BUFFER and one ELEMENT_ARRAY_BUFFER may be bound at
   * any time.
   */
  bool bind();

  /**
   * Release the buffer. This should be done after rendering is complete.
   */
  bool release();

  /**
   * Return a string describing errors.
   */
  std::string error() const { return m_error; }

private:
  bool uploadInternal(const void *buffer, size_t size, ObjectType objectType);

  struct Private;
  Private *d;
  bool  m_dirty;

  std::string m_error;
};

template <class T>
inline bool BufferObject::upload(const T &array,
                                 BufferObject::ObjectType objectType)
{
  if (array.empty()) {
    m_error = "Refusing to upload empty array.";
    return false;
  }
  return uploadInternal(&array[0],
                        array.size() * sizeof(typename T::value_type),
                        objectType);
}

}

#endif
