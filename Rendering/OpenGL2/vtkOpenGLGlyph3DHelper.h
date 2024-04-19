// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLGlyph3DHelper
 * @brief   PolyDataMapper using OpenGL to render.
 *
 * PolyDataMapper that uses a OpenGL to do the actual rendering.
 */

#ifndef vtkOpenGLGlyph3DHelper_h
#define vtkOpenGLGlyph3DHelper_h

#include "vtkNew.h"                   // For vtkNew
#include "vtkOpenGLBufferObject.h"    // For vtkOpenGLBufferObject
#include "vtkOpenGLHelper.h"          // For vtkOpenGLHelper
#include "vtkOpenGLInstanceCulling.h" // For vtkOpenGLInstanceCulling
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkBitArray;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLGlyph3DHelper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLGlyph3DHelper* New();
  vtkTypeMacro(vtkOpenGLGlyph3DHelper, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Fast path for rendering glyphs comprised of only one type of primitive
   * Must set this->CurrentInput explicitly before calling.
   */
  void GlyphRender(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
    std::vector<unsigned char>& colors, std::vector<float>& matrices,
    std::vector<float>& normalMatrices, std::vector<vtkIdType>& pickIds, vtkMTimeType pointMTime,
    bool culling);

  void SetLODs(std::vector<std::pair<float, float>>& lods);

  void SetLODColoring(bool val);

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
  vtkOpenGLGlyph3DHelper();
  ~vtkOpenGLGlyph3DHelper() override = default;

  // special opengl 32 version that uses instances
  void GlyphRenderInstances(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts,
    std::vector<unsigned char>& colors, std::vector<float>& matrices,
    std::vector<float>& normalMatrices, vtkMTimeType pointMTime, bool culling);

  /**
   * Create the basic shaders before replacement
   */
  void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;

  ///@{
  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* actor) override;
  void ReplaceShaderPicking(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderNormal(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderClip(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderPositionVC(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderPointSize(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  ///@}

  /**
   * Set the shader parameters related to the actor/mapper
   */
  void SetMapperShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  void BuildCullingShaders(vtkRenderer* ren, vtkActor* actor, vtkIdType numPts, bool withNormals);

  bool UsingInstancing;

  vtkNew<vtkOpenGLBufferObject> NormalMatrixBuffer;
  vtkNew<vtkOpenGLBufferObject> MatrixBuffer;
  vtkNew<vtkOpenGLBufferObject> ColorBuffer;
  vtkTimeStamp InstanceBuffersBuildTime;
  vtkTimeStamp InstanceBuffersLoadTime;

  std::vector<std::pair<float, float>> LODs;
  vtkNew<vtkOpenGLInstanceCulling> InstanceCulling;

private:
  vtkOpenGLGlyph3DHelper(const vtkOpenGLGlyph3DHelper&) = delete;
  void operator=(const vtkOpenGLGlyph3DHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
