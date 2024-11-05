// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUTextureView_h
#define vtkWebGPUTextureView_h

#include "vtkObject.h"
#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWebGPUTexture.h"         // for TextureFormat, TextureDimension, ...

VTK_ABI_NAMESPACE_BEGIN

/**
 * Abstraction class for WebGPU texture views. This class mainly holds a bunch of parameters needed
 * for the creation of a texture view.
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUTextureView : public vtkObject
{
public:
  vtkTypeMacro(vtkWebGPUTextureView, vtkObject);
  static vtkWebGPUTextureView* New();

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * What will the shader sample from the texture when calling a sampling function.
   *
   * This may be useful for example when your texture has the DEPTH24_STENCIL8 format but you're
   * only interested in the depth in the shader. For this example, you will use TextureAspect::DEPTH
   * so that the shader samplers only sample the depth.
   *
   * ASPECT_ALL: Sample everything. Default.
   * ASPECT_DEPTH: Only sample the depth part
   * ASPECT_STENCIL: Only sample the stencil part
   */
  enum TextureViewAspect
  {
    ASPECT_ALL = 0,
    ASPECT_DEPTH,
    ASPECT_STENCIL
  };

  /**
   * The mode of the texture view to define what operations will be doable on the texture in the
   * shader.
   *
   * UNDEFINED: Texture view mode not set. Default.
   * READ_ONLY: The compute shader can only read from the texture and a sampler can be usedc
   * READ_ONLY_STORAGE: The compute shader can only read from the texture and a sampler cannot be
   * used.
   * WRITE_ONLY_STORAGE: The compute shader can only write to the texture and a sampler cannot
   * be used READ_WRITE_STORAGE: The compute shader can read and write to the texture and a sampler
   * cannot be used
   */
  enum TextureViewMode
  {
    UNDEFINED = 0,
    READ_ONLY,
    READ_ONLY_STORAGE,
    WRITE_ONLY_STORAGE,
    READ_WRITE_STORAGE
  };

  ///@{
  /**
   * Get/set mipmap level that this texture view represents
   */
  vtkGetMacro(BaseMipLevel, int);
  vtkSetMacro(BaseMipLevel, int);
  ///@}

  ///@{
  /**
   * Get/set the number of mipmaps that this texture view represents. Can only be 1 for storage
   * textures.
   */
  vtkGetMacro(MipLevelCount, int);
  vtkSetMacro(MipLevelCount, int);
  ///@}

  ///@{
  /**
   * Get/set the bind group index of the texture view
   */
  vtkGetMacro(Group, vtkIdType);
  vtkSetMacro(Group, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/set the binding index of the texture view
   */
  vtkGetMacro(Binding, vtkIdType);
  vtkSetMacro(Binding, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/set the aspect of the texture view
   */
  vtkGetMacro(Aspect, vtkWebGPUTextureView::TextureViewAspect);
  vtkSetMacro(Aspect, vtkWebGPUTextureView::TextureViewAspect);
  ///@}

  ///@{
  /**
   * Get/set the dimension of the texture view
   */
  vtkGetMacro(Dimension, vtkWebGPUTexture::TextureDimension);
  vtkSetMacro(Dimension, vtkWebGPUTexture::TextureDimension);
  ///@}

  ///@{
  /**
   * Get/set the format of the texture view
   */
  vtkGetMacro(Format, vtkWebGPUTexture::TextureFormat);
  vtkSetMacro(Format, vtkWebGPUTexture::TextureFormat);
  ///@}

  ///@{
  /**
   * Get/set the mode of the texture view
   */
  vtkGetMacro(Mode, vtkWebGPUTextureView::TextureViewMode);
  vtkSetMacro(Mode, vtkWebGPUTextureView::TextureViewMode);
  ///@}

  ///@{
  /**
   * Get/set the label of the texture view. This attribute is used for debugging is something goes
   * wrong
   */
  vtkGetMacro(Label, std::string&);
  vtkSetMacro(Label, std::string);
  ///@}

protected:
  vtkWebGPUTextureView();
  ~vtkWebGPUTextureView() override;

private:
  vtkWebGPUTextureView(const vtkWebGPUTextureView&) = delete;
  void operator=(const vtkWebGPUTextureView&) = delete;

  // Mip level of the base texture that this texture view gives a view on
  int BaseMipLevel = 0;
  // How many mip levels this texture view give the shader access to
  int MipLevelCount = 1;

  // Bind group of the texture view
  vtkIdType Group = -1;

  // Binding of the texture view
  vtkIdType Binding = -1;

  // What aspect of the texture is going to be sampled by the samplers in the shaders sampling this
  // texture
  vtkWebGPUTextureView::TextureViewAspect Aspect = TextureViewAspect::ASPECT_ALL;

  // Dimension of the texture view
  vtkWebGPUTexture::TextureDimension Dimension = vtkWebGPUTexture::TextureDimension::DIMENSION_2D;

  // Format of the texture view
  vtkWebGPUTexture::TextureFormat Format = vtkWebGPUTexture::TextureFormat::RGBA8_UNORM;

  // Mode of the texture view
  vtkWebGPUTextureView::TextureViewMode Mode = vtkWebGPUTextureView::TextureViewMode::UNDEFINED;

  // Label used for debugging if something goes wrong
  std::string Label = "Texture view";
};

VTK_ABI_NAMESPACE_END

#endif
