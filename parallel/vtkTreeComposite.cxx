/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkTreeComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// This software and ancillary information known as vtk_ext (and
// herein called "SOFTWARE") is made available under the terms
// described below.  The SOFTWARE has been approved for release with
// associated LA_CC Number 99-44, granted by Los Alamos National
// Laboratory in July 1999.
//
// Unless otherwise indicated, this SOFTWARE has been authored by an
// employee or employees of the University of California, operator of
// the Los Alamos National Laboratory under Contract No. W-7405-ENG-36
// with the United States Department of Energy.
//
// The United States Government has rights to use, reproduce, and
// distribute this SOFTWARE.  The public may copy, distribute, prepare
// derivative works and publicly display this SOFTWARE without charge,
// provided that this Notice and any statement of authorship are
// reproduced on all copies.
//
// Neither the U. S. Government, the University of California, nor the
// Advanced Computing Laboratory makes any warranty, either express or
// implied, nor assumes any liability or responsibility for the use of
// this SOFTWARE.
//
// If SOFTWARE is modified to produce derivative works, such modified
// SOFTWARE should be clearly marked, so as not to confuse it with the
// version available from Los Alamos National Laboratory.

#include "vtkTreeComposite.h"
#include "vtkCommand.h"
#include "vtkPolyDataMapper.h"
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"


#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#elif defined(VTK_USE_MESA)
#include "vtkMesaRenderWindow.h"
#endif

// Structures to communicate render info.
struct vtkCompositeRenderWindowInfo 
{
  int Size[2];
  int NumberOfRenderers;
  float DesiredUpdateRate;
};

struct vtkCompositeRendererInfo 
{
  float CameraPosition[3];
  float CameraFocalPoint[3];
  float CameraViewUp[3];
  float CameraClippingRange[2];
  float LightPosition[3];
  float LightFocalPoint[3];
};


//-------------------------------------------------------------------------
vtkTreeComposite* vtkTreeComposite::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTreeCompssite");
  if(ret)
    {
    return (vtkTreeComposite*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTreeComposite;
}

//-------------------------------------------------------------------------
vtkTreeComposite::vtkTreeComposite()
{
  this->RenderWindow = NULL;
  this->RenderWindowInteractor = NULL;
  this->Controller = vtkMultiProcessController::GetGlobalController();

  this->StartTag = this->EndTag = 0;
  this->StartInteractorTag = 0;
  this->EndInteractorTag = 0;

  this->PData = this->ZData = NULL;
  
  this->Lock = 0;
  this->UseChar = 0;
  this->UseCompositing = 1;
  
  this->ReductionFactor = 1;
  
  this->GetBuffersTime = 0;
  this->SetBuffersTime = 0;
  this->TransmitTime = 0;
}

  
//-------------------------------------------------------------------------
vtkTreeComposite::~vtkTreeComposite()
{
  this->SetRenderWindow(NULL);
  
  this->SetWindowSize(0,0);
  
  if (this->Lock)
    {
    vtkErrorMacro("Destructing while locked!");
    }
}


//-------------------------------------------------------------------------
// We may want to pass the render window as an argument for a sanity check.
void vtkTreeCompositeStartRender(vtkObject *caller,
				 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  vtkTreeComposite *self = (vtkTreeComposite *)clientData;
  
  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->StartRender();
}

//-------------------------------------------------------------------------
void vtkTreeCompositeEndRender(vtkObject *caller,
			       unsigned long vtkNotUsed(event), 
                               void *clientData, void *)
{
  vtkTreeComposite *self = (vtkTreeComposite *)clientData;
  
  if (caller != self->GetRenderWindow())
    { // Sanity check.
    vtkGenericWarningMacro("Caller mismatch.");
    return;
    }

  self->EndRender();
}

//-------------------------------------------------------------------------
void vtkTreeCompositeStartInteractor(vtkObject *vtkNotUsed(o),
				     unsigned long vtkNotUsed(event), 
                                     void *clientData, void *)
{
  vtkTreeComposite *self = (vtkTreeComposite *)clientData;
  
  // If we stay with event driven compositing, I think in the future
  // we should pass all events through the RenderWindow.
  //if (caller != self->GetRenderWindowInteractor) // private
  //  { // Sanity check.
  //  vtkGenericErrorMacro("Caller mismatch.");
  //  return;
  //  }

  self->StartInteractor();
}

//-------------------------------------------------------------------------
void vtkTreeCompositeExitInteractor(vtkObject *vtkNotUsed(o),
				    unsigned long vtkNotUsed(event), 
                                    void *clientData, void *)
{
  vtkTreeComposite *self = (vtkTreeComposite *)clientData;
  
  // If we stay with event driven compositing, I think in the future
  // we should pass all events through the RenderWindow.
  //if (caller != self->GetRenderWindowInteractor) // private
  //  { // Sanity check.
  //  vtkGenericErrorMacro("Caller mismatch.");
  //  return;
  //  }

  self->ExitInteractor();
}

//-------------------------------------------------------------------------
void vtkTreeCompositeResetCamera(vtkObject *caller,
				 unsigned long vtkNotUsed(event), 
                                 void *clientData, void *)
{
  vtkTreeComposite *self = (vtkTreeComposite *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  self->ResetCamera(ren);
}

//-------------------------------------------------------------------------
void vtkTreeCompositeResetCameraClippingRange(vtkObject *caller, 
                                              unsigned long vtkNotUsed(event), 
                                              void *clientData, void *)
{
  vtkTreeComposite *self = (vtkTreeComposite *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  self->ResetCameraClippingRange(ren);
}

//----------------------------------------------------------------------------
void vtkTreeCompositeRenderRMI(void *arg, void *, int, int)
{
  vtkTreeComposite* self = (vtkTreeComposite*) arg;
  
  self->RenderRMI();
}

//----------------------------------------------------------------------------
void vtkTreeCompositeComputeVisiblePropBoundsRMI(void *arg, void *, int, int)
{
  vtkTreeComposite* self = (vtkTreeComposite*) arg;
  
  self->ComputeVisiblePropBoundsRMI();
}

//-------------------------------------------------------------------------
// Only process 0 needs start and end render callbacks.
void vtkTreeComposite::SetRenderWindow(vtkRenderWindow *renWin)
{
  vtkRendererCollection *rens;
  vtkRenderer *ren;
  
  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->RenderWindow)
    {
    // Remove all of the observers.
    if (this->Controller && this->Controller->GetLocalProcessId() == 0)
      {
      this->RenderWindow->RemoveObserver(this->StartTag);
      this->RenderWindow->RemoveObserver(this->EndTag);
      
      // Will make do with first renderer. (Assumes renderer does not change.)
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();
      ren = rens->GetNextItem();
      if (ren)
	{
	ren->RemoveObserver(this->ResetCameraTag);
	ren->RemoveObserver(this->ResetCameraClippingRangeTag);
	}
      }
    // Delete the reference.
    this->RenderWindow->UnRegister(this);
    this->RenderWindow =  NULL;
    this->SetRenderWindowInteractor(NULL);
    }
  if (renWin)
    {
    renWin->Register(this);
    this->RenderWindow = renWin;
    this->SetRenderWindowInteractor(renWin->GetInteractor());
    if (this->Controller && this->Controller->GetLocalProcessId() == 0)
      {
      vtkCallbackCommand *cbc;
      cbc= new vtkCallbackCommand;
      cbc->SetCallback(vtkTreeCompositeStartRender);
      cbc->SetClientData((void*)this);
      // renWin will delete the cbc when the observer is removed.
      this->StartTag = renWin->AddObserver(vtkCommand::StartEvent,cbc);
      
      cbc = new vtkCallbackCommand;
      cbc->SetCallback(vtkTreeCompositeEndRender);
      cbc->SetClientData((void*)this);
      // renWin will delete the cbc when the observer is removed.
      this->EndTag = renWin->AddObserver(vtkCommand::EndEvent,cbc);
      
      // Will make do with first renderer. (Assumes renderer does not change.)
      rens = this->RenderWindow->GetRenderers();
      rens->InitTraversal();
      ren = rens->GetNextItem();
      if (ren)
	{
	cbc = new vtkCallbackCommand;
	cbc->SetCallback(vtkTreeCompositeResetCameraClippingRange);
	cbc->SetClientData((void*)this);
	// ren will delete the cbc when the observer is removed.
	this->ResetCameraClippingRangeTag = 
	  ren->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);

	cbc = new vtkCallbackCommand;
	cbc->SetCallback(vtkTreeCompositeResetCamera);
	cbc->SetClientData((void*)this);
	// ren will delete the cbc when the observer is removed.
	this->ResetCameraTag = 
	  ren->AddObserver(vtkCommand::ResetCameraEvent,cbc);
	}
      }
    }
}

//-------------------------------------------------------------------------
// Only satelite processes process interactor loops specially.
// We only setup callbacks in those processes (not process 0).
void 
vtkTreeComposite::SetRenderWindowInteractor(vtkRenderWindowInteractor *iren)
{
  if (this->RenderWindowInteractor == iren)
    {
    return;
    }

  if (this->Controller == NULL)
    {
    return;
    }
  
  if (this->RenderWindowInteractor)
    {
    this->RenderWindowInteractor->UnRegister(this);
    this->RenderWindowInteractor =  NULL;
    if (this->Controller->GetLocalProcessId() > 0)
      {
      this->RenderWindowInteractor->RemoveObserver(this->StartInteractorTag);
      }
    else
      {
      this->RenderWindowInteractor->RemoveObserver(this->EndInteractorTag);
      }
    }
  if (iren)
    {
    iren->Register(this);
    this->RenderWindowInteractor = iren;

    if (this->Controller->GetLocalProcessId() > 0)
      {
      vtkCallbackCommand *cbc;
      cbc= new vtkCallbackCommand;
      cbc->SetCallback(vtkTreeCompositeStartInteractor);
      cbc->SetClientData((void*)this);
      // IRen will delete the cbc when the observer is removed.
      this->StartInteractorTag = iren->AddObserver(vtkCommand::StartEvent,cbc);
      }
    else
      {
      vtkCallbackCommand *cbc;
      cbc= new vtkCallbackCommand;
      cbc->SetCallback(vtkTreeCompositeExitInteractor);
      cbc->SetClientData((void*)this);
      // IRen will delete the cbc when the observer is removed.
      this->EndInteractorTag = iren->AddObserver(vtkCommand::ExitEvent,cbc);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTreeComposite::RenderRMI()
{
  int i;
  vtkCompositeRenderWindowInfo winInfo;
  vtkCompositeRendererInfo renInfo;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;
  
  vtkDebugMacro("RenderRMI");
  
  // Receive the window size.
  controller->Receive((char*)(&winInfo), 
                      sizeof(struct vtkCompositeRenderWindowInfo), 0, 
                      vtkTreeComposite::WIN_INFO_TAG);
  renWin->SetSize(winInfo.Size);
  renWin->SetDesiredUpdateRate(winInfo.DesiredUpdateRate);

  // Synchronize the renderers.
  rens = renWin->GetRenderers();
  rens->InitTraversal();
  for (i = 0; i < winInfo.NumberOfRenderers; ++i)
    {
    // Receive the camera information.
    controller->Receive((char*)(&renInfo), 
                        sizeof(struct vtkCompositeRendererInfo), 
                        0, vtkTreeComposite::REN_INFO_TAG);
    ren = rens->GetNextItem();
    if (ren == NULL)
      {
      vtkErrorMacro("Renderer mismatch.");
      }
    else
      {
      cam = ren->GetActiveCamera();
      lc = ren->GetLights();
      lc->InitTraversal();
      light = lc->GetNextItem();
  
      cam->SetPosition(renInfo.CameraPosition);
      cam->SetFocalPoint(renInfo.CameraFocalPoint);
      cam->SetViewUp(renInfo.CameraViewUp);
      cam->SetClippingRange(renInfo.CameraClippingRange);
      if (light)
        {
        light->SetPosition(renInfo.LightPosition);
        light->SetFocalPoint(renInfo.LightFocalPoint);
        }
      }
    }
  renWin->Render();  
  
  
  this->SetWindowSize(winInfo.Size[0], winInfo.Size[1]);
  this->Composite();
}

//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkTreeComposite::InitializeRMIs()
{
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }

  this->Controller->AddRMI(vtkTreeCompositeRenderRMI, (void*)this, 
                           vtkTreeComposite::RENDER_RMI_TAG); 

  this->Controller->AddRMI(vtkTreeCompositeComputeVisiblePropBoundsRMI,
     (void*)this, vtkTreeComposite::COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);  
}

//-------------------------------------------------------------------------
// This is only called in the satellite processes (not 0).
void vtkTreeComposite::StartInteractor()
{
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }

  this->InitializeRMIs();
  this->Controller->ProcessRMIs();
}

//-------------------------------------------------------------------------
// This is only called in process 0.
void vtkTreeComposite::ExitInteractor()
{
  int numProcs, id;
  
  if (this->Controller == NULL)
    {
    vtkErrorMacro("Missing Controller.");
    return;
    }

  numProcs = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < numProcs; ++id)
    {
    this->Controller->TriggerRMI(id, vtkMultiProcessController::BREAK_RMI_TAG);
    }
}


//-------------------------------------------------------------------------
// Only called in process 0.
void vtkTreeComposite::StartRender()
{
  struct vtkCompositeRenderWindowInfo winInfo;
  struct vtkCompositeRendererInfo renInfo;
  int id, numProcs;
  int *size;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  
  vtkDebugMacro("StartRender");
  
  if (!this->UseCompositing)
    {
    return;
    }
  
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;

  if (controller == NULL || this->Lock)
    {
    return;
    }
  
  // Lock here, unlock at end render.
  this->Lock = 1;
  
  // Trigger the satelite processes to start their render routine.
  rens = this->RenderWindow->GetRenderers();
  numProcs = controller->GetNumberOfProcesses();
  size = this->RenderWindow->GetSize();
  // It would probably be better to reduce the viewport not the size.
  if (this->ReductionFactor > 0)
    {
    winInfo.Size[0] = size[0] / this->ReductionFactor;
    winInfo.Size[1] = size[1] / this->ReductionFactor;
    }
  else
    {
    winInfo.Size[0] = size[0];
    winInfo.Size[1] = size[1];
    }
  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  winInfo.DesiredUpdateRate = this->RenderWindow->GetDesiredUpdateRate();
  for (id = 1; id < numProcs; ++id)
    {
    controller->TriggerRMI(id, NULL, 0, vtkTreeComposite::RENDER_RMI_TAG);
    // Synchronize the size of the windows.
    controller->Send((char*)(&winInfo), 
                     sizeof(vtkCompositeRenderWindowInfo), id, 
                     vtkTreeComposite::WIN_INFO_TAG);
    }
  
  // Make sure the satellite renderers have the same camera I do.
  // Note:  This will lockup unless every process 
  // has the same number of renderers.
  rens->InitTraversal();
  while ( (ren = rens->GetNextItem()) )
    {
    cam = ren->GetActiveCamera();
    lc = ren->GetLights();
    lc->InitTraversal();
    light = lc->GetNextItem();
    cam->GetPosition(renInfo.CameraPosition);
    cam->GetFocalPoint(renInfo.CameraFocalPoint);
    cam->GetViewUp(renInfo.CameraViewUp);
    cam->GetClippingRange(renInfo.CameraClippingRange);
    if (light)
      {
      light->GetPosition(renInfo.LightPosition);
      light->GetFocalPoint(renInfo.LightFocalPoint);
      }
    
    for (id = 1; id < numProcs; ++id)
      {
      controller->Send((char*)(&renInfo),
                       sizeof(struct vtkCompositeRendererInfo), id, 
                       vtkTreeComposite::REN_INFO_TAG);
      }
    }
  
  // Turn swap buffers off before the render so the end render method has a chance
  // to add to the back buffer.
  renWin->SwapBuffersOff();
}

//-------------------------------------------------------------------------
void vtkTreeComposite::EndRender()
{
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;
  int *windowSize;
  int numProcs;
  
  if (controller == NULL)
    {
    return;
    }

  if (!this->UseCompositing)
    {
    return;
    }
  
  windowSize = renWin->GetSize();
  numProcs = controller->GetNumberOfProcesses();

  if (numProcs > 1)
    {
    // It would be more efficient to save these arrays as ivars.
    this->SetWindowSize(windowSize[0], windowSize[1]);
    this->Composite();
    }
  
  // Force swap buffers here.
  renWin->SwapBuffersOn();  
  renWin->Frame();
  
  // Release lock.
  this->Lock = 0;
}


//-------------------------------------------------------------------------
void vtkTreeComposite::ResetCamera(vtkRenderer *ren)
{
  float bounds[6];

  if (this->Controller == NULL || this->Lock)
    {
    return;
    }

  this->Lock = 1;
  
  this->ComputeVisiblePropBounds(ren, bounds);
  ren->ResetCamera(bounds);
  
  this->Lock = 0;
}

//-------------------------------------------------------------------------
void vtkTreeComposite::ResetCameraClippingRange(vtkRenderer *ren)
{
  float bounds[6];

  if (this->Controller == NULL || this->Lock)
    {
    return;
    }

  this->Lock = 1;
  
  this->ComputeVisiblePropBounds(ren, bounds);
  ren->ResetCameraClippingRange(bounds);

  this->Lock = 0;
}

//----------------------------------------------------------------------------
void vtkTreeComposite::ComputeVisiblePropBounds(vtkRenderer *ren, 
                                                float bounds[6])
{
  float tmp[6];
  int id, num;
  
  num = this->Controller->GetNumberOfProcesses();  
  for (id = 1; id < num; ++id)
    {
    this->Controller->TriggerRMI(id,COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);
    }

  ren->ComputeVisiblePropBounds(bounds);

  for (id = 1; id < num; ++id)
    {
    this->Controller->Receive(tmp, 6, id, vtkTreeComposite::BOUNDS_TAG);
    if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];}
    if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];}
    if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];}
    if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];}
    if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];}
    if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];}
    }
}

//----------------------------------------------------------------------------
void vtkTreeComposite::ComputeVisiblePropBoundsRMI()
{
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  float bounds[6];
  
  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  ren = rens->GetNextItem();

  ren->ComputeVisiblePropBounds(bounds);

  this->Controller->Send(bounds, 6, 0, vtkTreeComposite::BOUNDS_TAG);
}

//-------------------------------------------------------------------------
void vtkTreeComposite::InitializePieces()
{
  vtkRendererCollection *rens;
  vtkRenderer *ren;
  vtkActorCollection *actors;
  vtkActor *actor;
  vtkMapper *mapper;
  vtkPolyDataMapper *pdMapper;
  int piece, numPieces;

  if (this->RenderWindow == NULL || this->Controller == NULL)
    {
    return;
    }
  piece = this->Controller->GetLocalProcessId();
  numPieces = this->Controller->GetNumberOfProcesses();

  rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  while ( (ren = rens->GetNextItem()) )
    {
    actors = ren->GetActors();
    actors->InitTraversal();
    while ( (actor = actors->GetNextItem()) )
      {
      mapper = actor->GetMapper();
      pdMapper = vtkPolyDataMapper::SafeDownCast(mapper);
      if (pdMapper)
        {
        pdMapper->SetPiece(piece);
        pdMapper->SetNumberOfPieces(numPieces);
        }
      }
    }
}

//-------------------------------------------------------------------------
void vtkTreeComposite::InitializeOffScreen()
{
  vtkDebugMacro("InitializeOffScreen");
  if (this->RenderWindow == NULL || this->Controller == NULL)
    {
    vtkDebugMacro("Missing object: Window = " << this->RenderWindow
		  << ", Controller = " << this->Controller);
    return;
    }
  
  // Do not make process 0 off screen.
  if (this->Controller->GetLocalProcessId() == 0)
    {
    vtkDebugMacro("Process 0.  Keep OnScreen.");
    return;
    }
  
#ifdef _WIN32
  vtkWin32OpenGLRenderWindow *renWin;
  
  renWin = vtkWin32OpenGLRenderWindow::SafeDownCast(this->RenderWindow);
  if (renWin)
    {
    // I do not want to replace the original.
    renWin = renWin;
    }
#elif defined(VTK_USE_MESA)
  vtkMesaRenderWindow *renWin;
  
  renWin = vtkMesaRenderWindow::SafeDownCast(this->RenderWindow);
  if (renWin)
    {
    renWin->SetOffScreenRendering(1);
    }
#endif    
  
}

void vtkTreeComposite::SetWindowSize(int x, int y)
{
  if (this->WindowSize[0] == x && this->WindowSize[1] == y)
    {
    return;
    }
  
  if (this->PData)
    {
    delete this->PData;
    this->PData = NULL;
    }
  
  if (this->ZData)
    {
    delete this->ZData;
    this->ZData = NULL;
    }
  
  int numPixels = x * y;
  if (numPixels > 0)
    {
    this->PData = new float[4*numPixels];
    this->ZData = new float[numPixels];
    }
  this->WindowSize[0] = x;
  this->WindowSize[1] = y;
}


void vtkTreeComposite::ReduceBuffer(float *localZdata, float *localPdata, 
                                    int windowSize[2])
{
  float *pz1;
  float *pz2;
  float *pp1;
  float *pp2;
  int   x, y;
  int   xDim, yDim;
  // Continous increments.
  int   pIncY, zIncY, pIncX; 

  pz1 = pz2 = localZdata;
  pp1 = pp2 = localPdata;
  xDim = windowSize[0] / this->ReductionFactor;
  yDim = windowSize[1] / this->ReductionFactor;
  // Skip remainder and extra rows.
  zIncY = pIncY = windowSize[0] - (xDim * this->ReductionFactor) 
    + ((this->ReductionFactor-1)*windowSize[0]);
    
  if (this->UseChar)
    {    
    for (y = 0; y < yDim; y++)
      {
      for (x = 0; x < xDim; x++)
        {
        *pz1++ = *pz2;
        *pp1++ = *pp2;
        pz2 += this->ReductionFactor;
        pp2 += this->ReductionFactor;
        }
      pz2 += zIncY;
      pp2 += pIncY;
      }
    }
  else
    {
    pIncY = pIncY * 4;
    pIncX = 4 * (this->ReductionFactor - 1);
    for (y = 0; y < yDim; y++)
      {
      for (x = 0; x < xDim; x++)
        {
        *pp1++ = *pp2++;
        *pp1++ = *pp2++;
        *pp1++ = *pp2++;
        *pp1++ = *pp2++;
        pp2 += pIncX;
        *pz1++ = *pz2;
        pz2 += this->ReductionFactor;
        }
      pz2 += zIncY;
      pp2 += pIncY;
      }
    }

  windowSize[0] = xDim;
  windowSize[1] = yDim;
}
  
// We have do do this backward so we can keep it inplace.     
void vtkTreeComposite::MagnifyBuffer(float *localPdata, int windowSize[2])
{
  float *rowp, *subp;
  float *pp1;
  float *pp2;
  int   x, y, xi, yi;
  int   xInDim, yInDim;
  int   xOutDim, yOutDim;
  // Local increments for input.
  int   pIncY; 

  xInDim = windowSize[0];
  yInDim = windowSize[1];
  xOutDim = xInDim * this->ReductionFactor;
  yOutDim = yInDim * this->ReductionFactor;
  pIncY = xInDim;
  
  if (this->UseChar)
    {
    // Get the last pixel.
    rowp = localPdata + ((xInDim*yInDim)-1);
    pp2 = localPdata + ((xOutDim*yOutDim)-1);
    for (y = 0; y < yInDim; y++)
      {
      // Duplicate the row rowp N times.
      for (yi = 0; yi < this->ReductionFactor; ++yi)
	{
	pp1 = rowp;
	for (x = 0; x < xInDim; x++)
	  {
	  // Duplicate the pixel p11 N times.
	  for (xi = 0; xi < this->ReductionFactor; ++xi)
	    {
	    *pp2-- = *pp1;
	    }
	  --pp1;
	  }
	}
      rowp -= pIncY;
      }
    }
  else
    {
    // Get the last pixel.
    pIncY *= 4;
    rowp = localPdata + ((xInDim*yInDim*4)-1);
    pp2 = localPdata + ((xOutDim*yOutDim*4)-1);
    for (y = 0; y < yInDim; y++)
      {
      // Duplicate the row rowp N times.
      for (yi = 0; yi < this->ReductionFactor; ++yi)
	{
	pp1 = rowp;
	for (x = 0; x < xInDim; x++)
	  {
	  // Duplicate the pixel p11 N times.
	  for (xi = 0; xi < this->ReductionFactor; ++xi)
	    {
	    subp = pp1;
	    *pp2-- = *subp--;
	    *pp2-- = *subp--;
	    *pp2-- = *subp--;
	    *pp2-- = *subp;
	    }
	  pp1 -= 4;
	  }
	}
      rowp -= pIncY;
      }
    }
  
  windowSize[0] = xOutDim;
  windowSize[1] = yOutDim;
}
  
     
//-------------------------------------------------------------------------
// Jim's composite stuff
//-------------------------------------------------------------------------
// Results are put in the local data.
void vtkCompositeImagePair(float *localZdata, float *localPdata, 
			   float *remoteZdata, float *remotePdata, 
			   int total_pixels, int useCharFlag) 
{
  int i,j;
  int pixel_data_size;
  float *pEnd;

  if (useCharFlag) 
    {
    pEnd = remoteZdata + total_pixels;
    while(remoteZdata != pEnd) 
      {
      if (*remoteZdata < *localZdata) 
	{
	*localZdata++ = *remoteZdata++;
	*localPdata++ = *remotePdata++;
	}
      else
	{
	++localZdata;
	++remoteZdata;
	++localPdata;
	++remotePdata;
	}
      }
    } 
  else 
    {
    pixel_data_size = 4;
    for (i = 0; i < total_pixels; i++) 
      {
      if (remoteZdata[i] < localZdata[i]) 
	{
	localZdata[i] = remoteZdata[i];
	for (j = 0; j < pixel_data_size; j++) 
	  {
	  localPdata[i*pixel_data_size+j] = remotePdata[i*pixel_data_size+j];
	  }
	}
      }
    }
}


#define vtkTCPow2(j) (1 << (j))
//----------------------------------------------------------------------------
void vtkTreeComposite::Composite()
{
  float *localZdata = NULL;
  float *localPdata = NULL;
  int windowSize[2];
  int *size;
  int total_pixels;
  int pdata_size, zdata_size;
  int myId, numProcs;
  int i, id;
  
  vtkTimerLog *timer = vtkTimerLog::New();
  
  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

  size = this->RenderWindow->GetSize();
  windowSize[0] = size[0];
  windowSize[1] = size[1];
  total_pixels = windowSize[0] * windowSize[1];

  // Get the z buffer.
  timer->StartTimer();
  localZdata = this->RenderWindow->GetZbufferData(0,0,windowSize[0]-1, windowSize[1]-1);
  zdata_size = total_pixels;

  // Get the pixel data.
  if (this->UseChar) 
    { 
    // Condition is here until we fix the resize bug in vtkMesaRenderWindow.
    localPdata = (float*)this->RenderWindow->GetRGBACharPixelData(0,0,windowSize[0]-1,
								  windowSize[1]-1,0);    
    pdata_size = total_pixels;
    } 
  else 
    {
    localPdata = this->RenderWindow->GetRGBAPixelData(0,0,windowSize[0]-1, \
						      windowSize[1]-1,0);
    pdata_size = 4*total_pixels;
    }
  
  timer->StopTimer();
  this->GetBuffersTime = timer->GetElapsedTime();
  
  // Handle reduction factor for process 0 here.
  if (this->ReductionFactor > 1)
    {
    // localZdata, localPdata and window size get modified.
    this->ReduceBuffer(localZdata, localPdata, windowSize);
    // Recompute array sizes.
    pdata_size = total_pixels = windowSize[0] * windowSize[1];
    if ( ! this->UseChar)
      {
      pdata_size *= 4;
      }
    }
  
  double doubleLogProcs = log((double)numProcs)/log((double)2);
  int logProcs = (int)doubleLogProcs;

  // not a power of 2 -- need an additional level
  if (doubleLogProcs != (double)logProcs) 
    {
    logProcs++;
    }

  timer->StartTimer();
  
  for (i = 0; i < logProcs; i++) 
    {
    if ((myId % (int)vtkTCPow2(i)) == 0) 
      { // Find participants
      if ((myId % (int)vtkTCPow2(i+1)) < vtkTCPow2(i)) 
	{
	// receivers
	id = myId+vtkTCPow2(i);
	
	// only send or receive if sender or receiver id is valid
	// (handles non-power of 2 cases)
	if (id < numProcs) 
	  {
	  this->Controller->Receive(this->ZData, zdata_size, id, 99);
	  this->Controller->Receive(this->PData, pdata_size, id, 99);
	  
	  // notice the result is stored as the local data
	  vtkCompositeImagePair(localZdata, localPdata, this->ZData, this->PData, 
				total_pixels, this->UseChar);
	  }
	}
      else 
	{
	id = myId-vtkTCPow2(i);
	if (id < numProcs) 
	  {
	  this->Controller->Send(localZdata, zdata_size, id, 99);
	  this->Controller->Send(localPdata, pdata_size, id, 99);
	  }
	}
	}
    }
  timer->StopTimer();
  this->TransmitTime = timer->GetElapsedTime();
  
  if (myId ==0) 
    {
    if (this->ReductionFactor > 1)
      {
      // localZdata, localPdata and window size get modified.
      this->MagnifyBuffer(localPdata, windowSize);
      }
  
    timer->StartTimer();
    if (this->UseChar) 
      {
      this->RenderWindow->SetRGBACharPixelData(0,0, windowSize[0]-1, \
			     windowSize[1]-1,(unsigned char*)localPdata,0);
      } 
    else 
      {
      this->RenderWindow->SetRGBAPixelData(0,0,windowSize[0]-1, 
			       windowSize[1]-1,localPdata,0);
      }
    timer->StopTimer();
    this->SetBuffersTime = timer->GetElapsedTime();
    }
  
  if (localPdata)
    {
    delete localPdata;
    }  
  if (localZdata)
    {
    delete localZdata;
    }
  
  timer->Delete();
  timer = NULL;
}


void vtkTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  
  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
  if (this->UseChar)
    {
    os << indent << "UseChar: On\n";
    }
  else
    {
    os << indent << "UseChar: Off\n";
    }  
  
  if ( this->RenderWindow )
    {
    os << indent << "RenderWindow: " << this->RenderWindow << "\n";
    }
  else
    {
    os << indent << "RenderWindow: (none)\n";
    }
  
  os << indent << "SetBuffersTime: " << this->SetBuffersTime << "\n";
  os << indent << "GetBuffersTime: " << this->GetBuffersTime << "\n";
  os << indent << "TransmitTime: " << this->TransmitTime << "\n";
  if (this->UseCompositing)
    {
    os << indent << "UseCompositing: On\n";
    }
  else
    {
    os << indent << "UseCompositing: Off\n";
    }
}
