// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkOpenGLCompositePolyDataMapperDelegator
 * @brief An OpenGL delegator for batched rendering of multiple polydata with similar structure.
 *
 * This class delegates work to vtkOpenGLBatchedPolyDataMapper which can do batched rendering
 * of many polydata.
 *
 * @sa vtkOpenGLBatchedPolyDataMapper
 */

#ifndef vtkOpenGLCompositePolyDataMapperDelegator_h
#define vtkOpenGLCompositePolyDataMapperDelegator_h

#include "vtkCompositePolyDataMapperDelegator.h"

#include "vtkNew.h"                    // for ivar
#include "vtkOpenGLPolyDataMapper.h"   // for PrimitiveEnd
#include "vtkRenderingOpenGL2Module.h" // for export macro

#include "vtk_glad.h"

VTK_ABI_NAMESPACE_BEGIN
#ifdef GL_ES_VERSION_3_0
class vtkOpenGLLowMemoryBatchedPolyDataMapper;
#define GLDelegateClass vtkOpenGLLowMemoryBatchedPolyDataMapper
#else
class vtkOpenGLBatchedPolyDataMapper;
#define GLDelegateClass vtkOpenGLBatchedPolyDataMapper
#endif
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLCompositePolyDataMapperDelegator
  : public vtkCompositePolyDataMapperDelegator
{
public:
  static vtkOpenGLCompositePolyDataMapperDelegator* New();
  vtkTypeMacro(vtkOpenGLCompositePolyDataMapperDelegator, vtkCompositePolyDataMapperDelegator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // This class extends the rendering attributes for a vtkPolyData with OpenGL specifics.
  using BatchElement = vtkCompositePolyDataMapperDelegator::BatchElement;
  struct GLBatchElement
  {
    BatchElement Parent;

    unsigned int StartVertex;
    unsigned int NextVertex;

    // point line poly strip edge stripedge
    unsigned int StartIndex[vtkOpenGLPolyDataMapper::PrimitiveEnd];
    unsigned int NextIndex[vtkOpenGLPolyDataMapper::PrimitiveEnd];

    // stores the mapping from vtk cells to gl_PrimitiveId
    vtkNew<vtkOpenGLCellToVTKCellMap> CellCellMap;
  };

  ///@{
  /**
   * Implement parent class API.
   */
  // Copy array names used for selection. Ex: PointIdArrayName, CompositeIdArrayName, ..
  void ShallowCopy(vtkCompositePolyDataMapper* mapper) override;
  void ClearUnmarkedBatchElements() override;
  void UnmarkBatchElements() override;
  ///@}

protected:
  vtkOpenGLCompositePolyDataMapperDelegator();
  ~vtkOpenGLCompositePolyDataMapperDelegator() override;

  ///@{
  /**
   * Implement parent class API.
   */
  std::vector<vtkPolyData*> GetRenderedList() const override;
  void SetParent(vtkCompositePolyDataMapper* mapper) override;
  void Insert(BatchElement&& item) override;
  BatchElement* Get(vtkPolyData* polydata) override;
  void Clear() override;
  ///@}

  // The actual mapper which renders multiple vtkPolyData.
  // Constructor assigns it to vtkBatchedPolyDataMapperDelegator::Delegate.
  // From that point on, parent class manages lifetime of this GLDelegate.
  // Instead of repeatedly downcasting the abstract Delegate, we trampoline
  // calls to GLDelegate.
  GLDelegateClass* GLDelegate = nullptr;

private:
  vtkOpenGLCompositePolyDataMapperDelegator(
    const vtkOpenGLCompositePolyDataMapperDelegator&) = delete;
  void operator=(const vtkOpenGLCompositePolyDataMapperDelegator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
