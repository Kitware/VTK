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
#ifndef viskores_interop_internal_BufferTypePicker_h
#define viskores_interop_internal_BufferTypePicker_h

#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/interop/internal/OpenGLHeaders.h>

namespace viskores
{
namespace interop
{
namespace internal
{

namespace detail
{

template <typename NumericTag, typename DimensionalityTag>
static inline VISKORES_CONT GLenum BufferTypePickerImpl(NumericTag, DimensionalityTag)
{
  return GL_ARRAY_BUFFER;
}

VISKORES_CONT
static inline GLenum BufferTypePickerImpl(viskores::TypeTraitsIntegerTag,
                                          viskores::TypeTraitsScalarTag)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

} //namespace detail

static inline VISKORES_CONT GLenum BufferTypePicker(viskores::Int32)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline VISKORES_CONT GLenum BufferTypePicker(viskores::UInt32)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline VISKORES_CONT GLenum BufferTypePicker(viskores::Int64)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

static inline VISKORES_CONT GLenum BufferTypePicker(viskores::UInt64)
{
  return GL_ELEMENT_ARRAY_BUFFER;
}

/// helper function that guesses what OpenGL buffer type is the best default
/// given a primitive type. Currently GL_ELEMENT_ARRAY_BUFFER is used for
/// integer types, and GL_ARRAY_BUFFER is used for everything else
///
template <typename T>
static inline VISKORES_CONT GLenum BufferTypePicker(T)
{
  using Traits = viskores::TypeTraits<T>;
  return detail::BufferTypePickerImpl(typename Traits::NumericTag(),
                                      typename Traits::DimensionalityTag());
}
}
}
} //namespace viskores::interop::internal

#endif //viskores_interop_internal_BufferTypePicker_h
