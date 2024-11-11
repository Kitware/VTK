// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputeOcclusionCuller.h"
#include "OcclusionCullingCopyDepthTexture.h" // for copy depth buffer shader
#include "OcclusionCullingDepthMipmaps.h"     // for depth buffer mipmap computation shader
#include "OcclusionCullingPropsCulling.h"     // for occlusion culling shader
#include "vtkCamera.h"                        // for manipulating the camera
#include "vtkMatrix4x4.h"                     // for the view projection matrix
#include "vtkObjectFactory.h"                 // for vtk standard new macro
#include "vtkRendererCollection.h"
#include "vtkWebGPUComputeBuffer.h"
#include "vtkWebGPUComputePipeline.h"      // for the occlusion culling pipeline
#include "vtkWebGPUComputeRenderTexture.h" // for using the depth buffer of the render pipeline
#include "vtkWebGPUComputeTextureView.h"   // for texture views
#include "vtkWebGPURenderWindow.h" // for submitting command buffers to the render window of the renderer
#include "vtkWebGPURenderer.h" // for getting the actors rendered last frame

#include <unordered_set>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputeOcclusionCuller);

//------------------------------------------------------------------------------
vtkWebGPUComputeOcclusionCuller::vtkWebGPUComputeOcclusionCuller()
{
  this->OcclusionCullingPipeline = vtkSmartPointer<vtkWebGPUComputePipeline>::New();
  this->OcclusionCullingPipeline->SetLabel("WebGPU Occlusion Culler Internal Compute Pipeline");

  this->DepthBufferCopyPass = this->OcclusionCullingPipeline->CreateComputePass();
  this->DepthMipmapsPass = this->OcclusionCullingPipeline->CreateComputePass();
  this->CullingPass = this->OcclusionCullingPipeline->CreateComputePass();
}

//------------------------------------------------------------------------------
vtkWebGPUComputeOcclusionCuller::~vtkWebGPUComputeOcclusionCuller() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Occlusion culling pipeline: ";
  this->OcclusionCullingPipeline->PrintSelf(os, indent);

  os << indent << "Depth buffer copy pass";
  this->DepthBufferCopyPass->PrintSelf(os, indent);

  os << indent
     << "HierarchicalZBufferTextureIndexCopyPass: " << this->HierarchicalZBufferTextureIndexCopyPass
     << std::endl;
  os << indent << "HierarchicalZBufferTextureIndexMipmapsPass: "
     << this->HierarchicalZBufferTextureIndexMipmapsPass << std::endl;
  os << indent << "HierarchicalZBufferTextureIndexCullingPass: "
     << this->HierarchicalZBufferTextureIndexCullingPass << std::endl;

  os << indent << "HierarchicalZ Buffer mipmap views: ";
  for (vtkSmartPointer<vtkWebGPUComputeTextureView> mipmapView :
    this->HierarchicalZBufferMipmapViews)
  {
    mipmapView->PrintSelf(os, indent);
  }

  os << indent << "HierarchicalZ Buffer mipmap views indices: ";
  for (int index : this->HierarchicalZBufferMipmapViewsIndices)
  {
    os << indent << index << std::endl;
  }

  os << indent << "HierarchicalZ buffer mipmap count: " << this->HierarchicalZBufferMipmapCount
     << std::endl;
  os << indent << "HierarchicalZ Buffer mimaps [widths, heights]: " << std::endl;
  for (std::size_t index = 0; index < this->MipmapWidths.size(); index++)
  {
    os << indent << "\t [" << this->MipmapWidths[index] << ", " << this->MipmapHeights[index] << "]"
       << std::endl;
  }

  os << indent << "Depth mipmap pass: ";
  this->DepthMipmapsPass->PrintSelf(os, indent);

  os << indent << "Culling pass: ";
  this->CullingPass->PrintSelf(os, indent);

  os << indent << "CullingPassboundsBufferIndex: " << this->CullingPassBoundsBufferIndex
     << std::endl;
  os << indent
     << "CullingPassOutputIndicesBufferIndex: " << this->CullingPassOutputIndicesBufferIndex
     << std::endl;
  os << indent << "CullingPassOutputIndicesCountBufferIndex: "
     << this->CullingPassOutputIndicesCountBufferIndex << std::endl;

  os << indent << "CullingPassOutputIndicesCulledBufferIndex: "
     << this->CullingPassOutputIndicesCulledBufferIndex << std::endl;
  os << indent << "CullingPassOutputIndicesCulledCountBufferIndex: "
     << this->CullingPassOutputIndicesCulledCountBufferIndex << std::endl;
  os << indent << "CullingPassBoundsCountBufferIndex: " << this->CullingPassBoundsCountBufferIndex
     << std::endl;
  os << indent << "CullingPassMVPMatrixBufferIndex: " << this->CullingPassMVPMatrixBufferIndex
     << std::endl;

  os << indent << "First frame?: " << this->FirstFrame << std::endl;
  os << indent << "Initialized: " << this->Initialized << std::endl;
  os << indent << "WebGPURenderWindow: " << this->WebGPURenderWindow << std::endl;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::SetRenderWindow(vtkWebGPURenderWindow* renderWindow)
{
  this->WebGPURenderWindow = renderWindow;
  if (this->WebGPURenderWindow == nullptr)
  {
    vtkLog(ERROR,
      "Calling vtkWebGPUComputeOcclusionCuller::SetRenderWindow with a nullptr renderWindow "
      "parameter.");

    return;
  }

  if (!renderWindow->GetInitialized())
  {
    // Check for the user in case they forgot to call RenderWindow::Initialize before setting up the
    // render window on the occlusion culler
    vtkLog(ERROR,
      "You must call RenderWindow::Initialize() before setting the RenderWindow on the "
      "vtkWebGPUOcclusionCuller.");

    return;
  }

  this->WindowResizedCallbackCommand = vtkSmartPointer<vtkCallbackCommand>::New();
  this->WindowResizedCallbackCommand->SetCallback(
    vtkWebGPUComputeOcclusionCuller::WindowResizedCallback);
  this->WindowResizedCallbackCommand->SetClientData(this);
  this->WebGPURenderWindow->AddObserver(
    vtkCommand::WindowResizeEvent, this->WindowResizedCallbackCommand);

  this->OcclusionCullingPipeline->SetWGPUConfiguration(
    this->WebGPURenderWindow->GetWGPUConfiguration());

  // Setting everything up so that everything is ready when Cull() will be called
  this->SetupDepthBufferCopyPass();
  this->SetupMipmapsPass();
  this->SetupCullingPass();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::SetupDepthBufferCopyPass()
{
  if (this->WebGPURenderWindow == nullptr)
  {
    return;
  }

  vtkSmartPointer<vtkWebGPUComputeRenderTexture> depthTexture;
  depthTexture = this->WebGPURenderWindow->AcquireDepthBufferRenderTexture();
  depthTexture->SetLabel("Depth buffer texture for depth buffer copy pass");

  this->DepthBufferCopyPass->SetShaderSource(OcclusionCullingCopyDepthTexture);
  this->DepthBufferCopyPass->SetShaderEntryPoint("computeMain");

  const int index = this->DepthBufferCopyPass->AddRenderTexture(depthTexture);

  auto depthTextureView = this->DepthBufferCopyPass->CreateTextureView(index);
  depthTextureView->SetGroup(0);
  depthTextureView->SetBinding(0);
  depthTextureView->SetLabel("Depth buffer texture view depth buffer copy pass");
  depthTextureView->SetMode(vtkWebGPUTextureView::TextureViewMode::READ_ONLY);
  depthTextureView->SetAspect(vtkWebGPUTextureView::TextureViewAspect::ASPECT_DEPTH);
  depthTextureView->SetFormat(vtkWebGPUTexture::TextureFormat::DEPTH_24_PLUS);
  this->DepthBufferCopyPass->AddTextureView(depthTextureView);

  this->DepthBufferCopyPass->SetLabel("Depth buffer copy compute pass");
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::SetupMipmapsPass()
{
  this->DepthMipmapsPass->SetShaderSource(OcclusionCullingDepthMipmaps);
  this->DepthMipmapsPass->SetShaderEntryPoint("computeMain");
  this->DepthMipmapsPass->SetLabel("Depth buffer mipmaps compute pass");
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::SetupCullingPass()
{
  this->CullingPass->SetShaderSource(OcclusionCullingPropsCulling);
  this->CullingPass->SetShaderEntryPoint("computeMain");
  this->CullingPass->SetLabel("Occlusion culler culling pass");
}

//------------------------------------------------------------------------------
double vtkWebGPUComputeOcclusionCuller::Cull(
  vtkRenderer* renderer, vtkProp** propList, int& listLength, int& initialized)
{
  if (this->WebGPURenderWindow == nullptr)
  {
    // Render window not set
    vtkLog(ERROR,
      "The render window of this occlusion culler wasn't set by calling SetRenderWindow()! The "
      "occlusion culler cannot "
      "continue.");

    return listLength;
  }

  if (!this->Initialized)
  {
    // Adding the occlusion culling pipeline to the renderer so that it can reuse the texture (depth
    // buffer mainly) of the render window of the renderer
    this->AddOcclusionCullingPipelineToRenderer(renderer);

    // Some setup can only be done once we have the render window size so that's why we're doing
    // it here. Also, buffers can only be created now because before this point, the render window
    // Device and Adapter wasn't set on the occlusion culler pipeline which means that we would be
    // creating buffers with a device that is not the one from the render window. We need to
    // create the buffers on the device of the render window.
    this->CreateHierarchicalZBuffer();
    this->FinishSetupDepthCopyPass();
    this->FinishSetupMipmapsPass();
    this->FinishSetupCullingPass();

    this->Initialized = true;
  }

  // First rendering the actors that were rendered last frame to fill the z-buffer
  this->FirstPassRender(renderer, propList, listLength);
  // Copy the depth buffer to the hierarchical z-buffer texture
  this->CopyDepthBuffer();
  // Compute the mipmaps of the depth buffer
  this->DepthMipmaps();
  // Culls the given list of props against the hierarchical z-buffer
  this->PropCulling(renderer, propList, listLength);

  this->OcclusionCullingPipeline->Update();
  this->FirstFrame = false;

  initialized = this->Initialized;

  return listLength;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::AddOcclusionCullingPipelineToRenderer(vtkRenderer* renderer)
{
  vtkWebGPURenderer* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  if (wgpuRenderer == nullptr)
  {
    vtkLog(ERROR,
      "Cannot add the occlusion culling compute pipeline to the renderer "
        << renderer << " because it is not a vtkWebGPURenderer");

    return;
  }

  wgpuRenderer->AddPreRenderComputePipeline(this->OcclusionCullingPipeline);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::CreateHierarchicalZBuffer()
{
  if (this->WebGPURenderWindow == nullptr)
  {
    return;
  }

  int* renderWindowSize = this->WebGPURenderWindow->GetSize();
  int width = renderWindowSize[0];
  int height = renderWindowSize[1];

  int numMipLevels = this->ComputeMipLevelsSizes(width, height);

  vtkSmartPointer<vtkWebGPUComputeTexture> hierarchicalZBuffer =
    vtkSmartPointer<vtkWebGPUComputeTexture>::New();
  // Read/write mode here because we're going to have to write and read from it when computing the
  // mipmaps
  hierarchicalZBuffer->SetFormat(vtkWebGPUComputeTexture::TextureFormat::R32_FLOAT);
  hierarchicalZBuffer->SetMode(vtkWebGPUComputeTexture::TextureMode::READ_WRITE_STORAGE);
  hierarchicalZBuffer->SetSampleType(
    vtkWebGPUComputeTexture::TextureSampleType::UNFILTERABLE_FLOAT);
  hierarchicalZBuffer->SetLabel("Compute occlusion culler hierarchical z-buffer texture");
  hierarchicalZBuffer->SetSize(width, height);
  hierarchicalZBuffer->SetMipLevelCount(numMipLevels);
  this->HierarchicalZBufferMipmapCount = numMipLevels;

  this->HierarchicalZBufferTextureIndexCopyPass =
    this->DepthBufferCopyPass->AddTexture(hierarchicalZBuffer);
  this->HierarchicalZBufferTextureIndexMipmapsPass =
    this->DepthMipmapsPass->AddTexture(hierarchicalZBuffer);
  this->HierarchicalZBufferTextureIndexCullingPass =
    this->CullingPass->AddTexture(hierarchicalZBuffer);
}

//------------------------------------------------------------------------------
int vtkWebGPUComputeOcclusionCuller::ComputeMipLevelsSizes(int width, int height)
{
  this->MipmapWidths.clear();
  this->MipmapHeights.clear();

  int numMipLevels = 0;
  int mipWidth = width;
  int mipHeight = height;
  // We're going to stop when the X or Y dimension (whichever is the smallest) reaches 2 pixel
  while (mipWidth > 0 || mipHeight > 0)
  {
    // Clamping at 1 to avoid zero dimension mips (will happen if the texture isn't square)
    mipWidth = std::max(1, mipWidth);
    mipHeight = std::max(1, mipHeight);

    this->MipmapWidths.push_back(mipWidth);
    this->MipmapHeights.push_back(mipHeight);
    numMipLevels++;

    mipWidth = std::floor(mipWidth / 2.0f);
    mipHeight = std::floor(mipHeight / 2.0f);
  }

  return numMipLevels;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::ResizeHierarchicalZBuffer(
  uint32_t newWidth, uint32_t newHeight)
{
  this->HierarchicalZBufferMipmapCount = this->ComputeMipLevelsSizes(newWidth, newHeight);

  // Updating the extents and the number of mip levels of the texture
  vtkSmartPointer<vtkWebGPUComputeTexture> texture =
    this->CullingPass->GetComputeTexture(this->HierarchicalZBufferTextureIndexCullingPass);
  texture->SetWidth(newWidth);
  texture->SetHeight(newHeight);
  texture->SetMipLevelCount(this->HierarchicalZBufferMipmapCount);

  // Update the number of mip levels of the texture view of the hierarchical z buffer
  vtkSmartPointer<vtkWebGPUComputeTextureView> hierarchicalZBufferView =
    this->CullingPass->GetTextureView(this->CullingPassHierarchicalZBufferView);
  hierarchicalZBufferView->SetMipLevelCount(this->HierarchicalZBufferMipmapCount);

  this->CullingPass->RecreateComputeTexture(this->HierarchicalZBufferTextureIndexCullingPass);
  this->CullingPass->RecreateTextureView(this->CullingPassHierarchicalZBufferView);
  // Because the size of the window has changed, we may have more or less mipmaps
  this->ResizeHierarchicalZBufferMipmapsChain();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::ResizeHierarchicalZBufferMipmapsChain()
{
  this->DepthMipmapsPass->DeleteTextureViews(this->HierarchicalZBufferTextureIndexMipmapsPass);
  this->FinishSetupMipmapsPass();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::FinishSetupDepthCopyPass()
{
  vtkSmartPointer<vtkWebGPUComputeTextureView> hiZBufferViewCopyDepth;
  hiZBufferViewCopyDepth =
    this->DepthBufferCopyPass->CreateTextureView(this->HierarchicalZBufferTextureIndexCopyPass);
  hiZBufferViewCopyDepth->SetBaseMipLevel(0);
  hiZBufferViewCopyDepth->SetMode(vtkWebGPUComputeTextureView::TextureViewMode::WRITE_ONLY_STORAGE);
  hiZBufferViewCopyDepth->SetGroup(0);
  hiZBufferViewCopyDepth->SetBinding(1);
  hiZBufferViewCopyDepth->SetLabel("Depth buffer copy pass HierarchicalZBuffer view");
  this->DepthBufferCopyPass->AddTextureView(hiZBufferViewCopyDepth);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::FinishSetupMipmapsPass()
{
  this->HierarchicalZBufferMipmapViews.resize(this->HierarchicalZBufferMipmapCount);
  this->HierarchicalZBufferMipmapViewsIndices.resize(this->HierarchicalZBufferMipmapCount);

  // Creating all the views for mipmaps of the hi-z buffer
  for (int i = 0; i < this->HierarchicalZBufferMipmapCount; i++)
  {
    vtkSmartPointer<vtkWebGPUComputeTextureView> hiZBufferView;
    hiZBufferView =
      this->DepthMipmapsPass->CreateTextureView(HierarchicalZBufferTextureIndexMipmapsPass);
    hiZBufferView->SetBaseMipLevel(i);
    hiZBufferView->SetAspect(vtkWebGPUComputeTextureView::TextureViewAspect::ASPECT_ALL);
    hiZBufferView->SetDimension(vtkWebGPUComputeTexture::TextureDimension::DIMENSION_2D);
    hiZBufferView->SetFormat(vtkWebGPUComputeTexture::TextureFormat::R32_FLOAT);
    hiZBufferView->SetLabel(
      std::string("Depth mipmap pass HierarchicalZBuffer view - mip ") + std::to_string(i));

    this->HierarchicalZBufferMipmapViews[i] = hiZBufferView;
    this->HierarchicalZBufferMipmapViewsIndices[i] =
      this->DepthMipmapsPass->AddTextureView(hiZBufferView);
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::FinishSetupCullingPass()
{
  /**
   * Input buffers of the occlusion culling
   */
  vtkSmartPointer<vtkWebGPUComputeBuffer> MVPBuffer;
  MVPBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  MVPBuffer->SetGroup(0);
  MVPBuffer->SetBinding(0);
  MVPBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
  MVPBuffer->SetLabel("Occlusion culler culling pass MVP matrix buffer");
  // 4x4 float matrix size
  MVPBuffer->SetByteSize(16 * sizeof(float));

  vtkSmartPointer<vtkWebGPUComputeBuffer> boundsBuffer;
  boundsBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  boundsBuffer->SetGroup(0);
  boundsBuffer->SetBinding(1);
  boundsBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_ONLY_COMPUTE_STORAGE);
  boundsBuffer->SetLabel("Occlusion culler culling pass input bounds buffer");
  // Initially set to 1 bounds capacity. The buffer will have to be resized on the first cull() call
  // anyway.
  boundsBuffer->SetByteSize(6 * sizeof(float));

  vtkSmartPointer<vtkWebGPUComputeBuffer> boundsCountBuffer;
  boundsCountBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  boundsCountBuffer->SetGroup(0);
  boundsCountBuffer->SetBinding(2);
  boundsCountBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::UNIFORM_BUFFER);
  boundsCountBuffer->SetLabel("Occlusion culler culling pass input bounds count buffer");
  boundsCountBuffer->SetByteSize(1 * sizeof(unsigned int));

  /**
   * Buffers for the results of the occlusion culling
   */
  vtkSmartPointer<vtkWebGPUComputeBuffer> outputIndicesBuffer;
  outputIndicesBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  outputIndicesBuffer->SetGroup(1);
  outputIndicesBuffer->SetBinding(0);
  outputIndicesBuffer->SetMode(vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputIndicesBuffer->SetLabel("Occlusion culler culling pass output bounds indices buffer");
  // Initially set to 1 output capacity, will have to be resized
  outputIndicesBuffer->SetByteSize(1 * sizeof(unsigned int));

  vtkSmartPointer<vtkWebGPUComputeBuffer> outputIndicesCountBuffer;
  outputIndicesCountBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  outputIndicesCountBuffer->SetGroup(1);
  outputIndicesCountBuffer->SetBinding(1);
  outputIndicesCountBuffer->SetMode(
    vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputIndicesCountBuffer->SetLabel(
    "Occlusion culler culling pass output bounds indices count buffer");
  outputIndicesCountBuffer->SetByteSize(1 * sizeof(unsigned int));

  vtkSmartPointer<vtkWebGPUComputeBuffer> outputCulledIndicesBuffer;
  outputCulledIndicesBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  outputCulledIndicesBuffer->SetGroup(1);
  outputCulledIndicesBuffer->SetBinding(2);
  outputCulledIndicesBuffer->SetMode(
    vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputCulledIndicesBuffer->SetLabel(
    "Occlusion culler culling pass output bounds culled indices buffer");
  outputCulledIndicesBuffer->SetByteSize(1 * sizeof(unsigned int));

  vtkSmartPointer<vtkWebGPUComputeBuffer> outputIndicesCulledCountBuffer;
  outputIndicesCulledCountBuffer = vtkSmartPointer<vtkWebGPUComputeBuffer>::New();
  outputIndicesCulledCountBuffer->SetGroup(1);
  outputIndicesCulledCountBuffer->SetBinding(3);
  outputIndicesCulledCountBuffer->SetMode(
    vtkWebGPUComputeBuffer::BufferMode::READ_WRITE_MAP_COMPUTE_STORAGE);
  outputIndicesCulledCountBuffer->SetLabel(
    "Occlusion culler culling pass output bounds culled indices count buffer");
  outputIndicesCulledCountBuffer->SetByteSize(1 * sizeof(unsigned int));

  // Hi-z buffer
  vtkSmartPointer<vtkWebGPUComputeTextureView> hiZBufferView;
  hiZBufferView =
    this->CullingPass->CreateTextureView(this->HierarchicalZBufferTextureIndexCullingPass);
  hiZBufferView->SetLabel("Occlusion culler - hierarchical z buffer view culling pass");
  hiZBufferView->SetBinding(3);
  hiZBufferView->SetGroup(0);
  hiZBufferView->SetFormat(vtkWebGPUComputeTexture::TextureFormat::R32_FLOAT);
  hiZBufferView->SetMode(vtkWebGPUComputeTextureView::TextureViewMode::READ_ONLY);
  hiZBufferView->SetMipLevelCount(this->HierarchicalZBufferMipmapCount);

  this->CullingPassMVPMatrixBufferIndex = this->CullingPass->AddBuffer(MVPBuffer);
  this->CullingPassBoundsBufferIndex = this->CullingPass->AddBuffer(boundsBuffer);

  this->CullingPassOutputIndicesBufferIndex = this->CullingPass->AddBuffer(outputIndicesBuffer);
  this->CullingPassOutputIndicesCountBufferIndex =
    this->CullingPass->AddBuffer(outputIndicesCountBuffer);
  this->CullingPassOutputIndicesCulledBufferIndex =
    this->CullingPass->AddBuffer(outputCulledIndicesBuffer);
  this->CullingPassOutputIndicesCulledCountBufferIndex =
    this->CullingPass->AddBuffer(outputIndicesCulledCountBuffer);
  this->CullingPassBoundsCountBufferIndex = this->CullingPass->AddBuffer(boundsCountBuffer);
  this->CullingPassHierarchicalZBufferView = this->CullingPass->AddTextureView(hiZBufferView);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::FirstPassRender(
  vtkRenderer* renderer, vtkProp** propList, int listLength)
{
  vtkWebGPURenderer* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  if (!wgpuRenderer)
  {
    vtkErrorWithObjectMacro(
      this, << "Could not get the vtkWebGPURenderer. Is this occlusion culler used outside of a "
               "vtkWebGPURenderer?");

    return;
  }

  vtkWebGPURenderWindow* wgpuRenderWindow;
  wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(wgpuRenderer->GetRenderWindow());
  if (wgpuRenderWindow == nullptr)
  {
    vtkErrorWithObjectMacro(
      this, << "The render window of the renderer used by this occlusion culler is null.");

    return;
  }

  // Building the list of actors that will need to be rendered in this first pass. We want the
  // actors that were rendered last frame but also the actors that passed the potential previous
  // culling passes hence why we compute the intersection of the two lists
  std::unordered_set<vtkProp*> propsRenderedLastFrame = wgpuRenderer->GetPropsRendered();
  std::vector<vtkProp*> propsToRenderFirstPass;
  propsToRenderFirstPass.reserve(propsRenderedLastFrame.size());
  for (int i = 0; i < listLength; i++)
  {
    vtkProp* prop = propList[i];

    if (this->FirstFrame)
    {
      // On the first frame, everyone is rendered to fill the z-buffer and then everyone is culled
      // so that objects that are initially occluded are not rendered in the following frames
      propsToRenderFirstPass.push_back(prop);
    }
    else
    {
      if (propsRenderedLastFrame.find(prop) != propsRenderedLastFrame.end())
      {
        // Rendering props from last frame by checking if the current prop we're iterating over is
        // in the list of props rendered last frame
        propsToRenderFirstPass.push_back(prop);
      }
    }
  }

  // Creating and submitting the draw command to the render window so that the props of the last
  // frame are rendered and the depth buffer is filled
  wgpu::CommandBuffer commandBuffer;

  commandBuffer = wgpuRenderer->EncodePropListRenderCommand(
    propsToRenderFirstPass.data(), propsToRenderFirstPass.size());
  wgpuRenderWindow->SubmitCommandBuffer(1, &commandBuffer);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::CopyDepthBuffer()
{
  if (this->WebGPURenderWindow == nullptr)
  {
    return;
  }

  int* renderWindowSize = this->WebGPURenderWindow->GetSize();
  int nbGroupsX = std::ceil(renderWindowSize[0] / 8.0f);
  int nbGroupsY = std::ceil(renderWindowSize[1] / 8.0f);

  this->DepthBufferCopyPass->SetWorkgroups(nbGroupsX, nbGroupsY, 1);
  this->DepthBufferCopyPass->Dispatch();
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::DepthMipmaps()
{
  for (int i = 0; i < this->HierarchicalZBufferMipmapCount - 1; i++)
  {
    // i + 1 here because we want one thread per pixel of the ** output ** mip level
    int nbGroupsX = std::ceil(this->MipmapWidths[i + 1] / 8.0f);
    int nbGroupsY = std::ceil(this->MipmapHeights[i + 1] / 8.0f);

    this->HierarchicalZBufferMipmapViews[i]->SetMode(
      vtkWebGPUComputeTextureView::TextureViewMode::READ_ONLY);
    this->HierarchicalZBufferMipmapViews[i + 1]->SetMode(
      vtkWebGPUComputeTextureView::TextureViewMode::WRITE_ONLY_STORAGE);

    this->DepthMipmapsPass->SetWorkgroups(nbGroupsX, nbGroupsY, 1);
    this->DepthMipmapsPass->RebindTextureView(0, 0, this->HierarchicalZBufferMipmapViewsIndices[i]);
    this->DepthMipmapsPass->RebindTextureView(
      0, 1, this->HierarchicalZBufferMipmapViewsIndices[i + 1]);
    this->DepthMipmapsPass->Dispatch();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::PropCulling(
  vtkRenderer* renderer, vtkProp** propList, int& listLength)
{
  int nbGroupsX = std::ceil(listLength / 32.0f);
  if (nbGroupsX == 0)
  {
    listLength = 0;

    // No props to cull
    return;
  }

  vtkWebGPURenderer* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);

  this->UpdateCameraMVPBuffer(renderer);
  this->UpdateBoundsBuffers(propList, listLength);
  this->CullingPass->SetWorkgroups(nbGroupsX, 1, 1);
  this->CullingPass->Dispatch();

  // Dynamically allocated so that it stays alive beyond the current scope (needed since
  // the function which uses the MapData will be called later as a callback).
  vtkWebGPUComputeOcclusionCuller::OutputIndicesCulledMapData* mapDataOutputIndicesCulled;
  mapDataOutputIndicesCulled = new vtkWebGPUComputeOcclusionCuller::OutputIndicesCulledMapData;
  mapDataOutputIndicesCulled->renderer = wgpuRenderer;
  mapDataOutputIndicesCulled->propList = propList;

  vtkWebGPUComputeOcclusionCuller::FillObjectsToDrawCallbackMapData* fillObjectsToDrawMapData;
  fillObjectsToDrawMapData = new vtkWebGPUComputeOcclusionCuller::FillObjectsToDrawCallbackMapData;
  fillObjectsToDrawMapData->listLength = &listLength;
  fillObjectsToDrawMapData->propList = propList;
  fillObjectsToDrawMapData->renderer = wgpuRenderer;

  // Reading the number of objects that passed the culling test
  this->CullingPass->ReadBufferFromGPU(this->CullingPassOutputIndicesCountBufferIndex,
    vtkWebGPUComputeOcclusionCuller::ReadIndicesCountCallback, &listLength);

  // Reading the number of objects that didn't pass the culling test
  this->CullingPass->ReadBufferFromGPU(this->CullingPassOutputIndicesCulledCountBufferIndex,
    vtkWebGPUComputeOcclusionCuller::ReadIndicesCountCallback,
    &mapDataOutputIndicesCulled->culledCount);

  this->CullingPass->ReadBufferFromGPU(this->CullingPassOutputIndicesCulledBufferIndex,
    vtkWebGPUComputeOcclusionCuller::OutputIndicesCulledCallback, mapDataOutputIndicesCulled);

  this->CullingPass->ReadBufferFromGPU(this->CullingPassOutputIndicesBufferIndex,
    vtkWebGPUComputeOcclusionCuller::FillObjectsToDrawCallback, fillObjectsToDrawMapData);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::ReadIndicesCountCallback(
  const void* mappedData, void* indicesCount)
{
  *static_cast<int*>(indicesCount) = *static_cast<const int*>(mappedData);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::FillObjectsToDrawCallback(const void* mappedData, void* data)
{
  vtkWebGPUComputeOcclusionCuller::FillObjectsToDrawCallbackMapData* mapData;
  mapData = static_cast<vtkWebGPUComputeOcclusionCuller::FillObjectsToDrawCallbackMapData*>(data);

  const unsigned int* passedIndices = static_cast<const unsigned int*>(mappedData);

  // List of props that passed the culling test. This list is going to be copied to the front of
  // propList at the end
  std::vector<vtkProp*> passedProps(*mapData->listLength);

  // Copying the props that passed the test to the temporary list
  for (int i = 0; i < *mapData->listLength; i++)
  {
    passedProps[i] = mapData->propList[passedIndices[i]];
  }

  // Adding the props that passed the culling AND that were not rendered in the first so that they
  // are rendered after the occlusion culling pipeline
  int propListIndex = 0;
  std::unordered_set<vtkProp*>& propsRenderedLastFrame = mapData->renderer->PropsRendered;
  for (std::size_t i = 0; i < passedProps.size(); i++)
  {
    vtkProp* prop = passedProps[i];
    if (propsRenderedLastFrame.find(prop) == propsRenderedLastFrame.end())
    {
      // If the prop wasn't rendered last frame
      mapData->propList[propListIndex++] = passedProps[i];
    }
    else
    {
      // If the prop passed the culling test (because it is being processed by this callback method)
      // but was already rendered last frame, meaning that is was rendered by the first pass, we
      // need not to render it again so we're decrementing the number of props to be rendered
      *mapData->listLength = *mapData->listLength - 1;
    }
  }

  delete mapData;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::OutputIndicesCulledCallback(
  const void* mappedData, void* data)
{
  vtkWebGPURenderer* wgpuRenderer;
  vtkWebGPUComputeOcclusionCuller::OutputIndicesCulledMapData* mapData;
  mapData = static_cast<vtkWebGPUComputeOcclusionCuller::OutputIndicesCulledMapData*>(data);
  wgpuRenderer = mapData->renderer;

  const unsigned int* culledIndices = static_cast<const unsigned int*>(mappedData);

  for (int i = 0; i < mapData->culledCount; i++)
  {
    vtkProp* prop = mapData->propList[culledIndices[i]];

    auto find = wgpuRenderer->PropsRendered.find(prop);
    if (find != wgpuRenderer->PropsRendered.end())
    {
      // Only removing if it was in the list already.
      // Erase works fine if the item to remove wasn't even in the set, however, we only want to
      // execute wgpuRenderer->NumberOfPropsRendered--; if we actually removed an item so the if
      // statement is necessary
      wgpuRenderer->PropsRendered.erase(prop);
      wgpuRenderer->NumberOfPropsRendered--;
    }
  }

  delete mapData;
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::WindowResizedCallback(
  vtkObject* caller, unsigned long vtkNotUsed(eid), void* clientdata, void* vtkNotUsed(calldata))
{
  vtkWebGPUComputeOcclusionCuller* occlusionCuller =
    static_cast<vtkWebGPUComputeOcclusionCuller*>(clientdata);

  vtkWebGPURenderWindow* renderWindow = vtkWebGPURenderWindow::SafeDownCast(caller);
  int* newSize = renderWindow->GetSize();

  occlusionCuller->ResizeHierarchicalZBuffer(newSize[0], newSize[1]);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::UpdateCameraMVPBuffer(vtkRenderer* renderer)
{
  vtkCamera* camera = renderer->GetActiveCamera();
  // Getting the view-projection matrix
  vtkMatrix4x4* viewMatrix = camera->GetModelViewTransformMatrix();

  // We're using [0, 1] for znear and zfar here to align with WebGPU convention but [-1, 1] as in
  // OpenGL would have worked too since we're not using the graphics pipeline (compute shader only)
  // that actually expects [0, 1]
  vtkMatrix4x4* projectionMatrix =
    camera->GetProjectionTransformMatrix(renderer->GetTiledAspectRatio(), -1, 1);
  vtkNew<vtkMatrix4x4> viewProj;
  vtkMatrix4x4::Multiply4x4(projectionMatrix, viewMatrix, viewProj);
  // WebGPU uses column major matrices but VTK is row major
  viewProj->Transpose();

  // Getting the matrix data as floats
  std::vector<float> matrixData(16);
  for (int i = 0; i < 16; i++)
  {
    matrixData[i] = static_cast<float>(viewProj->GetData()[i]);
  }

  this->CullingPass->UpdateBufferData(this->CullingPassMVPMatrixBufferIndex, matrixData);
}

//------------------------------------------------------------------------------
void vtkWebGPUComputeOcclusionCuller::UpdateBoundsBuffers(vtkProp** propList, int listLength)
{
  // Resizing if necessary
  unsigned int necessaryByteSize = sizeof(float) * 6 * listLength;
  if (this->CullingPass->GetBufferByteSize(this->CullingPassBoundsBufferIndex) < necessaryByteSize)
  {
    // If we now have more props to cull than the buffers were sized for
    this->CullingPass->ResizeBuffer(this->CullingPassBoundsBufferIndex, necessaryByteSize);
    this->CullingPass->ResizeBuffer(
      this->CullingPassOutputIndicesBufferIndex, listLength * sizeof(unsigned int));
    this->CullingPass->ResizeBuffer(
      this->CullingPassOutputIndicesCulledBufferIndex, listLength * sizeof(unsigned int));
  }

  // Updating the buffers data
  std::vector<float> boundsData;
  boundsData.reserve(listLength * 6);
  for (int i = 0; i < listLength; i++)
  {
    double* bounds = propList[i]->GetBounds();
    for (int j = 0; j < 6; j++)
    {
      boundsData.push_back(static_cast<float>(bounds[j]));
    }
  }

  std::vector<unsigned int> boundsCountData(1, listLength);
  std::vector<unsigned int> zeroData(1, 0);

  this->CullingPass->UpdateBufferData(this->CullingPassBoundsBufferIndex, boundsData);
  this->CullingPass->UpdateBufferData(this->CullingPassBoundsCountBufferIndex, boundsCountData);
  this->CullingPass->UpdateBufferData(this->CullingPassOutputIndicesCountBufferIndex, zeroData);
  this->CullingPass->UpdateBufferData(
    this->CullingPassOutputIndicesCulledCountBufferIndex, zeroData);
}
