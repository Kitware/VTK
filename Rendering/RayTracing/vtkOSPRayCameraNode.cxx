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
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkViewNodeCollection.h"
#include "vtkWindow.h"

#include "RTWrapper/RTWrapper.h"

//============================================================================
vtkStandardNewMacro(vtkOSPRayCameraNode);

//----------------------------------------------------------------------------
vtkOSPRayCameraNode::vtkOSPRayCameraNode() {}

//----------------------------------------------------------------------------
vtkOSPRayCameraNode::~vtkOSPRayCameraNode() {}

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
    vtkOSPRayRendererNode* orn =
      static_cast<vtkOSPRayRendererNode*>(this->GetFirstAncestorOfType("vtkOSPRayRendererNode"));
    vtkRenderer* ren = vtkRenderer::SafeDownCast(orn->GetRenderable());
    RTW::Backend* backend = orn->GetBackend();
    if (backend == nullptr)
      return;

    vtkRenderWindow* rwin = vtkRenderWindow::SafeDownCast(ren->GetVTKWindow());
    bool stereo = false;
    bool right = false;
    if (rwin)
    {
      if (rwin->GetStereoRender() == 1)
      {
        stereo = true;
      }
    }

    double* vp = orn->GetViewport();
    int* ts = orn->GetScale();

    vtkCamera* cam = static_cast<vtkCamera*>(this->Renderable);
    double myDistance = cam->GetDistance();
    double myEyeSeparation = cam->GetEyeSeparation();
    double myScaledEyeSeparation = myEyeSeparation * myDistance;
    double shiftDistance = (myScaledEyeSeparation / 2);
    double* myFocalPoint = cam->GetFocalPoint();
    double myFocalDisk = cam->GetFocalDisk();
    double myFocalDistance = cam->GetFocalDistance();
    if (!cam->GetLeftEye())
    {
      right = true;
    }

    OSPCamera ospCamera;
    if (cam->GetParallelProjection())
    {
      // TODO: looks like imageStart/imageEnd doesn't work in ortho mode
      double height = cam->GetParallelScale() * 2 * ts[0];
      ospCamera = ospNewCamera("orthographic");
      ospSetf(ospCamera, "height", height);
    }
    else
    {
      // TODO: there's a rounding error here since TileScale is an int
      double fovy = cam->GetViewAngle() * ts[0];
      ospCamera = ospNewCamera("perspective");
      ospSetf(ospCamera, "fovy", fovy);

      if (myFocalDistance > 0.0)
      {
        ospSetf(ospCamera, "focusDistance", myFocalDistance);
        ospSetf(ospCamera, "apertureRadius", 0.5 * myFocalDisk);
      }
      else
      {
        ospSetf(ospCamera, "focusDistance", 0.0f);
        ospSetf(ospCamera, "apertureRadius", 0.0f);
      }
    }

    ospSetObject(orn->GetORenderer(), "camera", ospCamera);
    int tiledSize[2];
    orn->GetSize(tiledSize);
    ospSetf(ospCamera, "aspect", float(tiledSize[0]) / float(tiledSize[1]));
    double* pos = cam->GetPosition();

    double shiftedCamPos[3];
    if (stereo)
    {
      // todo this is good for starters, but we should reuse the code
      // or results from vtk proper to ensure 1:1 match with GL
      if (!right)
      {
        shiftedCamPos[0] = pos[0] - shiftDistance;
      }
      else
      {
        shiftedCamPos[0] = pos[0] + shiftDistance;
      }
    }
    else
    {
      shiftedCamPos[0] = pos[0];
    }
    shiftedCamPos[1] = pos[1];
    shiftedCamPos[2] = pos[2];

    ospSet3f(ospCamera, "pos", shiftedCamPos[0], shiftedCamPos[1], shiftedCamPos[2]);

    double* up = cam->GetViewUp();
    ospSet3f(ospCamera, "up", up[0], up[1], up[2]);

    // double *dop = cam->GetDirectionOfProjection();
    double shiftedDOP[3];
    shiftedDOP[0] = myFocalPoint[0] - shiftedCamPos[0];
    shiftedDOP[1] = myFocalPoint[1] - shiftedCamPos[1];
    shiftedDOP[2] = myFocalPoint[2] - shiftedCamPos[2];
    // ospSet3f(ospCamera, "dir", dop[0], dop[1], dop[2]);
    ospSet3f(ospCamera, "dir", shiftedDOP[0], shiftedDOP[1], shiftedDOP[2]);
    ospSet2f(ospCamera, "imageStart", (float)vp[0], (float)vp[1]);
    ospSet2f(ospCamera, "imageEnd", (float)vp[2], (float)vp[3]);
    ospCommit(ospCamera);
    ospRelease(ospCamera);
  }
}
