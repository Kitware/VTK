// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAnariVolumeMapper.h"
#include "vtkAnariPass.h"
#include "vtkAnariSceneGraph.h"

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN

//============================================================================
vtkStandardNewMacro(vtkAnariVolumeMapper)

  // ----------------------------------------------------------------------------
  vtkAnariVolumeMapper::vtkAnariVolumeMapper()
  : InternalRenderer(nullptr)
  , InternalAnariPass(nullptr)
  , Initialized(false)
{
}

// ----------------------------------------------------------------------------
vtkAnariVolumeMapper::~vtkAnariVolumeMapper()
{
  if (this->InternalRenderer)
  {
    this->InternalRenderer->SetPass(nullptr);
    this->InternalRenderer->Delete();
  }

  if (this->InternalAnariPass)
  {
    this->InternalAnariPass->Delete();
  }
}

// ----------------------------------------------------------------------------
void vtkAnariVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
void vtkAnariVolumeMapper::Init()
{
  this->InternalAnariPass = vtkAnariPass::New();
  this->InternalRenderer = vtkRenderer::New();

#if 0
  // e.g. export ANARI_LIBRARY=helide
  vtkAnariSceneGraph::SetLibraryName(this->InternalRenderer, "environment");
#endif
  vtkAnariSceneGraph::SetCompositeOnGL(this->InternalRenderer, 1);

  this->InitializedOn();
}

// ----------------------------------------------------------------------------
// Render the volume
void vtkAnariVolumeMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  if (!ren)
  {
    return;
  }

  if (!this->GetInitialized())
  {
    this->Init();
  }

  this->InternalRenderer->SetRenderWindow(ren->GetRenderWindow());
  this->InternalRenderer->SetActiveCamera(ren->GetActiveCamera());
  this->InternalRenderer->SetBackground(ren->GetBackground());

  if (!this->InternalRenderer->HasViewProp(vol))
  {
    this->InternalRenderer->RemoveAllViewProps();
    this->InternalRenderer->AddVolume(vol);
  }

  this->InternalRenderer->SetPass(this->InternalAnariPass);
  this->InternalRenderer->Render();
  this->InternalRenderer->SetPass(0);
  vtkAnariSceneGraph::SetCompositeOnGL(this->InternalRenderer, ren->GetNumberOfPropsRendered() > 0);
  this->InternalRenderer->SetErase(ren->GetNumberOfPropsRendered() < 1);
  this->InternalRenderer->RemoveVolume(vol); // prevent a mem leak
}

// ----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this mapper.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkAnariVolumeMapper::ReleaseGraphicsResources(vtkWindow*) {}

VTK_ABI_NAMESPACE_END
