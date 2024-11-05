// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUTexture_h
#define vtkWebGPUTexture_h

#include "vtkObject.h"
#include "vtkRenderingWebGPUModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

/**
 * Abstraction class for WebGPU textures. This class mainly holds a bunch of parameters needed for
 * the creation of a texture.
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUTexture : public vtkObject
{
public:
  vtkTypeMacro(vtkWebGPUTexture, vtkObject);
  static vtkWebGPUTexture* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * RGBA8_UNORM: Uses RGB + alpha. Default.
   * BGRA8_UNORM: Uses BGR + alpha. Used by the framebuffer of WebGPU render windows
   * R32_FLOAT: Only a 32 bit float red channel
   * DEPTH_24_PLUS: 24 bit depth format
   * DEPTH_24_PLUS_8_STENCIL: 24 bit depth format + 8 bit stencil
   */
  enum TextureFormat
  {
    RGBA8_UNORM = 0,
    BGRA8_UNORM,
    R32_FLOAT,
    DEPTH_24_PLUS,
    DEPTH_24_PLUS_8_STENCIL
  };

  /**
   * How the texture data is arranged. Affects the type of sampler used in the compute shader
   *
   * DIMENSION_1D: 1D texture
   * DIMENSION_2D: 2D texture. Default.
   * DIMENSION_3D: 3D texture
   */
  enum TextureDimension
  {
    DIMENSION_1D = 0,
    DIMENSION_2D,
    DIMENSION_3D
  };

  /**
   * How will the texture be used by the shader
   *
   * UNDEFINED: Texture mode not set. Default.
   * READ_ONLY: The compute shader can only read from the texture and a sampler can be used
   * WRITE_ONLY_STORAGE: The compute shader can only write to the texture and a sampler cannot be
   * used
   * READ_WRITE_STORAGE: The compute shader can read and write to the texture and a sampler
   * cannot be used
   */
  enum TextureMode
  {
    UNDEFINED = 0,
    READ_ONLY,
    WRITE_ONLY_STORAGE,
    READ_WRITE_STORAGE
  };

  /**
   * Determines what kind of value is returned when reading from the texture in the compute shader
   *
   * FLOAT: Reading from the texture returns float values. Default.
   * UNFILTERABLE_FLOAT: Float but cannot be filtered by a sampler
   * DEPTH: Used for depth textures. The depth is returned as a float in the first channel of the
   * return vec4
   * SIGNED_INT: Signed integers. Used for textures containing signed integers data
   * UNSIGNED_INT: Unsigned integers. Used for textures containing unsigned integers data
   */
  enum TextureSampleType
  {
    FLOAT = 0,
    UNFILTERABLE_FLOAT,
    DEPTH,
    SIGNED_INT,
    UNSIGNED_INT
  };

  /**
   * Because the compute texture can accept multiple data types as input (std::vector, vtkDataArray)
   * but will ultimately only use one, it has to be determined which data to use thanks to this
   * enum.
   *
   * STD_VECTOR = Use the data given to the texture in the form of an std::vector. Default.
   * VTK_DATA_ARRAY = Use the data given to the texture in the form of a vtkDataArray.
   */
  enum TextureDataType
  {
    VTK_DATA_ARRAY = 0,
    STD_VECTOR
  };

  /**
   * Number of bytes used per pixel
   */
  virtual unsigned int GetBytesPerPixel() const;

  /**
   * Returns the number of components per pixel for the format of this texture. 1 for R32_FLOAT or
   * for RGBA8 for example.
   */
  unsigned int GetPixelComponentsCount() const;

  ///@{
  /**
   * Get/set the width in pixels of the texture
   */
  unsigned int GetWidth() const { return Extents[0]; }
  void SetWidth(unsigned int width) { Extents[0] = width; }
  ///@}

  ///@{
  /**
   * Get/set the height in pixels of the texture
   */
  unsigned int GetHeight() const { return Extents[1]; }
  void SetHeight(unsigned int height) { Extents[1] = height; }
  ///@}

  ///@{
  /**
   * Get/set the depth in pixels of the texture
   */
  unsigned int GetDepth() const { return Extents[2]; }
  void SetDepth(unsigned int depth) { Extents[2] = depth; }
  ///@}

  ///@{
  /**
   * Get/set the size of the texture
   *
   * The Z coordinate can be omitted since vtkWebGPUComputeTexture use 1 by default (meaning that
   * there is no depth)
   */
  void GetSize(unsigned int& x, unsigned int& y, unsigned int& z) const;
  void GetSize(unsigned int& x, unsigned int& y) const;
  void GetSize(unsigned int* xyz);
  unsigned int* GetSize();

  virtual void SetSize(unsigned int x, unsigned int y, unsigned int z = 1);
  virtual void SetSize(unsigned int* xyz);
  ///@}

  ///@{
  /**
   * Get/set the texture dimension
   */
  vtkGetEnumMacro(Dimension, TextureDimension);
  vtkSetEnumMacro(Dimension, TextureDimension);
  ///@}

  ///@{
  /**
   * Get/set the texture format
   */
  vtkGetEnumMacro(Format, TextureFormat);
  vtkSetEnumMacro(Format, TextureFormat);
  ///@}

  ///@{
  /**
   * Get/set the texture format
   */
  vtkGetEnumMacro(Mode, TextureMode);
  vtkSetEnumMacro(Mode, TextureMode);
  ///@}

  ///@{
  /**
   * Get/set the texture sample type
   */
  vtkGetEnumMacro(SampleType, TextureSampleType);
  vtkSetEnumMacro(SampleType, TextureSampleType);
  ///@}

  ///@{
  /**
   * Get/set the maximum mipmap levels used by the texture
   */
  vtkGetMacro(MipLevelCount, unsigned int);
  vtkSetMacro(MipLevelCount, unsigned int);
  ///@}

protected:
  vtkWebGPUTexture();
  ~vtkWebGPUTexture() override;

private:
  vtkWebGPUTexture(const vtkWebGPUTexture&) = delete;
  void operator=(const vtkWebGPUTexture&) = delete;

  // Number of pixels in X, Y and Z direction.
  // Defaulting to 1 in the Z direction because 2D textures are assumed to be the common case.
  unsigned int Extents[3] = { 0, 0, 1 };

  // Is the texture 1D, 2D or 3D
  TextureDimension Dimension = TextureDimension::DIMENSION_2D;

  // The format of the texture: RGB or RGBA?
  TextureFormat Format = TextureFormat::RGBA8_UNORM;

  // The read/write mode of the texture
  TextureMode Mode = TextureMode::UNDEFINED;

  // The type of value produced when sampling the texture in the shader
  TextureSampleType SampleType = TextureSampleType::FLOAT;

  // Maximum number of mipmap levels supported by the texture
  unsigned int MipLevelCount = 1;
};

VTK_ABI_NAMESPACE_END

#endif
