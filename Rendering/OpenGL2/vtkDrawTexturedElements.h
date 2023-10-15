// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDrawTexturedElements
 * @brief   A base class for mappers or render responders that need to
 *          draw primitives via vertex-pulling.
 *
 * Since this class is intended to be inherited by other classes that
 * must *also* inherit a VTK mapper or responder class, it is not a
 * subclass of vtkObject; instead, it provides methods your subclass
 * can invoke during rendering.
 *
 * This currently handles hexahedra and tetrahedra.
 */
#ifndef vtkDrawTexturedElements_h
#define vtkDrawTexturedElements_h

#include "vtkRenderingOpenGL2Module.h" // For export macro

#include "vtkMatrix3x3.h"                       // for ivar
#include "vtkMatrix4x4.h"                       // for ivar
#include "vtkNew.h"                             // for ivar
#include "vtkOpenGLArrayTextureBufferAdapter.h" // for ivar
#include "vtkOpenGLBufferObject.h"              // for ObjectType
#include "vtkOpenGLTexture.h"                   // for ivar
#include "vtkOpenGLVertexArrayObject.h"         // for ivar
#include "vtkShader.h"                          // for ivar
#include "vtkSmartPointer.h"                    // for ivar
#include "vtkStringToken.h"                     // for passing shader and array names

#include <unordered_map>

VTK_ABI_NAMESPACE_BEGIN

class vtkActor;
class vtkDataArray;
class vtkGLSLRuntimeModBase;
class vtkMapper;
class vtkMatrix3x3;
class vtkMatrix4x4;
class vtkOpenGLRenderWindow;
class vtkOpenGLVertexArrayObject;
class vtkOpenGLTexture;
class vtkRenderer;
class vtkScalarsToColors;
class vtkShaderProgram;
class vtkTextureObject;
class vtkWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkDrawTexturedElements
{
public:
  vtkDrawTexturedElements();
  virtual ~vtkDrawTexturedElements();

  /// The type of primitive to output.
  enum ElementShape
  {
    Point,         //!< Send points to the shader.
    Line,          //!< Send line segments to the shader.
    LineStrip,     //!< Send line segments to the shader.
    Triangle,      //!< Send triangles to the shader.
    TriangleStrip, //!< Send triangles to the shader (moving window of 3 vertices).
    TriangleFan    //!< Send triangle fans to the shader (first vertex is constant).
  };

  /// Return a shader of the given type (creating as needed).
  vtkShader* GetShader(vtkShader::Type shaderType);

  /// Bind a data array to the given \a textureName (used in shader program texelFetch calls).
  ///
  /// If \a asScalars is false (the default), then the array's components are treated as
  /// components of single texture values. If \a asScalars is true, then a 2-d texture
  /// image is uploaded where each value is a scalar (row indices are tuple IDs, column
  /// indices are component IDs).
  void BindArrayToTexture(vtkStringToken textureName, vtkDataArray* array, bool asScalars = false);
  void AppendArrayToTexture(
    vtkStringToken textureName, vtkDataArray* array, bool asScalars = false);
  bool UnbindArray(vtkStringToken);

  /// Set/get the number of element instances to draw.
  vtkIdType GetNumberOfInstances() { return this->NumberOfInstances; }
  virtual bool SetNumberOfInstances(vtkIdType numberOfInstances);

  /// Set/get the number of elements (primitives) to draw per instance.
  vtkIdType GetNumberOfElements() { return this->NumberOfElements; }
  virtual bool SetNumberOfElements(vtkIdType numberOfElements);

  ///@{
  /// Set/get the type of elements to draw.
  ///
  /// This determines the number of vertices rendered per element.
  /// Values must come from the ElementShape enum;
  /// the default is ElementShape::TriangleStrip.
  int GetElementType() { return this->ElementType; }
  virtual bool SetElementType(int elementType);
  ///@}

  ///@{
  /// Set/get whether to upload a colormap texture.
  ///
  /// If enabled (the default), then create (if needed) and
  /// upload a colormap texture image bound to a "color_map" uniform sampler.
  bool GetIncludeColormap() { return this->IncludeColormap; }
  virtual bool SetIncludeColormap(bool includeColormap);
  ///@}

  /// Render geometry.
  ///
  /// This just calls glDrawElementInstanced().
  void DrawInstancedElements(vtkRenderer* ren, vtkActor* a, vtkMapper* mapper);

  /// Release any graphics resources associated with the \a window.
  void ReleaseResources(vtkWindow* window);

  /// Return the internal shader program so subclasses can create/replace shaders.
  vtkShaderProgram* GetShaderProgram();

  /// Return the GLSL mods.
  vtkCollection* GetGLSLModCollection() const;

protected:
  /// Set any custom uniforms provided by the actor.
  void SetCustomUniforms(vtkRenderer* ren, vtkActor* a);
  void ReadyShaderProgram(vtkRenderer* ren);
  void PreDraw(vtkRenderer* ren, vtkActor* a, vtkMapper* mapper);
  void DrawInstancedElementsImpl(vtkRenderer* ren, vtkActor* a, vtkMapper* mapper);
  void PostDraw(vtkRenderer* ren, vtkActor* a, vtkMapper* mapper);

  using ShaderMap = std::map<vtkShader::Type, vtkShader*>;
  struct Internal;

  /// Private data for this class.
  Internal* P{ nullptr };
  vtkIdType FirstVertexId{ 0 };
  vtkIdType NumberOfInstances{ 1 };
  vtkIdType NumberOfElements{ 1 };
  int ElementType{ ElementShape::TriangleStrip };
  bool IncludeColormap{ true };
  std::unordered_map<vtkStringToken, vtkOpenGLArrayTextureBufferAdapter> Arrays;
  ShaderMap Shaders;
  vtkSmartPointer<vtkShaderProgram> ShaderProgram;
  vtkNew<vtkOpenGLVertexArrayObject> VAO;
  vtkNew<vtkOpenGLTexture> ColorTextureGL;
  vtkNew<vtkCollection> GLSLMods;

private:
  vtkDrawTexturedElements(const vtkDrawTexturedElements&) = delete;
  void operator=(const vtkDrawTexturedElements&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkDrawTexturedElements_h

// Need to skip header testing since we do not inherit vtkObject:
// VTK-HeaderTest-Exclude: vtkDrawTexturedElements.h
