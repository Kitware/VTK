/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRendererNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkRendererNode.h"
#include "vtkViewNodeCollection.h"

//============================================================================
vtkStandardNewMacro(vtkRendererNode);

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkRendererNode::~vtkRendererNode() {}

//----------------------------------------------------------------------------
void vtkRendererNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
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
    this->AddMissingNode(mine->GetActiveCamera());
    this->RemoveUnusedNodes();
  }
}
