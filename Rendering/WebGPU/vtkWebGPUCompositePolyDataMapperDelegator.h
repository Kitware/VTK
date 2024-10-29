// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkWebGPUCompositePolyDataMapperDelegator
 * @brief An OpenGL delegator for batched rendering of multiple polydata with similar structure.
 *
 * This class delegates work to vtkOpenGLBatchedPolyDataMapper which can do batched rendering
 * of many polydata.
 *
 * @sa vtkOpenGLBatchedPolyDataMapper
 */

#ifndef vtkWebGPUCompositePolyDataMapperDelegator_h
#define vtkWebGPUCompositePolyDataMapperDelegator_h

#include "vtkCompositePolyDataMapperDelegator.h"

#include "vtkRenderingWebGPUModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUBatchedPolyDataMapper;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUCompositePolyDataMapperDelegator
  : public vtkCompositePolyDataMapperDelegator
{
public:
  static vtkWebGPUCompositePolyDataMapperDelegator* New();
  vtkTypeMacro(vtkWebGPUCompositePolyDataMapperDelegator, vtkCompositePolyDataMapperDelegator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using BatchElement = vtkCompositePolyDataMapperDelegator::BatchElement;

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
  vtkWebGPUCompositePolyDataMapperDelegator();
  ~vtkWebGPUCompositePolyDataMapperDelegator() override;

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
  // From that point on, parent class manages lifetime of this WebGPUDelegate.
  // Instead of repeatedly downcasting the abstract Delegate, we trampoline
  // calls to WebGPUDelegate.
  vtkWebGPUBatchedPolyDataMapper* WebGPUDelegate = nullptr;

private:
  vtkWebGPUCompositePolyDataMapperDelegator(
    const vtkWebGPUCompositePolyDataMapperDelegator&) = delete;
  void operator=(const vtkWebGPUCompositePolyDataMapperDelegator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
