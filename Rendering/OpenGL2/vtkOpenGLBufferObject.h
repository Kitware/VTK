// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkOpenGLBufferObject_h
#define vtkOpenGLBufferObject_h

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // for export macro
#include <cstddef>                     // for ptrdiff_t
#include <string>                      // used for std::string
#include <vector>                      // used for method args

VTK_ABI_NAMESPACE_BEGIN
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
  static vtkOpenGLBufferObject* New();
  vtkTypeMacro(vtkOpenGLBufferObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum ObjectType
  {
    ArrayBuffer,
    ElementArrayBuffer,
    TextureBuffer
  };
  enum ObjectUsage
  {
    StreamDraw,
    StreamRead,
    StreamCopy,
    StaticDraw,
    StaticRead,
    StaticCopy,
    DynamicDraw,
    DynamicRead,
    DynamicCopy
  };
  /** Get the type of the buffer object. */
  ObjectType GetType() const;

  /** Set the type of the buffer object. */
  void SetType(ObjectType value);

  /** Get the usage of the buffer object. */
  ObjectUsage GetUsage() const;

  /** Set the usage of the buffer object. */
  void SetUsage(ObjectUsage value);

  /** Get the handle of the buffer object. */
  int GetHandle() const;

  /** Determine if the buffer object is ready to be used. */
  bool IsReady() const { return this->Dirty == false; }

  /** Indicate that the buffer object needs to be re-uploaded. */
  void FlagBufferAsDirty() { this->Dirty = true; }

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
  bool Upload(const T& array, ObjectType type);
  template <class T>
  bool UploadRange(const T& array, ptrdiff_t offset, ObjectType type);
  // non vector version
  template <class T>
  bool Upload(const T* array, size_t numElements, ObjectType type);
  template <class T>
  bool UploadRange(const T* array, ptrdiff_t offset, size_t numElements, ObjectType type);
  /**
   * Allocates a buffer of `type` with `size` bytes.
   */
  bool Allocate(size_t size, ObjectType type, ObjectUsage usage);

  /**
   * Get size of the buffer in bytes.
   */
  size_t GetSize();

  /**
   * Download data from the buffer object.
   */
  template <class T>
  bool Download(T* array, size_t numElements);
  template <class T>
  bool DownloadRange(T* array, ptrdiff_t offset, size_t numElements);

  /**
   * Bind the buffer object ready for rendering.
   * @note Only one ARRAY_BUFFER and one ELEMENT_ARRAY_BUFFER may be bound at
   * any time.
   */
  bool Bind();

  /**
   * Bind the buffer to a shader storage point.
   */
  bool BindShaderStorage(int index);

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
  bool Dirty;
  std::string Error;

  bool UploadInternal(const void* buffer, size_t size, ObjectType objectType);
  bool UploadRangeInternal(
    const void* buffer, ptrdiff_t offset, ptrdiff_t size, ObjectType objectType);

private:
  bool DownloadRangeInternal(void* buffer, ptrdiff_t offset, size_t size);

  vtkOpenGLBufferObject(const vtkOpenGLBufferObject&) = delete;
  void operator=(const vtkOpenGLBufferObject&) = delete;
  struct Private;
  Private* Internal;
};

template <class T>
inline bool vtkOpenGLBufferObject::Upload(
  const T& array, vtkOpenGLBufferObject::ObjectType objectType)
{
  if (array.empty())
  {
    this->Error = "Refusing to upload empty array.";
    return false;
  }

  return this->UploadInternal(&array[0], array.size() * sizeof(typename T::value_type), objectType);
}

template <class T>
inline bool vtkOpenGLBufferObject::Upload(
  const T* array, size_t numElements, vtkOpenGLBufferObject::ObjectType objectType)
{
  if (!array)
  {
    this->Error = "Refusing to upload empty array.";
    return false;
  }
  return this->UploadInternal(array, numElements * sizeof(T), objectType);
}

template <class T>
inline bool vtkOpenGLBufferObject::UploadRange(
  const T& array, ptrdiff_t offset, vtkOpenGLBufferObject::ObjectType objectType)
{
  if (array.empty())
  {
    this->Error = "Refusing to upload empty array.";
    return false;
  }

  return this->UploadRangeInternal(
    &array[0], offset, array.size() * sizeof(typename T::value_type), objectType);
}
template <class T>
inline bool vtkOpenGLBufferObject::UploadRange(const T* array, ptrdiff_t offset, size_t numElements,
  vtkOpenGLBufferObject::ObjectType objectType)
{
  if (!array)
  {
    this->Error = "Refusing to upload empty array.";
    return false;
  }
  return this->UploadRangeInternal(array, offset, numElements * sizeof(T), objectType);
}

template <class T>
inline bool vtkOpenGLBufferObject::Download(T* array, size_t numElements)
{
  return this->DownloadRangeInternal(array, 0, numElements * sizeof(T));
}

template <class T>
inline bool vtkOpenGLBufferObject::DownloadRange(T* array, ptrdiff_t offset, size_t numElements)
{
  return this->DownloadRangeInternal(array, offset, numElements * sizeof(T));
}

VTK_ABI_NAMESPACE_END
#endif
