/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRenderPass - Perform part of the rendering of a vtkRenderer.
// .SECTION Description
// vtkRenderPass is a deferred class with a simple deferred method Render.
// This method performs a rendering pass of the scene described in
// vtkRenderState.
// Subclasses define what really happens during rendering.
//
// Directions to write a subclass of vtkRenderPass:
// It is up to the subclass to decide if it needs to delegate part of its job
// to some other vtkRenderPass objects ("delegates").
// - The subclass has to define ivar to set/get its delegates.
// - The documentation of the subclass has to describe:
//  - what each delegate is supposed to perform
//  - if a delegate is supposed to be used once or multiple times 
//  - what it expects to have in the framebuffer before starting (status
// of colorbuffers, depth buffer, stencil buffer)
//  - what it will change in the framebuffer.
// - A pass cannot modify the vtkRenderState where it will perform but
// it can build a new vtkRenderState (it can change the FrameBuffer, change the
// prop array, changed the required prop properties keys (usually adding some
// to a copy of the existing list) but it has to keep the same vtkRenderer
// object), make it current and pass it to its delegate.
// - at the end of the execution of Render, the pass has to ensure the
// current vtkRenderState is the one it has in argument.
// .SECTION See Also
// vtkRenderState vtkRenderer 

#ifndef __vtkRenderPass_h
#define __vtkRenderPass_h

#include "vtkObject.h"

class vtkRenderState;
class vtkWindow;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkRenderPass : public vtkObject
{
 public:
  vtkTypeMacro(vtkRenderPass,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // It modifies NumberOfRenderedProps.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s)=0;
  //ETX
  
  // Description:
  // Number of props rendered at the last Render call.
  vtkGetMacro(NumberOfRenderedProps,int);
  
  // Description:
  // Release graphics resources and ask components to release their own
  // resources. Default implementation is empty.
  // \pre w_exists: w!=0
  virtual void ReleaseGraphicsResources(vtkWindow *w);
  
 protected:
  // Description:
  // Default constructor. Do nothing.
  vtkRenderPass();

  // Description:
  // Destructor. Do nothing.
  virtual ~vtkRenderPass();
  
  // Description:
  // Call UpdateCamera() on Renderer. This ugly mechanism gives access to
  // a protected method of Renderer to subclasses of vtkRenderPass.
  // \pre renderer_exists: renderer!=0
  void UpdateCamera(vtkRenderer *renderer);
  
  // Description:
  // Call ClearLights() on Renderer. See note about UpdateCamera().
  // \pre renderer_exists: renderer!=0
  void ClearLights(vtkRenderer *renderer);
  
  // Description:
  // Call UpdateLightGeometry() on Renderer. See note about UpdateCamera().
  // \pre renderer_exists: renderer!=0
  void UpdateLightGeometry(vtkRenderer *renderer);
  
  // Description:
  // Call UpdateLights() on Renderer. See note about UpdateCamera().
  // \pre renderer_exists: renderer!=0
  void UpdateLights(vtkRenderer *renderer);

  // Description:
  // Call UpdateGeometry() on Renderer. See note about UpdateCamera().
  // \pre renderer_exists: renderer!=0
  void UpdateGeometry(vtkRenderer *renderer);
  
  // Description:
  // Modify protected member LastRenderingUsedDepthPeeling on Renderer.
  // See note about UpdateCamera().
  // \pre renderer_exists: renderer!=0
  void SetLastRenderingUsedDepthPeeling(vtkRenderer *renderer,
                                        bool value);
  
  int NumberOfRenderedProps;
  
 private:
  vtkRenderPass(const vtkRenderPass&);  // Not implemented.
  void operator=(const vtkRenderPass&);  // Not implemented.
};

#endif
