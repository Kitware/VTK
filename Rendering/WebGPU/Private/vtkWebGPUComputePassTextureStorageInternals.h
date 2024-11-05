// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePassTextureStorageInternals_h
#define vtkWebGPUComputePassTextureStorageInternals_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"               // for smart pointers
#include "vtkWeakPointer.h"                // for the weak pointer of the parent compute pass
#include "vtkWebGPUComputePass.h"          // for enum
#include "vtkWebGPUComputeRenderTexture.h" // for compute render textures
#include "vtkWebGPUComputeTexture.h"       // for compute textures
#include "vtkWebGPUComputeTextureView.h"   // for compute texture views
#include "vtkWebGPUConfiguration.h"        // for ivar
#include "vtk_wgpu.h"                      // for webgpu

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUComputePass;

/**
 * This class manages the creation/deletion/recreation/ of compute textures used by
 * a compute pass.
 *
 * A compute pass delegates calls that want to modify textures to this class.
 */
class vtkWebGPUComputePassTextureStorageInternals : public vtkObject
{
public:
  static vtkWebGPUComputePassTextureStorageInternals* New();
  vtkTypeMacro(vtkWebGPUComputePassTextureStorageInternals, vtkObject);

  /**
   * Sets the device that will be used by this texture storage when creating textures / texture
   * views.
   *
   * This device must be the one used by the parent compute pass.
   */
  vtkSetSmartPointerMacro(ParentPassWGPUConfiguration, vtkWebGPUConfiguration);

  /**
   * Sets the compute pass that uses the textures and texture views used by this storage
   */
  void SetComputePass(vtkWeakPointer<vtkWebGPUComputePass> parentComputePass);

  /**
   * Checks if a given index is suitable for indexing this->Textures. Logs an error if the index is
   * negative or greater than the number of texture of the compute pass. The callerFunctionName
   * parameter is using to give more information on what function used an invalid texture index
   *
   * Returns true if the texture index is valid, false if it's not.
   */
  bool CheckTextureIndex(std::size_t textureIndex, const std::string& callerFunctionName);

  /**
   * Checks if a given index is suitable for indexing this->TextureViews. Logs an error if the index
   * is negative or greater than the number of texture views of the compute pass. The
   * callerFunctionName parameter is using to give more information on what function used an invalid
   * texture view index
   *
   * Returns true if the texture view index is valid, false if it's not.
   */
  bool CheckTextureViewIndex(std::size_t textureViewIndex, const std::string& callerFunctionName);

  /**
   * Makes sure the texture is correct with regards to its properties (size, ...)
   */
  bool CheckTextureCorrectness(vtkWebGPUComputeTexture* texture);

  /**
   * Makes sure the texture view is correct with regards to its properties (binding, group, ...)
   */
  bool CheckTextureViewCorrectness(vtkWebGPUComputeTextureView* textureView);

  /**
   * Checks whether or not the associated ParentComputePass and ParentPassDevice are non-null.
   */
  bool CheckParentComputePass(const std::string& callerFunctionName);

  /**
   * Destroys and recreates the texture with the given index.
   */
  void RecreateTexture(std::size_t textureIndex);

  /**
   * Retrieves the compute texture associated with the given texture index
   *
   * @warning The texture will need to be recreated by calling RecreateComputeTexture for all the
   * changes done to this compute texture to take effect
   */
  vtkSmartPointer<vtkWebGPUComputeTexture> GetComputeTexture(std::size_t textureIndex);

  /**
   * Retrieves the texture view associated with the given texture view index
   *
   * @warning The texture view will need to be recreated by calling RecreateTextureView() for all
   * the changes done to this texture view to take effect
   */
  vtkSmartPointer<vtkWebGPUComputeTextureView> GetTextureView(std::size_t textureViewIndex);

  /**
   * Recreates a compute texture. Must be called if the compute texture has been modified (after a
   * call to GetComputeTexure for example) for the  changes to take effect.
   *
   * This function also recreate the texture views that existed on the texture.
   */
  void RecreateComputeTexture(std::size_t textureIndex);

  /**
   * Recreates all the texture views of a texture given its index.
   *
   * Useful when a texture has been recreated, meaning that the associated wgpu::Texture
   * has changed --> the texture view do not point to a correct texture anymore and need to be
   * recreated.
   */
  void RecreateTextureViews(std::size_t textureIndex);

  /**
   * Recreates a compute texture view. Must be called if the compute texture view has been modified
   * (after a call to GetTextureView for example) for the  changes to take effect.
   */
  void RecreateTextureView(std::size_t textureViewIndex);

  /**
   * Utilitary method to create a wgpu::TextureView from a ComputeTextureView and the texture this
   * wgpu::TextureView is going to be a view off
   */
  wgpu::TextureView CreateWebGPUTextureView(
    vtkSmartPointer<vtkWebGPUComputeTextureView> textureView, wgpu::Texture wgpuTexture);

  /**
   * Makes sure that the compute texture given in parameter internally points to the given
   * newWgpuTexture. If this is not initially the case, it will be true after the call to this
   * function. Also, all texture views of this texture will now be views of the given newWgpuTexture
   *
   * This is useful when recreating the compute texture from another compute pass: the compute
   * pipeline will be responsible for calling on all its compute passes (which will call the texture
   * storages) to make sure that if a compute pass was using the texture that was recreated, it now
   * uses the recreated texture and not the old one
   */
  void UpdateComputeTextureAndViews(
    vtkSmartPointer<vtkWebGPUComputeTexture> texture, wgpu::Texture newWgpuTexture);

  /**
   * Adds a render texture to the storage.
   *
   * A render texture can be obtained from
   * vtkWebGPURenderWindow::AcquireDepthBufferRenderTexture() and analogous methods.
   *
   * The main difference in terms of implementation between this method and AddTexture() is that
   * AddRenderTexture() does not create a wgpu::Texture object since the render texture already
   * contains the texture object (configured when AcquireXXXRenderTexture() was called).
   */
  int AddRenderTexture(vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture);

  /**
   * Adds a texture to the storage and upload its data to the device
   *
   * Returns the index of the texture that can for example be used as input to the
   * ReadTextureFromGPU() function
   */
  int AddTexture(vtkSmartPointer<vtkWebGPUComputeTexture> texture);

  /**
   * Adds a texture view to the compute pass and returns its index
   */
  int AddTextureView(vtkSmartPointer<vtkWebGPUComputeTextureView> textureView);

  /**
   * Returns a new texture view on the given texture (given by the index) that can be configured and
   * then added to the compute pass by AddTextureView()
   */
  vtkSmartPointer<vtkWebGPUComputeTextureView> CreateTextureView(std::size_t textureIndex);

  /**
   * Recreates a render texture given a new textureView and possibly new parameters as specified in
   * the 'renderTexture' parameter. This also recreates the texture views that were created on this
   * render texture.
   *
   * This function is mainly called after the render window has been resized and render textures
   * have thus also been resized.
   */
  void RecreateRenderTexture(vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture);

  /**
   * Deletes all the texture views of a given texture (given by its index)
   */
  void DeleteTextureViews(std::size_t textureIndex);

  /**
   * This function allows the usage of multiple texture views on a single binding point (group /
   * binding combination) in the shader (although not at the same time). It acts as AddTextureView()
   * if no texture view was bound to the group/binding in the first place.
   *
   * For example, consider that your shader has the following binding:
   * \@group(0) \@binding(0) var inputTexture: texture_2d<f32>;
   *
   * Depending on your needs, you may want to execute a compute pass twice but with a different
   * texture as input to the shader each time. To achieve that, you would create 2 TextureViews on
   * the 2 Textures that you want your shader to manipulate and call RebindTextureView() on your
   * second texture view index before Dispatching the second compute pass so that the shader samples
   * the second texture (through the second texture view that has been rebound thanks to this
   * function).
   */
  void RebindTextureView(std::size_t group, uint32_t binding, std::size_t textureViewIndex);

  /*
   * This function maps the buffer, making it accessible to the CPU. This is
   * an asynchronous operation, meaning that the given callback will be called
   * when the mapping is done.
   *
   * The buffer data can then be read from the callback and stored
   * in a buffer (std::vector<>, vtkDataArray, ...) passed in via the userdata pointer for example
   */
  void ReadTextureFromGPU(std::size_t textureIndex, int mipLevel,
    vtkWebGPUComputePass::TextureMapAsyncCallback callback, void* userdata);

  /**
   * Uploads the given data to the texture starting at pixel (0, 0)
   */
  void WriteTexture(std::size_t textureIndex, const void* bytes, std::size_t numBytes)
  {
    if (!CheckTextureIndex(textureIndex, "UpdateTextureData"))
    {
      return;
    }

    const auto wgpuTexture = this->WebGPUTextures[textureIndex];
    auto& texture = this->Textures[textureIndex];

    if (numBytes > static_cast<std::size_t>(texture->GetByteSize()))
    {
      vtkLog(ERROR,
        "The given data is larger than what the texture \""
          << texture->GetLabel() << "\" with byte size: " << texture->GetByteSize());

      return;
    }

    // Uploading from std::vector or vtkDataArray if one of the two is present
    const std::string textureLabel = texture->GetLabel();
    const char* textureLabelCStr = textureLabel.c_str();
    const uint32_t bytesPerRow = texture->GetBytesPerPixel() * texture->GetWidth();
    this->ParentPassWGPUConfiguration->WriteTexture(
      wgpuTexture, bytesPerRow, numBytes, bytes, textureLabelCStr);
  }

  /**
   * Releases the textures & resources held by this texture storage.
   */
  void ReleaseResources();

  /**
   * Internal method used to convert the user friendly Dimension enum to its wgpu::TextureDimension
   * equivalent
   */
  static wgpu::TextureDimension ComputeTextureDimensionToWebGPU(
    vtkWebGPUComputeTexture::TextureDimension dimension);

  /**
   * This function does a simple mapping between the dimension of the texture
   * (vtkWebGPUComputeTexture::TextureDimension) and that of the texture view
   * (wgpu::TextureViewDimension).
   *
   * The API currently assumes that the view created on a texture is unique and completely matches
   * the texture in terms of X, Y and Z sizes. This means that the texture view has the same extents
   * and the same dimension.
   */
  static wgpu::TextureViewDimension ComputeTextureDimensionToViewDimension(
    vtkWebGPUComputeTexture::TextureDimension dimension);

  /**
   * Internal method used to convert the user friendly TextureFormat enum to its wgpu::TextureFormat
   * equivalent
   */
  static wgpu::TextureFormat ComputeTextureFormatToWebGPU(
    vtkWebGPUComputeTexture::TextureFormat format);

  /**
   * Internal method used to convert the user friendly TextureMode enum to its wgpu::TextureUsage
   * equivalent.
   *
   * The texture label parameter is used for error logging.
   */
  static wgpu::TextureUsage ComputeTextureModeToUsage(
    vtkWebGPUComputeTexture::TextureMode mode, const std::string& textureLabel);

  /**
   * Internal method used to get the StorageTextureAccess mode associated with a TextureMode
   *
   * The texture label parameter is used for error logging.
   */
  static wgpu::StorageTextureAccess ComputeTextureModeToShaderStorage(
    vtkWebGPUComputeTexture::TextureMode mode, const std::string& textureLabel);

  /**
   * Internal method used to get the StorageTextureAccess mode associated with a TextureViewMode
   *
   * The texture view label parameter is used for error logging.
   */
  static wgpu::StorageTextureAccess ComputeTextureViewModeToShaderStorage(
    vtkWebGPUComputeTextureView::TextureViewMode mode, const std::string& textureViewLabel);

  /**
   * Internal method used to convert the user friendly TextureSampleType enum to its
   * wgpu::TextureSampleType equivalent.
   */
  static wgpu::TextureSampleType ComputeTextureSampleTypeToWebGPU(
    vtkWebGPUComputeTexture::TextureSampleType sampleType);

  /**
   * Internal method used to convert the user friendly TextureAspect enum to its
   * wgpu::TextureAspect equivalent.
   */
  static wgpu::TextureAspect ComputeTextureViewAspectToWebGPU(
    vtkWebGPUComputeTextureView::TextureViewAspect aspect);

protected:
  vtkWebGPUComputePassTextureStorageInternals() = default;
  ~vtkWebGPUComputePassTextureStorageInternals() override;

private:
  friend class vtkWebGPUComputePassInternals;

  vtkWebGPUComputePassTextureStorageInternals(
    const vtkWebGPUComputePassTextureStorageInternals&) = delete;
  void operator=(const vtkWebGPUComputePassTextureStorageInternals&) = delete;

  // Compute pass that uses this texture storage
  vtkWeakPointer<vtkWebGPUComputePass> ParentComputePass = nullptr;
  // Device of the parent compute pass that is used when creating textures and texture views
  vtkSmartPointer<vtkWebGPUConfiguration> ParentPassWGPUConfiguration;

  // Compute textures of the storage
  std::vector<vtkSmartPointer<vtkWebGPUComputeTexture>> Textures;
  // Compute render textures of this the storage
  std::vector<vtkSmartPointer<vtkWebGPUComputeRenderTexture>> RenderTextures;
  // Maps the compute render texture to the internal wgpu::Texture that they use
  std::unordered_map<vtkSmartPointer<vtkWebGPUComputeRenderTexture>, wgpu::Texture>
    RenderTexturesToWebGPUTexture;
  // WebGPU textures associated with the compute texture in the same order
  std::vector<wgpu::Texture> WebGPUTextures;

  // A map of the compute textures associated with all the texture views of it
  // that have been created
  std::unordered_map<vtkSmartPointer<vtkWebGPUComputeTexture>,
    std::unordered_set<vtkSmartPointer<vtkWebGPUComputeTextureView>>>
    ComputeTextureToViews;
  // List of the texture views added by the user. Can be used to find a texture
  // view from its index (indices which the user manipulates)
  std::vector<vtkSmartPointer<vtkWebGPUComputeTextureView>> TextureViews;
  // Compute textures views mapped to their WebGPU textures views
  std::unordered_map<vtkSmartPointer<vtkWebGPUComputeTextureView>, wgpu::TextureView>
    TextureViewsToWebGPUTextureViews;
};

VTK_ABI_NAMESPACE_END

#endif
