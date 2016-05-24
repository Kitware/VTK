/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOSPRayPass - a render pass that uses OSPRay instead of GL
// .SECTION Description
// This is a render pass that can be put into a vtkRenderWindow which makes
// it use OSPRay instead of OpenGL to render. Adding/Removing the pass
// will swap back and forth between the two.

#ifndef vtkOSPRayPass_h
#define vtkOSPRayPass_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkCameraPass;
class vtkLightsPass;
class vtkOSPRayPassInternals;
class vtkOSPRayRendererNode;
class vtkOverlayPass;
class vtkRenderPassCollection;
class vtkSequencePass;
class vtkVolumetricPass;

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayPass : public vtkRenderPass
{
public:
  static vtkOSPRayPass *New();
  vtkTypeMacro(vtkOSPRayPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform rendering according to a render state s.
  virtual void Render(const vtkRenderState *s);

  // Description:
  // Tells the pass what it will render.
  void SetSceneGraph(vtkOSPRayRendererNode *);
  vtkGetObjectMacro(SceneGraph, vtkOSPRayRendererNode);

  // Description:
  // Called by the internals of this class
  virtual void RenderInternal(const vtkRenderState *s);

  void SetMaxDepthTexture(void *dt);

 protected:
  // Description:
  // Default constructor.
  vtkOSPRayPass();

  // Description:
  // Destructor.
  virtual ~vtkOSPRayPass();

  vtkOSPRayRendererNode *SceneGraph;
  vtkCameraPass *CameraPass;
  vtkLightsPass *LightsPass;
  vtkOverlayPass *OverlayPass;
  vtkVolumetricPass *VolumetricPass;
  vtkSequencePass *SequencePass;
  vtkRenderPassCollection *RenderPassCollection;

 private:
  vtkOSPRayPass(const vtkOSPRayPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOSPRayPass&) VTK_DELETE_FUNCTION;

  class Internals;
  vtkOSPRayPassInternals *Internal;
};

#endif
