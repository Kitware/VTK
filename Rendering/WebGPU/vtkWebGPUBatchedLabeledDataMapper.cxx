// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUBatchedLabeledDataMapper.h"
#include "Private/vtkWebGPUBatchedLabeledDataMapperInternals.h"

#include "vtkActor.h"
#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUBatchedLabeledDataMapper);

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUBatchedLabeledDataMapper::CreateOverrideAttributes()
{
  return vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
}

//----------------------------------------------------------------------------
vtkWebGPUBatchedLabeledDataMapper::vtkWebGPUBatchedLabeledDataMapper()
{
  this->Helper->Parent = this;
  // The shaped-points pipeline (used for label quads) is skipped when pointSize <= 1.
  // Set it to 2 so the pipeline runs; our shader ignores the actual point size value.
  this->DummyActor->GetProperty()->SetPointSize(2.0f);
}

//----------------------------------------------------------------------------
vtkWebGPUBatchedLabeledDataMapper::~vtkWebGPUBatchedLabeledDataMapper() = default;

//----------------------------------------------------------------------------
void vtkWebGPUBatchedLabeledDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HelperSetup: " << (this->HelperSetup ? "true\n" : "false\n");
}

//----------------------------------------------------------------------------
void vtkWebGPUBatchedLabeledDataMapper::SetLabelTextProperty(vtkTextProperty* prop, int type)
{
  this->Superclass::SetLabelTextProperty(prop, type);
  this->HelperSetup = false;
}

//----------------------------------------------------------------------------
void vtkWebGPUBatchedLabeledDataMapper::SetupHelper()
{
  // Unlike OpenGL which calls MapDataArrayToVertexAttribute for each per-label array,
  // WebGPU uploads per-label data as raw instance buffers in UpdateInstanceBuffers.
  // No VBO attribute mapping is needed here.
  this->HelperSetup = true;
}

//----------------------------------------------------------------------------
void vtkWebGPUBatchedLabeledDataMapper::RenderOpaqueGeometry(
  vtkViewport* viewport, vtkActor2D* vtkNotUsed(actor))
{
  // Check input first, then renderer — same order as OpenGL.
  vtkDataObject* inputDO = this->GetInputDataObject(0, 0);
  if (!inputDO)
  {
    this->NumberOfLabels = 0;
    vtkErrorMacro(<< "Need input data to render labels");
    return;
  }

  vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
  if (!ren)
  {
    return;
  }

  // WebGPU-specific: need the render window to upload the atlas texture.
  auto* wgpuRenderWindow = vtkWebGPURenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (!wgpuRenderWindow)
  {
    return;
  }

  this->UpdateRenderWindowDPI(ren->GetRenderWindow()->GetDPI());

  if (this->GetMTime() > this->BuildTime || inputDO->GetMTime() > this->BuildTime)
  {
    this->BuildLabels();
    // Unlike OpenGL, BuildTime is NOT stamped here. The atlas upload below
    // needs to see an old BuildTime so that atlas->GetMTime() > BuildTime fires
    // on the first render after a rebuild. BuildTime is stamped after the full
    // render at the end of this function.
  }

  if (!this->HelperSetup)
  {
    this->SetupHelper();
  }

  // Upload the atlas texture. Deferred from UploadGlyphAtlas because
  // vtkWebGPURenderWindow is needed to allocate GPU resources.
  vtkImageData* atlas = this->GetGlyphAtlas();
  if (atlas && (!this->Helper->GlyphsTexture || atlas->GetMTime() > this->BuildTime))
  {
    auto* wgpuConfiguration = wgpuRenderWindow->GetWGPUConfiguration();
    int* dims = atlas->GetDimensions();
    if (this->Helper->GlyphsTexture)
    {
      this->Helper->GlyphsTexture.Destroy();
    }
    this->Helper->GlyphsTexture = wgpuConfiguration->CreateTexture(
      { static_cast<uint32_t>(dims[0]), static_cast<uint32_t>(dims[1]), 1 },
      wgpu::TextureDimension::e2D, wgpu::TextureFormat::RGBA8Unorm,
      wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst);
    this->Helper->GlyphsTextureView = wgpuConfiguration->CreateView(this->Helper->GlyphsTexture,
      wgpu::TextureViewDimension::e2D, wgpu::TextureAspect::All, wgpu::TextureFormat::RGBA8Unorm,
      /*baseMipLevel=*/0, /*mipLevelCount=*/1);
    wgpuConfiguration->WriteTexture(this->Helper->GlyphsTexture, static_cast<uint32_t>(dims[0]) * 4,
      static_cast<uint32_t>(dims[0]) * static_cast<uint32_t>(dims[1]) * 4,
      atlas->GetScalarPointer());
    this->Helper->InvalidatePipelines();
  }

  this->Helper->SetInputData(this->GetPreparedPolyData());
  // WebGPU: DummyActor->Render instead of Helper->RenderPiece. The actor Render
  // sets up per-actor bind group 1 (transform buffer); bypassing it via RenderPiece
  // causes WebGPU pipeline validation errors. In OpenGL the equivalent state is set
  // as shader uniforms, so RenderPiece works there.
  this->DummyActor->Render(ren, this->Helper);
  this->BuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkWebGPUBatchedLabeledDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->Helper->ReleaseGraphicsResources(win);
}

VTK_ABI_NAMESPACE_END
