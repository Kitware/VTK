/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    TreeComposite.cxx
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

#include "vtkSphereSource.h"
#include "vtkElevationFilter.h"
#include "vtkMultiProcessController.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkMath.h"



class vtkNodeInfo
{
public:  
  vtkRenderer* Ren;
  vtkRenderWindow* RenWindow;
  vtkMultiProcessController* Controller;
};


#define RENDER_HACK_TAG 1234

// A structure to communicate renderer info.
struct vtkCompositeRenderInfo 
{
  float CameraPosition[3];
  float CameraFocalPoint[3];
  float CameraViewUp[3];
  float CameraClippingRange[2];
  float LightPosition[3];
  float LightFocalPoint[3];
  int WindowSize[2];
};

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

void vtkTreeComposite(vtkRenderWindow *renWin, 
		      vtkMultiProcessController *controller,
		      int flag, float *remoteZdata, 
		      float *remotePdata) 
{
  float *localZdata, *localPdata;
  int *windowSize;
  int total_pixels;
  int pdata_size, zdata_size;
  int myId, numProcs;
  int i, id;
  

  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();

  windowSize = renWin->GetSize();
  total_pixels = windowSize[0] * windowSize[1];

  // Get the z buffer.
  localZdata = renWin->GetZbufferData(0,0,windowSize[0]-1, windowSize[1]-1);
  zdata_size = total_pixels;

  // Get the pixel data.
  if (flag) 
    { 
    localPdata = renWin->GetRGBAPixelData(0,0,windowSize[0]-1, \
					  windowSize[1]-1,0);
    pdata_size = 4*total_pixels;
    } 
  else 
    {
    // Condition is here until we fix the resize bug in vtkMesarenderWindow.
    localPdata = (float*)renWin->GetRGBACharPixelData(0,0,windowSize[0]-1,windowSize[1]-1,0);    
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
	  controller->Receive(remoteZdata, zdata_size, id, 99);
	  controller->Receive(remotePdata, pdata_size, id, 99);
	  
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
	  controller->Send(localZdata, zdata_size, id, 99);
	  controller->Send(localPdata, pdata_size, id, 99);
	  }
	}
      }
    }

  if (myId ==0) 
    {
    if (flag) 
      {
      renWin->SetRGBAPixelData(0,0,windowSize[0]-1, 
			       windowSize[1]-1,localPdata,0);
      } 
    else 
      {
      renWin->SetRGBACharPixelData(0,0, windowSize[0]-1, \
			     windowSize[1]-1,(unsigned char*)localPdata,0);
      }
    }
}

//-------------------------------------------------------------------------
void start_render(void* arg)
{
  vtkNodeInfo* ri = (vtkNodeInfo*) arg;
  struct vtkCompositeRenderInfo info;
  int id, num;
  int *windowSize;

  vtkRenderer* ren = ri->Ren;
  vtkRenderWindow* renWin = ri->RenWindow;
  vtkMultiProcessController *controller = ri->Controller;

  // Get a global (across all processes) clipping range.
  // ren->ResetCameraClippingRange();
  
  // Make sure the satellite renderers have the same camera I do.
  vtkCamera *cam = ren->GetActiveCamera();
  vtkLightCollection *lc = ren->GetLights();
  lc->InitTraversal();
  vtkLight *light = lc->GetNextItem();
  cerr << light << endl;
  cam->GetPosition(info.CameraPosition);
  cam->GetFocalPoint(info.CameraFocalPoint);
  cam->GetViewUp(info.CameraViewUp);
  cam->GetClippingRange(info.CameraClippingRange);
  light->GetPosition(info.LightPosition);
  light->GetFocalPoint(info.LightFocalPoint);
  // Make sure the render slave size matches our size
  windowSize = renWin->GetSize();
  info.WindowSize[0] = windowSize[0];
  info.WindowSize[1] = windowSize[1];
  num = controller->GetNumberOfProcesses();

  for (id = 1; id < num; ++id)
    {
	cout << "Calling trigger rmi" << endl;
	controller->TriggerRMI(id, NULL, 0, RENDER_HACK_TAG);
	controller->Send((char*)(&info), 
			 sizeof(struct vtkCompositeRenderInfo), id, 133);
    }
  
  // Turn swap buffers off before the render so the end render method has a chance
  // to add to the back buffer.
  renWin->SwapBuffersOff();

}

void end_render(void* arg)
{
  vtkNodeInfo* ri = (vtkNodeInfo*) arg;

  vtkRenderer* ren = ri->Ren;
  vtkRenderWindow* renWin = ri->RenWindow;
  vtkMultiProcessController *controller = ri->Controller;

  int *windowSize;
  int numPixels;
  int numProcs;
  float *pdata, *zdata;    
  
  windowSize = renWin->GetSize();
  numProcs = controller->GetNumberOfProcesses();
  numPixels = (windowSize[0] * windowSize[1]);

  cout << "In end_render, window size is: " << numPixels << endl;
  if (numProcs > 1)
    {
    pdata = new float[numPixels];
    zdata = new float[numPixels];
    vtkTreeComposite(renWin, controller, 0, zdata, pdata);
    
    delete [] zdata;
    delete [] pdata;    
    }
  
  // Force swap buffers here.
  renWin->SwapBuffersOn();  
  renWin->Frame();
}

void render_hack(void *arg, void *, int, int remoteId)
{
  vtkNodeInfo* ri = (vtkNodeInfo*) arg;

  vtkRenderer* ren = ri->Ren;
  vtkRenderWindow* renWin = ri->RenWindow;
  vtkMultiProcessController *controller = ri->Controller;

  cout << "Process : " <<  controller->GetLocalProcessId()
       << " received a render_hack request." << endl;

  unsigned char *pdata;
  int *window_size;
  int length, numPixels;
  int myId, numProcs;
  vtkCompositeRenderInfo info;
  
  myId = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
  
  // Makes an assumption about how the tasks are setup (UI id is 0).
  // Receive the camera information.
  controller->Receive((char*)(&info), sizeof(struct vtkCompositeRenderInfo), 0, 133);
  vtkCamera *cam = ren->GetActiveCamera();
  vtkLightCollection *lc = ren->GetLights();
  lc->InitTraversal();
  vtkLight *light = lc->GetNextItem();
  
  cam->SetPosition(info.CameraPosition);
  cam->SetFocalPoint(info.CameraFocalPoint);
  cam->SetViewUp(info.CameraViewUp);
  cam->SetClippingRange(info.CameraClippingRange);
  if (light)
    {
    light->SetPosition(info.LightPosition);
    light->SetFocalPoint(info.LightFocalPoint);
    }
  
  renWin->SetSize(info.WindowSize);
  
  renWin->Render();

  window_size = renWin->GetSize();
  
  numPixels = (window_size[0] * window_size[1]);
  
  if (1)
    {
    float *pdata, *zdata;
    pdata = new float[numPixels];
    zdata = new float[numPixels];
    vtkTreeComposite(renWin, controller, 0, zdata, pdata);
    delete [] zdata;
    delete [] pdata;
    }
  else
    {
    length = 3*numPixels;  
    pdata = renWin->GetPixelData(0,0,window_size[0]-1, window_size[1]-1,1);
    controller->Send((char*)pdata, length, 0, 99);
    }
  
  return;
}

void process(vtkMultiProcessController *controller, void *arg )
{
  vtkSphereSource *sphere;
  vtkElevationFilter *elev;
  int myid, numProcs;
  float val;
  
  
  myid = controller->GetLocalProcessId();
  numProcs = controller->GetNumberOfProcesses();
    
  // Compute a different color for each process.
  sphere = vtkSphereSource::New();
  
  elev = vtkElevationFilter::New();
  elev->SetInput(sphere->GetOutput());
  vtkMath::RandomSeed(myid * 100);
  val = vtkMath::Random();
  elev->SetScalarRange(val, val+0.001);
    
  int i, j;
  vtkRenderer *ren = vtkRenderer::New();
  vtkRenderWindow *renWindow = vtkRenderWindow::New();
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();

  vtkPolyDataMapper *mapper = vtkPolyDataMapper::New();
  vtkActor *actor = vtkActor::New();
  vtkCamera *cam = vtkCamera::New();

  vtkNodeInfo* nodeInfo = new vtkNodeInfo();
  nodeInfo->Ren = ren;
  nodeInfo->RenWindow = renWindow;
  nodeInfo->Controller = controller;
 
  renWindow->AddRenderer(ren);

  iren->SetRenderWindow(renWindow);
  ren->SetBackground(0.9, 0.9, 0.9);
  renWindow->SetSize( 400, 400);
  
  mapper->SetPiece(myid);
  mapper->SetNumberOfPieces(numProcs);
  mapper->SetInput(elev->GetPolyDataOutput());
  actor->SetMapper(mapper);
  
  // assign our actor to the renderer
  ren->AddActor(actor);
 
  
  cam->SetFocalPoint(0, 0, 0);
  cam->SetPosition(0, 0, 10);
  cam->SetViewUp(0, 1, 0);
  cam->SetViewAngle(30);
  // this was causing an update.
  //ren->ResetCameraClippingRange();
  //{
  //double *range = ren->GetActiveCamera()->GetClippingRange();
  //cerr << range[0] << ", " << range[1] << endl;
  //}
  cam->SetClippingRange(5.0, 15.0);
  ren->SetActiveCamera(cam);
  ren->CreateLight();  
  
  if ( myid == 0 )
    {
    ren->SetStartRenderMethod(start_render, nodeInfo);
    ren->SetEndRenderMethod(end_render, nodeInfo);
    //  Begin mouse interaction
    iren->Start();
    }
  else
    {
    controller->AddRMI(render_hack, nodeInfo, RENDER_HACK_TAG); 
    controller->ProcessRMIs();
    }
}


void main( int argc, char *argv[] )
{
  vtkMultiProcessController *controller;
  char save_filename[100]="\0";

    
  controller = vtkMultiProcessController::New();

  controller->Initialize(argc, argv);
  // Needed for threaded controller.
  // controller->SetNumberOfProcesses(2);
  controller->SetSingleMethod(process, save_filename);
  if (controller->IsA("vtkThreadedController"))
    {
    controller->SetNumberOfProcesses(8);
    } 
  controller->SingleMethodExecute();

  controller->Delete();


}





