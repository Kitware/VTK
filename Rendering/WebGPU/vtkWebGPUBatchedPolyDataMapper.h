// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkWebGPUBatchedPolyDataMapper
 * @brief A WebGPU mapper for batched rendering of vtkPolyData.
 *
 * @sa vtkWebGPUPolyDataMapper vtkWebGPUCompositePolyDataMapperDelegator
 */

#ifndef vtkWebGPUBatchedPolyDataMapper_h
#define vtkWebGPUBatchedPolyDataMapper_h

#include "vtkWebGPUPolyDataMapper.h"

#include "vtkRenderingWebGPUModule.h"                  // for export macro
#include "vtkWebGPUCompositePolyDataMapperDelegator.h" // for struct BatchElement

#include <cstdint> // for std::uintptr_t
#include <memory>  // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositePolyDataMapper;
class vtkPolyData;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUBatchedPolyDataMapper : public vtkWebGPUPolyDataMapper
{
public:
  static vtkWebGPUBatchedPolyDataMapper* New();
  vtkTypeMacro(vtkWebGPUBatchedPolyDataMapper, vtkWebGPUPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * API to add and query a BatchElement instance per vtkPolyData.
   */
  using BatchElement = vtkWebGPUCompositePolyDataMapperDelegator::BatchElement;
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
   * Returns the maximum of our and Parent vtkCompositePolyDataMapper's MTime
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkWebGPUBatchedPolyDataMapper();
  ~vtkWebGPUBatchedPolyDataMapper() override;

  // Reference to CPDM
  vtkCompositePolyDataMapper* Parent = nullptr;
  // Maps an address of a vtkPolyData to its rendering attributes.
  std::map<std::uintptr_t, std::unique_ptr<BatchElement>> VTKPolyDataToBatchElement;
  std::map<unsigned int, std::uintptr_t> FlatIndexToPolyData;

  // Upload timestamp of override colors.
  vtkTimeStamp OverrideColorUploadTimestamp;
  bool LastBlockVisibility = true;
  bool LastUseNanColor = false;

  void UpdateMeshDescriptor(vtkSmartPointer<vtkWebGPUConfiguration> wgpuConfiguration,
    bool applyOverrides, double overrideOpacity, const vtkColor3d& overrideAmbientColor,
    const vtkColor3d& overrideDiffuseColor, vtkTypeUInt32 compositeId, bool pickable);

private:
  vtkWebGPUBatchedPolyDataMapper(const vtkWebGPUBatchedPolyDataMapper&) = delete;
  void operator=(const vtkWebGPUBatchedPolyDataMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
