// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUHardwareSelector
 * @brief   implements the device specific code of
 *  vtkWebGPUHardwareSelector.
 *
 *
 * Implements the device specific code of vtkWebGPUHardwareSelector.
 *
 * @sa
 * vtkHardwareSelector
 */

#ifndef vtkWebGPUHardwareSelector_h
#define vtkWebGPUHardwareSelector_h

#include "vtkHardwareSelector.h"

#include "vtkRenderingWebGPUModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUHardwareSelector : public vtkHardwareSelector
{
public:
  static vtkWebGPUHardwareSelector* New();
  vtkTypeMacro(vtkWebGPUHardwareSelector, vtkHardwareSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Called by the mapper before and after
   * rendering each prop.
   */
  void BeginRenderProp() override;
  void EndRenderProp() override;

  /**
   * Called by any vtkMapper or vtkProp subclass to render a composite-index.
   * Currently indices >= 0xffffff are not supported.
   */
  void RenderCompositeIndex(unsigned int index) override;

  /**
   * Called by any vtkMapper or subclass to render process id. This has any
   * effect when this->UseProcessIdFromData is true.
   */
  void RenderProcessId(unsigned int processid) override;

  // we need to initialize the depth buffer
  void BeginSelection() override;
  void EndSelection() override;

protected:
  vtkWebGPUHardwareSelector();
  ~vtkWebGPUHardwareSelector() override;

  void PreCapturePass(int pass) override;
  void PostCapturePass(int pass) override;

  // Called internally before each prop is rendered
  // for device specific configuration/preparation etc.
  void BeginRenderProp(vtkRenderWindow*) override;
  void EndRenderProp(vtkRenderWindow*) override;

  void SavePixelBuffer(int passNo) override;

  int OriginalMultiSample;
  bool OriginalBlending;

private:
  vtkWebGPUHardwareSelector(const vtkWebGPUHardwareSelector&) = delete;
  void operator=(const vtkWebGPUHardwareSelector&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
