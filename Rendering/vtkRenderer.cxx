/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderer.cxx
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
#include <stdlib.h>
#include <string.h>

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkMath.h"
#include "vtkVolume.h"
#include "vtkTimerLog.h"
#include "vtkCuller.h"
#include "vtkFrustumCoverageCuller.h"
#include "vtkGraphicsFactory.h"
#include "vtkOutputWindow.h"
#include "vtkAssemblyNode.h"
#include "vtkPicker.h"
#include "vtkCommand.h"
#include "vtkRayCaster.h"

// Create a vtkRenderer with a black background, a white ambient light, 
// two-sided lighting turned on, a viewport of (0,0,1,1), and backface culling
// turned off.
vtkRenderer::vtkRenderer()
{
  this->PickedProp   = NULL;
  this->ActiveCamera = NULL;

  this->Ambient[0] = 1;
  this->Ambient[1] = 1;
  this->Ambient[2] = 1;

  this->AllocatedRenderTime = 100;
  this->TimeFactor = 1.0;
  
  this->CreatedLight = NULL;
  
  this->TwoSidedLighting        = 1;
  this->BackingStore            = 0;
  this->BackingImage            = NULL;
  this->LastRenderTimeInSeconds = -1.0;
  
  this->RenderWindow = NULL;
  this->Lights  =  vtkLightCollection::New();
  this->Actors  =  vtkActorCollection::New();
  this->Volumes = vtkVolumeCollection::New();

  this->LightFollowCamera = 1;

  this->NumberOfPropsToRayCast         = 0;
  this->NumberOfPropsRenderedAsGeometry = 0;
  this->NumberOfPropsToRenderIntoImage = 0;

  this->PropArray                = NULL;
  this->RayCastPropArray         = NULL;
  this->RenderIntoImagePropArray = NULL;   

  this->Layer                    = 0;
  this->Interactive              = 1;
  this->Cullers = vtkCullerCollection::New();  
  vtkFrustumCoverageCuller *cull = vtkFrustumCoverageCuller::New();
  this->Cullers->AddItem(cull);
  cull->Delete();
  
  this->RayCaster = vtkRayCaster::New();
}

vtkRenderer::~vtkRenderer()
{
  this->SetRenderWindow( NULL );
  
  if (this->ActiveCamera)
    {
    this->ActiveCamera->UnRegister(this);
    this->ActiveCamera = NULL;
    }

  if (this->CreatedLight)
    {
    this->CreatedLight->UnRegister(this);
    this->CreatedLight = NULL;
    }

  if (this->BackingImage)
    {
    delete [] this->BackingImage;
    }
  
  this->Actors->Delete();
  this->Actors = NULL;
  this->Volumes->Delete();
  this->Volumes = NULL;
  this->Lights->Delete();
  this->Lights = NULL;
  this->Cullers->Delete();
  this->Cullers = NULL;
  
  this->RayCaster->Delete();
}

// return the correct type of Renderer 
vtkRenderer *vtkRenderer::New()
{ 
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkRenderer");
  return (vtkRenderer *)ret;
}

// Concrete render method.
void vtkRenderer::Render(void)
{
  double   t1, t2;
  int      i;
  vtkProp  *aProp;

  t1 = vtkTimerLog::GetCurrentTime();

  this->InvokeEvent(vtkCommand::StartEvent,NULL);

  // if backing store is on and we have a stored image
  if (this->BackingStore && this->BackingImage &&
      this->MTime < this->RenderTime &&
      this->ActiveCamera->GetMTime() < this->RenderTime &&
      this->RenderWindow->GetMTime() < this->RenderTime)
    {
    int mods = 0;
    vtkLight *light;
    
    // now we just need to check the lights and actors
    for(this->Lights->InitTraversal(); 
        (light = this->Lights->GetNextItem()); )
      {
      if (light->GetSwitch() && 
          light->GetMTime() > this->RenderTime)
        {
        mods = 1;
	goto completed_mod_check;
        }
      }
    for (this->Props->InitTraversal(); 
         (aProp = this->Props->GetNextProp()); )
      {
      // if it's invisible, we can skip the rest 
      if (aProp->GetVisibility())
        {
        if (aProp->GetRedrawMTime() > this->RenderTime)
          {
          mods = 1;
	  goto completed_mod_check;
          }
        }
      }
    
    completed_mod_check:

    if (!mods)
      {
      int rx1, ry1, rx2, ry2;
      
      // backing store should be OK, lets use it
      // calc the pixel range for the renderer
      rx1 = (int)(this->Viewport[0]*(this->RenderWindow->GetSize()[0] - 1));
      ry1 = (int)(this->Viewport[1]*(this->RenderWindow->GetSize()[1] - 1));
      rx2 = (int)(this->Viewport[2]*(this->RenderWindow->GetSize()[0] - 1));
      ry2 = (int)(this->Viewport[3]*(this->RenderWindow->GetSize()[1] - 1));
      this->RenderWindow->SetPixelData(rx1,ry1,rx2,ry2,this->BackingImage,0);
      this->InvokeEvent(vtkCommand::EndEvent,NULL);
      return;
      }
    }

  // Create the initial list of visible props
  // This will be passed through AllocateTime(), where
  // a time is allocated for each prop, and the list
  // maybe re-ordered by the cullers. Also create the
  // sublists for the props that need ray casting, and
  // the props that need to be rendered into an image.
  // Fill these in later (in AllocateTime) - get a 
  // count of them there too
  if ( this->Props->GetNumberOfItems() > 0 )
    {
    this->PropArray                = new vtkProp *[this->Props->GetNumberOfItems()];
    this->RayCastPropArray         = new vtkProp *[this->Props->GetNumberOfItems()];
    this->RenderIntoImagePropArray = new vtkProp *[this->Props->GetNumberOfItems()];
    }
  else
    {
    this->PropArray = NULL;
    this->RayCastPropArray = NULL;
    this->RenderIntoImagePropArray = NULL;
    }

  this->PropArrayCount = 0;
  for ( i = 0, this->Props->InitTraversal(); 
        (aProp = this->Props->GetNextProp());i++ )
    {
    if ( aProp->GetVisibility() )
      {
      this->PropArray[this->PropArrayCount++] = aProp;
      }
    }
  
  if ( this->PropArrayCount == 0 )
    {
    vtkDebugMacro( << "There are no visible props!" );
    this->NumberOfPropsToRayCast = 0;
    this->NumberOfPropsToRenderIntoImage = 0;
    }
  else
    {
    // Call all the culling methods to set allocated time
    // for each prop and re-order the prop list if desired

    this->AllocateTime();
    }

  // do the render library specific stuff
  this->DeviceRender();

  // If we aborted, restore old estimated times
  // Setting the allocated render time to zero also sets the 
  // estimated render time to zero, so that when we add back
  // in the old value we have set it correctly.
  if ( this->RenderWindow->GetAbortRender() )
    {
    for ( i = 0; i < this->PropArrayCount; i++ )
      {
      this->PropArray[i]->RestoreEstimatedRenderTime();
      }
    }

  // Clean up the space we allocated before. If the PropArray exists,
  // they all should exist
  if ( this->PropArray)
    {
    delete [] this->PropArray;
    delete [] this->RayCastPropArray;
    delete [] this->RenderIntoImagePropArray;

    this->PropArray                = NULL;
    this->RayCastPropArray         = NULL;
    this->RenderIntoImagePropArray = NULL;   
    }

  if (this->BackingStore)
    {
    if (this->BackingImage)
      {
      delete [] this->BackingImage;
      }
    
    int rx1, ry1, rx2, ry2;
    
    // backing store should be OK, lets use it
    // calc the pixel range for the renderer
    rx1 = (int)(this->Viewport[0]*(this->RenderWindow->GetSize()[0] - 1));
    ry1 = (int)(this->Viewport[1]*(this->RenderWindow->GetSize()[1] - 1));
    rx2 = (int)(this->Viewport[2]*(this->RenderWindow->GetSize()[0] - 1));
    ry2 = (int)(this->Viewport[3]*(this->RenderWindow->GetSize()[1] - 1));
    this->BackingImage = this->RenderWindow->GetPixelData(rx1,ry1,rx2,ry2,0);
    }
    

  // If we aborted, do not record the last render time.
  // Lets play around with determining the acuracy of the 
  // EstimatedRenderTimes.  We can try to adjust for bad 
  // estimates with the TimeFactor.
  if ( ! this->RenderWindow->GetAbortRender() )
    {
    // Measure the actual RenderTime
    t2 = vtkTimerLog::GetCurrentTime();
    this->LastRenderTimeInSeconds = (float) (t2 - t1);

    if (this->LastRenderTimeInSeconds == 0.0)
      {
      this->LastRenderTimeInSeconds = 0.0001;
      }
    this->TimeFactor = this->AllocatedRenderTime/this->LastRenderTimeInSeconds;
    }
}

float vtkRenderer::GetAllocatedRenderTime()
{
  return this->AllocatedRenderTime;
}

float vtkRenderer::GetTimeFactor()
{
  return this->TimeFactor;
}

void vtkRenderer::RenderOverlay()
{
  vtkProp *aProp;
  
  for (this->Props->InitTraversal(); 
       (aProp = this->Props->GetNextProp()); )
    {
    if ( aProp->GetVisibility() )
      {
      aProp->RenderOverlay(this);
      }
    }

  this->InvokeEvent(vtkCommand::EndEvent,NULL);
  this->RenderTime.Modified();
}

// Ask active camera to load its view matrix.
int vtkRenderer::UpdateCamera ()
{
  if (!this->ActiveCamera)
    {
    vtkDebugMacro(<< "No cameras are on, creating one.");
    // the get method will automagically create a camera
    // and reset it since one hasn't been specified yet
    this->GetActiveCamera();
    }

  // update the viewing transformation
  this->ActiveCamera->Render((vtkRenderer *)this); 
  
  return 1;
}

int vtkRenderer::UpdateLightGeometry()
{
  vtkCamera *camera;
  vtkLight *light;
  vtkMatrix4x4 *lightMatrix;

  if (this->LightFollowCamera) 
  {
    // only update the light's geometry if this Renderer is tracking
    // this lights.  That allows one renderer to view the lights that
    // another renderer is setting up.

    camera = this->GetActiveCamera();
    lightMatrix = camera->GetCameraLightTransformMatrix();

    for(this->Lights->InitTraversal(); 
	(light = this->Lights->GetNextItem()); )
    {
      if (light->LightTypeIsSceneLight())
	{
	  // nothing needs to be done.
	  ;
	}
      else if (light->LightTypeIsHeadlight())
	{
	  // update position and orientation of light to match camera.
	  light->SetPosition(camera->GetPosition());
	  light->SetFocalPoint(camera->GetFocalPoint());
	}
      else if (light->LightTypeIsCameraLight())
	{
	  light->SetTransformMatrix(lightMatrix);
	}
      else 
	{
	  vtkErrorMacro(<< "light has unknown light type");
	}
    }
  }
  return 1;
}

// Do all outer culling to set allocated time for each prop.
// Possibly re-order the actor list.
void vtkRenderer::AllocateTime()
{
  int          initialized = 0;
  float        renderTime;
  float        totalTime;
  int          i;
  vtkCuller    *aCuller;
  vtkProp      *aProp;

  // Give each of the cullers a chance to modify allocated rendering time
  // for the entire set of props. Each culler returns the total time given
  // by AllocatedRenderTime for all props. Each culler is required to
  // place any props that have an allocated render time of 0.0 
  // at the end of the list. The PropArrayCount value that is
  // returned is the number of non-zero, visible actors.
  // Some cullers may do additional sorting of the list (by distance,
  // importance, etc).
  //
  // The first culler will initialize all the allocated render times. 
  // Any subsequent culling will multiply the new render time by the 
  // existing render time for an actor.

  totalTime = this->PropArrayCount;
  this->ComputeAspect();

  for (this->Cullers->InitTraversal(); 
       (aCuller=this->Cullers->GetNextItem());)
    {
    totalTime = 
      aCuller->Cull((vtkRenderer *)this, 
                    this->PropArray, this->PropArrayCount,
                    initialized );
    }

  // loop through all props and set the AllocatedRenderTime
  for ( i = 0; i < this->PropArrayCount; i++ )
    {
    aProp = this->PropArray[i];

    // If we don't have an outer cull method in any of the cullers,
    // then the allocated render time has not yet been initialized
    renderTime = (initialized)?(aProp->GetRenderTimeMultiplier()):(1.0);

    // We need to divide by total time so that the total rendering time
    // (all prop's AllocatedRenderTime added together) would be equal
    // to the renderer's AllocatedRenderTime.
    aProp->
      SetAllocatedRenderTime(( renderTime / totalTime ) * 
                             this->AllocatedRenderTime, 
			     this );  
    }
}

// Ask actors to render themselves. As a side effect will cause 
// visualization network to update.
int vtkRenderer::UpdateGeometry()
{
  int        i;
  
  this->NumberOfPropsRenderedAsGeometry = 0;

  if ( this->PropArrayCount == 0 ) 
    {
    return 0;
    }

  // We can render everything because if it was
  // not visible it would not have been put in the
  // list in the first place, and if it was allocated
  // no time (culled) it would have been removed from
  // the list
  
  // loop through props and give them a change to 
  // render themselves as opaque geometry
  for ( i = 0; i < this->PropArrayCount; i++ )
    {
    
    this->NumberOfPropsRenderedAsGeometry += 
      this->PropArray[i]->RenderOpaqueGeometry(this);
    }
 
  
  // loop through props and give them a chance to 
  // render themselves as translucent geometry
  for ( i = 0; i < this->PropArrayCount; i++ )
    {
    this->NumberOfPropsRenderedAsGeometry += 
      this->PropArray[i]->RenderTranslucentGeometry(this);
    }

  
  vtkDebugMacro( << "Rendered " << 
                    this->NumberOfPropsRenderedAsGeometry << " actors" );

  return  this->NumberOfPropsRenderedAsGeometry;
}

vtkWindow *vtkRenderer::GetVTKWindow()
{
  return this->RenderWindow;
}

// Specify the camera to use for this renderer.
void vtkRenderer::SetActiveCamera(vtkCamera *cam)
{
  if (this->ActiveCamera == cam)
    {
    return;
    }

  if (this->ActiveCamera)
    {
    this->ActiveCamera->UnRegister(this);
    this->ActiveCamera = NULL;
    }
  if (cam)
    {
    cam->Register(this);
    }

  this->ActiveCamera = cam;
  this->Modified();
}

// Get the current camera.
vtkCamera *vtkRenderer::GetActiveCamera()
{
  if ( this->ActiveCamera == NULL )
    {
    vtkCamera *cam = vtkCamera::New();
    this->SetActiveCamera(cam);
    cam->Delete();
    this->ResetCamera();
    }

  return this->ActiveCamera;
}

// Add a light to the list of lights.
void vtkRenderer::AddLight(vtkLight *light)
{
  this->Lights->AddItem(light);
}

// look through the props and get all the actors
vtkActorCollection *vtkRenderer::GetActors()
{
  vtkProp *aProp;
  
  // clear the collection first
  this->Actors->RemoveAllItems();
  
  for (this->Props->InitTraversal(); 
       (aProp = this->Props->GetNextProp()); )
    {
    aProp->GetActors(this->Actors);
    }
  return this->Actors;
}

// look through the props and get all the volumes
vtkVolumeCollection *vtkRenderer::GetVolumes()
{
  vtkProp *aProp;
  
  // clear the collection first
  this->Volumes->RemoveAllItems();
  
  for (this->Props->InitTraversal(); 
       (aProp = this->Props->GetNextProp()); )
    {
    aProp->GetVolumes(this->Volumes);
    }
  return this->Volumes;
}

// Remove a light from the list of lights.
void vtkRenderer::RemoveLight(vtkLight *light)
{
  this->Lights->RemoveItem(light);
}

// Add an culler to the list of cullers.
void vtkRenderer::AddCuller(vtkCuller *culler)
{
  this->Cullers->AddItem(culler);
}

// Remove an actor from the list of cullers.
void vtkRenderer::RemoveCuller(vtkCuller *culler)
{
  this->Cullers->RemoveItem(culler);
}

void vtkRenderer::CreateLight(void)
{
  if (this->CreatedLight)
    {
    this->CreatedLight->UnRegister(this);
    this->CreatedLight = NULL;
    }

  // I do not see why UnRegister is used on CreatedLight, but lets be consistent. 
  vtkLight *l = vtkLight::New();
  this->CreatedLight = l;
  this->CreatedLight->Register(this);
  this->AddLight(this->CreatedLight);
  l->Delete();
  l = NULL;

  this->CreatedLight->SetLightTypeToHeadlight();

  // set these values just to have a good default should LightFollowCamera
  // be turned off.
  this->CreatedLight->SetPosition(this->GetActiveCamera()->GetPosition());
  this->CreatedLight->SetFocalPoint(this->GetActiveCamera()->GetFocalPoint());
}

// Compute the bounds of the visible props
void vtkRenderer::ComputeVisiblePropBounds( float allBounds[6] )
{
  vtkProp    *prop;
  float      *bounds;
  int        nothingVisible=1;

  allBounds[0] = allBounds[2] = allBounds[4] = VTK_LARGE_FLOAT;
  allBounds[1] = allBounds[3] = allBounds[5] = -VTK_LARGE_FLOAT;
  
  // loop through all props
  for (this->Props->InitTraversal(); (prop = this->Props->GetNextProp()); )
    {
    // if it's invisible, or has no geometry, we can skip the rest 
    if ( prop->GetVisibility() )
      {
      bounds = prop->GetBounds();
      // make sure we haven't got bogus bounds
      if ( bounds != NULL &&
           bounds[0] > -VTK_LARGE_FLOAT && bounds[1] < VTK_LARGE_FLOAT &&
           bounds[2] > -VTK_LARGE_FLOAT && bounds[3] < VTK_LARGE_FLOAT &&
           bounds[4] > -VTK_LARGE_FLOAT && bounds[5] < VTK_LARGE_FLOAT )
        {
        nothingVisible = 0;

        if (bounds[0] < allBounds[0])
          {
          allBounds[0] = bounds[0]; 
          }
        if (bounds[1] > allBounds[1])
          {
          allBounds[1] = bounds[1]; 
          }
        if (bounds[2] < allBounds[2])
          {
          allBounds[2] = bounds[2]; 
          }
        if (bounds[3] > allBounds[3])
          {
          allBounds[3] = bounds[3]; 
          }
        if (bounds[4] < allBounds[4])
          {
          allBounds[4] = bounds[4]; 
          }
        if (bounds[5] > allBounds[5])
          {
          allBounds[5] = bounds[5]; 
          }
        }//not bogus
      }
    }
  
  if ( nothingVisible )
    {
    vtkDebugMacro(<< "Can't compute bounds, no 3D props are visible");
    return;
    }
}

// Automatically set up the camera based on the visible actors.
// The camera will reposition itself to view the center point of the actors,
// and move along its initial view plane normal (i.e., vector defined from 
// camera position to focal point) so that all of the actors can be seen.
void vtkRenderer::ResetCamera()
{
  float      allBounds[6];

  this->ComputeVisiblePropBounds( allBounds );

  if ( allBounds[0] == VTK_LARGE_FLOAT )
    {
    vtkDebugMacro( << "Cannot reset camera!" );
    return;
    }

  this->ResetCamera(allBounds);

  // Here to let parallel/distributed compositing intercept 
  // and do the right thing.
  this->InvokeEvent(vtkCommand::ResetCameraEvent,this);
}

// Automatically set the clipping range of the camera based on the
// visible actors
void vtkRenderer::ResetCameraClippingRange()
{
  float      allBounds[6];

  this->ComputeVisiblePropBounds( allBounds );

  if ( allBounds[0] == VTK_LARGE_FLOAT )
    {
    vtkDebugMacro( << "Cannot reset camera clipping range!" );
    return;
    }

  this->ResetCameraClippingRange(allBounds);

  // Here to let parallel/distributed compositing intercept 
  // and do the right thing.
  this->InvokeEvent(vtkCommand::ResetCameraClippingRangeEvent,this);
}


// Automatically set up the camera based on a specified bounding box
// (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
// that its focal point is the center of the bounding box, and adjust its
// distance and position to preserve its initial view plane normal 
// (i.e., vector defined from camera position to focal point). Note: is 
// the view plane is parallel to the view up axis, the view up axis will
// be reset to one of the three coordinate axes.
void vtkRenderer::ResetCamera(float bounds[6])
{
  float center[3];
  float distance;
  float width;
  double vn[3], *vup;
  
  this->GetActiveCamera();
  if ( this->ActiveCamera != NULL )
    {
    this->ActiveCamera->GetViewPlaneNormal(vn);
    }
  else
    {
    vtkErrorMacro(<< "Trying to reset non-existant camera");
    return;
    }

  center[0] = (bounds[0] + bounds[1])/2.0;
  center[1] = (bounds[2] + bounds[3])/2.0;
  center[2] = (bounds[4] + bounds[5])/2.0;

  width = bounds[3] - bounds[2];
  if (width < (bounds[1] - bounds[0]))
    {
    width = bounds[1] - bounds[0];
    }
  distance = 
    0.8*width/tan(this->ActiveCamera->GetViewAngle()*vtkMath::Pi()/360.0);
  distance = distance + (bounds[5] - bounds[4])/2.0;

  // check view-up vector against view plane normal
  vup = this->ActiveCamera->GetViewUp();
  if ( fabs(vtkMath::Dot(vup,vn)) > 0.999 )
    {
    vtkWarningMacro(<<"Resetting view-up since view plane normal is parallel");
    this->ActiveCamera->SetViewUp(-vup[2], vup[0], vup[1]);
    }

  // update the camera
  this->ActiveCamera->SetFocalPoint(center[0],center[1],center[2]);
  this->ActiveCamera->SetPosition(center[0]+distance*vn[0],
                                  center[1]+distance*vn[1],
                                  center[2]+distance*vn[2]);

  this->ResetCameraClippingRange( bounds );

  // setup default parallel scale
  this->ActiveCamera->SetParallelScale(width);
}
  
// Alternative version of ResetCamera(bounds[6]);
void vtkRenderer::ResetCamera(float xmin, float xmax, float ymin, float ymax, 
                              float zmin, float zmax)
{
  float bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->ResetCamera(bounds);
}

// Reset the camera clipping range to include this entire bounding box
void vtkRenderer::ResetCameraClippingRange( float bounds[6] )
{
  double  vn[3], position[3], a, b, c, d;
  double  range[2], dist;
  int     i, j, k;

  this->GetActiveCamera();
  if ( this->ActiveCamera == NULL )
    {
    vtkErrorMacro(<< "Trying to reset clipping range of non-existant camera");
    return;
    }
  
  // Find the plane equation for the camera view plane
  this->ActiveCamera->GetViewPlaneNormal(vn);
  this->ActiveCamera->GetPosition(position);
  a = -vn[0];
  b = -vn[1];
  c = -vn[2];
  d = -(a*position[0] + b*position[1] + c*position[2]);

  // Set the max near clipping plane and the min far clipping plane
  range[0] = a*bounds[0] + b*bounds[2] + c*bounds[4] + d;
  range[1] = 1e-18;

  // Find the closest / farthest bounding box vertex
  for ( k = 0; k < 2; k++ )
    {
    for ( j = 0; j < 2; j++ )
      {
      for ( i = 0; i < 2; i++ )
        {
        dist = a*bounds[i] + b*bounds[2+j] + c*bounds[4+k] + d;
        range[0] = (dist<range[0])?(dist):(range[0]);
        range[1] = (dist>range[1])?(dist):(range[1]);
        }
      }
    }
  
  // Give ourselves a little breathing room
  range[0] = 0.99*range[0] - (range[1] - range[0])*0.5;
  range[1] = 1.01*range[1] + (range[1] - range[0])*0.5;

  // Make sure near is not bigger than far
  range[0] = (range[0] >= range[1])?(0.01*range[1]):(range[0]);

  // Make sure near is at least some fraction of far - this prevents near
  // from being behind the camera or too close in front. How close is too
  // close depends on the resolution of the depth buffer
  int ZBufferDepth = 16;
  if (this->RenderWindow)
    {
      ZBufferDepth = this->RenderWindow->GetDepthBufferSize();
    }
  //
  if ( ZBufferDepth <= 16 )
    {
    range[0] = (range[0] < 0.01*range[1])?(0.01*range[1]):(range[0]);
    }
  else if ( ZBufferDepth <= 24 )
    {
    range[0] = (range[0] < 0.01*range[1])?(0.01*range[1]):(range[0]);
    }
  else
    {
    range[0] = (range[0] < 0.01*range[1])?(0.01*range[1]):(range[0]);
    }

  this->ActiveCamera->SetClippingRange( range );
}

// Alternative version of ResetCameraClippingRange(bounds[6]);
void vtkRenderer::ResetCameraClippingRange(float xmin, float xmax, 
                                           float ymin, float ymax, 
                                           float zmin, float zmax)
{
  float bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->ResetCameraClippingRange(bounds);
}

// Specify the rendering window in which to draw. This is automatically set
// when the renderer is created by MakeRenderer.  The user probably
// shouldn't ever need to call this method.
// no reference counting!
void vtkRenderer::SetRenderWindow(vtkRenderWindow *renwin)
{
  vtkProp *aProp;
  
  if (renwin != this->RenderWindow)
    {
    // This renderer is be dis-associated with its previous render window.
    // this information needs to be passed to the renderer's actors and
    // volumes so they can release and render window specific (or graphics
    // context specific) information (such as display lists and texture ids)
    this->Props->InitTraversal();
    for ( aProp = this->Props->GetNextProp();
          aProp != NULL;
          aProp = this->Props->GetNextProp() )
      {
      aProp->ReleaseGraphicsResources(this->RenderWindow);
      }
    // what about lights?
    // what about cullers?
    
    }
  this->VTKWindow = renwin;
  this->RenderWindow = renwin;
}

// Given a pixel location, return the Z value
float vtkRenderer::GetZ (int x, int y)
{
  float *zPtr;
  float z;

  zPtr = this->RenderWindow->GetZbufferData (x, y, x, y);
  if (zPtr)
    {
    z = *zPtr;
    delete [] zPtr;
    }
  else
    {
    z = 1.0;
    }
  return z;
}


// Convert view point coordinates to world coordinates.
void vtkRenderer::ViewToWorld()
{
  vtkMatrix4x4 *mat = vtkMatrix4x4::New();
  float result[4];

  // get the perspective transformation from the active camera 
  mat->DeepCopy(
        this->ActiveCamera->GetCompositePerspectiveTransformMatrix(1,0,1));
  
  // use the inverse matrix 
  mat->Invert();
 
  // Transform point to world coordinates 
  result[0] = this->ViewPoint[0];
  result[1] = this->ViewPoint[1];
  result[2] = this->ViewPoint[2];
  result[3] = 1.0;

  mat->MultiplyPoint(result,result);
  
  // Get the transformed vector & set WorldPoint 
  // while we are at it try to keep w at one
  if (result[3])
    {
    result[0] /= result[3];
    result[1] /= result[3];
    result[2] /= result[3];
    result[3] = 1;
    }
  
  this->SetWorldPoint(result);
  mat->Delete();
}

void vtkRenderer::ViewToWorld(float &x, float &y, float &z)
{
  vtkMatrix4x4 *mat = vtkMatrix4x4::New();
  float result[4];

  // get the perspective transformation from the active camera 
  mat->DeepCopy(
    this->ActiveCamera->GetCompositePerspectiveTransformMatrix(1,0,1));
  
  // use the inverse matrix 
  mat->Invert();
 
  // Transform point to world coordinates 
  result[0] = x;
  result[1] = y;
  result[2] = z;
  result[3] = 1.0;

  mat->MultiplyPoint(result,result);
  
  // Get the transformed vector & set WorldPoint 
  // while we are at it try to keep w at one
  if (result[3])
    {
    x = result[0] / result[3];
    y = result[1] / result[3];
    z = result[2] / result[3];
    }
  mat->Delete();
}

// Convert world point coordinates to view coordinates.
void vtkRenderer::WorldToView()
{
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  float     view[4];
  float     *world;

  // get the perspective transformation from the active camera 
  matrix->DeepCopy(
           this->ActiveCamera->GetCompositePerspectiveTransformMatrix(1,0,1));

  world = this->WorldPoint;
  view[0] = world[0]*matrix->Element[0][0] + world[1]*matrix->Element[0][1] +
    world[2]*matrix->Element[0][2] + world[3]*matrix->Element[0][3];
  view[1] = world[0]*matrix->Element[1][0] + world[1]*matrix->Element[1][1] +
    world[2]*matrix->Element[1][2] + world[3]*matrix->Element[1][3];
  view[2] = world[0]*matrix->Element[2][0] + world[1]*matrix->Element[2][1] +
    world[2]*matrix->Element[2][2] + world[3]*matrix->Element[2][3];
  view[3] = world[0]*matrix->Element[3][0] + world[1]*matrix->Element[3][1] +
    world[2]*matrix->Element[3][2] + world[3]*matrix->Element[3][3];

  if (view[3] != 0.0)
    {
    this->SetViewPoint(view[0]/view[3],
                       view[1]/view[3],
                       view[2]/view[3]);
    }
  matrix->Delete();
}

// Convert world point coordinates to view coordinates.
void vtkRenderer::WorldToView(float &x, float &y, float &z)
{
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  float     view[4];

  // get the perspective transformation from the active camera
  matrix->DeepCopy(
    this->ActiveCamera->GetCompositePerspectiveTransformMatrix(1,0,1));

  view[0] = x*matrix->Element[0][0] + y*matrix->Element[0][1] +
    z*matrix->Element[0][2] + matrix->Element[0][3];
  view[1] = x*matrix->Element[1][0] + y*matrix->Element[1][1] +
    z*matrix->Element[1][2] + matrix->Element[1][3];
  view[2] = x*matrix->Element[2][0] + y*matrix->Element[2][1] +
    z*matrix->Element[2][2] + matrix->Element[2][3];
  view[3] = x*matrix->Element[3][0] + y*matrix->Element[3][1] +
    z*matrix->Element[3][2] + matrix->Element[3][3];

  if (view[3] != 0.0)
    {
    x = view[0]/view[3];
    y = view[1]/view[3];
    z = view[2]/view[3];
    }
  matrix->Delete();
}

void vtkRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkViewport::PrintSelf(os,indent);

  os << indent << "Ambient: (" << this->Ambient[0] << ", " 
     << this->Ambient[1] << ", " << this->Ambient[2] << ")\n";

  os << indent << "Backing Store: " << (this->BackingStore ? "On\n":"Off\n");
  os << indent << "Display Point: ("  << this->DisplayPoint[0] << ", " 
    << this->DisplayPoint[1] << ", " << this->DisplayPoint[2] << ")\n";
  os << indent << "Lights:\n";
  this->Lights->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Light Follow Camera: "
     << (this->LightFollowCamera ? "On\n" : "Off\n");

  os << indent << "View Point: (" << this->ViewPoint[0] << ", " 
    << this->ViewPoint[1] << ", " << this->ViewPoint[2] << ")\n";

  os << indent << "Two Sided Lighting: " 
     << (this->TwoSidedLighting ? "On\n" : "Off\n");

  os << indent << "Layer = " << this->Layer << "\n";
  os << indent << "Interactive = " << (this->Interactive ? "On" : "Off") 
     << "\n";

  os << indent << "Allocated Render Time: " << this->AllocatedRenderTime
     << "\n";

  os << indent << "Last Time To Render (Seconds): " 
     << this->LastRenderTimeInSeconds << endl;
  os << indent << "TimeFactor: " << this->TimeFactor << endl;

  // I don't want to print this since it is used just internally
  // os << indent << this->NumberOfPropsRenderedAsGeometry;

}

int vtkRenderer::VisibleActorCount()
{
  vtkProp *aProp;
  int count = 0;

  // loop through Props
  for (this->Props->InitTraversal();
       (aProp = this->Props->GetNextProp()); )
    {
    if (aProp->GetVisibility())
      {
      count++;
      }
    }
  return count;
}

int vtkRenderer::VisibleVolumeCount()
{
  int count = 0;
  vtkProp *aProp;

  // loop through volumes
  for (this->Props->InitTraversal(); 
        (aProp = this->Props->GetNextProp()); )
    {
    if (aProp->GetVisibility())
      {
      count++;
      }
    }
  return count;
}

unsigned long int vtkRenderer::GetMTime()
{
  unsigned long mTime=this-> vtkViewport::GetMTime();
  unsigned long time;

  if ( this->ActiveCamera != NULL )
    {
    time = this->ActiveCamera ->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  if ( this->CreatedLight != NULL )
    {
    time = this->CreatedLight ->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}


vtkAssemblyPath* vtkRenderer::PickProp(float selectionX, float selectionY)
{
  // initialize picking information
  this->CurrentPickId = 1; // start at 1, so 0 can be a no pick
  this->PickX = selectionX;
  this->PickY = selectionY;
  int numberPickFrom;
  vtkPropCollection *props;

  // Initialize the pick (we're picking a path, the path 
  // includes info about nodes) 
  if (this->PickFromProps)
    {
    props = this->PickFromProps;
    }
  else
    {
    props = this->Props;
    }
  // number determined from number of rendering passes plus reserved "0" slot
  numberPickFrom = 2*props->GetNumberOfPaths()*3 + 1;
  
  this->IsPicking = 1; // turn on picking
  this->StartPick(numberPickFrom);
  this->PathArray = new vtkAssemblyPath *[numberPickFrom];
  this->PathArrayCount = 0;

  // Actually perform the pick
  this->PickRender(props);  // do the pick render
  this->IsPicking = 0; // turn off picking
  this->DonePick();
  vtkDebugMacro(<< "z value for pick " << this->GetPickedZ() << "\n");
  vtkDebugMacro(<< "pick time " <<  this->LastRenderTimeInSeconds << "\n");

  // Get the pick id of the object that was picked
  if ( this->PickedProp != NULL )
    {
    this->PickedProp->UnRegister(this);
    this->PickedProp = NULL;
    }
  unsigned int pickedId = this->GetPickedId();
  if ( pickedId != 0 )
    {
    pickedId--; // pick ids start at 1, so move back one

    // wrap around, as there are twice as many pickid's as PropArrayCount,
    // because each Prop has both RenderOpaqueGeometry and 
    // RenderTranslucentGeometry called on it
    pickedId = pickedId % this->PathArrayCount;
    this->PickedProp = this->PathArray[pickedId];
    this->PickedProp->Register(this);
    }

  // Clean up stuff from picking after we use it
  delete [] this->PathArray;
  this->PathArray = NULL;

  // Return the pick!
  return this->PickedProp; //returns an assembly path
}

// Do a render in pick mode.  This is normally done with rendering turned off.
// Before each Prop is rendered the pick id is incremented
void vtkRenderer::PickRender(vtkPropCollection *props)
{
  vtkProp  *aProp;
  vtkAssemblyPath *path;

  this->InvokeEvent(vtkCommand::StartEvent,NULL);
  if( props->GetNumberOfItems() <= 0)
    {
    return;
    }
  
  // Create a place to store all props that remain after culling
  vtkPropCollection* pickFrom = vtkPropCollection::New();

  // Extract all the prop3D's out of the props collection.
  // This collection will be further culled by using a bounding box
  // pick later (vtkPicker). Things that are not vtkProp3D will get 
  // put into the Paths list directly.
  for (  props->InitTraversal(); (aProp = props->GetNextProp()); )
    {
    if ( aProp->GetPickable() && aProp->GetVisibility() )
      {
      if ( aProp->IsA("vtkProp3D") )
        {
        pickFrom->AddItem(aProp);
        }
      else //must be some other type of prop (e.g., vtkActor2D)
        {
        for ( aProp->InitPathTraversal(); (path=aProp->GetNextPath()); )
          {
          this->PathArray[this->PathArrayCount++] = path;
          }
        }
      }//pickable & visible
    }//for all props

  // For a first pass at the pick process, just use a vtkPicker to
  // intersect with bounding boxes of the objects.  This should greatly
  // reduce the number of polygons that the hardware has to pick from, and
  // speeds things up substantially.
  //
  // Create a picker to do the culling process
  vtkPicker* cullPicker = vtkPicker::New();
  // Add each of the Actors from the pickFrom list into the picker
  for ( pickFrom->InitTraversal(); (aProp = pickFrom->GetNextProp()); )
    {
    cullPicker->AddPickList(aProp);
    }

  // make sure this selects from the pickers list and not the renderers list
  cullPicker->PickFromListOn();

  // do the pick
  cullPicker->Pick(this->PickX, this->PickY, 0, this);
  vtkProp3DCollection* cullPicked = cullPicker->GetProp3Ds();

  // Put all the ones that were picked by the cull process
  // into the PathArray to be picked from
  for (cullPicked->InitTraversal(); (aProp = cullPicked->GetNextProp3D());)
    {
    if ( aProp != NULL )
      {
      for ( aProp->InitPathTraversal(); (path=aProp->GetNextPath()); )
        {
        this->PathArray[this->PathArrayCount++] = path;
        }
      }
    }

  // Clean picking support objects up
  pickFrom->Delete();
  cullPicker->Delete();
  
  if ( this->PathArrayCount == 0 )
    {
    vtkDebugMacro( << "There are no visible props!" );
    this->NumberOfPropsToRayCast = 0;
    this->NumberOfPropsToRenderIntoImage = 0;
    return;
    }

  // do the render library specific pick render
  this->DevicePickRender();
}

void vtkRenderer::PickGeometry()
{  
  int        i;
  
  this->NumberOfPropsRenderedAsGeometry = 0;

  if ( this->PathArrayCount == 0 ) 
    {
    return ;
    }

  // We can render everything because if it was
  // not visible it would not have been put in the
  // list in the first place, and if it was allocated
  // no time (culled) it would have been removed from
  // the list
  
  // loop through props and give them a change to 
  // render themselves as opaque geometry
  vtkProp *prop;
  vtkMatrix4x4 *matrix;
  for ( i = 0; i < this->PathArrayCount; i++ )
    {
    this->UpdatePickId();
    prop = this->PathArray[i]->GetLastNode()->GetProp();
    matrix = this->PathArray[i]->GetLastNode()->GetMatrix();
    prop->PokeMatrix(matrix);
    this->NumberOfPropsRenderedAsGeometry += prop->RenderOpaqueGeometry(this);
    prop->PokeMatrix(NULL);
    }
 
  // loop through props and give them a chance to 
  // render themselves as translucent geometry
  for ( i = 0; i < this->PathArrayCount; i++ )
    {
    this->UpdatePickId();
    prop = this->PathArray[i]->GetLastNode()->GetProp();
    matrix = this->PathArray[i]->GetLastNode()->GetMatrix();
    prop->PokeMatrix(matrix);
    this->NumberOfPropsRenderedAsGeometry += 
      prop->RenderTranslucentGeometry(this);
    prop->PokeMatrix(NULL);
    }
  
  vtkDebugMacro( << "Pick Rendered " << 
                    this->NumberOfPropsRenderedAsGeometry << " actors" );

}


int  vtkRenderer::Transparent()
{
  int  numLayers = this->RenderWindow->GetNumLayers();

  // If our layer is the last layer, then we are not transparent, else we are.
  return (this->Layer == numLayers-1 ? 0 : 1);
}


