// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkArrayRenderer
 * @brief   Render instanced elements textured with arrays from input data.
 *
 * This currently handles hexahedra and tetrahedra.
 */

#ifndef vtkArrayRenderer_h
#define vtkArrayRenderer_h

#include "vtkRenderingOpenGL2Module.h" // For export macro

#include "vtkDrawTexturedElements.h" // Inherited helper class.
#include "vtkMapper.h"

#include <set>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGOPENGL2_EXPORT vtkArrayRenderer
  : public vtkMapper
  , public vtkDrawTexturedElements
{
public:
  vtkTypeMacro(vtkArrayRenderer, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkArrayRenderer* New();

  /// Prepare a colormap for use in a shader.
  ///
  /// If you provide a lookup table, it will be uploaded as a 2-D texture
  /// named "color_map" for you to use in your shaders.
  /// If not, a default cool-to-warm colormap will be created.
  ///
  /// This function may call CreateColormapTexture().
  void PrepareColormap(vtkScalarsToColors* cmap = nullptr);

  using vtkMapper::GetBounds;
  double* GetBounds() VTK_SIZEHINT(6) override;

  /// Declare a new enum that inherits values from the base class
  /// so that python wrappings are generated. Otherwise, we would
  /// just use `using vtkDrawTexturedElements::ElementShape;`.
  enum ElementShape
  {
    Point = vtkDrawTexturedElements::ElementShape::Point,
    Line = vtkDrawTexturedElements::ElementShape::Line,
    LineStrip = vtkDrawTexturedElements::ElementShape::LineStrip,
    Triangle = vtkDrawTexturedElements::ElementShape::Triangle,
    TriangleStrip = vtkDrawTexturedElements::ElementShape::TriangleStrip,
    TriangleFan = vtkDrawTexturedElements::ElementShape::TriangleFan
  };

  using vtkDrawTexturedElements::BindArrayToTexture;
  using vtkDrawTexturedElements::GetShader;
  using vtkDrawTexturedElements::SetElementType;
  using vtkDrawTexturedElements::SetNumberOfElements;
  using vtkDrawTexturedElements::SetNumberOfInstances;

  /// Render geometry.
  ///
  /// This just calls glDrawElementInstanced().
  void Render(vtkRenderer* ren, vtkActor* a) override;

  /// Release any graphics resources associated with the \a window.
  void ReleaseGraphicsResources(vtkWindow* window) override;

  /// Set/get whether the data will render any fully opaque primitives.
  vtkGetMacro(HasOpaque, vtkTypeBool);
  vtkSetMacro(HasOpaque, vtkTypeBool);

  /// Set/get whether the data will render any semi-transparent (i.e., translucent) primitives.
  vtkGetMacro(HasTranslucent, vtkTypeBool);
  vtkSetMacro(HasTranslucent, vtkTypeBool);

  bool HasOpaqueGeometry() override { return this->HasOpaque; }
  bool HasTranslucentPolygonalGeometry() override { return this->HasTranslucent; }

  /// Set/get the source for the vertex shader.
  ///
  /// This is not identical to the source sent to OpenGL; there may be
  /// replacements made by any attached GLSL modifier objects.
  vtkSetStdStringFromCharMacro(VertexShaderSource);
  vtkGetCharFromStdStringMacro(VertexShaderSource);

  /// Set/get the source for the fragment shader.
  ///
  /// This is not identical to the source sent to OpenGL; there may be
  /// replacements made by any attached GLSL modifier objects.
  vtkSetStdStringFromCharMacro(FragmentShaderSource);
  vtkGetCharFromStdStringMacro(FragmentShaderSource);

  void ResetModsToDefault();
  void AddMod(const std::string& className);
  void AddMods(const std::vector<std::string>& classNames);
  void RemoveMod(const std::string& className);
  void RemoveAllMods();

protected:
  vtkArrayRenderer();
  ~vtkArrayRenderer() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void CreateColormapTexture();

  bool IsUpToDate(vtkRenderer* renderer, vtkActor* actor);
  void PrepareToRender(vtkRenderer* renderer, vtkActor* actor);

  vtkTypeBool HasOpaque{ true };
  vtkTypeBool HasTranslucent{ false };

  vtkTimeStamp RenderTimeStamp;
  std::string VertexShaderSource;
  std::string FragmentShaderSource;

  /// @name Mods
  /// These are the names of classes which are subclasess of vtkGLSLRuntimeModBase.
  /// The mods will be loaded one by one and applied in the order they were added.
  std::vector<std::string> ModNames;
  std::set<std::string> ModNamesUnique;
  static std::vector<std::string> DefaultModNames;

private:
  vtkArrayRenderer(const vtkArrayRenderer&) = delete;
  void operator=(const vtkArrayRenderer&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkArrayRenderer_h
// VTK-HeaderTest-Exclude: vtkArrayRenderer.h
