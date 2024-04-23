// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPURenderWindow_h
#define vtkWebGPURenderWindow_h

#include "vtkRenderWindow.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkTypeUInt8Array.h"        // for ivar
#include "vtk_wgpu.h"                 // for webgpu

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPURenderWindow : public vtkRenderWindow
{
public:
  vtkTypeMacro(vtkWebGPURenderWindow, vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set preference for a high-performance or low-power device.
   * The default preference is a high-performance device.
   * NOTE: Make sure to call this before the first call to Render if you wish to change the
   * preference. @warning: Changing the power preference after the render window is initialized has
   * no effect.
   */
  void PreferHighPerformanceAdapter();
  void PreferLowPowerAdapter();
  ///@}

  ///@{
  /**
   * Get/Set backend type.
   * The default backend is platform specific.
   * DirectX 12 on Windows
   * Vulkan on Linux and Android
   * Metal on macOS/iOS.
   * NOTE: Make sure to call this before the first call to Render if you wish to change the backend.
   * @warning: Changing the backend after the render window is initialized has no effect.
   */
  void SetBackendTypeToD3D11();
  void SetBackendTypeToD3D12();
  void SetBackendTypeToMetal();
  void SetBackendTypeToVulkan();
  void SetBackendTypeToOpenGL();
  void SetBackendTypeToOpenGLES();
  std::string GetBackendTypeAsString();
  ///@}

  /**
   * Concrete render windows must create a platform window and initialize this->WindowId.
   * Upon success, please call WGPUInit().
   */
  virtual bool Initialize() = 0;

  /**
   * Create a not-off-screen window.
   */
  virtual void CreateAWindow() = 0;

  /**
   * Destroy a not-off-screen window.
   */
  virtual void DestroyWindow() = 0;

  void Start() override;

  /**
   * Update the system, if needed, at end of render process
   */
  void End() override;

  /**
   * Handle opengl specific code and calls superclass
   */
  void Render() override;

  /**
   * Intermediate method performs operations required between the rendering
   * of the left and right eye.
   */
  void StereoMidpoint() override;
  void Frame() override;

  const char* GetRenderingBackend() override;

  /**
   * Reads pixels into the `CachedPixelBytes` variable.
   */
  void ReadPixels();

  ///@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBRGB...
   * front in this context indicates that the read should come from the
   * display buffer versus the render buffer
   */
  unsigned char* GetPixelData(int x, int y, int x2, int y2, int front, int right) override;
  int GetPixelData(
    int x, int y, int x2, int y2, int front, vtkUnsignedCharArray* data, int right) override;
  int SetPixelData(
    int x, int y, int x2, int y2, unsigned char* data, int front, int right) override;
  int SetPixelData(
    int x, int y, int x2, int y2, vtkUnsignedCharArray* data, int front, int right) override;
  ///@}

  ///@{
  /**
   * Set/Get the pixel data of an image, transmitted as RGBARGBA...
   */
  float* GetRGBAPixelData(int x, int y, int x2, int y2, int front, int right = 0) override;
  int GetRGBAPixelData(
    int x, int y, int x2, int y2, int front, vtkFloatArray* data, int right = 0) override;
  int SetRGBAPixelData(
    int x, int y, int x2, int y2, float* data, int front, int blend = 0, int right = 0) override;
  int SetRGBAPixelData(int x, int y, int x2, int y2, vtkFloatArray* data, int front, int blend = 0,
    int right = 0) override;
  void ReleaseRGBAPixelData(float* data) override;
  unsigned char* GetRGBACharPixelData(
    int x, int y, int x2, int y2, int front, int right = 0) override;
  int GetRGBACharPixelData(
    int x, int y, int x2, int y2, int front, vtkUnsignedCharArray* data, int right = 0) override;
  int SetRGBACharPixelData(int x, int y, int x2, int y2, unsigned char* data, int front,
    int blend = 0, int right = 0) override;
  int SetRGBACharPixelData(int x, int y, int x2, int y2, vtkUnsignedCharArray* data, int front,
    int blend = 0, int right = 0) override;
  ///@}

  ///@{
  /**
   * Set/Get the zbuffer data from an image
   */
  float* GetZbufferData(int x1, int y1, int x2, int y2) override;
  int GetZbufferData(int x1, int y1, int x2, int y2, float* z) override;
  int GetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer) override;
  int SetZbufferData(int x1, int y1, int x2, int y2, float* buffer) override;
  int SetZbufferData(int x1, int y1, int x2, int y2, vtkFloatArray* buffer) override;
  ///@}

  /**
   * Get the size of the color buffer.
   * Returns 0 if not able to determine otherwise sets R G B and A into buffer.
   */
  int GetColorBufferSizes(int* rgba) override;
  /**
   * Block the thread until work queue completes all submitted work.
   */
  void WaitForCompletion() override;

  /**
   * Does this render window support OpenGL? 0-false, 1-true
   */
  int SupportsOpenGL() override;

  /**
   * Get report of capabilities for the render window
   */
  const char* ReportCapabilities() override;

  /**
   * Initialize the render window from the information associated
   * with the currently activated OpenGL context.
   */
  bool InitializeFromCurrentContext() override;

  /**
   * Free up any graphics resources associated with this window
   * a value of NULL means the context may already be destroyed
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  inline wgpu::RenderPassEncoder NewRenderPass(wgpu::RenderPassDescriptor& descriptor)
  {
    return this->CommandEncoder.BeginRenderPass(&descriptor);
  }

  inline wgpu::RenderBundleEncoder NewRenderBundleEncoder(
    wgpu::RenderBundleEncoderDescriptor& descriptor)
  {
    return this->Device.CreateRenderBundleEncoder(&descriptor);
  }

  inline wgpu::CommandEncoder GetCommandEncoder() { return this->CommandEncoder; }
  inline wgpu::TextureView GetOffscreenColorAttachmentView() { return this->ColorAttachment.View; }
  inline wgpu::TextureView GetDepthStencilView() { return this->DepthStencil.View; }
  inline wgpu::TextureFormat GetDepthStencilFormat() { return this->DepthStencil.Format; }
  inline bool HasStencil() { return this->DepthStencil.HasStencil; }
  inline wgpu::Device GetDevice() { return this->Device; }
  inline wgpu::Adapter GetAdapter() { return this->Adapter; }

  wgpu::TextureFormat GetPreferredSwapChainTextureFormat();

protected:
  vtkWebGPURenderWindow();
  ~vtkWebGPURenderWindow() override;

  /**
   * Construct the window title as "Visualization Toolkit - <WindowSystem> <GraphicsBackend>"
   * Ex: "Visualization Toolkit - X11 Vulkan"
   *     "Visualization Toolkit - X11 OpenGL"
   *     "Visualization Toolkit - Cocoa Metal"
   *     "Visualization Toolkit - Cocoa OpenGL"
   *     "Visualization Toolkit - Win32 D3D12"
   */
  virtual std::string MakeDefaultWindowNameWithBackend() = 0;

  bool WGPUInit();
  void WGPUFinalize();

  void CreateSwapChain();
  void DestroySwapChain();

  void CreateOffscreenColorAttachments();
  void DestroyOffscreenColorAttachments();

  void CreateDepthStencilTexture();
  void DestroyDepthStencilTexture();

  void CreateFSQGraphicsPipeline();
  void DestroyFSQGraphicsPipeline();

  void RenderOffscreenTexture();

  void FlushCommandBuffers(vtkTypeUInt32 count, wgpu::CommandBuffer* buffers);

  bool WGPUInitialized = false;

  wgpu::PowerPreference PowerPreference = wgpu::PowerPreference::HighPerformance;
#if defined(__APPLE__)
  wgpu::BackendType RenderingBackendType = wgpu::BackendType::Metal;
#elif defined(_WIN32)
  wgpu::BackendType RenderingBackendType = wgpu::BackendType::D3D12;
#else
  wgpu::BackendType RenderingBackendType = wgpu::BackendType::Vulkan;
#endif

  wgpu::Adapter Adapter;
  wgpu::Device Device;
  wgpu::Surface Surface;
  wgpu::CommandEncoder CommandEncoder;

  struct vtkWGPUSwapChain
  {
    wgpu::SwapChain Instance;
    wgpu::TextureView Framebuffer;
    wgpu::TextureFormat TexFormat;
    wgpu::PresentMode PresentMode;
    int Width = 0;
    int Height = 0;
  };
  vtkWGPUSwapChain SwapChain;

  struct vtkWGPUDeptStencil
  {
    wgpu::Texture Texture;
    wgpu::TextureView View;
    wgpu::TextureFormat Format;
    bool HasStencil;
  };
  vtkWGPUDeptStencil DepthStencil;

  struct vtkWGPUColorAttachment
  {
    wgpu::Texture Texture;
    wgpu::TextureView View;
    wgpu::TextureFormat Format;
    wgpu::Buffer OffscreenBuffer;
  };
  vtkWGPUColorAttachment ColorAttachment;

  struct vtkWGPUUserStagingPixelData
  {
    wgpu::Origin3D Origin;
    wgpu::Extent3D Extent;
    wgpu::TextureDataLayout Layout;
    wgpu::Buffer Buffer; // for SetPixelData
  };
  vtkWGPUUserStagingPixelData StagingPixelData;

  struct vtkWGPUFullScreenQuad
  {
    wgpu::RenderPipeline Pipeline;
    wgpu::BindGroup BindGroup;
  };
  vtkWGPUFullScreenQuad FSQ;

  struct MappingContext
  {
    vtkSmartPointer<vtkTypeUInt8Array> dst;
    wgpu::Buffer src;
    unsigned long size;
    vtkWebGPURenderWindow* window;
  } BufferMapReadContext;

  vtkNew<vtkTypeUInt8Array> CachedPixelBytes;

  int ScreenSize[2];

private:
  vtkWebGPURenderWindow(const vtkWebGPURenderWindow&) = delete;
  void operator=(const vtkWebGPURenderWindow&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
