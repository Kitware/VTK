// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUTexture
 * @brief   vtkWebGPUTexture is a concrete implementation of the abstract class
 * vtkTexture for WebGPU.
 */
#ifndef vtkWebGPUTexture_h
#define vtkWebGPUTexture_h

#include "vtkTexture.h"

#include "vtkRenderingWebGPUModule.h"             // For export macro
#include "vtkWeakPointer.h"                       // for weak pointer
#include "vtkWebGPURenderTextureDeviceResource.h" // for vtkWebGPURenderTextureDeviceResource
#include "vtkWrappingHints.h"                     // For VTK_MARSHALAUTO
#include "vtk_wgpu.h"                             // for webgpu

#include <array> // for std::array
#include <tuple> // for std::tuple

VTK_ABI_NAMESPACE_BEGIN
class vtkOverrideAttribute;
class vtkRenderWindow;
class vtkWebGPURenderTextureDeviceResource;
class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPUTexture : public vtkTexture
{
public:
  static vtkWebGPUTexture* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkWebGPUTexture, vtkTexture);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Renders a texture map. It first checks the object's modified time
   * to make sure the texture maps Input is valid, then it invokes the
   * Load() method.
   */
  void Render(vtkRenderer* ren) override;

  /**
   * Implement base class method.
   */
  void Load(vtkRenderer*) override;

  /**
   * Clean up after the rendering is complete.
   */
  void PostRender(vtkRenderer*) override;

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release. Using the same texture object in multiple
   * render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * TODO
   */
  int GetTextureUnit() override;

  /**
   * TODO
   */
  int IsTranslucent() override;

  vtkSmartPointer<vtkWebGPURenderTextureDeviceResource> GetDeviceResource() const;

protected:
  vtkWebGPUTexture();
  ~vtkWebGPUTexture() override;

private:
  vtkWebGPUTexture(const vtkWebGPUTexture&) = delete;
  void operator=(const vtkWebGPUTexture&) = delete;

  vtkTimeStamp LoadTime;
  vtkWeakPointer<vtkRenderWindow> RenderWindow;
  int TextureIndex;

  bool IsValidTextureIndex();
  bool CanDataPlaneBeUsedDirectlyAsColors(vtkDataArray* inputScalars);
  std::tuple<vtkWebGPURenderTextureDeviceResource::TextureDimension, std::array<unsigned int, 3>>
  GetTextureDimensionAndSize(vtkImageData* imageData);
  vtkWebGPURenderTextureDeviceResource::TextureFormat GetTextureFormatFromImageData(
    int numComponents, int dataType);
};
#define vtkWebGPUTexture_OVERRIDE_ATTRIBUTES vtkWebGPUTexture::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
