// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePass_h
#define vtkWebGPUComputePass_h

#include "vtkObject.h"
#include "vtkWebGPUComputeRenderBuffer.h"      // for compute render buffers used by the pipeline
#include "vtkWebGPUComputeRenderTexture.h"     // for compute render textures used by the pipeline
#include "vtkWebGPUComputeTextureView.h"       // for texture view management
#include "vtkWebGPUInternalsBindGroup.h"       // for bind group utilitary methods
#include "vtkWebGPUInternalsBindGroupLayout.h" // for bind group layouts utilitary methods
#include "vtkWebGPUInternalsBuffer.h"          // for internal buffer utils
#include "vtkWebGPUInternalsComputePass.h"     // for the internals
#include "vtkWebGPUInternalsComputePassBufferStorage.h"  // for buffer storage management
#include "vtkWebGPUInternalsComputePassTextureStorage.h" // for texture storage management
#include "vtk_wgpu.h"                                    // for webgpu

#include <unordered_map>
#include <unordered_set>

class vtkWebGPUComputePipeline;

VTK_ABI_NAMESPACE_BEGIN

/**
 * A compute pass is an abstraction for offloading computation from the CPU onto the GPU using
 * WebGPU compute shaders.
 *
 * The basic usage of a compute pass outside a rendering pipeline is:
 *  - Create a vtkWebGPUComputePipeline
 *  - Obtain a compute pass from this compute pipeline
 *  - Set its shader source code
 *  - Set its shader entry point
 *  - Create the vtkWebGPUComputeBuffers that contain the data manipulated by the compute pass
 *  - Add the buffers to the compute pass
 *  - Set the number of workgroups
 *  - Dispatch the compute pass
 *  - ReadBufferFromGPU() to make results from the GPU available to the CPU
 *  - Update() the pipeline so that the compute pass is executed
 *
 * Integrated into a rendering pipeline, the only difference in the usage of the class is going to
 * be the creation of the buffers. You will not create the vtkWebGPUComputeBuffer yourself but
 * rather acquire one (or many) by calling AcquirePointAttributeComputeRenderBuffer() on a
 * vtkWebGPURenderer. The returned buffers can then be added to the compute pass with
 * AddRenderBuffer(). Other steps are identical. This remark also applies to render textures.
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputePass : public vtkObject
{
public:
  /**
   * Note to the VTK user: A compute pass should always be acquired through
   * vtkWebGPUComputePipeline::CreateComputePass(). You should not create compute pass through
   * vtkNew<> or vtkSmartPointer<> directly.
   */
  static vtkWebGPUComputePass* New();
  vtkTypeMacro(vtkWebGPUComputePass, vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent);

  ///@{
  /**
   * Set/get the WGSL source of the shader
   */
  vtkGetMacro(ShaderSource, std::string);
  vtkSetMacro(ShaderSource, std::string);
  ///@}

  void SetShaderSourceFromPath(const char* shaderFilePath);

  ///@{
  /**
   * Set/get the entry (name of the function) of the WGSL compute shader
   */
  vtkGetMacro(ShaderEntryPoint, std::string);
  vtkSetMacro(ShaderEntryPoint, std::string);
  ///@}

  ///@{
  /**
   * Set/get the label of the compute pass.
   * This label will be printed along with error/warning logs
   * to help with debugging
   */
  vtkGetMacro(Label, std::string&);
  void SetLabel(const std::string& label);
  ///@}

  /**
   * Adds a buffer to the pass and uploads its data to the device.
   *
   * Returns the index of the buffer that can for example be used as input to the
   * ReadBufferFromGPU() function
   */
  int AddBuffer(vtkSmartPointer<vtkWebGPUComputeBuffer> buffer);

  /**
   * Adds a render buffer to the pass. A render buffer can be obtained from
   * vtkWebGPURenderWindow::AcquireDepthBufferRenderTexture()
   */
  void AddRenderBuffer(vtkSmartPointer<vtkWebGPUComputeRenderBuffer> renderBuffer);

  /**
   * Adds a render texture to the pass. A render texture can be obtained from
   * vtkWebGPURenderWindow::AcquireDepthBufferRenderTexture() and analogous methods.
   */
  void AddRenderTexture(vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture);

  /**
   * Adds a texture to the pass and upload its data to the device
   *
   * Returns the index of the texture that can for example be used as input to the
   * ReadTextureFromGPU() function
   */
  int AddTexture(vtkSmartPointer<vtkWebGPUComputeTexture> texture);

  /**
   * Returns a new texture view on the given texture (given by the index) that can be configured and
   * then added to the compute pass by AddTextureView()
   */
  vtkSmartPointer<vtkWebGPUComputeTextureView> CreateTextureView(int textureIndex);

  /**
   * Adds a texture view to the compute pass and returns its index
   */
  int AddTextureView(vtkSmartPointer<vtkWebGPUComputeTextureView> textureView);

  /**
   * This function allows the usage of multiple texture views on a single binding point (group /
   * binding combination) in the shader (although not at the same time). It acts as AddTextureView()
   * if no texture view was bound to the group/binding in the first place.
   *
   * For example, consider that your shader has the following binding:
   * \@group(0) \@binding(0) var inputTexture: texture_2d<f32>;
   *
   * Depending on your needs, you may want to execute this compute pass twice but with a different
   * texture as input to the shader each time. To achieve that, you would create 2 TextureViews on
   * the 2 Textures that you want your shader to manipulate and call RebindTextureView() on your
   * second texture view index before Dispatching the second compute pass so that it the texture
   * view sampled in the shader samples the second texture.
   */
  void RebindTextureView(int group, int binding, int textureViewIndex);

  /**
   * Deletes all the texture views of a given texture (given by its index)
   */
  void DeleteTextureViews(int textureIndex);

  /**
   * Returns the size in bytes of a buffer
   */
  unsigned int GetBufferByteSize(int bufferIndex);

  /**
   * Resizes a buffer of the pass.
   *
   * @warning: After the resize, the data of the buffer is undefined and should be updated by a
   * call to UpdateBufferData()
   */
  void ResizeBuffer(int bufferIndex, vtkIdType newByteSize);

  /**
   * Retrieves the compute texture associated with the given texture index
   *
   * @warning The texture will need to be recreated by calling RecreateComputeTexture for all the
   * changes done to this compute texture to take effect
   */
  vtkSmartPointer<vtkWebGPUComputeTexture> GetComputeTexture(int textureIndex);

  /**
   * Recreates a compute texture. Must be called if the compute texture has been modified (after a
   * call to GetComputeTexure for example) for the  changes to take effect.
   */
  void RecreateComputeTexture(int textureIndex);

  /*
   * This function maps the buffer, making it accessible to the CPU. This is
   * an asynchronous operation, meaning that the given callback will be called
   * when the mapping is done.
   *
   * The buffer data can then be read from the callback and stored
   * in a buffer (std::vector<>, vtkDataArray, ...) passed in via the userdata pointer for example
   */
  void ReadBufferFromGPU(int bufferIndex,
    vtkWebGPUInternalsComputePassBufferStorage::BufferMapAsyncCallback callback, void* userdata);

  /**
   *
   * This function maps the texture into a linear memory block, making it accessible to the CPU.
   * This is an asynchronous operation, meaning that the given callback will be called when the
   * mapping is done.
   *
   * The texture data can then be read from the callback and stored
   * in a buffer (std::vector<>, vtkDataArray, ...) passed in via the userdata pointer for example
   */
  void ReadTextureFromGPU(int textureIndex, int mipLevel,
    vtkWebGPUInternalsComputePassTextureStorage::TextureMapAsyncCallback callback, void* userdata);

  /**
   * Updates the data of a buffer.
   * The given data is expected to be at most the size of the buffer.
   * If N bytes are given to update but the buffer size is > N, only the first N bytes
   * will be updated, the rest will remain unchanged.
   * The data is immediately available to the GPU (no call to vtkWebGPUComputePass::Update() is
   * necessary for this call to take effect)
   *
   * @note: This method can be used even if the buffer was initially configured with std::vector
   * data and the given data can safely be destroyed directly after calling this function.
   *
   */
  template <typename T>
  void UpdateBufferData(int bufferIndex, const std::vector<T>& newData)
  {
    this->Internals->BufferStorage->UpdateBufferData(bufferIndex, newData);
  }

  /**
   * Similar to the overload without offset of this function.
   * The offset is used to determine where in the buffer to reupload data.
   * Useful when only a portion of the buffer needs to be reuploaded.
   */
  template <typename T>
  void UpdateBufferData(int bufferIndex, vtkIdType byteOffset, const std::vector<T>& data)
  {
    this->Internals->BufferStorage->UpdateBufferData(bufferIndex, byteOffset, data);
  }

  /**
   * Updates the data of a buffer with a vtkDataArray.
   * The given data is expected to be at most the size of the buffer.
   * If N bytes are given to update but the buffer size is > N, only the first N bytes
   * will be updated, the rest will remain unchanged.
   * The data is immediately available to the GPU (no call to vtkWebGPUComputePass::Update() is
   * necessary for this call to take effect
   *
   * @note: This method can be used even if the buffer was initially configured with std::vector
   * data and the given data can safely be destroyed directly after calling this function.
   */
  void UpdateBufferData(int bufferIndex, vtkDataArray* newData);

  /**
   * Similar to the overload without offset of this function.
   * The offset is used to determine where in the buffer to reupload data.
   * Useful when only a portion of the buffer needs to be reuploaded.
   */
  void UpdateBufferData(int bufferIndex, vtkIdType byteOffset, vtkDataArray* newData);

  /**
   * Uploads the given data to the texture starting at pixel (0, 0)
   */
  template <typename T>
  void UpdateTextureData(int textureIndex, const std::vector<T>& data)
  {
    this->Internals->TextureStorage->UpdateTextureData(textureIndex, data);
  }

  ///@{
  /*
   * Set/get the number of workgroups in each dimension that are used by each Dispatch() call.
   */
  void SetWorkgroups(int groupsX, int groupsY, int groupsZ);
  ///@}

  /**
   * Dispatch the compute pass with (X, Y, Z) = (groupX, groupsY, groupZ) groups
   */
  void Dispatch();

protected:
  vtkWebGPUComputePass();
  ~vtkWebGPUComputePass();
  vtkWebGPUComputePass(const vtkWebGPUComputePass&) = delete;
  void operator=(const vtkWebGPUComputePass&) = delete;

private:
  friend class vtkWebGPUComputePipeline;
  friend class vtkWebGPUHelpers;
  friend class vtkWebGPUInternalsComputePass;
  friend class vtkWebGPUInternalsComputePassTextureStorage;
  friend class vtkWebGPUInternalsComputePassBufferStorage;
  friend class vtkWebGPURenderWindow;
  friend class vtkWebGPURenderer;

  std::string ShaderSource;
  std::string ShaderEntryPoint;

  // How many groups to launch when dispatching the compute
  unsigned int GroupsX = 0, GroupsY = 0, GroupsZ = 0;

  // Label used for the wgpu compute pipeline of this VTK compute pipeline
  std::string Label = "WebGPU compute pass";

  // Label used for the wgpu command encoders created and used by this VTK compute pipeline
  std::string WGPUCommandEncoderLabel = "WebGPU command encoder \"" + this->Label + "\"";
  std::string WGPUComputePipelineLabel = "WebGPU pipeline \"" + this->Label + "\"";

  // Internal implementation of the compute pass
  vtkSmartPointer<vtkWebGPUInternalsComputePass> Internals;
};

VTK_ABI_NAMESPACE_END

#endif
