// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRendererNode.h"

#include "vtkActor.h"
#include "vtkActorNode.h"
#include "vtkCamera.h"
#include "vtkCameraNode.h"
#include "vtkCollectionIterator.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkLightNode.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRendererNode);

//------------------------------------------------------------------------------
vtkRendererNode::vtkRendererNode()
{
  this->Size[0] = 0;
  this->Size[1] = 0;
  this->Viewport[0] = 0.0;
  this->Viewport[1] = 0.0;
  this->Viewport[2] = 1.0;
  this->Viewport[3] = 1.0;
  this->Scale[0] = 1;
  this->Scale[1] = 1;
}

//------------------------------------------------------------------------------
vtkRendererNode::~vtkRendererNode() = default;

//------------------------------------------------------------------------------
void vtkRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkRendererNode::Build(bool prepass)
{
  if (prepass)
  {
    vtkRenderer* mine = vtkRenderer::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }

    this->PrepareNodes();
    this->AddMissingNodes(mine->GetLights());
    this->AddMissingNodes(mine->GetActors());
    this->AddMissingNodes(mine->GetVolumes());

    // make sure we have a camera setup
    if (!mine->IsActiveCameraCreated())
    {
      mine->GetActiveCamera();
      mine->ResetCamera();
    }
    this->AddMissingNode(mine->GetActiveCamera());
    this->RemoveUnusedNodes();
  }
}
VTK_ABI_NAMESPACE_END
