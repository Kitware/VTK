/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOptiXPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOptiXPass
 * @brief   a render pass that uses OptiX instead of GL
 *
 * This is a render pass that can be put into a vtkRenderWindow which makes
 * it use OptiX instead of OpenGL to render. Adding/Removing the pass
 * will swap back and forth between the two.
 *
*/

#ifndef vtkOptiXPass_h
#define vtkOptiXPass_h

#include "vtkRenderingOptiXModule.h" // For export macro
#include "vtkRenderPass.h"

class vtkOptiXRendererNode;
class vtkCameraPass;
class vtkLightsPass;
class vtkOverlayPass;
class vtkRenderPassCollection;
class vtkSequencePass;
class vtkVolumetricPass;
class vtkOptiXPassInternals;

class VTKRENDERINGOPTIX_EXPORT vtkOptiXPass : public vtkRenderPass
{
public:
  static vtkOptiXPass *New();
  vtkTypeMacro(vtkOptiXPass,vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Perform rendering according to a render state s.
   */
  virtual void Render(const vtkRenderState *s);

  //@{
  /**
   * Tells the pass what it will render.
   */
  void SetSceneGraph(vtkOptiXRendererNode *);
  vtkGetObjectMacro(SceneGraph, vtkOptiXRendererNode);
  //@}

  /**
   * Called by the internals of this class
   */
  virtual void RenderInternal(const vtkRenderState *s);

 protected:

  vtkOptiXPass();
  virtual ~vtkOptiXPass();

  vtkOptiXRendererNode *SceneGraph;
  vtkCameraPass *CameraPass;
  vtkLightsPass *LightsPass;
  vtkOverlayPass *OverlayPass;
  vtkVolumetricPass *VolumetricPass;
  vtkSequencePass *SequencePass;
  vtkRenderPassCollection *RenderPassCollection;

 private:
  vtkOptiXPass(const vtkOptiXPass&) = delete;
  void operator=(const vtkOptiXPass&) = delete;

  vtkOptiXPassInternals *Internals;
};

#endif
