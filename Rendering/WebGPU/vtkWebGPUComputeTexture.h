// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputeTexture_h
#define vtkWebGPUComputeTexture_h

#include "vtkDataArray.h"             // for vtkDataArray used in SetData
#include "vtkLogger.h"                // for the logger
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWebGPUTexture.h"
#include "vtk_wgpu.h" // for dawn classes

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPURenderer;

/**
 * Represents the set of parameters that will be used to create a compute shader texture on the
 * device when it will be added to a pipeline using vtkWebGPUComputePipeline::AddTexture()
 *
 * Some parameters have defaults for convenience:
 *
 * - Format defaults to RGBA
 * - Dimension defaults to 2D
 * - The depth of the texture (extents/size in the Z coordinate) defaults to 1
 * - The maximum number of mip levels defaults to 0
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputeTexture : public vtkWebGPUTexture
{
public:
  vtkTypeMacro(vtkWebGPUComputeTexture, vtkObject);
  static vtkWebGPUComputeTexture* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/set the size in bytes of the texture
   */
  vtkGetMacro(ByteSize, vtkIdType);
  vtkSetMacro(ByteSize, vtkIdType);
  ///@}

  /**
   * Sets the data that will be used by the texture.
   *
   * @warning: This does not copy the data so the data given to this texture needs to stay valid
   * (i.e. not freed) until the texture is added to a compute pass using
   * vtkWebGPUComputePass::AddTexture().
   */
  template <typename T>
  void SetData(const std::vector<T>& data)
  {
    this->DataPointer = data.data();
    this->ByteSize = data.size() * sizeof(T);
  }

  /**
   * Sets the data that will be used by the texture.
   *
   * NOTE: This does not copy the data so the data given to this texture needs to stay valid (i.e.
   * not freed) until the texture is added to a compute pass using
   * vtkWebGPUComputePass::AddTexture().
   */
  void SetData(vtkDataArray* data);

  ///@{
  /**
   * Get/set what data type to use for the texture
   */
  vtkGetMacro(DataType, TextureDataType);
  vtkSetMacro(DataType, TextureDataType);
  ///@}

  /**
   * The pointer to the std::vector<> data passed with SetData(std::vector)
   *
   * @warning: This pointer only points to a valid location as long as
   * the std::vector given with SetData() is alive.
   */
  const void* GetDataPointer() { return this->DataPointer; }

  /**
   * The pointer to the vtkDataArray passed with SetData(vtkDataArray)
   *
   * @warning: This pointer to the array is only valid as long as
   * the vtkDataArray given with SetData() is alive.
   */
  vtkDataArray* GetDataArray() { return this->DataArray; }

  ///@{
  /**
   * Get/set the label used for debugging in case of errors
   */
  const std::string& GetLabel() const { return this->Label; }
  vtkGetMacro(Label, std::string&);
  vtkSetMacro(Label, std::string);
  ///@}

protected:
  vtkWebGPUComputeTexture();
  ~vtkWebGPUComputeTexture() override;

private:
  vtkWebGPUComputeTexture(const vtkWebGPUComputeTexture&) = delete;
  void operator=(const vtkWebGPUComputeTexture&) = delete;

  // Total size of the texture in bytes
  vtkIdType ByteSize = -1;

  // The type of data that will be uploaded to the GPU
  TextureDataType DataType = TextureDataType::STD_VECTOR;

  // Pointer to the data that this buffer will contain. This variable DataPointer is only used when
  // the user set the buffer data (with SetData()) using an std::vector.
  const void* DataPointer = nullptr;
  // Data array containing the data that will be uploaded to the buffer. Only relevant if the user
  // called SetData(vtkDataArray).
  vtkDataArray* DataArray = nullptr;

  // Label used for debugging if something goes wrong.
  std::string Label = "Compute texture";
};

VTK_ABI_NAMESPACE_END

#endif
