// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputePass_h
#define vtkWebGPUComputePass_h

#include "vtkObject.h"

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkSmartPointer.h"          // for arg
#include "vtk_wgpu.h"                 // for webgpu

#include <type_traits> // for enable_if_t

class vtkDataArray;
class vtkWebGPUComputeBuffer;
class vtkWebGPUComputePipeline;
class vtkWebGPUComputeRenderBuffer;
class vtkWebGPUComputeRenderTexture;
class vtkWebGPUComputeTexture;
class vtkWebGPUComputeTextureView;
class vtkWebGPUComputePassInternals;
class vtkWebGPUComputePassBufferStorageInternals;
class vtkWebGPUComputePassTextureStorageInternals;

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

  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  int AddRenderTexture(vtkSmartPointer<vtkWebGPUComputeRenderTexture> renderTexture);

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
   * @warning The texture will need to be recreated by calling RecreateComputeTexture() for all the
   * changes done to this compute texture to take effect
   */
  vtkSmartPointer<vtkWebGPUComputeTexture> GetComputeTexture(int textureIndex);

  /**
   * Retrieves the texture view associated with the given texture view index
   *
   * @warning The texture view will need to be recreated by calling RecreateTextureView() for all
   * the changes done to this texture view to take effect
   */
  vtkSmartPointer<vtkWebGPUComputeTextureView> GetTextureView(int textureViewIndex);

  /**
   * Recreates a compute texture. Must be called if the compute texture has been modified (after a
   * call to GetComputeTexure() + modifications of the parameters for example) for the  changes to
   * take effect.
   */
  void RecreateComputeTexture(int textureIndex);

  /**
   * Recreates a compute texture view. Must be called if the texture view has been modified (after a
   * call to GetComputeTextureView for example) for the  changes to take effect.
   */
  void RecreateTextureView(int textureViewIndex);

  /*
   * Callback called when the asynchronous mapping of a buffer is done
   * and data is ready to be copied.
   * This callback takes three parameters:
   *
   * - A first void pointer to the data mapped from the GPU ready to be copied
   * - A second void pointer pointing to user data, which can essentially be anything
   *      needed by the callback to copy the data to the CPU
   */
  using BufferMapAsyncCallback = std::function<void(const void*, void*)>;

  /*
   * This function maps the buffer, making it accessible to the CPU. This is
   * an asynchronous operation, meaning that the given callback will be called
   * when the mapping is done.
   *
   * The buffer data can then be read from the callback and stored
   * in a buffer (std::vector<>, vtkDataArray, ...) passed in via the userdata pointer for example
   */
  void ReadBufferFromGPU(int bufferIndex, BufferMapAsyncCallback callback, void* userdata);

  /*
   * Callback called when the asynchronous mapping of a texture is done
   * and data is ready to be copied.
   * This callback takes three parameters:
   *
   * - A void pointer to the data mapped from the GPU ready to be copied.
   *
   * - An integer representing how many bytes per row the mapped data contains. This is
   * useful because some padding has probably be done on the buffer to satisfy WebGPU size
   * constraints. At the time of writing, buffers for texture mapping need a number of bytes per row
   * that is a multiple of 256 bytes. This means that for a texture of 300x300 RGBA (300 * 4 = 1200
   * bytes per row), there will be 80 bytes of additional padding to achieve 1280 bytes per row
   * which is a multiple of 256. In this case, the integer argument of the callback will contain the
   * value '1280' and it is then the responsibility of the user to only read relevant data (i.e. the
   * 1200 first bytes of each row since the 80 last bytes are irrelevant padding).
   *
   * - A second void pointer pointing to user data, which can essentially be anything
   *      needed by the callback to copy the data to the CPU
   */
  using TextureMapAsyncCallback = std::function<void(const void*, int, void*)>;

  /**
   *
   * This function maps the texture into a linear memory block, making it accessible to the CPU.
   * This is an asynchronous operation, meaning that the given callback will be called when the
   * mapping is done.
   *
   * The texture data can then be read from the callback and stored
   * in a buffer (std::vector<>, vtkDataArray, ...) passed in via the userdata pointer for example
   */
  void ReadTextureFromGPU(
    int textureIndex, int mipLevel, TextureMapAsyncCallback callback, void* userdata);

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
  template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
  void UpdateBufferData(int bufferIndex, const std::vector<T>& newData)
  {
    this->WriteBufferData(bufferIndex, 0, newData.data(), newData.size() * sizeof(T));
  }

  /**
   * Similar to the overload without offset of this function.
   * The offset is used to determine where in the buffer to reupload data.
   * Useful when only a portion of the buffer needs to be reuploaded.
   */
  template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
  void UpdateBufferData(int bufferIndex, vtkIdType byteOffset, const std::vector<T>& data)
  {
    this->WriteBufferData(bufferIndex, byteOffset, data.data(), data.size() * sizeof(T));
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
  template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
  void UpdateTextureData(int textureIndex, const std::vector<T>& data)
  {
    this->WriteTextureData(textureIndex, data.data(), data.size() * sizeof(T));
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

  /**
   * Releases the resources used by this compute pass. After this call, the compute pass will be in
   * an unusable state and it should be removed from the compute pipeline it belongs to. A new
   * compute pass should then be created from the compute pipeline.
   */
  void ReleaseResources();

protected:
  vtkWebGPUComputePass();
  ~vtkWebGPUComputePass() override;

private:
  vtkWebGPUComputePass(const vtkWebGPUComputePass&) = delete;
  void operator=(const vtkWebGPUComputePass&) = delete;

  void WriteBufferData(
    int bufferIndex, vtkIdType byteOffset, const void* data, std::size_t numBytes);

  void WriteTextureData(int textureIndex, const void* data, std::size_t numBytes);

  friend class vtkWebGPUComputePassInternals;
  friend class vtkWebGPUComputePassTextureStorageInternals;
  friend class vtkWebGPUComputePassBufferStorageInternals;
  friend class vtkWebGPUComputePipeline;
  friend class vtkWebGPUHelpers;
  // For the mapper to be able to access the internals to access the wgpu::Buffer objects for use
  // in a render pipeline
  friend class vtkWebGPUPointCloudMapperInternals;
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
  vtkSmartPointer<vtkWebGPUComputePassInternals> Internals;
};

VTK_ABI_NAMESPACE_END

#endif
