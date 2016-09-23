/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayCameraNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOSPRayCameraNode.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkRenderer.h"
#include "vtkViewNodeCollection.h"

#include "ospray/ospray.h"

//============================================================================
vtkStandardNewMacro(vtkOSPRayCameraNode);

//----------------------------------------------------------------------------
vtkOSPRayCameraNode::vtkOSPRayCameraNode()
{
}

//----------------------------------------------------------------------------
vtkOSPRayCameraNode::~vtkOSPRayCameraNode()
{
}

//----------------------------------------------------------------------------
void vtkOSPRayCameraNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOSPRayCameraNode::Render(bool prepass)
{
  if (prepass)
  {
    vtkOSPRayRendererNode *orn =
      static_cast<vtkOSPRayRendererNode *>(
        this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));

    vtkRenderer *ren = vtkRenderer::SafeDownCast(orn->GetRenderable());
    int tiledSize[2];
    int tiledOrigin[2];
    ren->GetTiledSizeAndOrigin(
      &tiledSize[0], &tiledSize[1], &tiledOrigin[0], &tiledOrigin[1]);

    vtkCamera *cam = static_cast<vtkCamera *>(this->Renderable);

    OSPCamera ospCamera;
    if (cam->GetParallelProjection())
    {
      ospCamera = ospNewCamera("orthographic");
      ospSetf(ospCamera, "height", cam->GetParallelScale() * 2);
    }
    else
    {
      ospCamera = ospNewCamera("perspective");
      ospSetf(ospCamera, "fovy", cam->GetViewAngle());
    }

    ospSetObject(orn->GetORenderer(), "camera", ospCamera);
    ospSetf(ospCamera, "aspect", float(tiledSize[0]) / float(tiledSize[1]));
    double *pos = cam->GetPosition();
    ospSet3f(ospCamera, "pos", pos[0], pos[1], pos[2]);
    double *up = cam->GetViewUp();
    ospSet3f(ospCamera, "up", up[0], up[1], up[2]);
    double *dop = cam->GetDirectionOfProjection();
    ospSet3f(ospCamera, "dir", dop[0], dop[1], dop[2]);
    ospCommit(ospCamera);
    ospRelease(ospCamera);
  }
}
