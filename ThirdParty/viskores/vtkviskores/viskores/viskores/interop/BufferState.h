//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_interop_BufferState_h
#define viskores_interop_BufferState_h

//gl headers needs to be buffer anything to do with buffer's
#include <viskores/interop/internal/BufferTypePicker.h>
#include <viskores/interop/internal/OpenGLHeaders.h>

#include <viskores/internal/ExportMacros.h>

#include <memory>

namespace viskores
{
namespace interop
{

namespace internal
{

VISKORES_SILENCE_WEAK_VTABLE_WARNING_START

/// \brief Device backend and opengl interop resources management
///
/// \c TransferResource manages a context for a given device backend and a
/// single OpenGL buffer as efficiently as possible.
///
/// Default implementation is a no-op
class TransferResource
{
public:
  virtual ~TransferResource() {}
};

VISKORES_SILENCE_WEAK_VTABLE_WARNING_END
}

/// \brief Manages the state for transferring an ArrayHandle to opengl.
///
/// \c BufferState holds all the relevant data information for a given ArrayHandle
/// mapping into OpenGL. Reusing the state information for all renders of an
/// ArrayHandle will allow for the most efficient interop between backends and
/// OpenGL ( especially for CUDA ).
///
///
/// The interop code in viskores uses a lazy buffer re-allocation.
///
class BufferState
{
public:
  /// Construct a BufferState using an existing GLHandle
  BufferState(GLuint& gLHandle)
    : OpenGLHandle(&gLHandle)
    , BufferType(GL_INVALID_VALUE)
    , SizeOfActiveSection(0)
    , CapacityOfBuffer(0)
    , DefaultGLHandle(0)
    , Resource()
  {
  }

  /// Construct a BufferState using an existing GLHandle and type
  BufferState(GLuint& gLHandle, GLenum type)
    : OpenGLHandle(&gLHandle)
    , BufferType(type)
    , SizeOfActiveSection(0)
    , CapacityOfBuffer(0)
    , DefaultGLHandle(0)
    , Resource()
  {
  }

  BufferState()
    : OpenGLHandle(nullptr)
    , BufferType(GL_INVALID_VALUE)
    , SizeOfActiveSection(0)
    , CapacityOfBuffer(0)
    , DefaultGLHandle(0)
    , Resource()
  {
    this->OpenGLHandle = &this->DefaultGLHandle;
  }

  ~BufferState()
  {
    //don't delete this as it points to user memory, or stack allocated
    //memory inside this object instance
    this->OpenGLHandle = nullptr;
  }

  /// \brief get the OpenGL buffer handle
  ///
  GLuint* GetHandle() const { return this->OpenGLHandle; }

  /// \brief return if this buffer has a valid OpenGL buffer type
  ///
  bool HasType() const { return this->BufferType != GL_INVALID_VALUE; }

  /// \brief return what OpenGL buffer type we are bound to
  ///
  /// will return GL_INVALID_VALUE if we don't have a valid type set
  GLenum GetType() const { return this->BufferType; }

  /// \brief Set what type of OpenGL buffer type we should bind as
  ///
  void SetType(GLenum type) { this->BufferType = type; }

  /// \brief deduce the buffer type from the template value type that
  /// was passed in, and set that as our type
  ///
  /// Will be GL_ELEMENT_ARRAY_BUFFER for
  /// viskores::Int32, viskores::UInt32, viskores::Int64, viskores::UInt64, viskores::Id, and viskores::IdComponent
  /// will be GL_ARRAY_BUFFER for everything else.
  template <typename T>
  void DeduceAndSetType(T t)
  {
    this->BufferType = viskores::interop::internal::BufferTypePicker(t);
  }

  /// \brief Get the size of the buffer in bytes
  ///
  /// Get the size of the active section of the buffer
  ///This will always be <= the capacity of the buffer
  viskores::Int64 GetSize() const { return this->SizeOfActiveSection; }

  //Set the size of buffer in bytes
  //This will always needs to be <= the capacity of the buffer
  //Note: This call should only be used internally by viskores
  void SetSize(viskores::Int64 size) { this->SizeOfActiveSection = size; }

  /// \brief Get the capacity of the buffer in bytes
  ///
  /// The buffers that viskores allocate in OpenGL use lazy resizing. This allows
  /// viskores to not have to reallocate a buffer while the size stays the same
  /// or shrinks. This allows allows the cuda to OpenGL to perform significantly
  /// better as we than don't need to call cudaGraphicsGLRegisterBuffer as
  /// often
  viskores::Int64 GetCapacity() const { return this->CapacityOfBuffer; }

  // Helper function to compute when we should resize  the capacity of the
  // buffer
  bool ShouldRealloc(viskores::Int64 desiredSize) const
  {
    const bool haveNotEnoughRoom = this->GetCapacity() < desiredSize;
    const bool haveTooMuchRoom = this->GetCapacity() > (desiredSize * 2);
    return (haveNotEnoughRoom || haveTooMuchRoom);
  }

  //Set the capacity of buffer in bytes
  //The capacity of a buffer can be larger than the active size of buffer
  //Note: This call should only be used internally by viskores
  void SetCapacity(viskores::Int64 capacity) { this->CapacityOfBuffer = capacity; }

  //Note: This call should only be used internally by viskores
  viskores::interop::internal::TransferResource* GetResource() { return this->Resource.get(); }

  //Note: This call should only be used internally by viskores
  void SetResource(viskores::interop::internal::TransferResource* resource)
  {
    this->Resource.reset(resource);
  }

private:
  // BufferState doesn't support copy or move semantics
  BufferState(const BufferState&) = delete;
  void operator=(const BufferState&) = delete;

  GLuint* OpenGLHandle;
  GLenum BufferType;
  viskores::Int64 SizeOfActiveSection; //must be Int64 as size can be over 2billion
  viskores::Int64 CapacityOfBuffer;    //must be Int64 as size can be over 2billion
  GLuint DefaultGLHandle;
  std::unique_ptr<viskores::interop::internal::TransferResource> Resource;
};
}
}

#endif //viskores_interop_BufferState_h
