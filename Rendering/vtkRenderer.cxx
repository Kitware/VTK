/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderer.h"

#include "vtkAreaPicker.h"
#include "vtkAssemblyNode.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCullerCollection.h"
#include "vtkCuller.h"
#include "vtkFrustumCoverageCuller.h"
#include "vtkGraphicsFactory.h"
#include "vtkHardwareSelector.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkOutputWindow.h"
#include "vtkPainterPolyDataMapper.h"
#include "vtkPicker.h"
#include "vtkProp3DCollection.h"
#include "vtkPropCollection.h"
#include "vtkRendererDelegate.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkVolume.h"
#include "vtkRenderPass.h"
#include "vtkRenderState.h"
#include "vtkTexture.h"

vtkCxxSetObjectMacro(vtkRenderer, Delegate, vtkRendererDelegate);
vtkCxxSetObjectMacro(vtkRenderer, Pass, vtkRenderPass);
vtkCxxSetObjectMacro(vtkRenderer, BackgroundTexture, vtkTexture);

#if !defined(VTK_LEGACY_REMOVE)
#include "vtkIdentColoredPainter.h"
vtkCxxSetObjectMacro(vtkRenderer, IdentPainter, vtkIdentColoredPainter);
#endif

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkRenderer);
//----------------------------------------------------------------------------

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
  this->AutomaticLightCreation = 1;

  this->TwoSidedLighting        = 1;
  this->BackingStore            = 0;
  this->BackingImage            = NULL;
  this->BackingStoreSize[0]     = -1;
  this->BackingStoreSize[1]     = -1;
  this->LastRenderTimeInSeconds = -1.0;

  this->RenderWindow = NULL;
  this->Lights  =  vtkLightCollection::New();
  this->Actors  =  vtkActorCollection::New();
  this->Volumes = vtkVolumeCollection::New();

  this->LightFollowCamera = 1;

  this->NumberOfPropsRendered = 0;

  this->PropArray                = NULL;
  this->PropArrayCount = 0;

  this->PathArray = NULL;
  this->PathArrayCount = 0;

  this->Layer                    = 0;
  this->PreserveDepthBuffer = 0;

  this->ComputedVisiblePropBounds[0] = VTK_DOUBLE_MAX;
  this->ComputedVisiblePropBounds[1] = -VTK_DOUBLE_MAX;
  this->ComputedVisiblePropBounds[2] = VTK_DOUBLE_MAX;
  this->ComputedVisiblePropBounds[3] = -VTK_DOUBLE_MAX;
  this->ComputedVisiblePropBounds[4] = VTK_DOUBLE_MAX;
  this->ComputedVisiblePropBounds[5] = -VTK_DOUBLE_MAX;

  this->Interactive              = 1;
  this->Cullers = vtkCullerCollection::New();
  vtkFrustumCoverageCuller *cull = vtkFrustumCoverageCuller::New();
  this->Cullers->AddItem(cull);
  cull->Delete();

  // a value of 0 indicates it is uninitialized
  this->NearClippingPlaneTolerance = 0;

  this->Erase = 1;
  this->Draw = 1;

#if !defined(VTK_LEGACY_REMOVE)
  this->SelectMode = vtkRenderer::NOT_SELECTING;
  this->SelectConst = 1;
  this->PropsSelectedFrom = NULL;
  this->PropsSelectedFromCount = 0;
  this->IdentPainter = NULL;
#endif

  this->UseDepthPeeling=0;
  this->OcclusionRatio=0.0;
  this->MaximumNumberOfPeels=4;
  this->LastRenderingUsedDepthPeeling=0;

  this->Selector = 0;
  this->Delegate=0;
  this->Pass=0;

  this->TexturedBackground = false;
  this->BackgroundTexture = NULL;
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

#if !defined(VTK_LEGACY_REMOVE)
  if ( this->PropsSelectedFrom)
    {
    delete [] this->PropsSelectedFrom;
    this->PropsSelectedFrom = NULL;
    }
  this->PropsSelectedFromCount = 0;
  if (this->IdentPainter)
    {
    this->IdentPainter->Delete();
    this->IdentPainter = NULL;
    }
#endif

  if(this->Delegate!=0)
    {
    this->Delegate->UnRegister(this);
    }
  if(this->Pass!=0)
    {
    this->Pass->UnRegister(this);
    }

  if(this->BackgroundTexture != NULL)
    {
    this->BackgroundTexture->Delete();
    }
}

// return the correct type of Renderer
vtkRenderer *vtkRenderer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkRenderer");
  return static_cast<vtkRenderer *>(ret);
}

// Concrete render method.
void vtkRenderer::Render(void)
{
  if(this->Delegate!=0 && this->Delegate->GetUsed())
    {
      this->Delegate->Render(this);
      return;
    }

  double   t1, t2;
  int      i;
  vtkProp  *aProp;
  int *size;

  // If Draw is not on, ignore the render.
  if (!this->Draw)
    {
    vtkDebugMacro("Ignoring render because Draw is off.");
    return;
    }

  t1 = vtkTimerLog::GetUniversalTime();

  this->InvokeEvent(vtkCommand::StartEvent,NULL);

  size = this->RenderWindow->GetSize();

  // if backing store is on and we have a stored image
  if (this->BackingStore && this->BackingImage &&
      this->MTime < this->RenderTime &&
      this->ActiveCamera->GetMTime() < this->RenderTime &&
      this->RenderWindow->GetMTime() < this->RenderTime &&
      this->BackingStoreSize[0] == size[0] &&
      this->BackingStoreSize[1] == size[1])
    {
    int mods = 0;
    vtkLight *light;

    // now we just need to check the lights and actors
    vtkCollectionSimpleIterator sit;
    for(this->Lights->InitTraversal(sit);
        (light = this->Lights->GetNextLight(sit)); )
      {
      if (light->GetSwitch() &&
          light->GetMTime() > this->RenderTime)
        {
        mods = 1;
        goto completed_mod_check;
        }
      }
    vtkCollectionSimpleIterator pit;
    for (this->Props->InitTraversal(pit);
         (aProp = this->Props->GetNextProp(pit)); )
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
      rx1 = static_cast<int>(this->Viewport[0]*
                             (this->RenderWindow->GetSize()[0] - 1));
      ry1 = static_cast<int>(this->Viewport[1]*
                             (this->RenderWindow->GetSize()[1] - 1));
      rx2 = static_cast<int>(this->Viewport[2]*
                             (this->RenderWindow->GetSize()[0] - 1));
      ry2 = static_cast<int>(this->Viewport[3]*
                             (this->RenderWindow->GetSize()[1] - 1));
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
    this->PropArray = new vtkProp *[this->Props->GetNumberOfItems()];
    }
  else
    {
    this->PropArray = NULL;
    }

  this->PropArrayCount = 0;
  vtkCollectionSimpleIterator pit;
  for ( this->Props->InitTraversal(pit);
        (aProp = this->Props->GetNextProp(pit)); )
    {
    if ( aProp->GetVisibility() )
      {
      this->PropArray[this->PropArrayCount++] = aProp;
      }
    }

  if ( this->PropArrayCount == 0 )
    {
    vtkDebugMacro( << "There are no visible props!" );
    }
  else
    {
    // Call all the culling methods to set allocated time
    // for each prop and re-order the prop list if desired

    this->AllocateTime();
    }

  // do the render library specific stuff
  if(this->Pass!=0)
    {
    vtkRenderState s(this);
    s.SetPropArrayAndCount(this->PropArray,this->PropArrayCount);
    s.SetFrameBuffer(0);
    this->Pass->Render(&s);
    }
  else
    {
    this->DeviceRender();
    }

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
    this->PropArray                = NULL;
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
    rx1 = static_cast<int>(this->Viewport[0]*(size[0] - 1));
    ry1 = static_cast<int>(this->Viewport[1]*(size[1] - 1));
    rx2 = static_cast<int>(this->Viewport[2]*(size[0] - 1));
    ry2 = static_cast<int>(this->Viewport[3]*(size[1] - 1));
    this->BackingImage = this->RenderWindow->GetPixelData(rx1,ry1,rx2,ry2,0);
    this->BackingStoreSize[0] = size[0];
    this->BackingStoreSize[1] = size[1];
    }


  // If we aborted, do not record the last render time.
  // Lets play around with determining the acuracy of the
  // EstimatedRenderTimes.  We can try to adjust for bad
  // estimates with the TimeFactor.
  if ( ! this->RenderWindow->GetAbortRender() )
    {
    // Measure the actual RenderTime
    t2 = vtkTimerLog::GetUniversalTime();
    this->LastRenderTimeInSeconds = static_cast<double>(t2 - t1);

    if (this->LastRenderTimeInSeconds == 0.0)
      {
      this->LastRenderTimeInSeconds = 0.0001;
      }
    this->TimeFactor = this->AllocatedRenderTime/this->LastRenderTimeInSeconds;
    }
  this->InvokeEvent(vtkCommand::EndEvent,NULL);
}

// ----------------------------------------------------------------------------
// Description:
// Render translucent polygonal geometry. Default implementation just call
// UpdateTranslucentPolygonalGeometry().
// Subclasses of vtkRenderer that can deal with depth peeling must
// override this method.
void vtkRenderer::DeviceRenderTranslucentPolygonalGeometry()
{
  // Have to be set before a call to UpdateTranslucentPolygonalGeometry()
  // because UpdateTranslucentPolygonalGeometry() will eventually call
  // vtkOpenGLActor::Render() that uses this flag.
  this->LastRenderingUsedDepthPeeling=0;

  this->UpdateTranslucentPolygonalGeometry();
}

// ----------------------------------------------------------------------------
double vtkRenderer::GetAllocatedRenderTime()
{
  return this->AllocatedRenderTime;
}

double vtkRenderer::GetTimeFactor()
{
  return this->TimeFactor;
}

// Ask active camera to load its view matrix.
int vtkRenderer::UpdateCamera ()
{
  if (!this->ActiveCamera)
    {
    vtkDebugMacro(<< "No cameras are on, creating one.");
    // the get method will automagically create a camera
    // and reset it since one hasn't been specified yet.
    // If is very unlikely that this can occur - if this
    // renderer is part of a vtkRenderWindow, the camera
    // will already have been created as part of the
    // DoStereoRender() method.
    this->GetActiveCameraAndResetIfCreated();
    }

  // update the viewing transformation
  this->ActiveCamera->Render(this);

  return 1;
}

int vtkRenderer::UpdateLightsGeometryToFollowCamera()
{
  vtkCamera *camera;
  vtkLight *light;
  vtkMatrix4x4 *lightMatrix;

  // only update the light's geometry if this Renderer is tracking
  // this lights.  That allows one renderer to view the lights that
  // another renderer is setting up.
  camera = this->GetActiveCameraAndResetIfCreated();
  lightMatrix = camera->GetCameraLightTransformMatrix();

  vtkCollectionSimpleIterator sit;
  for(this->Lights->InitTraversal(sit);
      (light = this->Lights->GetNextLight(sit)); )
    {
    if (light->LightTypeIsSceneLight())
      {
      // Do nothing. Don't reset the transform matrix because applications
      // may have set a custom matrix. Only reset the transform matrix in
      // vtkLight::SetLightTypeToSceneLight()
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
  return 1;
}

int vtkRenderer::UpdateLightGeometry()
{
  if (this->LightFollowCamera)
    {
    // only update the light's geometry if this Renderer is tracking
    // this lights.  That allows one renderer to view the lights that
    // another renderer is setting up.

    return this->UpdateLightsGeometryToFollowCamera();
    }

  return 1;
}

// Do all outer culling to set allocated time for each prop.
// Possibly re-order the actor list.
void vtkRenderer::AllocateTime()
{
  int          initialized = 0;
  double        renderTime;
  double        totalTime;
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

  // It is very likely that the culler framework will call our
  // GetActiveCamera (say, to get the view frustrum planes for example).
  // This does not reset the camera anymore. If no camera has been
  // created though, we want it not only to be created but also reset
  // so that it behaves nicely for people who never bother with the camera
  // (i.e. neither call GetActiveCamera or ResetCamera). Of course,
  // it is very likely that the camera has already been created
  // (guaranteed if this renderer is being rendered as part of a
  // vtkRenderWindow).

  if ( this->Cullers->GetNumberOfItems())
    {
    this->GetActiveCameraAndResetIfCreated();
    }

  vtkCollectionSimpleIterator sit;
  for (this->Cullers->InitTraversal(sit);
       (aCuller=this->Cullers->GetNextCuller(sit));)
    {
    totalTime =
      aCuller->Cull(this,this->PropArray, this->PropArrayCount,initialized );
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

  this->NumberOfPropsRendered = 0;

  if ( this->PropArrayCount == 0 )
    {
    return 0;
    }

#if !defined(VTK_LEGACY_REMOVE)
  if (this->SelectMode != vtkRenderer::NOT_SELECTING)
    {
    //we are doing a visible polygon selection instead of a normal render
    int ret = this->UpdateGeometryForSelection();

    this->RenderTime.Modified();

    vtkDebugMacro( << "Rendered " <<
                   this->NumberOfPropsRendered << " actors" );

    return ret;
    }
#endif

  if (this->Selector)
    {
    // When selector is present, we are performing a selection,
    // so do the selection rendering pass instead of the normal passes.
    // Delegate the rendering of the props to the selector itself.
    this->NumberOfPropsRendered = this->Selector->Render(this,
      this->PropArray, this->PropArrayCount);
    this->RenderTime.Modified();
    vtkDebugMacro("Rendered " << this->NumberOfPropsRendered << " actors" );
    return this->NumberOfPropsRendered;
    }

  // We can render everything because if it was
  // not visible it would not have been put in the
  // list in the first place, and if it was allocated
  // no time (culled) it would have been removed from
  // the list

  // loop through props and give them a chance to
  // render themselves as opaque geometry
  for ( i = 0; i < this->PropArrayCount; i++ )
    {
    this->NumberOfPropsRendered +=
      this->PropArray[i]->RenderOpaqueGeometry(this);
    }

  // do the render library specific stuff about translucent polygonal geometry.
  // As it can be expensive, do a quick check if we can skip this step
  int hasTranslucentPolygonalGeometry=0;
  for ( i = 0; !hasTranslucentPolygonalGeometry && i < this->PropArrayCount;
        i++ )
    {
    hasTranslucentPolygonalGeometry=
      this->PropArray[i]->HasTranslucentPolygonalGeometry();
    }
  if(hasTranslucentPolygonalGeometry)
    {
    this->DeviceRenderTranslucentPolygonalGeometry();
    }

  // loop through props and give them a chance to
  // render themselves as volumetric geometry.
  for ( i = 0; i < this->PropArrayCount; i++ )
    {
    this->NumberOfPropsRendered +=
      this->PropArray[i]->RenderVolumetricGeometry(this);
    }

  // loop through props and give them a chance to
  // render themselves as an overlay (or underlay)
  for ( i = 0; i < this->PropArrayCount; i++ )
    {
    this->NumberOfPropsRendered +=
      this->PropArray[i]->RenderOverlay(this);
    }

  this->RenderTime.Modified();

  vtkDebugMacro( << "Rendered " <<
                    this->NumberOfPropsRendered << " actors" );

  return  this->NumberOfPropsRendered;
}

// ----------------------------------------------------------------------------
// Description:
// Ask all props to update and draw any translucent polygonal
// geometry. This includes both vtkActors and vtkVolumes
// Return the number of rendered props.
// It is called once with alpha blending technique. It is called multiple
// times with depth peeling technique.
int vtkRenderer::UpdateTranslucentPolygonalGeometry()
{
  int result=0;
  // loop through props and give them a chance to
  // render themselves as translucent geometry
  for (int i = 0; i < this->PropArrayCount; i++ )
    {
    int rendered=this->PropArray[i]->RenderTranslucentPolygonalGeometry(this);
    this->NumberOfPropsRendered += rendered;
    result+=rendered;
    }
  return result;
}

// ----------------------------------------------------------------------------
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
  this->InvokeEvent(vtkCommand::ActiveCameraEvent, cam);
}

//----------------------------------------------------------------------------
vtkCamera* vtkRenderer::MakeCamera()
{
  vtkCamera *cam = vtkCamera::New();
  this->InvokeEvent(vtkCommand::CreateCameraEvent, cam);
  return cam;
}

//----------------------------------------------------------------------------
vtkCamera *vtkRenderer::GetActiveCamera()
{
  if ( this->ActiveCamera == NULL )
    {
    vtkCamera *cam = this->MakeCamera();
    this->SetActiveCamera(cam);
    cam->Delete();
    // The following line has been commented out as it has a lot of
    // side effects (like computing the bounds of all props, which will
    // eventually call UpdateInformation() on data objects, etc).
    // Instead, the rendering code has been updated to internally use
    // GetActiveCameraAndResetIfCreated which will reset the camera
    // if it gets created
    // this->ResetCamera();
    }

  return this->ActiveCamera;
}

//----------------------------------------------------------------------------
vtkCamera *vtkRenderer::GetActiveCameraAndResetIfCreated()
{
  if (this->ActiveCamera == NULL)
    {
    this->GetActiveCamera();
    this->ResetCamera();
    }
  return this->ActiveCamera;
}

//----------------------------------------------------------------------------
void vtkRenderer::AddActor(vtkProp* p)
{
  this->AddViewProp(p);
}

//----------------------------------------------------------------------------
void vtkRenderer::AddVolume(vtkProp* p)
{
  this->AddViewProp(p);
}

//----------------------------------------------------------------------------
void vtkRenderer::RemoveActor(vtkProp* p)
{
  this->Actors->RemoveItem(p);
  this->RemoveViewProp(p);
}

//----------------------------------------------------------------------------
void vtkRenderer::RemoveVolume(vtkProp* p)
{
  this->Volumes->RemoveItem(p);
  this->RemoveViewProp(p);
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

  vtkCollectionSimpleIterator pit;
  for (this->Props->InitTraversal(pit);
       (aProp = this->Props->GetNextProp(pit)); )
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

  vtkCollectionSimpleIterator pit;
  for (this->Props->InitTraversal(pit);
       (aProp = this->Props->GetNextProp(pit)); )
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

// Remove all lights from the list of lights.
void vtkRenderer::RemoveAllLights()
{
  this->Lights->RemoveAllItems();
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

// ----------------------------------------------------------------------------
void vtkRenderer::SetLightCollection(vtkLightCollection *lights)
{
  assert("pre lights_exist" && lights!=0);

  this->Lights->Delete(); // this->Lights is always not NULL.
  this->Lights=lights;
  this->Lights->Register(this);
  this->Modified();

  assert("post: lights_set" && lights==this->GetLights());
}

// ----------------------------------------------------------------------------
vtkLight *vtkRenderer::MakeLight()
{
  return vtkLight::New();
}

void vtkRenderer::CreateLight(void)
{
  if ( !this->AutomaticLightCreation )
    {
    return;
    }

  if (this->CreatedLight)
    {
    this->CreatedLight->UnRegister(this);
    this->CreatedLight = NULL;
    }

  // I do not see why UnRegister is used on CreatedLight, but lets be
  // consistent.
  vtkLight *l = this->MakeLight();
  this->CreatedLight = l;
  this->CreatedLight->Register(this);
  this->AddLight(this->CreatedLight);
  l->Delete();

  this->CreatedLight->SetLightTypeToHeadlight();

  // set these values just to have a good default should LightFollowCamera
  // be turned off.
  this->CreatedLight->SetPosition(this->GetActiveCamera()->GetPosition());
  this->CreatedLight->SetFocalPoint(this->GetActiveCamera()->GetFocalPoint());
}

// Compute the bounds of the visible props
void vtkRenderer::ComputeVisiblePropBounds( double allBounds[6] )
{
  vtkProp    *prop;
  double      *bounds;
  int        nothingVisible=1;

  this->InvokeEvent(vtkCommand::ComputeVisiblePropBoundsEvent, this);

  allBounds[0] = allBounds[2] = allBounds[4] = VTK_DOUBLE_MAX;
  allBounds[1] = allBounds[3] = allBounds[5] = -VTK_DOUBLE_MAX;

  // loop through all props
  vtkCollectionSimpleIterator pit;
  for (this->Props->InitTraversal(pit);
       (prop = this->Props->GetNextProp(pit)); )
    {
    // if it's invisible, or if its bounds should be ignored,
    // or has no geometry, we can skip the rest
    if ( prop->GetVisibility() && prop->GetUseBounds())
      {
      bounds = prop->GetBounds();
      // make sure we haven't got bogus bounds
      if ( bounds != NULL && vtkMath::AreBoundsInitialized(bounds))
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
    vtkMath::UninitializeBounds(allBounds);
    vtkDebugMacro(<< "Can't compute bounds, no 3D props are visible");
    return;
    }
}

double *vtkRenderer::ComputeVisiblePropBounds()
  {
  this->ComputeVisiblePropBounds(this->ComputedVisiblePropBounds);
  return this->ComputedVisiblePropBounds;
  }

// Automatically set up the camera based on the visible actors.
// The camera will reposition itself to view the center point of the actors,
// and move along its initial view plane normal (i.e., vector defined from
// camera position to focal point) so that all of the actors can be seen.
void vtkRenderer::ResetCamera()
{
  double      allBounds[6];

  this->ComputeVisiblePropBounds( allBounds );

  if (!vtkMath::AreBoundsInitialized(allBounds))
    {
    vtkDebugMacro( << "Cannot reset camera!" );
    }
  else
    {
    this->ResetCamera(allBounds);
    }

  // Here to let parallel/distributed compositing intercept
  // and do the right thing.
  this->InvokeEvent(vtkCommand::ResetCameraEvent,this);
}

// Automatically set the clipping range of the camera based on the
// visible actors
void vtkRenderer::ResetCameraClippingRange()
{
  double      allBounds[6];

  this->ComputeVisiblePropBounds( allBounds );

  if (!vtkMath::AreBoundsInitialized(allBounds))
    {
    vtkDebugMacro( << "Cannot reset camera clipping range!" );
    }
  else
    {
    this->ResetCameraClippingRange(allBounds);
    }

  // Here to let parallel/distributed compositing intercept
  // and do the right thing.
  this->InvokeEvent(vtkCommand::ResetCameraClippingRangeEvent,this);
}


// Automatically set up the camera based on a specified bounding box
// (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
// that its focal point is the center of the bounding box, and adjust its
// distance and position to preserve its initial view plane normal
// (i.e., vector defined from camera position to focal point). Note: if
// the view plane is parallel to the view up axis, the view up axis will
// be reset to one of the three coordinate axes.
void vtkRenderer::ResetCamera(double bounds[6])
{
  double center[3];
  double distance;
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

  this->ExpandBounds(bounds, this->ActiveCamera->GetModelTransformMatrix());

  center[0] = (bounds[0] + bounds[1])/2.0;
  center[1] = (bounds[2] + bounds[3])/2.0;
  center[2] = (bounds[4] + bounds[5])/2.0;

  double w1 = bounds[1] - bounds[0];
  double w2 = bounds[3] - bounds[2];
  double w3 = bounds[5] - bounds[4];
  w1 *= w1;
  w2 *= w2;
  w3 *= w3;
  double radius = w1 + w2 + w3;

  // If we have just a single point, pick a radius of 1.0
  radius = (radius==0)?(1.0):(radius);

  // compute the radius of the enclosing sphere
  radius = sqrt(radius)*0.5;

  // default so that the bounding sphere fits within the view fustrum

  // compute the distance from the intersection of the view frustum with the
  // bounding sphere. Basically in 2D draw a circle representing the bounding
  // sphere in 2D then draw a horizontal line going out from the center of
  // the circle. That is the camera view. Then draw a line from the camera
  // position to the point where it intersects the circle. (it will be tangent
  // to the circle at this point, this is important, only go to the tangent
  // point, do not draw all the way to the view plane). Then draw the radius
  // from the tangent point to the center of the circle. You will note that
  // this forms a right triangle with one side being the radius, another being
  // the target distance for the camera, then just find the target dist using
  // a sin.
  double angle=vtkMath::RadiansFromDegrees(this->ActiveCamera->GetViewAngle());
  double parallelScale=radius;

  this->ComputeAspect();
  double aspect[2];
  this->GetAspect(aspect);

  if(aspect[0]>=1.0) // horizontal window, deal with vertical angle|scale
    {
    if(this->ActiveCamera->GetUseHorizontalViewAngle())
      {
      angle=2.0*atan(tan(angle*0.5)/aspect[0]);
      }
    }
  else // vertical window, deal with horizontal angle|scale
    {
    if(!this->ActiveCamera->GetUseHorizontalViewAngle())
      {
      angle=2.0*atan(tan(angle*0.5)*aspect[0]);
      }

    parallelScale=parallelScale/aspect[0];
    }

  distance =radius/sin(angle*0.5);

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
  this->ActiveCamera->SetParallelScale(parallelScale);
}

// Alternative version of ResetCamera(bounds[6]);
void vtkRenderer::ResetCamera(double xmin, double xmax,
                              double ymin, double ymax,
                              double zmin, double zmax)
{
  double bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->ResetCamera(bounds);
}

// Reset the camera clipping range to include this entire bounding box
void vtkRenderer::ResetCameraClippingRange( double bounds[6] )
{
  double  vn[3], position[3], a, b, c, d;
  double  range[2], dist;
  int     i, j, k;

  // Don't reset the clipping range when we don't have any 3D visible props
  if (!vtkMath::AreBoundsInitialized(bounds))
    {
    return;
    }

  this->GetActiveCameraAndResetIfCreated();
  if ( this->ActiveCamera == NULL )
    {
    vtkErrorMacro(<< "Trying to reset clipping range of non-existant camera");
    return;
    }

  if(!this->ActiveCamera->GetUseOffAxisProjection())
    {
    this->ActiveCamera->GetViewPlaneNormal(vn);
    this->ActiveCamera->GetPosition(position);
    this->ExpandBounds(bounds, this->ActiveCamera->GetModelTransformMatrix());
    }
  else
    {
    this->ActiveCamera->GetEyePosition(position);
    this->ActiveCamera->GetEyePlaneNormal(vn);
    this->ExpandBounds(bounds, this->ActiveCamera->GetModelViewTransformMatrix());
    }

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

  // Do not let the range behind the camera throw off the calculation.
  if (range[0] < 0.0)
    {
    range[0] = 0.0;
    }

  // Give ourselves a little breathing room
  range[0] = 0.99*range[0] - (range[1] - range[0])*0.5;
  range[1] = 1.01*range[1] + (range[1] - range[0])*0.5;

  // Make sure near is not bigger than far
  range[0] = (range[0] >= range[1])?(0.01*range[1]):(range[0]);

  // Make sure near is at least some fraction of far - this prevents near
  // from being behind the camera or too close in front. How close is too
  // close depends on the resolution of the depth buffer
  if (!this->NearClippingPlaneTolerance)
    {
    this->NearClippingPlaneTolerance = 0.01;
    if (this->RenderWindow)
      {
      int ZBufferDepth = this->RenderWindow->GetDepthBufferSize();
      if ( ZBufferDepth > 16 )
        {
        this->NearClippingPlaneTolerance = 0.001;
        }
      }
    }

  // make sure the front clipping range is not too far from the far clippnig
  // range, this is to make sure that the zbuffer resolution is effectively
  // used
  if (range[0] < this->NearClippingPlaneTolerance*range[1])
    {
    range[0] = this->NearClippingPlaneTolerance*range[1];
    }

  this->ActiveCamera->SetClippingRange( range );
}

// Alternative version of ResetCameraClippingRange(bounds[6]);
void vtkRenderer::ResetCameraClippingRange(double xmin, double xmax,
                                           double ymin, double ymax,
                                           double zmin, double zmax)
{
  double bounds[6];

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
    vtkCollectionSimpleIterator pit;
    this->Props->InitTraversal(pit);
    for ( aProp = this->Props->GetNextProp(pit);
          aProp != NULL;
          aProp = this->Props->GetNextProp(pit) )
      {
      aProp->ReleaseGraphicsResources(this->RenderWindow);
      }
    // what about lights?
    // what about cullers?

    if(this->Pass!=0 && this->RenderWindow!=0)
      {
      this->Pass->ReleaseGraphicsResources(this->RenderWindow);
      }

    if(this->BackgroundTexture != 0 && this->RenderWindow!=0)
      {
      this->BackgroundTexture->ReleaseGraphicsResources(this->RenderWindow);
      }

    this->VTKWindow = renwin;
    this->RenderWindow = renwin;
    }
}

// Given a pixel location, return the Z value
double vtkRenderer::GetZ (int x, int y)
{
  float *zPtr;
  double z;

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
  double result[4];
  result[0] = this->ViewPoint[0];
  result[1] = this->ViewPoint[1];
  result[2] = this->ViewPoint[2];
  result[3] = 1.0;
  this->ViewToWorld(result[0],result[1],result[2]);
  this->SetWorldPoint(result);
}

void vtkRenderer::ViewToWorld(double &x, double &y, double &z)
{
  double mat[16];
  double result[4];

  // get the perspective transformation from the active camera
  vtkMatrix4x4 *matrix = this->ActiveCamera->
                GetCompositeProjectionTransformMatrix(
                  this->GetTiledAspectRatio(),0,1);

  // use the inverse matrix
  vtkMatrix4x4::Invert(*matrix->Element, mat);

  // Transform point to world coordinates
  result[0] = x;
  result[1] = y;
  result[2] = z;
  result[3] = 1.0;

  vtkMatrix4x4::MultiplyPoint(mat,result,result);

  // Get the transformed vector & set WorldPoint
  // while we are at it try to keep w at one
  if (result[3])
    {
    x = result[0] / result[3];
    y = result[1] / result[3];
    z = result[2] / result[3];
    }
}

// Convert world point coordinates to view coordinates.
void vtkRenderer::WorldToView()
{
  double result[3];
  result[0] = this->WorldPoint[0];
  result[1] = this->WorldPoint[1];
  result[2] = this->WorldPoint[2];
  this->WorldToView(result[0], result[1], result[2]);
  this->SetViewPoint(result[0], result[1], result[2]);
}

// Convert world point coordinates to view coordinates.
void vtkRenderer::WorldToView(double &x, double &y, double &z)
{
  double     mat[16];
  double     view[4];

  // get the perspective transformation from the active camera
  if (!this->ActiveCamera)
    {
    vtkErrorMacro("WorldToView: no active camera, cannot compute world to view, returning 0,0,0");
    x = y = z = 0.0;
    return;
    }
  vtkMatrix4x4::DeepCopy(mat, this->ActiveCamera->
                GetCompositeProjectionTransformMatrix(
                  this->GetTiledAspectRatio(),0,1));

  view[0] = x*mat[0] + y*mat[1] + z*mat[2] + mat[3];
  view[1] = x*mat[4] + y*mat[5] + z*mat[6] + mat[7];
  view[2] = x*mat[8] + y*mat[9] + z*mat[10] + mat[11];
  view[3] = x*mat[12] + y*mat[13] + z*mat[14] + mat[15];

  if (view[3] != 0.0)
    {
    x = view[0]/view[3];
    y = view[1]/view[3];
    z = view[2]/view[3];
    }
}

void vtkRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Near Clipping Plane Tolerance: "
     << this->NearClippingPlaneTolerance << "\n";

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

  os << indent << "Automatic Light Creation: "
     << (this->AutomaticLightCreation ? "On\n" : "Off\n");

  os << indent << "Layer = " << this->Layer << "\n";
  os << indent << "PreserveDepthBuffer: " <<
    (this->PreserveDepthBuffer? "On" : "Off") << "\n";
  os << indent << "Interactive = " << (this->Interactive ? "On" : "Off")
     << "\n";

  os << indent << "Allocated Render Time: " << this->AllocatedRenderTime
     << "\n";

  os << indent << "Last Time To Render (Seconds): "
     << this->LastRenderTimeInSeconds << endl;
  os << indent << "TimeFactor: " << this->TimeFactor << endl;

  os << indent << "Erase: "
     << (this->Erase ? "On\n" : "Off\n");

  os << indent << "Draw: "
     << (this->Draw ? "On\n" : "Off\n");

  os << indent << "UseDepthPeeling: "
     << (this->UseDepthPeeling ? "On" : "Off")<< "\n";

  os << indent << "OcclusionRation: "
     << this->OcclusionRatio << "\n";

  os << indent << "MaximumNumberOfPeels: "
     << this->MaximumNumberOfPeels << "\n";

  os << indent << "LastRenderingUsedDepthPeeling: "
     << (this->LastRenderingUsedDepthPeeling ? "On" : "Off")<< "\n";

  // I don't want to print this since it is used just internally
  // os << indent << this->NumberOfPropsRendered;

  os << indent << "Delegate:";
  if(this->Delegate!=0)
    {
      os << "exists" << endl;
    }
  else
    {
      os << "null" << endl;
    }
  os << indent << "Selector: " << this->Selector << endl;

  os << indent << "Pass:";
  if(this->Pass!=0)
    {
      os << "exists" << endl;
    }
  else
    {
      os << "null" << endl;
    }

  os << indent << "TexturedBackground: "
    << (this->TexturedBackground ? "On" : "Off") << "\n";

  os << indent << "BackgroundTexture:";
  if(this->BackgroundTexture != 0)
    {
    os << "exists" << endl;
    }
  else
    {
    os << "null" << endl;
    }
}

int vtkRenderer::VisibleActorCount()
{
  vtkProp *aProp;
  int count = 0;

  // loop through Props
  vtkCollectionSimpleIterator pit;
  for (this->Props->InitTraversal(pit);
       (aProp = this->Props->GetNextProp(pit)); )
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
  vtkCollectionSimpleIterator pit;
  for (this->Props->InitTraversal(pit);
        (aProp = this->Props->GetNextProp(pit)); )
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


vtkAssemblyPath* vtkRenderer::PickProp(double selectionX1, double selectionY1,
                                       double selectionX2, double selectionY2)
{
  // initialize picking information
  this->CurrentPickId = 1; // start at 1, so 0 can be a no pick
  this->PickX1 = (selectionX1 < selectionX2) ? selectionX1 : selectionX2;
  this->PickY1 = (selectionY1 < selectionY2) ? selectionY1 : selectionY2;
  this->PickX2 = (selectionX1 > selectionX2) ? selectionX1 : selectionX2;
  this->PickY2 = (selectionY1 > selectionY2) ? selectionY1 : selectionY2;
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
  this->StartPick(static_cast<unsigned int>(numberPickFrom));
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

    // wrap around, as there are thrice as many pickid's as PathArrayCount,
    // because each Prop has RenderOpaqueGeometry,
    // RenderTranslucentPolygonalGeometry, RenderVolumetricGeometry and
    // RenderOverlay called on it.
    pickedId = pickedId % static_cast<unsigned int>(this->PathArrayCount);
    this->PickedProp = this->PathArray[pickedId];
    this->PickedProp->Register(this);
    }

  //convert the list of picked props from integers to prop pointers
  if (this->PickResultProps != NULL)
    {
    this->PickResultProps->Delete();
    this->PickResultProps = NULL;
    }
  this->PickResultProps = vtkPropCollection::New();
  unsigned int numPicked = this->GetNumPickedIds();
  unsigned int *idBuff = new unsigned int[numPicked];
  this->GetPickedIds(numPicked, idBuff);
  unsigned int nextId;
  for (unsigned int pIdx = 0; pIdx < numPicked; pIdx++)
    {
    nextId = idBuff[pIdx] - 1; // pick ids start at 1, so move back one
    nextId = nextId % static_cast<unsigned int>(this->PathArrayCount);
    vtkProp *propCandidate = this->PathArray[nextId]->GetLastNode()->GetViewProp();
    this->PickResultProps->AddItem(propCandidate);
    }

  // Clean up stuff from picking after we use it
  delete [] idBuff;
  delete [] this->PathArray;
  this->PathArray = NULL;

  // Return the pick!
  return this->PickedProp; //returns an assembly path
}

// Do a render in pick or select mode.  This is normally done with
// rendering turned off. Before each Prop is rendered the pick id is
// incremented
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
  vtkCollectionSimpleIterator pit;
  for (  props->InitTraversal(pit); (aProp = props->GetNextProp(pit)); )
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

  vtkPicker* pCullPicker = NULL;
  vtkAreaPicker *aCullPicker = NULL;
  vtkProp3DCollection* cullPicked;
  if (this->GetPickWidth()==1 && this->GetPickHeight()==1)
    {
    // Create a picker to do the culling process
    pCullPicker = vtkPicker::New();

    // Add each of the Actors from the pickFrom list into the picker
    for ( pickFrom->InitTraversal(pit); (aProp = pickFrom->GetNextProp(pit)); )
      {
      pCullPicker->AddPickList(aProp);
      }

    // make sure this selects from the pickers list and not the renderers list
    pCullPicker->PickFromListOn();

    // do the pick
    pCullPicker->Pick(this->GetPickX(), this->GetPickY(), 0, this);

    cullPicked = pCullPicker->GetProp3Ds();
    }
  else
    {
    aCullPicker = vtkAreaPicker::New();

    // Add each of the Actors from the pickFrom list into the picker
    for ( pickFrom->InitTraversal(pit); (aProp = pickFrom->GetNextProp(pit)); )
      {
      aCullPicker->AddPickList(aProp);
      }

    // make sure this selects from the pickers list and not the renderers list
    aCullPicker->PickFromListOn();

    // do the pick
    aCullPicker->AreaPick(this->PickX1, this->PickY1,
                          this->PickX2, this->PickY2,
                          this);

    cullPicked = aCullPicker->GetProp3Ds();
    }

  // Put all the ones that were picked by the cull process
  // into the PathArray to be picked from
  vtkCollectionSimpleIterator p3dit;
  for (cullPicked->InitTraversal(p3dit);
       (aProp = cullPicked->GetNextProp3D(p3dit));)
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
  if (pCullPicker)
    {
    pCullPicker->Delete();
    }
  if (aCullPicker)
    {
    aCullPicker->Delete();
    }

  if ( this->PathArrayCount == 0 )
    {
    vtkDebugMacro( << "There are no visible props!" );
    return;
    }

  // do the render library specific pick render
  this->DevicePickRender();
}

void vtkRenderer::PickGeometry()
{
  int        i;

  this->NumberOfPropsRendered = 0;

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
    prop = this->PathArray[i]->GetLastNode()->GetViewProp();
    matrix = this->PathArray[i]->GetLastNode()->GetMatrix();
    prop->PokeMatrix(matrix);
    this->NumberOfPropsRendered += prop->RenderOpaqueGeometry(this);
    prop->PokeMatrix(NULL);
    }

  // loop through props and give them a chance to
  // render themselves as translucent polygonal geometry
  for ( i = 0; i < this->PathArrayCount; i++ )
    {
    this->UpdatePickId();
    prop = this->PathArray[i]->GetLastNode()->GetViewProp();
    matrix = this->PathArray[i]->GetLastNode()->GetMatrix();
    prop->PokeMatrix(matrix);
    this->NumberOfPropsRendered +=
      prop->RenderTranslucentPolygonalGeometry(this);
    prop->PokeMatrix(NULL);
    }

   // loop through props and give them a chance to
  // render themselves as volumetric geometry
  for ( i = 0; i < this->PathArrayCount; i++ )
    {
    this->UpdatePickId();
    prop = this->PathArray[i]->GetLastNode()->GetViewProp();
    matrix = this->PathArray[i]->GetLastNode()->GetMatrix();
    prop->PokeMatrix(matrix);
    this->NumberOfPropsRendered +=
      prop->RenderVolumetricGeometry(this);
    prop->PokeMatrix(NULL);
    }

  for ( i = 0; i < this->PathArrayCount; i++ )
    {
    this->UpdatePickId();
    prop = this->PathArray[i]->GetLastNode()->GetViewProp();
    matrix = this->PathArray[i]->GetLastNode()->GetMatrix();
    prop->PokeMatrix(matrix);
    this->NumberOfPropsRendered +=
      prop->RenderOverlay(this);
    prop->PokeMatrix(NULL);
    }

  vtkDebugMacro( << "Pick Rendered " <<
                    this->NumberOfPropsRendered << " actors" );

}

void vtkRenderer::ExpandBounds(double bounds[6], vtkMatrix4x4 *matrix)
{
  if(!bounds)
    {
    vtkErrorMacro(<<"ERROR: Invalid bounds\n");
    return;
    }

  if(!matrix)
    {
    vtkErrorMacro("<<ERROR: Invalid matrix \n");
    return;
    }

  // Expand the bounding box by model view transform matrix.
  double pt[8][4] = {{bounds[0], bounds[2], bounds[5], 1.0},
                     {bounds[1], bounds[2], bounds[5], 1.0},
                     {bounds[1], bounds[2], bounds[4], 1.0},
                     {bounds[0], bounds[2], bounds[4], 1.0},
                     {bounds[0], bounds[3], bounds[5], 1.0},
                     {bounds[1], bounds[3], bounds[5], 1.0},
                     {bounds[1], bounds[3], bounds[4], 1.0},
                     {bounds[0], bounds[3], bounds[4], 1.0}};

  // \note: Assuming that matrix doesn not have projective component. Hence not
  // dividing by the homogeneous coordinate after multiplication
  for (int i = 0; i < 8; ++i)
    {
      matrix->MultiplyPoint(pt[i],pt[i]);
    }

  // min = mpx = pt[0]
  double min[4], max[4];
  for (int i = 0; i < 4; ++i)
    {
      min[i] = pt[0][i];
      max[i] = pt[0][i];
    }

  for (int i = 1; i < 8; ++i)
    {
      for (int j = 0; j < 3; ++j)
        {
          if(min[j] > pt[i][j])
            min[j] = pt[i][j];
          if(max[j] < pt[i][j])
            max[j] = pt[i][j];
        }
    }

  // Copy values back to bounds.
  bounds[0] = min[0];
  bounds[2] = min[1];
  bounds[4] = min[2];

  bounds[1] = max[0];
  bounds[3] = max[1];
  bounds[5] = max[2];
}

int  vtkRenderer::Transparent()
{
  // If our layer is the 0th layer, then we are not transparent, else we are.
  return (this->Layer == 0 ? 0 : 1);
}

double vtkRenderer::GetTiledAspectRatio()
{
  int usize, vsize;
  this->GetTiledSize(&usize,&vsize);

  // some renderer subclasses may have more complicated computations for the
  // aspect ratio. SO take that into account by computing the difference
  // between our simple aspect ratio and what the actual renderer is
  // reporting.
  double aspect[2];
  this->ComputeAspect();
  this->GetAspect(aspect);
  double aspect2[2];
  this->vtkViewport::ComputeAspect();
  this->vtkViewport::GetAspect(aspect2);
  double aspectModification = aspect[0]*aspect2[1]/(aspect[1]*aspect2[0]);

  double finalAspect = 1.0;
  if(vsize && usize)
    {
    finalAspect = aspectModification*usize/vsize;
    }
  return finalAspect;
}

#if !defined(VTK_LEGACY_REMOVE)
//----------------------------------------------------------------------------
int vtkRenderer::UpdateGeometryForSelection()
{

  int        i;
  if ( this->PropsSelectedFrom)
    {
    delete [] this->PropsSelectedFrom;
    this->PropsSelectedFrom = NULL;
    }
  this->PropsSelectedFromCount = this->PropArrayCount;
  this->PropsSelectedFrom = new vtkProp *[this->PropsSelectedFromCount];

  //change the renderer's background to black, which will indicate a miss
  double origBG[3];
  this->GetBackground(origBG);
  this->SetBackground(0.0,0.0,0.0);
  bool origGrad = this->GetGradientBackground();
  this->GradientBackgroundOff();
  this->Clear();

  //todo: save off and swap in other renderer/renderwindow settings that
  //could affect colors

  //create holders for prop's original rendering settings
  int orig_visibility;
  vtkPainter*orig_painter = NULL;

  //create a painter that will color each cell with an index
  if (this->IdentPainter==NULL)
    {
    this->IdentPainter = vtkIdentColoredPainter::New();
    }

  switch (this->SelectMode)
    {
    case COLOR_BY_PROCESSOR:
      //SelectConst should have been set to this node's rank
      this->IdentPainter->ColorByConstant(this->SelectConst);
      break;
    case COLOR_BY_ACTOR:
      //SelectConst will be incremented with each prop
      break;
    case COLOR_BY_CELL_ID_HIGH:
      //Each polygon will gets its own color
      this->IdentPainter->ColorByIncreasingIdent(2);
      break;
    case COLOR_BY_CELL_ID_MID:
      //Each polygon will gets its own color
      this->IdentPainter->ColorByIncreasingIdent(1);
      break;
    case COLOR_BY_CELL_ID_LOW:
      //Each polygon will gets its own color
      this->IdentPainter->ColorByIncreasingIdent(0);
      break;
    case COLOR_BY_VERTEX:
      //Each polygon will gets its own color
      this->IdentPainter->ColorByVertex();
      break;
    default:
      //should never get here
      return 0;
    }

  // loop through props and give them a chance to
  // render themselves as opaque geometry
  for ( i = 0; i < this->PropArrayCount; i++ )
    {
    this->PropsSelectedFrom[i] = this->PropArray[i];
    if (this->SelectMode == vtkRenderer::COLOR_BY_ACTOR)
      {
      this->IdentPainter->ColorByActorId(this->PropArray[i]);
      }
    else if (this->SelectMode == vtkRenderer::COLOR_BY_CELL_ID_HIGH ||
             this->SelectMode == vtkRenderer::COLOR_BY_CELL_ID_MID ||
             this->SelectMode == vtkRenderer::COLOR_BY_CELL_ID_LOW ||
             this->SelectMode == vtkRenderer::COLOR_BY_VERTEX)
      {
      //each actor starts it's cell count at 0
      this->IdentPainter->ResetCurrentId();
      }

    //try to swap the ident color painter for the original one
    //if this prop can not be selected, its visibility is turned off
    orig_painter = this->SwapInSelectablePainter(this->PropArray[i], orig_visibility);

    //render the prop
    if (this->PropArray[i]->GetVisibility())
      {
      this->NumberOfPropsRendered +=
        this->PropArray[i]->RenderOpaqueGeometry(this);
      }

    //restore the prop's original settings
    this->SwapOutSelectablePainter(this->PropArray[i], orig_painter, orig_visibility);
    }

  //restore original background
  this->SetBackground(origBG);
  this->SetGradientBackground(origGrad);

  return this->NumberOfPropsRendered;
}

//----------------------------------------------------------------------------
vtkPainter* vtkRenderer::SwapInSelectablePainter(
  vtkProp *prop,
  int &orig_visibility)
{
  vtkPainter* orig_painter = NULL;
  vtkPainterPolyDataMapper *orig_mapper = NULL;

  //try to find a polydatapainter that we can swap out
  vtkActor *actor = vtkActor::SafeDownCast(prop);
  if (actor &&
      !
      (actor->IsA("vtkFollower") ||
       actor->IsA("vtkLODActor") ||
       !actor->GetPickable())
    )
    {
    orig_mapper =
      vtkPainterPolyDataMapper::SafeDownCast(actor->GetMapper());

    if (orig_mapper)
      {
      //found it, now swap it out
      orig_painter = orig_mapper->GetPainter();

      //Register this to prevent orig_painter from being deleted
      //while we momentarily swap in a different painter
      orig_painter->Register(this);

      //ident painter colors each polygon based on the current selectmode.
      orig_mapper->SetPainter(this->IdentPainter);
      }
    }
  if (!orig_painter)
    {
    //if we couldn't find it, don't render the prop
    orig_visibility = prop->GetVisibility();
    prop->VisibilityOff();
    }
  return orig_painter;
}

//----------------------------------------------------------------------------
void vtkRenderer::SwapOutSelectablePainter(
  vtkProp *prop,
  vtkPainter* orig_painter,
  int orig_visibility)
{
  vtkPainterPolyDataMapper *orig_mapper = NULL;
  //try to restore the swapped out painter
  vtkActor *actor = vtkActor::SafeDownCast(prop);
  if (actor &&
      !
      (actor->IsA("vtkFollower") ||
       actor->IsA("vtkLODActor")  ||
       !actor->GetPickable()))
    {
    orig_mapper =
      vtkPainterPolyDataMapper::SafeDownCast(actor->GetMapper());

    if (orig_painter)
      {
      orig_mapper->SetPainter(orig_painter);
      orig_painter->UnRegister(this);
      }
    }
  if (!orig_painter)
    {
    //if we never swapped in the ident painter, restore the prop's original
    //visibility setting
    prop->SetVisibility(orig_visibility);
    }
}
#endif
