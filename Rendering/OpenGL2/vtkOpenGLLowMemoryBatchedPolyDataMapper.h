// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkOpenGLLowMemoryBatchedPolyDataMapper
 * @brief An OpenGL mapper for batched rendering of vtkPolyData.
 *
 * @sa vtkOpenGLPolyDataMapper vtkOpenGLCompositePolyDataMapperDelegator
 */

#ifndef vtkOpenGLLowMemoryBatchedPolyDataMapper_h
#define vtkOpenGLLowMemoryBatchedPolyDataMapper_h

#include "vtkBoundingBox.h"
#include "vtkOpenGLLowMemoryPolyDataMapper.h"

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

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLLowMemoryBatchedPolyDataMapper
  : public vtkOpenGLLowMemoryPolyDataMapper
{
public:
  static vtkOpenGLLowMemoryBatchedPolyDataMapper* New();
  vtkTypeMacro(vtkOpenGLLowMemoryBatchedPolyDataMapper, vtkOpenGLLowMemoryPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * API to add and query a GLBatchElement instance per vtkPolyData.
   */
  using BatchElement = vtkOpenGLCompositePolyDataMapperDelegator::BatchElement;
  struct GLBatchElement
  {
    BatchElement Parent;
    std::size_t CellGroupId;
  };
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
  vtkOpenGLLowMemoryBatchedPolyDataMapper();
  ~vtkOpenGLLowMemoryBatchedPolyDataMapper() override;

  void RenderPieceDraw(vtkRenderer* renderer, vtkActor* actor) override;

  bool IsDataObjectUpToDate() override;
  vtkDataArray* GetColors(vtkPolyData* mesh) override;
  bool BindArraysToTextureBuffers(vtkRenderer* renderer, vtkActor* actor,
    vtkCellGraphicsPrimitiveMap::CellTypeMapperOffsets& offsets) override;
  bool IsShaderColorSourceUpToDate(vtkActor* actor) override;
  bool IsShaderNormalSourceUpToDate(vtkActor* actor) override;
  void UpdateShaders(vtkRenderer* renderer, vtkActor* actor) override;
  void UpdateShiftScale(vtkRenderer* renderer, vtkActor* actor) override;
  void ReplaceShaderColor(
    vtkRenderer* renderer, vtkActor* actor, std::string& vsSource, std::string& fsSource) override;
  int CanUseTextureMapForColoring(vtkDataObject*) override;

  void DrawPrimitives(vtkRenderer* renderer, vtkActor*, PrimitiveInformation& primitive);
  /**
   * Applies rendering attributes for the corresponding polydata in the glBatchElement
   */
  virtual void SetShaderValues(GLBatchElement* glBatchElement);

  // Reference to CPDM
  vtkCompositePolyDataMapper* Parent = nullptr;
  // Maps an address of a vtkPolyData to its rendering attributes.
  std::map<std::uintptr_t, std::unique_ptr<GLBatchElement>> VTKPolyDataToGLBatchElement;
  // Whether override color is used for a vtkPolyData
  bool OverideColorUsed;
  // Reference to the current selector.
  vtkHardwareSelector* CurrentSelector = nullptr;
  // used by the hardware selector
  std::vector<std::vector<unsigned int>> PickPixels;
  // cached array map
  std::map<vtkAbstractArray*, vtkDataArray*> ColorArrayMap;
  // cached bbox of all points
  vtkBoundingBox PointsBBox;

private:
  vtkOpenGLLowMemoryBatchedPolyDataMapper(const vtkOpenGLLowMemoryBatchedPolyDataMapper&) = delete;
  void operator=(const vtkOpenGLLowMemoryBatchedPolyDataMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
