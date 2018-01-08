/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkOpenGLBufferObject_h
#define vtkOpenGLBufferObject_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkObject.h"
#include <string> // used for std::string
#include <vector> // used for method args

class vtkCellArray;
class vtkDataArray;
class vtkPoints;

/**
 * @brief OpenGL buffer object
 *
 * OpenGL buffer object to store index, geometry and/or attribute data on the
 * GPU.
 */

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLBufferObject : public vtkObject
{
public:
  static vtkOpenGLBufferObject *New();
  vtkTypeMacro(vtkOpenGLBufferObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ObjectType
  {
    ArrayBuffer,
    ElementArrayBuffer,
    TextureBuffer
  };

  /** Get the type of the buffer object. */
  ObjectType GetType() const;

  /** Set the type of the buffer object. */
  void SetType(ObjectType value);

  /** Get the handle of the buffer object. */
  int GetHandle() const;

  /** Determine if the buffer object is ready to be used. */
  bool IsReady() const { return this->Dirty == false; }

  /** Generate the opengl buffer for this Handle */
  bool GenerateBuffer(ObjectType type);

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

protected:
  vtkOpenGLBufferObject();
  ~vtkOpenGLBufferObject() override;
  bool  Dirty;
  std::string Error;

  bool UploadInternal(const void *buffer, size_t size, ObjectType objectType);

private:
  vtkOpenGLBufferObject(const vtkOpenGLBufferObject&) = delete;
  void operator=(const vtkOpenGLBufferObject&) = delete;
  struct Private;
  Private *Internal;
};

template <class T>
inline bool vtkOpenGLBufferObject::Upload(
  const T &array,
  vtkOpenGLBufferObject::ObjectType objectType)
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
inline bool vtkOpenGLBufferObject::Upload(
  const T *array, size_t numElements,
  vtkOpenGLBufferObject::ObjectType objectType)
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

#endif
