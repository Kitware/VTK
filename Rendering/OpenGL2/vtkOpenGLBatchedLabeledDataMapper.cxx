// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOpenGLBatchedLabeledDataMapper.h"
#include "Private/vtkOpenGLBatchedLabeledDataMapperInternals.h"

#include "vtkDataObject.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOverrideAttribute.h"
#include "vtkRenderer.h"
#include "vtkTextureObject.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkOpenGLBatchedLabeledDataMapper);

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkOpenGLBatchedLabeledDataMapper::CreateOverrideAttributes()
{
  return vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "OpenGL", nullptr);
}

//----------------------------------------------------------------------------
vtkOpenGLBatchedLabeledDataMapper::vtkOpenGLBatchedLabeledDataMapper()
{
  this->Helper->Parent = this;
  // Unlike WebGPU, no SetPointSize call is needed. WebGPU skips the shaped-points
  // pipeline when pointSize <= 1; OpenGL has no such gate.
}

//----------------------------------------------------------------------------
vtkOpenGLBatchedLabeledDataMapper::~vtkOpenGLBatchedLabeledDataMapper() = default;

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapper::SetupHelper()
{
  // Map per-label point arrays to named vertex attributes consumed by the vertex
  // shader. Called lazily so that the VBO mapping is refreshed whenever
  // SetLabelTextProperty resets HelperSetup. Unlike WebGPU, which uploads these
  // arrays as raw instance buffers in UpdateInstanceBuffers, OpenGL uses the
  // VBO attribute mechanism provided by the superclass mapper.
  this->Helper->MapDataArrayToVertexAttribute(
    "glyphExtentsVS", "glyphExtents", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->Helper->MapDataArrayToVertexAttribute(
    "coff", "coff", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->Helper->MapDataArrayToVertexAttribute(
    "propid", "propid", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->Helper->MapDataArrayToVertexAttribute(
    "framecolors", "framecolors", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->HelperSetup = true;
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapper::SetLabelTextProperty(vtkTextProperty* prop, int type)
{
  this->Superclass::SetLabelTextProperty(prop, type);
  this->HelperSetup = false;
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapper::UploadGlyphAtlas(vtkImageData* atlas)
{
  // OpenGL can upload the atlas immediately because GlyphsTO holds a reference to
  // the context set earlier in RenderOpaqueGeometry. WebGPU defers the upload to
  // RenderOpaqueGeometry because a vtkWebGPURenderWindow is needed for allocation.
  int* dims = atlas->GetDimensions();
  this->GlyphsTO->Create2DFromRaw(static_cast<unsigned int>(dims[0]),
    static_cast<unsigned int>(dims[1]), 4, VTK_UNSIGNED_CHAR, atlas->GetScalarPointer());
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapper::ActivateGlyphTexture()
{
  // OpenGL requires explicit texture-unit binding/unbinding around the draw call.
  // WebGPU manages texture access through bind groups, so these are no-ops there.
  this->GlyphsTO->Activate();
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapper::DeactivateGlyphTexture()
{
  this->GlyphsTO->Deactivate();
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapper::RenderOpaqueGeometry(
  vtkViewport* viewport, vtkActor2D* vtkNotUsed(actor))
{
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
    vtkErrorMacro(<< "vtkOpenGLBatchedLabeledDataMapper requires a vtkRenderer viewport.");
    return;
  }

  vtkOpenGLRenderWindow* oglWindow = vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  this->UpdateRenderWindowDPI(ren->GetRenderWindow()->GetDPI());

  if (this->GetMTime() > this->BuildTime || inputDO->GetMTime() > this->BuildTime ||
    this->GlyphsTO->GetContext() != oglWindow)
  {
    this->GlyphsTO->SetContext(oglWindow);
    this->BuildLabels();
    // BuildTime is stamped here (inside the rebuild 'if') because UploadGlyphAtlas
    // already uploaded the atlas synchronously. WebGPU cannot do this — its atlas
    // upload is deferred to later in RenderOpaqueGeometry — so it stamps BuildTime
    // after the full render instead.
    this->BuildTime.Modified();
  }

  if (!this->HelperSetup)
  {
    this->SetupHelper();
  }

  this->Helper->SetInputData(this->GetPreparedPolyData());

  this->GlyphsTO->Activate();
  // OpenGL: Helper->RenderPiece is sufficient because shader uniforms (program state)
  // carry per-actor transform data. WebGPU must use DummyActor->Render to set up
  // per-actor bind group 1 (transform buffer) before the draw call.
  this->Helper->RenderPiece(ren, this->DummyActor);
  this->GlyphsTO->Deactivate();
}

//----------------------------------------------------------------------------
void vtkOpenGLBatchedLabeledDataMapper::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  this->GlyphsTO->ReleaseGraphicsResources(win);
  this->Helper->ReleaseGraphicsResources(win);
}

VTK_ABI_NAMESPACE_END
