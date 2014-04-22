/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkTexture2D_h
#define __vtkTexture2D_h

#include "vtkRenderingOpenGL2Module.h"

#include "vtkglVector.h" // For Vector types
#include "vtkTypeTraits.h" // For type traits inline template

namespace vtkgl {

/**
 * @brief The Texture2D class manages texture resources in graphics libraries.
 */
class VTKRENDERINGOPENGL2_EXPORT Texture2D
{
public:
  /**
   * @brief The FilterOption enum defines options for interpolating
   * texels onto pixels.
   */
  enum FilterOption {
    InvalidFilter = -1,
    /// Use the nearest texel for the pixel value.
    Nearest = 0,
    /// Interpolate the four neighboring texels for the pixel value.
    Linear
  };

  /**
   * @brief The WrappingOption enum defines options for handling texture
   * coordinates outside of the range [0, 1]. Note that these are specified
   * separately for each dimension of the texture.
   */
  enum WrappingOption {
    InvalidWrapping = -1,
    /// Use the texel at the nearest edge.
    ClampToEdge = 0,
    /// Repeat the texels such that texture coordinates in the range [1, 2] map
    /// to [1, 0]; [2, 3] map to [0, 1], and so on.
    MirroredRepeat,
    /// Repeat the texels such that integer components of the texture
    /// coordinates are ignored (e.g. 1.7 becomes 0.7, 2.3 becomes 0.3, etc).
    Repeat
  };

  /**
   * @brief The InternalFormat enum defines how the texture data will be stored
   * by the graphics library implementation.
   */
  enum InternalFormat {
    InvalidInternalFormat = -1,
    /// Each element is a single depth component.
    InternalDepth = 0,
    /// Each element contains a depth component and a stencil value.
    InternalDepthStencil,
    /// Each element contains a single color value.
    InternalR,
    /// Each element contains two color values.
    InternalRG,
    /// Each element contains three color values.
    InternalRGB,
    /// Each element contains four color values.
    InternalRGBA
  };

  /**
   * @brief The IncomingFormat enum defines the supported formats for incoming
   * texture data.
   */
  enum IncomingFormat {
    InvalidIncomingFormat = -1,
    /// Each element is a single red component.
    IncomingR = 0,
    /// Each element is a red then green component.
    IncomingRG,
    /// Each element is a red, green, then blue component.
    IncomingRGB,
    /// Each element is a blue, green, then red component.
    IncomingBGR,
    /// Each element is a red, green, blue, then alpha component.
    IncomingRGBA,
    /// Each element is a blue, green, red, then alpha component.
    IncomingBGRA,
    /// Each element is a single depth component.
    IncomingDepth,
    /// Each element is a depth component followed by a stencil value.
    IncomingDepthStencil
  };

  Texture2D();
  ~Texture2D();

  /** A unique integer value identifying the texture. */
  int handle() const;

  /** True if the texture is ready to use. */
  bool ready() const { return !m_dirty; }

  /** Filtering options for interpolating pixel values. Default is Linear. {@ */
  void setMinFilter(FilterOption opt);
  FilterOption minFilter() const;
  void setMagFilter(FilterOption opt);
  FilterOption magFilter() const;
  /** @} */

  /** Wrapping options in the s-direction. Default is Repeat. @{ */
  void setWrappingS(WrappingOption opt);
  WrappingOption wrappingS() const;
  /** @} */

  /** Wrapping options in the t-direction. Default is Repeat. @{ */
  void setWrappingT(WrappingOption opt);
  WrappingOption wrappingT() const;
  /** @} */

  /**
   * Upload a buffer of texture data to the graphics library.
   *
   * @param buffer The texture data (see below for requirements of T).
   * @param dims The width and height of the texture data.
   * @param dataFormat The ordering of components in the buffer data.
   * @param internalFormat The internal storage ordering of components in the
   * buffer data.
   *
   * The buffer must contain homogenous elements of the follow types: unsigned
   * char, char, unsigned short, short, unsigned int, int, or float.
   *
   * The first element of @a buffer is the lower-left texel. Subsequent elements
   * are ordered such that rows are contiguous, moving right and up.
   *
   * The T must satisfy the following requirements (short version:
   * use std::vector):
   * - T must have tightly packed values of T::value_type
   * - elements must be accessible by reference via T::operator[].
   * - ContainterT::size() must return the number of elements in the container
   *   as integral type T::size_type.
   */
  template <class T>
  bool upload(const T &buffer, const Vector2i &dims,
              IncomingFormat dataFormat, InternalFormat internalFormat);

  /** Bind the texture for rendering. */
  bool bind() const;

  bool release() const;

  std::string error() const { return m_error; }

private:
  bool uploadInternal(const void *buffer, const Vector2i &dims,
                      IncomingFormat dataFormat, int dataType,
                      InternalFormat internalFormat);

  // Used to preserve GL state when setting/querying texture properties:
  int pushTexture() const; // -1 indicates error
  void popTexture(int) const;

  // Create a texture handle and set the defaults.
  bool generateTextureHandle();

  class Private;
  Private * const d;
  bool m_dirty;
  mutable std::string m_error;
};

template <class T> inline
bool Texture2D::upload(const T &buffer, const Vector2i &dims,
                       IncomingFormat incomingFormat,
                       InternalFormat internalFormat)
{
  if (buffer.size() == 0 || dims[0] == 0 || dims[1] == 0) {
    m_error = "Refusing to upload empty array.";
    return false;
  }

  if (buffer.size()
      < static_cast<typename T::size_type>(dims[0] * dims[1])) {
    m_error = "Buffer data is smaller than specified dimensions.";
    return false;
  }

  int incomingType =
      static_cast<int>(vtkTypeTraits<typename T::value_type>::VTKTypeID());
  switch (incomingType) {
  case VTK_CHAR:
  case VTK_UNSIGNED_CHAR:
  case VTK_SHORT:
  case VTK_UNSIGNED_SHORT:
  case VTK_INT:
  case VTK_UNSIGNED_INT:
  case VTK_FLOAT:
    break;
  default:
    m_error = "Unsupported type for texture data: '";
    m_error += vtkTypeTraits<typename T::value_type>::SizedName();
    m_error += "'.";
    return false;
  }

  return uploadInternal(&buffer[0], dims, incomingFormat, incomingType,
                        internalFormat);
}

}

#endif
