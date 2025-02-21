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
   * The super class repeatedly renders the frame for different passes.
   * We do not wish to do that as it is quite expensive. This class
   * leverages webgpu features to achieve selection within a single pass.
   */
  bool CaptureBuffers() override;

  /**
   * Called by the mapper before and after
   * rendering each prop.
   */
  void BeginRenderProp() override{};
  void EndRenderProp() override{};

  /**
   * Called by any vtkMapper or vtkProp subclass to render a composite-index.
   * Currently indices >= 0xffffff are not supported.
   */
  void RenderCompositeIndex(unsigned int /*index*/) override{};

  /**
   * Called by any vtkMapper or subclass to render process id. This has any
   * effect when this->UseProcessIdFromData is true.
   */
  void RenderProcessId(unsigned int /*processid*/) override{};

  // we need to initialize the depth buffer
  void BeginSelection() override;
  void EndSelection() override;

  /**
   * returns the prop associated with a ID. This is valid only until
   * ReleasePixBuffers() gets called.
   */
  vtkProp* GetPropFromID(int id) override;

  PixelInformation GetPixelInformation(const unsigned int inDisplayPosition[2], int maxDist,
    unsigned int outSelectedPosition[2]) override;

protected:
  vtkWebGPUHardwareSelector();
  ~vtkWebGPUHardwareSelector() override;

  void PreCapturePass(int /*pass*/) override{};
  void PostCapturePass(int /*pass*/) override{};

  void BeginRenderProp(vtkRenderWindow*) override{};
  void EndRenderProp(vtkRenderWindow*) override{};

  void SavePixelBuffer(int) override{};

  void ReleasePixBuffers() override;

  int Convert(int xx, int yy, unsigned char* pb) override;

private:
  friend class vtkWebGPURenderWindow;
  struct Ids
  {
    vtkTypeUInt32 AttributeId;
    vtkTypeUInt32 PropId;
    vtkTypeUInt32 CompositeId;
    vtkTypeUInt32 ProcessId;
  };

  bool MapReady = false;
  std::vector<Ids> IdBuffer;

  vtkProp** PropArray = nullptr;

  vtkWebGPUHardwareSelector(const vtkWebGPUHardwareSelector&) = delete;
  void operator=(const vtkWebGPUHardwareSelector&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
