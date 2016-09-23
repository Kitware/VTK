/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformFeedback.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkTransformFeedback
 * @brief   Manages a TransformFeedback buffer.
 *
 *
 * OpenGL's TransformFeedback allows varying attributes from a vertex/geometry
 * shader to be captured into a buffer for later processing. This is used in
 * VTK to capture vertex information during GL2PS export when using the OpenGL2
 * backend as a replacement for the deprecated OpenGL feedback buffer.
*/

#ifndef vtkTransformFeedback_h
#define vtkTransformFeedback_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

#include <string> // For string
#include <vector> // For vector

class vtkShaderProgram;
class vtkWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkTransformFeedback : public vtkObject
{
public:
  static vtkTransformFeedback *New();
  vtkTypeMacro(vtkTransformFeedback, vtkObject)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * The role a captured varying fills. Useful for parsing later.
   */
  enum VaryingRole
  {
    Vertex_ClipCoordinate_F, // Projected XYZW
    Color_RGBA_F
  };

  struct VaryingMetaData
  {
    VaryingMetaData(VaryingRole role, const std::string &id)
      : Role(role), Identifier(id) {}
    VaryingRole Role;
    std::string Identifier;
  };

  /**
   * Clear the list of varying attributes to capture.
   */
  void ClearVaryings();

  /**
   * Capture the varying 'var' with the indicated role.
   */
  void AddVarying(VaryingRole role, const std::string &var);

  /**
   * Get the list of captured varyings,
   */
  const std::vector<VaryingMetaData>& GetVaryings() const
  {
    return this->Varyings;
  }

  /**
   * Returns the number of data elements each vertex requires for a given role.
   */
  static size_t GetBytesPerVertex(VaryingRole role);

  /**
   * Returns the number of bytes per vertexs, accounting for all roles.
   */
  size_t GetBytesPerVertex() const;

  //@{
  /**
   * The number of vertices expected to be captured. If the drawMode setter is
   * used, PrimitiveMode will also be set appropriately.
   * For the single argument version set function, set the exact number of
   * vertices expected to be emitted, accounting for primitive expansion (e.g.
   * triangle strips -> triangle strips).
   * The two argument setter is for convenience. Given the number of vertices
   * used as input to a draw command and the draw mode, it will calculate the
   * total number of vertices.
   */
  vtkSetMacro(NumberOfVertices, size_t)
  void SetNumberOfVertices(int drawMode, size_t inputVerts);
  vtkGetMacro(NumberOfVertices, size_t)
  //@}

  /**
   * The size (in bytes) of the capture buffer. Available after adding all
   * Varyings and setting NumberOfVertices.
   */
  size_t GetBufferSize() const;

  /**
   * GL_SEPARATE_ATTRIBS is not supported yet.
   * The bufferMode argument to glTransformFeedbackVaryings. Must be
   * GL_INTERLEAVED_ATTRIBS or GL_SEPARATE_ATTRIBS. Default is interleaved. Must
   * be set prior to calling BindVaryings.
   * vtkSetMacro(BufferMode, int)
   * vtkGetMacro(BufferMode, int)
   */

  /**
   * Call glTransformFeedbackVaryings(). Must be called after the shaders are
   * attached to \a prog, but before the program is linked.
   */
  void BindVaryings(vtkShaderProgram *prog);

  //@{
  /**
   * Get the handle to the transform buffer object. Only valid after calling
   * BindBuffer and before ReadBuffer.
   */
  vtkGetMacro(BufferHandle, int)
  //@}

  //@{
  /**
   * The type of primitive to capture. Must be one of GL_POINTS, GL_LINES, or
   * GL_TRIANGLES. Default is GL_POINTS. Must be set prior to calling
   * BindBuffer.
   */
  vtkSetMacro(PrimitiveMode, int)
  vtkGetMacro(PrimitiveMode, int)
  //@}

  /**
   * Generates, binds, and allocates the feedback buffer, then call
   * glBeginTransformFeedback with the specified PrimitiveMode. Must be called
   * after BindVaryings and before any relevant glDraw commands.
   */
  void BindBuffer();

  /**
   * Calls glEndTransformFeedback(), flushes the OpenGL command stream, and
   * reads the transform feedback buffer into BufferData. Must be called after
   * any relevant glDraw commands.
   */
  void ReadBuffer();

  //@{
  /**
   * Get the transform buffer data as a void pointer. Only valid after calling
   * ReadBuffer.
   */
  vtkGetMacro(BufferData, void*)
  //@}

  /**
   * Release any graphics resources used by this object.
   */
  void ReleaseGraphicsResources();

  /**
   * Release the memory used by the buffer data. If freeBuffer == true
   * (default), the data is deleted. If false, the caller is responsible for
   * deleting the BufferData with delete[].
   */
  void ReleaseBufferData(bool freeBuffer = true);

protected:
  vtkTransformFeedback();
  ~vtkTransformFeedback();

private:
  vtkTransformFeedback(const vtkTransformFeedback &) VTK_DELETE_FUNCTION;
  void operator=(const vtkTransformFeedback &) VTK_DELETE_FUNCTION;

  bool VaryingsBound;

  std::vector<VaryingMetaData> Varyings;
  size_t NumberOfVertices;
  int BufferMode;

  int BufferHandle;
  int PrimitiveMode;

  unsigned char *BufferData;
};

//------------------------------------------------------------------------------
inline size_t vtkTransformFeedback::GetBytesPerVertex(
    vtkTransformFeedback::VaryingRole role)
{
  switch (role)
  {
    case Vertex_ClipCoordinate_F:
      return 4 * sizeof(float);
    case Color_RGBA_F:
      return 4 * sizeof(float);
  }

  vtkGenericWarningMacro("Unknown role enum value: " << role);
  return 0;
}

#endif // vtkTransformFeedback_h
