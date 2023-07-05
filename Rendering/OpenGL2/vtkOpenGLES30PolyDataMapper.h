// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLES30PolyDataMapper
 * @brief   PolyDataMapper using OpenGLES30 to render surface meshes.
 *
 * This mapper is designed for GLES 3.0 compatibility. Since GLES30 3.0 lacks
 * geometry shaders and texture buffers, `vtkOpenGLPolyDataMapper` will not function
 * correctly when VTK is configured with `VTK_OPENGL_USE_GLES30=ON` since that mapper
 * works with GLES30 >= 3.2 or desktop GL 3.2 contexts
 */

#ifndef vtkOpenGLES30PolyDataMapper_h
#define vtkOpenGLES30PolyDataMapper_h

#include "vtkOpenGLPolyDataMapper.h"

#include "vtkNew.h"                    // for ivar
#include "vtkRenderingOpenGL2Module.h" // For export macro

#include <vector> // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLCellToVTKCellMap;
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLES30PolyDataMapper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLES30PolyDataMapper* New();
  vtkTypeMacro(vtkOpenGLES30PolyDataMapper, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void RenderPieceStart(vtkRenderer* ren, vtkActor* act) override;
  void RenderPieceDraw(vtkRenderer* ren, vtkActor* act) override;
  void RenderPieceFinish(vtkRenderer* ren, vtkActor* act) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkOpenGLES30PolyDataMapper();
  ~vtkOpenGLES30PolyDataMapper() override;

  /**
   * Create the basic shaders before replacement
   */
  void GetShaderTemplate(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;

  ///@{
  /**
   * Perform string replacements on the shader templates, called from
   * ReplaceShaderValues
   */
  void ReplaceShaderValues(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderNormal(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderCoincidentOffset(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderEdges(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  void ReplaceShaderPicking(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act) override;
  /**
   * In GLES 3.0, point size is set from the vertex shader.
   */
  void ReplaceShaderPointSize(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  /**
   * GLES 3.0 does not support wide lines (width > 1). Shader computations combined with
   * instanced rendering is used to emulate wide lines.
   */
  void ReplaceShaderWideLines(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* ren, vtkActor* act);
  ///@}

  /**
   * Set the shader parameters related to the mapper/input data, called by UpdateShader
   */
  void SetMapperShaderParameters(vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  /**
   * Set the shader parameters related to the property, called by UpdateShader
   */
  void SetPropertyShaderParameters(
    vtkOpenGLHelper& cellBO, vtkRenderer* ren, vtkActor* act) override;

  /**
   * Build the VBO, called by UpdateBufferObjects
   */
  void BuildBufferObjects(vtkRenderer* ren, vtkActor* act) override;
  void AppendOneBufferObject(vtkRenderer* ren, vtkActor* act, vtkPolyData* polydata,
    vtkOpenGLCellToVTKCellMap* prim2cellMap, vtkIdType& voffset);

  /**
   * Compute and set the maximum point and cell ID used in selection
   */
  void UpdateMaximumPointCellIds(vtkRenderer* ren, vtkActor* actor) override;

  /**
   * Get flat 0-based indices that form GL primitives for given vtk-cell connectivity and actor
   * representation.
   */
  static void BuildIndexArrays(std::vector<unsigned int> (&indexArrays)[PrimitiveEnd],
    std::vector<unsigned char>& edgeArray, vtkCellArray* prims[4], vtkPoints* points,
    int representation, bool draw_surf_with_edges = false, bool vertex_visibility = false,
    vtkDataArray* ef = nullptr);

  bool DrawingPoints(vtkActor* actor);
  bool DrawingLines(vtkActor* actor);

  vtkNew<vtkOpenGLVertexBufferObjectGroup> PrimitiveVBOGroup[PrimitiveEnd];
  std::vector<unsigned int> PrimitiveIndexArrays[PrimitiveEnd];

private:
  vtkOpenGLES30PolyDataMapper(const vtkOpenGLES30PolyDataMapper&) = delete;
  void operator=(const vtkOpenGLES30PolyDataMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLES30PolyDataMapper_h
