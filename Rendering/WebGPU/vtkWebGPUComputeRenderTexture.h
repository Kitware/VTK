// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUComputeRenderTexture_h
#define vtkWebGPUComputeRenderTexture_h

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkWeakPointer.h"           // for weak pointer on the associated compute pass
#include "vtkWebGPUComputeTexture.h"
#include "vtkWebGPUComputeTextureView.h" // For the texture view aspect attribute

VTK_ABI_NAMESPACE_BEGIN

class vtkWebGPUComputePass;

/**
 * Render textures are returned by calls to
 * vtkWebGPUPolyDataMapper::AcquireXXXXRenderTexture() and represent a buffer that
 * is used by the rendering pipeline and that can also be added to a compute pipeline
 */
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUComputeRenderTexture : public vtkWebGPUComputeTexture
{
public:
  vtkTypeMacro(vtkWebGPUComputeRenderTexture, vtkWebGPUComputeTexture);
  static vtkWebGPUComputeRenderTexture* New();

  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * What type of texture of the vtk render pipeline is this ComputeRenderTexture refering to.
   *
   * UNDEFINED: Undefined. Default.
   * DEPTH_BUFFER: When this ComputeRenderTexture refers to the depth buffer of a
   * vtkWebGPURenderWindow
   */
  enum RenderTextureType
  {
    UNDEFINED = 0,
    DEPTH_BUFFER
  };

  ///@{
  /**
   * Get/set the render texture type
   */
  vtkGetEnumMacro(Type, RenderTextureType);
  vtkSetEnumMacro(Type, RenderTextureType);
  ///@}

  ///@{
  /**
   * Get/set the texture aspect that is going to be passed to the texture view created for this
   * render texture in the compute pass.
   */
  vtkGetEnumMacro(Aspect, vtkWebGPUComputeTextureView::TextureViewAspect);
  vtkSetEnumMacro(Aspect, vtkWebGPUComputeTextureView::TextureViewAspect);
  ///@}

  ///@{
  /**
   * Get/set the bind group index of the texture
   */
  vtkGetMacro(Group, vtkIdType);
  vtkSetMacro(Group, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/set the binding index of the texture
   */
  vtkGetMacro(Binding, vtkIdType);
  vtkSetMacro(Binding, vtkIdType);
  ///@}

  ///@{
  /**
   * Get/set the WebGPU texture (used when this ComputeTexture points to an already existing device
   * buffer)
   */
  void SetWebGPUTexture(wgpu::Texture texture) { this->WebGPUTexture = texture; };
  wgpu::Texture GetWebGPUTexture() { return this->WebGPUTexture; };
  ///@}

  ///@{
  /**
   * Get/set the associated compute pass
   */
  vtkWeakPointer<vtkWebGPUComputePass> GetAssociatedComputePass();
  void SetAssociatedComputePass(vtkWeakPointer<vtkWebGPUComputePass> computePass);
  ///@}

protected:
  vtkWebGPUComputeRenderTexture();
  ~vtkWebGPUComputeRenderTexture();
  vtkWebGPUComputeRenderTexture(const vtkWebGPUComputeRenderTexture&) = delete;
  void operator=(const vtkWebGPUComputeRenderTexture&) = delete;

private:
  // Aspect for the future texture view of this texture in a compute pass
  vtkWebGPUComputeTextureView::TextureViewAspect Aspect =
    vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_ALL;

  // Bind group of the texture view
  vtkIdType Group = -1;

  // Binding of the texture view.
  vtkIdType Binding = -1;

  // We may want vtkWebGPUComputePipeline::AddTexture() not to create a new device texture for this
  // vtkWebGPUComputeBuffer but rather use an exisiting one that has been created elsewhere (by a
  // webGPUPolyDataMapper for example). This is the attribute that points to this 'already existing'
  // buffer.
  wgpu::Texture WebGPUTexture = nullptr;

  /**
   * The ComputePipeline this render texture is associated with.
   */
  vtkWeakPointer<vtkWebGPUComputePass> AssociatedComputePass = nullptr;

  /**
   * What type of texture is this ComputeRenderTexture refering to.
   */
  RenderTextureType Type = UNDEFINED;
};

VTK_ABI_NAMESPACE_END

#endif
