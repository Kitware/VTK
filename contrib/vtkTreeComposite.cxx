/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    vtkTreeComposite.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  
Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/


#include "vtkTreeComposite.h"
#include "vtkCommand.h"
#include "vtkPolyDataMapper.h"
#include "vtkObjectFactory.h"


#define VTK_COMPOSITE_RENDER_RMI_TAG 12721
#define VTK_COMPOSITE_WIN_INFO_TAG   134
#define VTK_COMPOSITE_REN_INFO_TAG   135

// Structures to communicate render info.
struct vtkCompositeRenderWindowInfo 
{
  int Size[2];
  int NumberOfRenderers;
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


//------------------------------------------------------------------------------
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
}

  
//-------------------------------------------------------------------------
vtkTreeComposite::~vtkTreeComposite()
{
  this->SetRenderWindow(NULL);
}


//-------------------------------------------------------------------------
// We may want to pass the render window as an argument for a sanity check.
void vtkTreeCompositeStartRender(vtkObject *caller, unsigned long event, 
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
void vtkTreeCompositeEndRender(vtkObject *caller, unsigned long event, 
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
void vtkTreeCompositeStartInteractor(vtkObject *o, unsigned long event, 
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
void vtkTreeCompositeExitInteractor(vtkObject *o, unsigned long event, 
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
void vtkTreeCompositeResetCamera(vtkObject *caller, unsigned long event, 
                                 void *clientData, void *)
{
  vtkTreeComposite *self = (vtkTreeComposite *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  self->ResetCamera(ren);
}
//-------------------------------------------------------------------------
void vtkTreeCompositeResetCameraClippingRange(vtkObject *caller, 
                                              unsigned long event, 
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

//-------------------------------------------------------------------------
// Only process 0 needs start and end render callbacks.
void vtkTreeComposite::SetRenderWindow(vtkRenderWindow *renWin)
{
  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->RenderWindow)
    {
    this->RenderWindow->UnRegister(this);
    this->RenderWindow =  NULL;
    this->SetRenderWindowInteractor(NULL);
    if (this->Controller && this->Controller->GetLocalProcessId() == 0)
      {
      this->RenderWindow->RemoveObserver(this->StartTag);
      this->RenderWindow->RemoveObserver(this->EndTag);
      }
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
  float *pdata, *zdata;
  int numPixels;
  int myId, numProcs, i;
  vtkCompositeRenderWindowInfo winInfo;
  vtkCompositeRendererInfo renInfo;
  vtkRendererCollection *rens;
  vtkRenderer* ren;
  vtkCamera *cam;
  vtkLightCollection *lc;
  vtkLight *light;
  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;
  
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  // Receive the window size.
  controller->Receive((char*)(&winInfo), 
                      sizeof(struct vtkCompositeRenderWindowInfo), 0, 
                      VTK_COMPOSITE_WIN_INFO_TAG);
  renWin->SetSize(winInfo.Size);

  // Synchronize the renderers.
  rens = renWin->GetRenderers();
  rens->InitTraversal();
  for (i = 0; i < winInfo.NumberOfRenderers; ++i)
    {
    // Receive the camera information.
    controller->Receive((char*)(&renInfo), 
                        sizeof(struct vtkCompositeRendererInfo), 
                        0, VTK_COMPOSITE_REN_INFO_TAG);
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
  numPixels = (winInfo.Size[0] * winInfo.Size[1]);
  
  pdata = new float[numPixels];
  zdata = new float[numPixels];
  this->Composite(0, zdata, pdata);
  delete [] zdata;
  delete [] pdata;
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

  this->Controller->AddRMI(vtkTreeCompositeRenderRMI, (void*)this, 
                           VTK_COMPOSITE_RENDER_RMI_TAG); 
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
    this->Controller->TriggerRMI(id, VTK_BREAK_RMI_TAG);
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

  vtkRenderWindow* renWin = this->RenderWindow;
  vtkMultiProcessController *controller = this->Controller;

  if (controller == NULL)
    {
    return;
    }

  // Trigger the satelite processes to start their render routine.
  rens = this->RenderWindow->GetRenderers();
  numProcs = controller->GetNumberOfProcesses();
  size = this->RenderWindow->GetSize();
  winInfo.Size[0] = size[0];
  winInfo.Size[1] = size[1];
  winInfo.NumberOfRenderers = rens->GetNumberOfItems();
  for (id = 1; id < numProcs; ++id)
    {
    controller->TriggerRMI(id, NULL, 0, VTK_COMPOSITE_RENDER_RMI_TAG);
    // Synchronize the size of the windows.
    controller->Send((char*)(&winInfo), 
                     sizeof(vtkCompositeRenderWindowInfo), id, 
                     VTK_COMPOSITE_WIN_INFO_TAG);
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
    light->GetPosition(renInfo.LightPosition);
    light->GetFocalPoint(renInfo.LightFocalPoint);

    for (id = 1; id < numProcs; ++id)
      {
      controller->TriggerRMI(id, NULL, 0, VTK_COMPOSITE_RENDER_RMI_TAG);
      controller->Send((char*)(&renInfo),
                       sizeof(struct vtkCompositeRendererInfo), id, 
                       VTK_COMPOSITE_REN_INFO_TAG);
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
  int numPixels;
  int numProcs;
  float *pdata, *zdata;    
  
  if (controller == NULL)
    {
    return;
    }

  windowSize = renWin->GetSize();
  numProcs = controller->GetNumberOfProcesses();
  numPixels = (windowSize[0] * windowSize[1]);

  if (numProcs > 1)
    {
    // It would be more efficient to save these arrays as ivars.
    pdata = new float[numPixels];
    zdata = new float[numPixels];
    this->Composite(0, zdata, pdata);
    
    delete [] zdata;
    delete [] pdata;    
    }
  
  // Force swap buffers here.
  renWin->SwapBuffersOn();  
  renWin->Frame();
}


//-------------------------------------------------------------------------
void vtkTreeComposite::ResetCamera(vtkRenderer *ren)
{
  float bounds[6];

  if (this->Controller == NULL)
    {
    return;
    }

  this->ComputeVisiblePropBounds(ren, bounds);
  ren->ResetCamera(bounds);
}

//-------------------------------------------------------------------------
void vtkTreeComposite::ResetCameraClippingRange(vtkRenderer *ren)
{
  float bounds[6];

  if (this->Controller == NULL)
    {
    return;
    }

  this->ComputeVisiblePropBounds(ren, bounds);
  ren->ResetCameraClippingRange(bounds);
}

//----------------------------------------------------------------------------
void vtkTreeComposite::ComputeVisiblePropBounds(vtkRenderer *ren, 
                                                float bounds[6])
{
  //float tmp[6];
  int id, num;
  
  num = this->Controller->GetNumberOfProcesses();  
  for (id = 1; id < num; ++id)
    {
    //pvApp->RemoteScript(id, "%s TransmitBounds", this->GetTclName());
    }

  ren->ComputeVisiblePropBounds(bounds);

  for (id = 1; id < num; ++id)
    {
    //controller->Receive(tmp, 6, id, 112);
    //if (tmp[0] < bounds[0]) {bounds[0] = tmp[0];}
    //if (tmp[1] > bounds[1]) {bounds[1] = tmp[1];}
    //if (tmp[2] < bounds[2]) {bounds[2] = tmp[2];}
    //if (tmp[3] > bounds[3]) {bounds[3] = tmp[3];}
    //if (tmp[4] < bounds[4]) {bounds[4] = tmp[4];}
    //if (tmp[5] > bounds[5]) {bounds[5] = tmp[5];}
    }
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
// Jim's composite stuff
//-------------------------------------------------------------------------
// Results are put in the local data.
void vtkCompositeImagePair(float *localZdata, float *localPdata, 
			   float *remoteZdata, float *remotePdata, 
			   int total_pixels, int flag) 
{
  int i,j;
  int pixel_data_size;
  float *pEnd;

  if (flag) 
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
  else 
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
}


#define vtkTCPow2(j) (1 << (j))
//----------------------------------------------------------------------------
void vtkTreeComposite::Composite(int flag, float *remoteZdata, 
				 float *remotePdata) 
{
  float *localZdata, *localPdata;
  int *windowSize;
  int total_pixels;
  int pdata_size, zdata_size;
  int myId, numProcs;
  int i, id;
  

  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

  windowSize = this->RenderWindow->GetSize();
  total_pixels = windowSize[0] * windowSize[1];

  // Get the z buffer.
  localZdata = this->RenderWindow->GetZbufferData(0,0,windowSize[0]-1, windowSize[1]-1);
  zdata_size = total_pixels;

  // Get the pixel data.
  if (flag) 
    { 
    localPdata = this->RenderWindow->GetRGBAPixelData(0,0,windowSize[0]-1, \
						      windowSize[1]-1,0);
    pdata_size = 4*total_pixels;
    } 
  else 
    {
    // Condition is here until we fix the resize bug in vtkMesarenderWindow.
    localPdata = (float*)this->RenderWindow->GetRGBACharPixelData(0,0,windowSize[0]-1,
								  windowSize[1]-1,0);    
    pdata_size = total_pixels;
    }
  
  double doubleLogProcs = log((double)numProcs)/log((double)2);
  int logProcs = (int)doubleLogProcs;

  // not a power of 2 -- need an additional level
  if (doubleLogProcs != (double)logProcs) 
    {
    logProcs++;
    }

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
	  this->Controller->Receive(remoteZdata, zdata_size, id, 99);
	  this->Controller->Receive(remotePdata, pdata_size, id, 99);
	  
	  // notice the result is stored as the local data
	  vtkCompositeImagePair(localZdata, localPdata, remoteZdata, remotePdata, 
				total_pixels, flag);
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

  if (myId ==0) 
    {
    if (flag) 
      {
      this->RenderWindow->SetRGBAPixelData(0,0,windowSize[0]-1, 
			       windowSize[1]-1,localPdata,0);
      } 
    else 
      {
      this->RenderWindow->SetRGBACharPixelData(0,0, windowSize[0]-1, \
			     windowSize[1]-1,(unsigned char*)localPdata,0);
      }
    }
}


void vtkTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
  
  if ( this->RenderWindow )
    {
    os << indent << "RenderWindow: " << this->RenderWindow << "\n";
    }
  else
    {
    os << indent << "RenderWindow: (none)\n";
    }
}
