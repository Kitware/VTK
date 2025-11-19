// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPURenderTextureCache
 * @brief   Class to create and retrieve render Textures based on a given key.
 *
 * vtkWebGPURenderTextureCache is meant to manage render Textures used in
 * VTK's WebGPU rendering backend.
 *
 * @sa
 * vtkWebGPURenderTextureDeviceResource, vtkWebGPUTexture
 */

#ifndef vtkWebGPURenderTextureCache_h
#define vtkWebGPURenderTextureCache_h

#include "vtkObject.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkSmartPointer.h"          // for vtkSmartPointer
#include "vtkWrappingHints.h"         // For VTK_MARSHALAUTO

#include <memory> // for unique_ptr

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderer;
class vtkWebGPURenderTextureDeviceResource;
class vtkWebGPURenderWindow;
class vtkWindow;

class VTKRENDERINGWEBGPU_EXPORT VTK_MARSHALAUTO vtkWebGPURenderTextureCache : public vtkObject
{
public:
  static vtkWebGPURenderTextureCache* New();
  vtkTypeMacro(vtkWebGPURenderTextureCache, vtkObject);

  /**
   * PrintSelf outputs the cache contents in the following format:
   *   RenderTextureCache:
   *   index: pointer
   *   index: pointer
   *   ...
   * This is useful for debugging and logging the current state of the cache.
   */
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Reset the Texture cache.
   * @param window The window associated with the cache.
   */
  void ReleaseGraphicsResources(vtkWindow* window);

  /**
   * Add a render Texture to the cache and returns its associated index.
   * @param renderTexture The render Texture to add.
   * @return Returns INVALID_TEXTURE_INDEX if the cache is at maximum capacity. Callers must check
   * for INVALID_TEXTURE_INDEX to handle allocation failure.
   */
  int AddRenderTexture(vtkSmartPointer<vtkWebGPURenderTextureDeviceResource> renderTexture);

  /**
   * Get a render Texture associated with the given index.
   * @param index The index of the render Texture to retrieve.
   * @return The render Texture associated with the given index.
   */
  vtkSmartPointer<vtkWebGPURenderTextureDeviceResource> GetRenderTexture(int index);

  /**
   * Remove a render Texture from the cache.
   * @param index The index of the render Texture to remove.
   * @return True if the render Texture was removed, false otherwise.
   */
  bool RemoveRenderTexture(int index);

protected:
  vtkWebGPURenderTextureCache();
  ~vtkWebGPURenderTextureCache() override;

private:
  vtkWebGPURenderTextureCache(const vtkWebGPURenderTextureCache&) = delete;
  void operator=(const vtkWebGPURenderTextureCache&) = delete;

  friend class vtkWebGPUTexture;
  static constexpr int INVALID_TEXTURE_INDEX = -1;
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
