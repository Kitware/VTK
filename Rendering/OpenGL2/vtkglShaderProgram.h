/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkglShaderProgram_h
#define __vtkglShaderProgram_h

#include "vtkRenderingOpenGL2Module.h"
#include "vtkglVector.h" // For API
#include "vtkglMatrix.h" // For API
#include "vtkVector.h" // For API
#include "vtkColor.h" // For API
#include "vtkTypeTraits.h" // For type traits inline template
#include <string> // For member variables.
#include <vector> // For member variables.
#include <map>    // For member variables.

namespace vtkgl {

class Shader;
class Texture2D;

/**
 * @brief The ShaderProgram uses one or more Shader objects.
 *
 * This class creates a Vertex or Fragment shader, that can be attached to a
 * ShaderProgram in order to render geometry etc.
 */

class VTKRENDERINGOPENGL2_EXPORT ShaderProgram
{
public:
  /** Options for attribute normalization. */
  enum NormalizeOption {
    /// The values range across the limits of the numeric type.
    /// This option instructs the rendering engine to normalize them to
    /// the range [0.0, 1.0] for unsigned types, and [-1.0, 1.0] for signed
    /// types.
    /// For example, unsigned char values will be mapped so that 0 = 0.0,
    /// and 255 = 1.0.
    /// The resulting floating point numbers will be passed into
    /// the shader program.
    Normalize,
    /// The values should be used as-is. Do not perform any normalization.
    NoNormalize
  };

  ShaderProgram();
  ~ShaderProgram();

  /**
   * Attach the supplied shader to this program.
   * @note A maximum of one Vertex shader and one Fragment shader can be
   * attached to a shader prorgram.
   * @return true on success.
   */
  bool attachShader(const Shader &shader);

  /** Detach the supplied shader from this program.
   * @note A maximum of one Vertex shader and one Fragment shader can be
   * attached to a shader prorgram.
   * @return true on success.
   */
  bool detachShader(const Shader &shader);

  /**
   * Attempt to link the shader program.
   * @return false on failure. Query error to get the reason.
   * @note The shaders attached to the program must have been compiled.
   */
  bool link();

  /**
   * Bind the program in order to use it. If the program has not been linked
   * then link() will be called.
   */
  bool bind();

  /** Releases the shader program from the current context. */
  void release();

  /** Get the error message (empty if none) for the shader program. */
  std::string error() const { return m_error; }

  /**
   * Enable the named attribute array. Return false if the attribute array is
   * not contained in the linked shader program.
   */
  bool enableAttributeArray(const std::string &name);

  /**
   * Disable the named attribute array. Return false if the attribute array is
   * not contained in the linked shader program.
   */
  bool disableAttributeArray(const std::string &name);

  /**
   * Use the named attribute array with the bound BufferObject.
   * @param name of the attribute (as seen in the shader program).
   * @param offset into the bound BufferObject.
   * @param stride The stride of the element access (i.e. the size of each
   * element in the currently bound BufferObject). 0 may be used to indicate
   * tightly packed data.
   * @param elementType Tag identifying the memory representation of the
   * element.
   * @param elementTupleSize The number of elements per vertex (e.g. a 3D
   * position attribute would be 3).
   * @param normalize Indicates the range used by the attribute data.
   * See NormalizeOption for more information.
   * @return false if the attribute array does not exist.
   */
  bool useAttributeArray(const std::string &name, int offset, size_t stride,
                         int elementType, int elementTupleSize,
                         NormalizeOption normalize);

  /**
   * Upload the supplied array of tightly packed values to the named attribute.
   * BufferObject attributes should be preferred and this may be removed in
   * future.
   *
   * @param name Attribute name
   * @param array Container of data. See note.
   * @param tupleSize The number of elements per vertex, e.g. a 3D coordinate
   * array will have a tuple size of 3.
   * @param  normalize Indicates the range used by the attribute data.
   * See NormalizeOption for more information.
   *
   * @note The T type must have tightly packed values of
   * T::value_type accessible by reference via T::operator[].
   * Additionally, the standard size() and empty() methods must be implemented.
   * The std::vector classes is an example of such a container.
   */
  template <class T>
  bool setAttributeArray(const std::string &name, const T &array,
                         int tupleSize, NormalizeOption normalize);

  /** Set the sampler @a samplerName to use the specified texture. */
  bool setTextureSampler(const std::string &samplerName,
                         const Texture2D &texture);

  /** Set the @p name uniform value to int @p i. */
  bool setUniformValue(const std::string &name, int i);

  /** Set the @p name uniform value to float @p f. */
  bool setUniformValue(const std::string &name, float f);

  /** Set the @p name uniform value to @p matrix. */
  bool setUniformValue(const std::string &name, const Matrix3f &matrix);
  bool setUniformValue(const std::string &name, const Matrix4f &matrix);

  /** Set the @p name uniform value to the supplied value. @{ */
  bool setUniformValue(const std::string &name, const Vector3f &v);
  bool setUniformValue(const std::string &name, const Vector2i &v);
  bool setUniformValue(const std::string &name, const Vector3ub &v);
  /** @} */

protected:
  bool setAttributeArrayInternal(const std::string &name, void *buffer,
                                 int type, int tupleSize,
                                 NormalizeOption normalize);
  int m_handle;
  int m_vertexShader;
  int m_fragmentShader;

  bool m_linked;

  std::string m_error;

  std::map<std::string, int> m_attributes;

  std::map<const Texture2D*, int> m_textureUnitBindings;
  std::vector<bool> m_boundTextureUnits;

private:
  void initializeTextureUnits();
  void releaseAllTextureUnits();
  int findAttributeArray(const std::string &name);
  int findUniform(const std::string &name);
};

template <class T>
inline bool ShaderProgram::setAttributeArray(
    const std::string &name, const T &array, int tupleSize,
    NormalizeOption normalize)
{
  if (array.empty()) {
    m_error = "Refusing to upload empty array for attribute " + name + ".";
    return false;
  }
  int type = vtkTypeTraits<typename T::value_type>::VTKTypeID();
  return setAttributeArrayInternal(name, &array[0], type, tupleSize, normalize);
}

}

#endif
