/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClientServerCompositePass 
// .SECTION Description
// vtkClientServerCompositePass is a render-pass that can handle client-server
// image delivery. This is designed to be used in configurations in
// two-processes configurations.

#ifndef __vtkClientServerCompositePass_h
#define __vtkClientServerCompositePass_h

#include "vtkRenderPass.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkClientServerCompositePass : public vtkRenderPass
{
public:
  static vtkClientServerCompositePass* New();
  vtkTypeMacro(vtkClientServerCompositePass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  //ETX
  
  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);

  // Description:
  // Controller
  // If it is NULL, nothing will be rendered and a warning will be emitted.
  // Initial value is a NULL pointer.
  // This must be set to the socket controller used for communicating between
  // the client and the server.
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
  virtual void SetController(vtkMultiProcessController *controller);

  // Description:
  // Get/Set the render pass used to do the actual rendering.
  // When ServerSideRendering is true, the rendering-pass is called only on the
  // server side.  
  void SetRenderPass(vtkRenderPass*);
  vtkGetObjectMacro(RenderPass, vtkRenderPass);

  // Description:
  // Set/Get the optional post-fetch render pass.
  // On the client-process this is called after the server-side image is fetched
  // (if ServerSideRendering is true). On server-process, this is called after the
  // image rendered by this->RenderPass is delivered to the client (if
  // ServerSideRendering is true). This is optional, so you can set this either on
  // one of the two processes or both or neither.
  void SetPostProcessingRenderPass(vtkRenderPass*);
  vtkGetObjectMacro(PostProcessingRenderPass, vtkRenderPass);

  // Description:
  // Set the current process type. This is needed since when using the socket
  // communicator there's no easy way of determining which process is the server
  // and which one is the client. 
  vtkSetMacro(ProcessIsServer,bool);
  vtkBooleanMacro(ProcessIsServer, bool);
  vtkGetMacro(ProcessIsServer, bool);

  // Description:
  // Enable/Disable fetching of the image from the server side to the client. If
  // this flag is disabled, then this pass just acts as a "pass-through" pass.
  // This flag must be set to the same value on both the processes.
  vtkSetMacro(ServerSideRendering, bool);
  vtkBooleanMacro(ServerSideRendering, bool);
  vtkGetMacro(ServerSideRendering, bool);

//BTX
protected:
  vtkClientServerCompositePass();
  ~vtkClientServerCompositePass();

  vtkRenderPass* RenderPass;
  vtkRenderPass* PostProcessingRenderPass;
  vtkMultiProcessController* Controller;

  bool ProcessIsServer;
  bool ServerSideRendering;
private:
  vtkClientServerCompositePass(const vtkClientServerCompositePass&); // Not implemented.
  void operator=(const vtkClientServerCompositePass&); // Not implemented.
//ETX
};

#endif


