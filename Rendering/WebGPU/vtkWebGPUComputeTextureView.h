// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputeTextureView_h
#define vtkWebGPUComputeTextureView_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkSetGet.h"                // for get/set macros
#include "vtkWebGPUTextureView.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputeTextureView : public vtkWebGPUTextureView
{
public:
  vtkTypeMacro(vtkWebGPUComputeTextureView, vtkObject);
  static vtkWebGPUComputeTextureView* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkWebGPUComputeTextureView();
  ~vtkWebGPUComputeTextureView() override;

private:
  friend class vtkWebGPUComputePass;
  friend class vtkWebGPUComputePassTextureStorageInternals;

  vtkWebGPUComputeTextureView(const vtkWebGPUComputeTextureView&) = delete;
  void operator=(const vtkWebGPUComputeTextureView&) = delete;

  ///@{
  /**
   * Get/set the index (within the compute pass that created the texture) of the texture that this
   * texture view is a view of.
   */
  vtkGetMacro(AssociatedTextureIndex, int);
  vtkSetMacro(AssociatedTextureIndex, int);
  ///@}

  // Texture index of the texture that this texture view views. This index is only viable in the
  // compute pass that created the texture.
  int AssociatedTextureIndex = -1;
};

VTK_ABI_NAMESPACE_END

#endif
