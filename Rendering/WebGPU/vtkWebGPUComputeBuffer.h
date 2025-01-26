// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputeBuffer_h
#define vtkWebGPUComputeBuffer_h

#include "vtkDataArray.h" // for vtkDataArray used in SetData
#include "vtkLogger.h"    // for the logger
#include "vtkObject.h"
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkSetGet.h"                // for get/set macro
#include "vtk_wgpu.h"                 // for dawn classes

#include <functional>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUComputePass;
class vtkWebGPURenderer;

/**
 * Represents the set of parameters that will be used to create a compute shader buffer on the
 * device when it will be added to a compute pass using vtkWebGPUComputePass::AddBuffer()
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputeBuffer : public vtkObject
{
public:
  vtkTypeMacro(vtkWebGPUComputeBuffer, vtkObject);
  static vtkWebGPUComputeBuffer* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * - UNDEFINED = Buffer mode not set
   *
   * - READ_ONLY_COMPUTE_STORAGE = The GPU can only read from this buffer.
   * A buffer with this mode is declared with <read, storage> in WGSL
   *
   * - READ_WRITE_COMPUTE_STORAGE = The GPU can read from and write to this
   * buffer from the compute shader. A buffer with this mode is declared with
   * <read_write, storage> in WGSL
   *
   * - READ_WRITE_MAP_COMPUTE_STORAGE = The GPU can read from and write to this
   * buffer from the compute shader. With this mode, the buffer can also be
   * mapped so that the CPU can read from it (typically to read back the results
   * from the compute shader). A buffer using this mode is declared with
   * <read_write, storage> in WGSL
   *
   * - UNIFORM_BUFFER = Uniform buffer, read only by the GPU.
   * A buffer with this mode is declared with var<uniform> in WGSL
   */
  enum BufferMode
  {
    UNDEFINED = 0,
    READ_ONLY_COMPUTE_STORAGE,
    READ_WRITE_COMPUTE_STORAGE,
    READ_WRITE_MAP_COMPUTE_STORAGE,
    UNIFORM_BUFFER
  };

  /**
   * Because the compute buffer can accept multiple data types as input (std::vector, vtkDataArray)
   * but will ultimately only use one, it has to be determined which data to use thanks to this
   * enum.
   *
   * STD_VECTOR = Use the data given to the buffer in the form of an std::vector. Default
   * VTK_DATA_ARRAY = Use the data given to the buffer in the form of a vtkDataArray
   */
  enum BufferDataType
  {
    VTK_DATA_ARRAY = 0,
    STD_VECTOR
  };

  ///@{
  /**
   * Get/set the buffer mode
   */
  vtkGetEnumMacro(Mode, BufferMode);
  vtkSetEnumMacro(Mode, BufferMode);
  ///@}

  ///@{
  /**
   * Get/set the group of the buffer in the compute shader. This refers to \@group(X) in the WGSL
   * shader code.
   *
   * @note: All buffers must have a unique combination of binding / group. No buffers can have the
   * same binding / group as another buffer
   */
  uint32_t GetGroup() const { return this->Group; }
  vtkGetMacro(Group, vtkIdType);
  vtkSetMacro(Group, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/set the binding of the buffer in the compute shader. This refers to \@binding(X) in the
   * WGSL shader code.
   *
   * @note: All buffers must have a unique combination of binding / group. No buffers can have the
   * same binding / group as another buffer
   */
  uint32_t GetBinding() const { return this->Binding; }
  vtkGetMacro(Binding, vtkIdType);
  vtkSetMacro(Binding, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/set the label of the buffer. Useful for debugging errors that may come up in the terminal
   * since these errors will include the given label
   */
  vtkGetMacro(Label, std::string);
  vtkSetMacro(Label, std::string);
  ///@}

  /**
   * Sets the data that will be used by the buffer.
   * Passing an empty vector to this method clears any previously given std::vector data, allowing
   * for the use of a vtkDataArray instead.
   *
   * @warning: This does not copy the data so the data given to this buffer needs to stay valid
   * (i.e. not freed) until the buffer is added to a compute pass using
   * vtkWebGPUComputePass::AddBuffer().
   */
  template <typename T>
  void SetData(const std::vector<T>& data)
  {
    this->DataPointer = data.data();
    this->ByteSize = data.size() * sizeof(T);
  }

  /**
   * Sets the data that will be used by the buffer.
   * Passing a nullptr vtkDataArray clears any previously given vtkDataArray data allowing the use
   * of std::vector data instead.
   *
   * NOTE: This does not copy the data so the data given to this buffer needs to stay valid (i.e.
   * not freed) until the buffer is added to a compute pass using
   * vtkWebGPUComputePass::AddBuffer().
   */
  void SetData(vtkDataArray* data);

  ///@{
  /**
   * Get/set what data type to use for the buffer
   */
  vtkGetMacro(DataType, BufferDataType);
  vtkSetMacro(DataType, BufferDataType);
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
   * Get/set the size in bytes of the data passed in via SetData().
   */
  vtkGetMacro(ByteSize, vtkIdType);
  vtkSetMacro(ByteSize, vtkIdType);
  ///@}

protected:
  vtkWebGPUComputeBuffer();
  ~vtkWebGPUComputeBuffer() override;

private:
  friend class vtkWebGPUComputePass;

  vtkWebGPUComputeBuffer(const vtkWebGPUComputeBuffer&) = delete;
  void operator=(const vtkWebGPUComputeBuffer&) = delete;

  // Bind group index
  vtkIdType Group = -1;
  // Binding within the bind group
  vtkIdType Binding = -1;
  // The mode of the buffer can be read only, write only, read/write, ...
  BufferMode Mode = BufferMode::UNDEFINED;
  // The type of data that will be uploaded to the GPU
  BufferDataType DataType = BufferDataType::STD_VECTOR;

  // Pointer to the data that this buffer will contain. This variable DataPointer is only used when
  // the user set the buffer data (with SetData()) using an std::vector.
  const void* DataPointer = nullptr;
  // Data array containing the data that will be uploaded to the buffer. Only relevant if the user
  // called SetData(vtkDataArray).
  vtkDataArray* DataArray = nullptr;
  // How many bytes will be uploaded from the buffer to the GPU
  vtkIdType ByteSize = 0;
  // Label that can be used to identify this buffer and help with debugging
  // in case something goes wrong
  std::string Label;
};

VTK_ABI_NAMESPACE_END
#endif
