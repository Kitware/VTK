// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkOpenGLBatchedPolyDataMapper
 * @brief An OpenGL mapper for batched rendering of vtkPolyData.
 *
 * The parent class is determined at compile time.
 * On OpenGL ES, the parent class is vtkOpenGLES30PolyDataMapper.
 * Everywhere else, the parent class is vtkOpenGLPolyDataMapper.
 *
 * @sa vtkOpenGLPolyDataMapper vtkOpenGLES30PolyDataMapper vtkOpenGLCompositePolyDataMapperDelegator
 */

#ifndef vtkOpenGLBatchedPolyDataMapper_h
#define vtkOpenGLBatchedPolyDataMapper_h

#include "vtkOpenGLPolyDataMapper.h"

#include "vtkColor.h"                                  // class uses vtkColor
#include "vtkNew.h"                                    // for ivar
#include "vtkOpenGLCompositePolyDataMapperDelegator.h" // for struct BatchElement
#include "vtkRenderingOpenGL2Module.h"                 // for export macro
#include "vtkSmartPointer.h"                           // for arg
#include "vtk_glad.h"                                  // for OpenGL defs

#include <cstdint> // for std::uintptr_t
#include <memory>  // for shared_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositePolyDataMapper;
class vtkPolyData;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLBatchedPolyDataMapper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkOpenGLBatchedPolyDataMapper* New();
  vtkTypeMacro(vtkOpenGLBatchedPolyDataMapper, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * API to add and query a GLBatchElement instance per vtkPolyData.
   */
  using BatchElement = vtkOpenGLCompositePolyDataMapperDelegator::BatchElement;
  using GLBatchElement = vtkOpenGLCompositePolyDataMapperDelegator::GLBatchElement;
  void AddBatchElement(unsigned int flatIndex, BatchElement&& batchElement);
  BatchElement* GetBatchElement(vtkPolyData* polydata);
  void ClearBatchElements();
  ///@}

  /**
   * Accessor to the ordered list of PolyData that we last drew.
   */
  std::vector<vtkPolyData*> GetRenderedList() const;
  void SetParent(vtkCompositePolyDataMapper* parent);

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(vtkRenderer* renderer, vtkActor* actor) override;
  void UnmarkBatchElements();
  void ClearUnmarkedBatchElements();

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  void ProcessSelectorPixelBuffers(
    vtkHardwareSelector* sel, std::vector<unsigned int>& pixeloffsets, vtkProp* prop) override;

  virtual void ProcessCompositePixelBuffers(vtkHardwareSelector* sel, vtkProp* prop,
    GLBatchElement* glBatchElement, std::vector<unsigned int>& mypixels);

  /**
   * Returns the maximum of our and Parent vtkCompositePolyDataMapper's MTime
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkOpenGLBatchedPolyDataMapper();
  ~vtkOpenGLBatchedPolyDataMapper() override;

  void RenderPieceDraw(vtkRenderer* renderer, vtkActor* actor) override;
  void UpdateCameraShiftScale(vtkRenderer* renderer, vtkActor* actoror) override;

  /**
   * Draws primitives
   */
  void DrawIBO(vtkRenderer* renderer, vtkActor* actoror, int primType, vtkOpenGLHelper& CellBO,
    GLenum mode, int pointSize);

  /**
   * Applies rendering attributes for the corresponding polydata in the glBatchElement
   */
  virtual void SetShaderValues(
    vtkShaderProgram* prog, GLBatchElement* glBatchElement, size_t primOffset);

  /**
   * Make sure appropriate shaders are defined, compiled and bound.  This method
   * orchistrates the process, much of the work is done in other methods
   */
  void UpdateShaders(vtkOpenGLHelper& cellBO, vtkRenderer* renderer, vtkActor* actor) override;

  /**
   * Perform string replacements on the shader templates, called from
   * ReplaceShaderValues
   */
  void ReplaceShaderColor(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkRenderer* renderer, vtkActor* actor) override;

  /**
   * Does the VBO/IBO need to be rebuilt
   */
  bool GetNeedToRebuildBufferObjects(vtkRenderer* renderer, vtkActor* actor) override;

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  void BuildBufferObjects(vtkRenderer* renderer, vtkActor* actor) override;
  virtual void AppendOneBufferObject(vtkRenderer* renderer, vtkActor* actor,
    GLBatchElement* glBatchElement, vtkIdType& vertexOffset, std::vector<unsigned char>& colors,
    std::vector<float>& norms);

  /**
   * Build the selection IBOs, called by UpdateBufferObjects
   */
  void BuildSelectionIBO(
    vtkPolyData* poly, std::vector<unsigned int> (&indices)[4], vtkIdType offset) override;

  /**
   * Returns if we can use texture maps for scalar coloring. Note this doesn't
   * say we "will" use scalar coloring. It says, if we do use scalar coloring,
   * we will use a texture. Always off for this mapper.
   */
  int CanUseTextureMapForColoring(vtkDataObject*) override;

  // Reference to CPDM
  vtkCompositePolyDataMapper* Parent = nullptr;
  // Maps an address of a vtkPolyData to its rendering attributes.
  std::map<std::uintptr_t, std::unique_ptr<GLBatchElement>> VTKPolyDataToGLBatchElement;
  std::map<unsigned int, std::uintptr_t> FlatIndexToPolyData;
  // Index arrays for vert, line, poly, strip, edge, stripedge
  std::vector<unsigned int> IndexArray[PrimitiveEnd];
  // Whether primitive IDs are used
  bool PrimIDUsed;
  // Whether override color is used for a vtkPolyData
  bool OverideColorUsed;
  // Reference to the current selector.
  vtkHardwareSelector* CurrentSelector;
  // used by the hardware selector
  std::vector<std::vector<unsigned int>> PickPixels;
  // cached array map
  std::map<vtkAbstractArray*, vtkDataArray*> ColorArrayMap;

private:
  vtkOpenGLBatchedPolyDataMapper(const vtkOpenGLBatchedPolyDataMapper&) = delete;
  void operator=(const vtkOpenGLBatchedPolyDataMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
