/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelRenderManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  Copyright 2003 Sandia Corporation. Under the terms of Contract
  DE-AC04-94AL85000, there is a non-exclusive license for use of this work by
  or on behalf of the U.S. Government. Redistribution and use in source and
  binary forms, with or without modification, are permitted provided that this
  Notice and any statement of authorship are reproduced on all copies.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkParallelRenderManager.h"

#include "vtkMultiProcessController.h"
#include "vtkCallbackCommand.h"
#include "vtkActorCollection.h"
#include "vtkActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

static void AbortRenderCheck(vtkObject *caller, unsigned long vtkNotUsed(event),
                 void *clientData, void *);
static void StartRender(vtkObject *caller, unsigned long vtkNotUsed(event),
            void *clientData, void *);
static void EndRender(vtkObject *caller, unsigned long vtkNotUsed(event),
            void *clientData, void *);
static void SatelliteStartRender(vtkObject *caller,
                unsigned long vtkNotUsed(event),
                void *clientData, void *);
static void SatelliteEndRender(vtkObject *caller,
                  unsigned long vtkNotUsed(event),
                  void *clientData, void *);
/*
static void ResetCamera(vtkObject *caller,
                        unsigned long vtkNotUsed(event),
                        void *clientData, void *);
static void ResetCameraClippingRange(vtkObject *caller,
                                     unsigned long vtkNotUsed(event),
                                     void *clientData, void *);
*/
static void RenderRMI(void *arg, void *, int, int);
static void ComputeVisiblePropBoundsRMI(void *arg, void *, int, int);
const int vtkParallelRenderManager::WIN_INFO_INT_SIZE = 
  sizeof(vtkParallelRenderManager::RenderWindowInfoInt)/sizeof(int);
const int vtkParallelRenderManager::WIN_INFO_DOUBLE_SIZE =
  sizeof(vtkParallelRenderManager::RenderWindowInfoDouble)/sizeof(double);
const int vtkParallelRenderManager::REN_INFO_INT_SIZE =
  sizeof(vtkParallelRenderManager::RendererInfoInt)/sizeof(int);
const int vtkParallelRenderManager::REN_INFO_DOUBLE_SIZE =
  sizeof(vtkParallelRenderManager::RendererInfoDouble)/sizeof(double);
const int vtkParallelRenderManager::LIGHT_INFO_DOUBLE_SIZE =
  sizeof(vtkParallelRenderManager::LightInfoDouble)/sizeof(double);

vtkCxxRevisionMacro(vtkParallelRenderManager, "1.62");

//----------------------------------------------------------------------------
vtkParallelRenderManager::vtkParallelRenderManager()
{
  this->RenderWindow = NULL;
  this->ObservingRenderWindow = 0;
  this->ObservingAbort = 0;

  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
  this->RootProcessId = 0;

  this->Renderers = vtkRendererCollection::New();
  this->SyncRenderWindowRenderers = 1;

  this->Lock = 0;

  this->ImageReductionFactor = 1;
  this->MaxImageReductionFactor = 16;
  this->AutoImageReductionFactor = 0;
  this->AverageTimePerPixel = 0.0;

  this->RenderTime = 0.0;
  this->ImageProcessingTime = 0.0;

  this->ParallelRendering = 1;
  this->WriteBackImages = 1;
  this->MagnifyImages = 1;
  this->MagnifyImageMethod = vtkParallelRenderManager::NEAREST;
  this->RenderEventPropagation = 1;
  this->UseCompositing = 1;

  this->FullImage = vtkUnsignedCharArray::New();
  this->ReducedImage = vtkUnsignedCharArray::New();
  this->FullImageUpToDate = 0;
  this->ReducedImageUpToDate = 0;
  this->RenderWindowImageUpToDate = 0;
  this->FullImageSize[0] = 0;
  this->FullImageSize[1] = 0;

  this->ReducedImageSize[0] = 0;
  this->ReducedImageSize[1] = 0;

  this->Viewports = vtkDoubleArray::New();
  this->Viewports->SetNumberOfComponents(4);

  this->UseRGBA = 1;

  this->AddedRMIs = 0;
  this->Timer = vtkTimerLog::New();
}

//----------------------------------------------------------------------------
vtkParallelRenderManager::~vtkParallelRenderManager()
{
  this->SetRenderWindow(NULL);
  if (this->Controller && this->AddedRMIs)
    {
    this->Controller->RemoveFirstRMI(vtkParallelRenderManager::RENDER_RMI_TAG);
    this->Controller->RemoveFirstRMI(COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);
    this->AddedRMIs = 0;
    }
  this->SetController(NULL);
  if (this->FullImage) this->FullImage->Delete();
  if (this->ReducedImage) this->ReducedImage->Delete();
  if (this->Viewports) this->Viewports->Delete();
  if (this->Timer) this->Timer->Delete();
  if (this->Renderers) this->Renderers->Delete();
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ParallelRendering: "
     << (this->ParallelRendering ? "on" : "off") << endl;
  os << indent << "RenderEventPropagation: "
     << (this->RenderEventPropagation ? "on" : "off") << endl;
  os << indent << "UseCompositing: "
     << (this->UseCompositing ? "on" : "off") << endl;
  os << indent << "SyncRenderWindowRenderers: "
     << (this->SyncRenderWindowRenderers ? "on" : "off") << endl;

  os << indent << "ObservingRenderWindow: "
     << (this->ObservingRenderWindow ? "yes" : "no") << endl;
  os << indent << "Locked: " << (this->Lock ? "yes" : "no") << endl;

  os << indent << "ImageReductionFactor: "
     << this->ImageReductionFactor << endl;
  os << indent << "MaxImageReductionFactor: "
     << this->MaxImageReductionFactor << endl;
  os << indent << "AutoImageReductionFactor: "
     << (this->AutoImageReductionFactor ? "on" : "off") << endl;

  if (this->MagnifyImageMethod == vtkParallelRenderManager::LINEAR)
    {
    os << indent << "MagnifyImageMethod: LINEAR\n";
    }
  else if (this->MagnifyImageMethod == vtkParallelRenderManager::NEAREST)
    {
    os << indent << "MagnifyImageMethod: NEAREST\n";
    }

  os << indent << "WriteBackImages: "
     << (this->WriteBackImages ? "on" : "off") << endl;
  os << indent << "MagnifyImages: "
     << (this->MagnifyImages ? "on" : "off") << endl;

  os << indent << "FullImageSize: ("
     << this->FullImageSize[0] << ", " << this->FullImageSize[1] << ")" << endl;
  os << indent << "ReducedImageSize: ("
     << this->ReducedImageSize[0] << ", "
     << this->ReducedImageSize[1] << ")" << endl;

  os << indent << "RenderWindow: " << this->RenderWindow << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "Renderers: " << this->Renderers << endl;
  os << indent << "RootProcessId: " << this->RootProcessId << endl;

  os << indent << "Last render time: " << this->RenderTime << endl;

  os << indent << "Last image processing time: "
     << this->ImageProcessingTime << endl;
  os << indent << "UseRGBA: " << this->UseRGBA << endl;

  os << indent << "FullImage: ";
  if (this->FullImage)
    {
    this->FullImage->PrintSelf(os, indent.GetNextIndent());;
    }
  else
    {
    os << "(none)" << endl;
    }
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkParallelRenderManager::MakeRenderWindow()
{
  vtkDebugMacro("MakeRenderWindow");

  return vtkRenderWindow::New();
}

//----------------------------------------------------------------------------
vtkRenderer *vtkParallelRenderManager::MakeRenderer()
{
  vtkDebugMacro("MakeRenderer");

  return vtkRenderer::New();
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::SetRenderWindow(vtkRenderWindow *renWin)
{
  vtkDebugMacro("SetRenderWindow");

  if (this->RenderWindow == renWin)
    {
    return;
    }
  this->Modified();

  if (this->RenderWindow)
    {
    // Remove all of the observers.
    if (this->ObservingRenderWindow)
      {
      this->RenderWindow->RemoveObserver(this->StartRenderTag);
      this->RenderWindow->RemoveObserver(this->EndRenderTag);
    
      this->ObservingRenderWindow = 0;
      }
    if (this->ObservingAbort)
      {
      this->RenderWindow->RemoveObserver(this->AbortRenderCheckTag);
      this->ObservingAbort = 0;
      }

    // Delete the reference.
    this->RenderWindow->UnRegister(this);
    this->RenderWindow = NULL;
    }

  this->RenderWindow = renWin;
  if (this->RenderWindow)
    {
    vtkCallbackCommand *cbc;

    this->RenderWindow->Register(this);

    // In case a subclass wants to raise aborts.
    cbc = vtkCallbackCommand::New();
    cbc->SetCallback(::AbortRenderCheck);
    cbc->SetClientData((void*)this);
    // renWin will delete the cbc when the observer is removed.
    this->AbortRenderCheckTag = renWin->AddObserver(vtkCommand::AbortCheckEvent,
      cbc);
    cbc->Delete();
    this->ObservingAbort = 1;

    if (this->Controller)
      {
      if (this->Controller->GetLocalProcessId() == this->RootProcessId)
        {
        this->ObservingRenderWindow = 1;

        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(::StartRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->StartRenderTag
          = this->RenderWindow->AddObserver(vtkCommand::StartEvent,cbc);
        cbc->Delete();

        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(::EndRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->EndRenderTag
          = this->RenderWindow->AddObserver(vtkCommand::EndEvent,cbc);
        cbc->Delete();
        }
      else // LocalProcessId != RootProcessId
        {
        this->ObservingRenderWindow = 1;

        cbc= vtkCallbackCommand::New();
        cbc->SetCallback(::SatelliteStartRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->StartRenderTag
          = this->RenderWindow->AddObserver(vtkCommand::StartEvent,cbc);
        cbc->Delete();
          
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(::SatelliteEndRender);
        cbc->SetClientData((void*)this);
        // renWin will delete the cbc when the observer is removed.
        this->EndRenderTag
          = this->RenderWindow->AddObserver(vtkCommand::EndEvent,cbc);
        cbc->Delete();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::SetController(vtkMultiProcessController *controller)
{
  //Regular vtkSetObjectMacro:
  vtkSetObjectBodyMacro(Controller,vtkMultiProcessController,controller)

  // We've changed the controller.  This may change how observers are attached
  // to the render window.
  if (this->RenderWindow)
    {
    vtkRenderWindow *saveRenWin = this->RenderWindow;
    saveRenWin->Register(this);
    this->SetRenderWindow(NULL);
    this->SetRenderWindow(saveRenWin);
    saveRenWin->UnRegister(this);
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::InitializePieces()
{
  vtkDebugMacro("InitializePieces");

  vtkRendererCollection *rens;
  vtkRenderer *ren;
  vtkActorCollection *actors;
  vtkActor *actor;
  vtkMapper *mapper;
  vtkPolyDataMapper *pdMapper;
  int piece, numPieces;

  if ((this->RenderWindow == NULL) || (this->Controller == NULL))
    {
    vtkWarningMacro("Called InitializePieces before setting RenderWindow or Controller");
    return;
    }
  piece = this->Controller->GetLocalProcessId();
  numPieces = this->Controller->GetNumberOfProcesses();

  rens = this->GetRenderers();
  vtkCollectionSimpleIterator rsit;
  rens->InitTraversal(rsit);
  while ( (ren = rens->GetNextRenderer(rsit)) )
    {
    actors = ren->GetActors();
    vtkCollectionSimpleIterator ait;
    actors->InitTraversal(ait);
    while ( (actor = actors->GetNextActor(ait)) )
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

//----------------------------------------------------------------------------
void vtkParallelRenderManager::InitializeOffScreen()
{
  vtkDebugMacro("InitializeOffScreen");

  if ((this->RenderWindow == NULL) || (this->Controller == NULL))
    {
    vtkWarningMacro("Called InitializeOffScreen before setting RenderWindow or Controller");
    return;
    }

  if ( (this->Controller->GetLocalProcessId() != this->RootProcessId) ||
       !this->WriteBackImages )
    {
    this->RenderWindow->OffScreenRenderingOn();
    }
  else
    {
    this->RenderWindow->OffScreenRenderingOff();
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::StartInteractor()
{
  vtkDebugMacro("StartInteractor");

  if ((this->Controller == NULL) || (this->RenderWindow == NULL))
    {
    vtkErrorMacro("Must set Controller and RenderWindow before starting interactor.");
    return;
    }

  if (this->Controller->GetLocalProcessId() == this->RootProcessId)
    {
    vtkRenderWindowInteractor *inter = this->RenderWindow->GetInteractor();
    if (!inter)
      {
      vtkErrorMacro("Render window does not have an interactor.");
      }
    else
      {
      inter->Initialize();
      inter->Start();
      }
    //By the time we reach here, the interaction is finished.
    this->StopServices();
    }
  else
    {
    this->StartServices();
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::StartServices()
{
  vtkDebugMacro("StartServices");
  
  if (!this->Controller)
    {
    vtkErrorMacro("Must set Controller before starting service");
    return;
    }
  if (this->Controller->GetLocalProcessId() == this->RootProcessId)
    {
    vtkWarningMacro("Starting service on root process (probably not what you wanted to do)");
    }

  this->InitializeRMIs();
  this->Controller->ProcessRMIs();
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::StopServices()
{
  vtkDebugMacro("StopServices");

  if (!this->Controller)
    {
    vtkErrorMacro("Must set Controller before stopping service");
    return;
    }
  if (this->Controller->GetLocalProcessId() != this->RootProcessId)
    {
    vtkErrorMacro("Can only stop services on root node");
    return;
    }

  int numProcs = this->Controller->GetNumberOfProcesses();
  for (int id = 0; id < numProcs; id++)
    {
    if (id == this->RootProcessId) continue;
    this->Controller->TriggerRMI(id,vtkMultiProcessController::BREAK_RMI_TAG);
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::StartRender()
{
  vtkParallelRenderManager::RenderWindowInfoInt winInfoInt;
  vtkParallelRenderManager::RenderWindowInfoDouble winInfoDouble;
  vtkParallelRenderManager::RendererInfoInt renInfoInt;
  vtkParallelRenderManager::RendererInfoDouble renInfoDouble;
  vtkParallelRenderManager::LightInfoDouble lightInfoDouble;

  vtkDebugMacro("StartRender");

  if ((this->Controller == NULL) || (this->Lock))
    {
    return;
    }
  this->Lock = 1;

  this->FullImageUpToDate = 0;
  this->ReducedImageUpToDate = 0;
  this->RenderWindowImageUpToDate = 0;

  if (this->FullImage->GetPointer(0) == this->ReducedImage->GetPointer(0))
    {
    // "Un-share" pointer for full/reduced images in case we need separate
    // arrays this run.
    this->ReducedImage->Initialize();
    }

  if (!this->ParallelRendering)
    {
    this->Lock = 0;
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent, NULL);

  this->ImageProcessingTime = 0;

  // Used to time the total render (without compositing).
  this->Timer->StartTimer();

  if (this->AutoImageReductionFactor)
    {
    this->SetImageReductionFactorForUpdateRate(
      this->RenderWindow->GetDesiredUpdateRate());
    }

  int id;
  int numProcs = this->Controller->GetNumberOfProcesses();

  // Make adjustments for window size.
  int *tilesize = this->RenderWindow->GetSize();
  // To me, it seems dangerous for RenderWindow to return a size bigger
  // than it actually supports or for GetSize to not return the same values
  // as SetSize.  Yet this is the case when tile rendering is established
  // in RenderWindow.  Correct for this.
  int size[2];
  int *tilescale;
  tilescale = this->RenderWindow->GetTileScale();
  size[0] = tilesize[0]/tilescale[0];  size[1] = tilesize[1]/tilescale[1];
  if ((size[0] == 0) || (size[1] == 0))
    {
    // It helps to have a real window size.
    vtkDebugMacro("Resetting window size to 300x300");
    size[0] = size[1] = 300;
    this->RenderWindow->SetSize(size[0], size[1]);
    }
  this->FullImageSize[0] = size[0];
  this->FullImageSize[1] = size[1];
  //Round up.
  this->ReducedImageSize[0] =
    (int)((size[0]+this->ImageReductionFactor-1)/this->ImageReductionFactor);
  this->ReducedImageSize[1] =
    (int)((size[1]+this->ImageReductionFactor-1)/this->ImageReductionFactor);

  // Collect and distribute information about current state of RenderWindow
  vtkRendererCollection *rens = this->GetRenderers();
  winInfoInt.FullSize[0] = this->FullImageSize[0];
  winInfoInt.FullSize[1] = this->FullImageSize[1];
  winInfoInt.ReducedSize[0] = this->ReducedImageSize[0];
  winInfoInt.ReducedSize[1] = this->ReducedImageSize[1];
  winInfoInt.NumberOfRenderers = rens->GetNumberOfItems();
  winInfoDouble.ImageReductionFactor = this->ImageReductionFactor;
  winInfoInt.UseCompositing = this->UseCompositing;
  winInfoDouble.DesiredUpdateRate = this->RenderWindow->GetDesiredUpdateRate();

  for (id = 0; id < numProcs; id++)
    {
    if (id == this->RootProcessId)
      {
      continue;
      }
    if (this->RenderEventPropagation)
      {
      this->Controller->TriggerRMI(id, NULL, 0,
                   vtkParallelRenderManager::RENDER_RMI_TAG);
      }
    this->Controller->Send((int *)(&winInfoInt), 
                           vtkParallelRenderManager::WIN_INFO_INT_SIZE, 
                           id,
                           vtkParallelRenderManager::WIN_INFO_INT_TAG);
    this->Controller->Send((double *)(&winInfoDouble), 
                           vtkParallelRenderManager::WIN_INFO_DOUBLE_SIZE,
                           id, 
                           vtkParallelRenderManager::WIN_INFO_DOUBLE_TAG);
    }

  this->SendWindowInformation();

  if (this->ImageReductionFactor > 1)
    {
    this->Viewports->SetNumberOfTuples(rens->GetNumberOfItems());
    }

  vtkCollectionSimpleIterator cookie;
  vtkRenderer *ren;
  int i;

  for (rens->InitTraversal(cookie), i = 0;
       (ren = rens->GetNextRenderer(cookie)) != NULL; i++)
    {
    ren->GetViewport(renInfoDouble.Viewport);

    // Adjust Renderer viewports to get reduced size image.
    if (this->ImageReductionFactor > 1)
      {
      this->Viewports->SetTuple(i, renInfoDouble.Viewport);
      if (this->ImageReduceRenderer(ren))
        {
        renInfoDouble.Viewport[0] /= this->ImageReductionFactor;
        renInfoDouble.Viewport[1] /= this->ImageReductionFactor;
        renInfoDouble.Viewport[2] /= this->ImageReductionFactor;
        renInfoDouble.Viewport[3] /= this->ImageReductionFactor;
        ren->SetViewport(renInfoDouble.Viewport);
        }
      }

    vtkCamera *cam = ren->GetActiveCamera();
    cam->GetPosition(renInfoDouble.CameraPosition);
    cam->GetFocalPoint(renInfoDouble.CameraFocalPoint);
    cam->GetViewUp(renInfoDouble.CameraViewUp);
    cam->GetClippingRange(renInfoDouble.CameraClippingRange);
    renInfoDouble.CameraViewAngle = cam->GetViewAngle();
    cam->GetWindowCenter(renInfoDouble.WindowCenter);
        
    ren->GetBackground(renInfoDouble.Background);
    if (cam->GetParallelProjection())
      {
      renInfoDouble.ParallelScale = cam->GetParallelScale();
      }
    else
      {
      renInfoDouble.ParallelScale = 0.0;
      }
    renInfoInt.Draw = ren->GetDraw();
    vtkLightCollection *lc = ren->GetLights();
    renInfoInt.NumberOfLights = lc->GetNumberOfItems();

    for (id = 0; id < numProcs; id++)
      {
      if (id == this->RootProcessId)
        {
        continue;
        }
      this->Controller->Send((int *)(&renInfoInt), 
                             vtkParallelRenderManager::REN_INFO_INT_SIZE, 
                             id,
                             vtkParallelRenderManager::REN_INFO_INT_TAG);
      this->Controller->Send((double *)(&renInfoDouble), 
                             vtkParallelRenderManager::REN_INFO_DOUBLE_SIZE,
                             id, 
                             vtkParallelRenderManager::REN_INFO_DOUBLE_TAG);
      }

    vtkLight *light;
    vtkCollectionSimpleIterator lsit;
    for (lc->InitTraversal(lsit); (light = lc->GetNextLight(lsit)); )
      {
      lightInfoDouble.Type = (double)(light->GetLightType());
      light->GetPosition(lightInfoDouble.Position);
      light->GetFocalPoint(lightInfoDouble.FocalPoint);
      
      for (id = 0; id < numProcs; id++)
        {
        if (id == this->RootProcessId) continue;
        this->Controller->Send((double *)(&lightInfoDouble),
                               vtkParallelRenderManager::LIGHT_INFO_DOUBLE_SIZE,
                               id,
                               vtkParallelRenderManager::LIGHT_INFO_DOUBLE_TAG);
        }
      }
    this->SendRendererInformation(ren);
    }

  this->PreRenderProcessing();
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::EndRender()
{
  if (!this->ParallelRendering)
    {
    return;
    }

  this->Timer->StopTimer();
  this->RenderTime = this->Timer->GetElapsedTime() - this->ImageProcessingTime;

  // Just because we are not doing compositing does not mean a subclass
  // does not need to do post render processing.
//   if (!this->UseCompositing)
//     {
//     this->Lock = 0;
//     return;
//     }

  if (this->CheckForAbortComposite())
    {
    this->Lock = 0;
    return;
    }  

  this->PostRenderProcessing();

  // Restore renderer viewports, if necessary.
  if (this->ImageReductionFactor > 1)
    {
    vtkRendererCollection *rens = this->GetRenderers();
    vtkCollectionSimpleIterator cookie;
    vtkRenderer *ren;
    int i;
    for (rens->InitTraversal(cookie), i = 0;
         (ren = rens->GetNextRenderer(cookie)) != NULL; i++)
      {
      ren->SetViewport(this->Viewports->GetPointer(4*i));
      }
    }

  this->WriteFullImage();

  this->InvokeEvent(vtkCommand::EndEvent, NULL);

  this->Lock = 0;
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::SatelliteEndRender()
{
  if (this->CheckForAbortComposite())
    {
    return;
    }
// It's a mistake to check ParallelRendering on the Satellites.
// The Root node decides if the render calls are to be propagated to the
// satellites...the satellites always reply to the Root nodes requests.
//  if (!this->ParallelRendering)
//    {
//    return;
//    }
  // Just because we are not doing compositing does not mean a subclass
  // does not need to do post render processing.
//   if (!this->UseCompositing)
//     {
//     return;
//     }

  this->PostRenderProcessing();

  this->WriteFullImage();

  this->InvokeEvent(vtkCommand::EndEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::RenderRMI()
{
  this->RenderWindow->Render();
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::ResetCamera(vtkRenderer *ren)
{
  vtkDebugMacro("ResetCamera");

  double bounds[6];

  if (this->Lock)
    {
    // Can't query other processes in the middle of a render.
    // Just grab local value instead.
    this->LocalComputeVisiblePropBounds(ren, bounds);
    ren->ResetCamera(bounds);
    return;
    }

  this->Lock = 1;

  this->ComputeVisiblePropBounds(ren, bounds);
  // Keep from setting camera from some outrageous value.
  if (!vtkMath::AreBoundsInitialized(bounds))
    {
    // See if the not pickable values are better.
    ren->ComputeVisiblePropBounds(bounds);
    if (!vtkMath::AreBoundsInitialized(bounds))
      {
      this->Lock = 0;
      return;
      }
    }
  ren->ResetCamera(bounds);
  
  this->Lock = 0;
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::ResetCameraClippingRange(vtkRenderer *ren)
{
  vtkDebugMacro("ResetCameraClippingRange");

  double bounds[6];

  if (this->Lock)
    {
    // Can't query other processes in the middle of a render.
    // Just grab local value instead.
    this->LocalComputeVisiblePropBounds(ren, bounds);
    ren->ResetCameraClippingRange(bounds);
    return;
    }

  this->Lock = 1;
  
  this->ComputeVisiblePropBounds(ren, bounds);
  ren->ResetCameraClippingRange(bounds);

  this->Lock = 0;
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::ComputeVisiblePropBoundsRMI()
{
  vtkDebugMacro("ComputeVisiblePropBoundsRMI");
  int i;

  // Get proper renderer.
  int renderId = -1;
  if (!this->Controller->Receive(&renderId, 1, this->RootProcessId,
                                 vtkParallelRenderManager::REN_ID_TAG))
    {
    return;
    }
  vtkRendererCollection *rens = this->GetRenderers();
  vtkRenderer *ren = NULL;
  vtkCollectionSimpleIterator rsit;
  rens->InitTraversal(rsit);
  for (i = 0; i <= renderId; i++)
    {
    ren = rens->GetNextRenderer(rsit);
    }

  if (ren == NULL)
    {
    vtkWarningMacro("Client requested invalid renderer in "
            "ComputeVisiblePropBoundsRMI\n"
            "Defaulting to first renderer");
    ren = rens->GetFirstRenderer();
    }

  double bounds[6];
  this->LocalComputeVisiblePropBounds(ren, bounds);

  this->Controller->Send(bounds, 6, this->RootProcessId,
                         vtkParallelRenderManager::BOUNDS_TAG);
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::LocalComputeVisiblePropBounds(vtkRenderer *ren,
                                                             double bounds[6])
{
  ren->ComputeVisiblePropBounds(bounds);
}


//----------------------------------------------------------------------------
void vtkParallelRenderManager::ComputeVisiblePropBounds(vtkRenderer *ren,
                                                        double bounds[6])
{
  vtkDebugMacro("ComputeVisiblePropBounds");

  if (!this->ParallelRendering)
    {
    ren->ComputeVisiblePropBounds(bounds);
    return;
    }

  if (this->Controller)
    {
    if (this->Controller->GetLocalProcessId() != this->RootProcessId)
      {
      vtkErrorMacro("ComputeVisiblePropBounds/ResetCamera can only be called on root process");
      return;
      }

    vtkRendererCollection *rens = this->GetRenderers();
    vtkCollectionSimpleIterator rsit;
    rens->InitTraversal(rsit);
    int renderId = 0;
    while (1)
      {
      vtkRenderer *myren = rens->GetNextRenderer(rsit);
      if (myren == NULL)
        {
        vtkWarningMacro("ComputeVisiblePropBounds called with unregistered renderer " << ren << "\nDefaulting to first renderer.");
        renderId = 0;
        break;
        }
      if (myren == ren)
        {
        //Found correct renderer.
        break;
        }
      renderId++;
      }

    //Invoke RMI's on servers to perform their own ComputeVisiblePropBounds.
    int numProcs = this->Controller->GetNumberOfProcesses();
    int id;
    for (id = 0; id < numProcs; id++)
      {
      if (id == this->RootProcessId)
        {
        continue;
        }
      this->Controller->TriggerRMI(
        id, vtkParallelRenderManager::COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);
      this->Controller->Send(&renderId, 1, id,
                             vtkParallelRenderManager::REN_ID_TAG);
      }
    
    //Now that all the RMI's have been invoked, we can safely query our
    //local bounds even if an Update requires a parallel operation.

    this->LocalComputeVisiblePropBounds(ren, bounds);
  
    //Collect all the bounds.
    for (id = 0; id < numProcs; id++)
      {
      double tmp[6];

      if (id == this->RootProcessId)
        {
        continue;
        }

      this->Controller->Receive(tmp, 6, id, vtkParallelRenderManager::BOUNDS_TAG);
      
      if (tmp[0] < bounds[0])
        {
        bounds[0] = tmp[0];
        }
      if (tmp[1] > bounds[1])
        {
        bounds[1] = tmp[1];
        }
      if (tmp[2] < bounds[2])
        {
        bounds[2] = tmp[2];
        }
      if (tmp[3] > bounds[3])
        {
        bounds[3] = tmp[3];
        }
      if (tmp[4] < bounds[4])
        {
        bounds[4] = tmp[4];
        }
      if (tmp[5] > bounds[5])
        {
        bounds[5] = tmp[5];
        }
      }
    }
  else
    {
    vtkWarningMacro("ComputeVisiblePropBounds/ResetCamera called before Controller set");

    ren->ComputeVisiblePropBounds(bounds);
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::InitializeRMIs()
{
  vtkDebugMacro("InitializeRMIs");

  if (this->Controller == NULL)
    {
    vtkErrorMacro("InitializeRMIs requires a controller.");
    return;
    }

  this->AddedRMIs = 1;
  this->Controller->AddRMI(::RenderRMI, this,
                           vtkParallelRenderManager::RENDER_RMI_TAG);
  this->Controller->AddRMI(::ComputeVisiblePropBoundsRMI, this,
                           vtkParallelRenderManager::
                           COMPUTE_VISIBLE_PROP_BOUNDS_RMI_TAG);
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::ResetAllCameras()
{
  vtkDebugMacro("ResetAllCameras");

  if (!this->RenderWindow)
    {
    vtkErrorMacro("Called ResetAllCameras before RenderWindow set");
    return;
    }

  vtkRendererCollection *rens;
  vtkRenderer *ren;

  rens = this->GetRenderers();
  vtkCollectionSimpleIterator rsit;
  for (rens->InitTraversal(rsit); (ren = rens->GetNextRenderer(rsit)); )
    {
    this->ResetCamera(ren);
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::SetImageReductionFactor(double factor)
{
  // Clamp factor.
  factor = (factor < 1) ? 1 : factor;
  factor = (factor > this->MaxImageReductionFactor)
    ? this->MaxImageReductionFactor : factor;

  if (this->MagnifyImageMethod == LINEAR)
    {
    // Make factor be a power of 2.
    int pow_of_2 = 1;
    while (pow_of_2 <= factor)
      {
      pow_of_2 <<= 1;
      }
    factor = pow_of_2 >> 1;
    }

  if (factor == this->ImageReductionFactor)
    {
    return;
    }

  this->ImageReductionFactor = factor;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::SetMagnifyImageMethod(int method)
{
  if (this->MagnifyImageMethod == method)
    {
    return;
    }

  this->MagnifyImageMethod = method;
  // May need to modify image reduction factor.
  this->SetImageReductionFactor(this->ImageReductionFactor);
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::SetImageReductionFactorForUpdateRate(double desiredUpdateRate)
{
  vtkDebugMacro("Setting reduction factor for update rate of "
        << desiredUpdateRate);

  if (desiredUpdateRate == 0.0)
    {
    this->SetImageReductionFactor(1);
    return;
    }

  int *size = this->RenderWindow->GetSize();
  int numPixels = size[0]*size[1];
  int numReducedPixels
    = (int)(numPixels/(this->ImageReductionFactor*this->ImageReductionFactor));

  double renderTime = this->GetRenderTime();
  double pixelTime = this->GetImageProcessingTime();

  double timePerPixel;
  if (numReducedPixels > 0)
    {
    timePerPixel = pixelTime/numReducedPixels;
    }
  else
    {
    // Must be before first render.
    this->SetImageReductionFactor(1);
    return;
    }

  this->AverageTimePerPixel = (3*this->AverageTimePerPixel + timePerPixel)/4;
  if (this->AverageTimePerPixel <= 0)
    {
    this->AverageTimePerPixel = 0;
    this->SetImageReductionFactor(1);
    return;
    }

  double allottedPixelTime = 1.0/desiredUpdateRate - renderTime;
  // Give ourselves at least 15% of render time.
  if (allottedPixelTime < 0.15*renderTime)
    {
    allottedPixelTime = 0.15*renderTime;
    }

  vtkDebugMacro("TimePerPixel: " << timePerPixel
        << ", AverageTimePerPixel: " << this->AverageTimePerPixel
        << ", AllottedPixelTime: " << allottedPixelTime);

  double pixelsToUse = allottedPixelTime/this->AverageTimePerPixel;

  if ( (pixelsToUse < 1) ||
       (numPixels/pixelsToUse > this->MaxImageReductionFactor) )
    {
    this->SetImageReductionFactor(this->MaxImageReductionFactor);
    }
  else if (pixelsToUse >= numPixels)
    {
    this->SetImageReductionFactor(1);
    }
  else
    {
    this->SetImageReductionFactor((int)(numPixels/pixelsToUse));
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::SetRenderWindowSize()
{
  if (!this->RenderWindow->GetOffScreenRendering())
    {
    // Make sure we can support the requested image size.
    int *screensize = this->RenderWindow->GetScreenSize();
    if (this->FullImageSize[0] > screensize[0])
      {
      // Reduce both dimensions to preserve aspect ratio.
      this->FullImageSize[1]
        = (this->FullImageSize[1]*screensize[0])/this->FullImageSize[0];
      this->FullImageSize[0] = screensize[0];
      }
    if (this->FullImageSize[1] > screensize[1])
      {
      // Reduce both dimensions to preserve aspect ratio.
      this->FullImageSize[0]
        = (this->FullImageSize[0]*screensize[1])/this->FullImageSize[1];
      this->FullImageSize[1] = screensize[1];
      }

    // Make sure the reduced image is no bigger than the full image.
    if (this->ReducedImageSize[0] > this->FullImageSize[0])
      {
      this->ReducedImageSize[0] = this->FullImageSize[0];
      }
    if (this->ReducedImageSize[1] > this->FullImageSize[1])
      {
      this->ReducedImageSize[1] = this->FullImageSize[1];
      }
    }

  // Correct image reduction factor.
  this->ImageReductionFactor
    = (double)this->FullImageSize[0]/this->ReducedImageSize[0];

  this->RenderWindow->SetSize(this->FullImageSize[0], this->FullImageSize[1]);
}

//-----------------------------------------------------------------------------
vtkRendererCollection *vtkParallelRenderManager::GetRenderers()
{
  if (this->SyncRenderWindowRenderers)
    {
    return this->RenderWindow->GetRenderers();
    }
  else
    {
    return this->Renderers;
    }
}

//-----------------------------------------------------------------------------
void vtkParallelRenderManager::AddRenderer(vtkRenderer *ren)
{
  this->Renderers->AddItem(ren);
}

//-----------------------------------------------------------------------------
void vtkParallelRenderManager::RemoveRenderer(vtkRenderer *ren)
{
  this->Renderers->RemoveItem(ren);
}

//-----------------------------------------------------------------------------
void vtkParallelRenderManager::RemoveAllRenderers()
{
  this->Renderers->RemoveAllItems();
}

//----------------------------------------------------------------------------
int vtkParallelRenderManager::LastRenderInFrontBuffer()
{
  return this->RenderWindow->GetSwapBuffers();
}

//----------------------------------------------------------------------------
int vtkParallelRenderManager::ChooseBuffer()
{
  // Choose the back buffer if double buffering is on.
  return (this->RenderWindow->GetDoubleBuffer() == 0);
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::MagnifyImageNearest(
                                             vtkUnsignedCharArray *fullImage,
                                             const int fullImageSize[2],
                                             vtkUnsignedCharArray *reducedImage,
                                             const int reducedImageSize[2],
                                             const int fullImageViewport[4],
                                             const int reducedImageViewport[4])
{
  int numComp = reducedImage->GetNumberOfComponents();

  fullImage->SetNumberOfComponents(4);
  fullImage->SetNumberOfTuples(fullImageSize[0]*fullImageSize[1]);

  int destLeft, destRight, destBottom, destTop, destWidth, destHeight;
  if (fullImageViewport)
    {
    destLeft   = fullImageViewport[0];
    destBottom = fullImageViewport[1];
    destRight  = fullImageViewport[2];
    destTop    = fullImageViewport[3];
    destWidth  = fullImageViewport[2] - fullImageViewport[0];
    destHeight = fullImageViewport[3] - fullImageViewport[1];
    }
  else
    {
    destLeft = destBottom = destRight = destTop = 0;
    destWidth = fullImageSize[0];  destHeight = fullImageSize[1];
    }

  int srcLeft, srcRight, srcBottom, srcTop, srcWidth, srcHeight;
  if (reducedImageViewport)
    {
    srcLeft   = reducedImageViewport[0];
    srcBottom = reducedImageViewport[1];
    srcRight  = reducedImageViewport[2];
    srcTop    = reducedImageViewport[3];
    srcWidth  = reducedImageViewport[2] - reducedImageViewport[0];
    srcHeight = reducedImageViewport[3] - reducedImageViewport[1];
    }
  else
    {
    srcLeft = srcBottom = srcRight = srcTop = 0;
    srcWidth = reducedImageSize[0];  srcHeight = reducedImageSize[1];
    }

  if (numComp == 4)
    {
    // If there are 4 components per pixel, we can speed up the inflation
    // by copying integers instead of characters.

    // Making a bunch of tmp variables for speed within the loops
    // Look I know the compiler should optimize this stuff
    // but I don't trust compilers... besides testing shows
    // this code is faster than the old code
    float xstep = (float)srcWidth/destWidth;
    float ystep = (float)srcHeight/destHeight;
    float xaccum=0, yaccum=0;
    int destlinesize = fullImageSize[0];
    int srclinesize = reducedImageSize[0];
    int xmemsize = 4*destWidth;
    unsigned int *lastsrcline = NULL;
    unsigned int *destline = (unsigned int*)fullImage->GetPointer(
                                                    4*(  destBottom*destlinesize
                                                       + destLeft));
    unsigned int *srcline = (unsigned int*)reducedImage->GetPointer(
                                                      4*(  srcBottom*srclinesize
                                                         + srcLeft));
    unsigned int *srczero = srcline;

    // Inflate image.
    for (int y=0; y < destHeight; ++y, yaccum+=ystep)
      {
      // If this line same as last one.
      if (srcline == lastsrcline)
        {
        memcpy(destline, destline - destlinesize, xmemsize);
        }
      else
        {
        for (int x = 0; x < destWidth; ++x, xaccum+=xstep)
          {
          destline[x] = srcline[(int)(xaccum)];
          }
        xaccum=0;
        lastsrcline = srcline;
        }
      destline += destlinesize;
      srcline = srczero + srclinesize * int(yaccum); // Performance fixme
      }
    }
  else
    {
    // Inflate image.
    double xstep = (double)srcWidth/destWidth;
    double ystep = (double)srcHeight/destHeight;
    unsigned char *lastsrcline = NULL;
    for (int y = 0; y < destHeight; y++)
      {
      unsigned char *destline =
        fullImage->GetPointer(4*(fullImageSize[0]*(y+destBottom) + destLeft));
      unsigned char *srcline = reducedImage->GetPointer(
           numComp*(reducedImageSize[0]*((int)(ystep*y)+srcBottom) + srcLeft) );
      if (srcline == lastsrcline)
        {
        // This line same as last one.
        memcpy(destline,
               (const unsigned char *)(destline - 4*fullImageSize[0]),
               4*destWidth);
        }
      else
        {
        for (int x = 0; x < destWidth; x++)
          {
          int srcloc = numComp*(int)(x*xstep);
          int destloc = 4*x;
          int i;
          for (i = 0; i < numComp; i++)
            {
            destline[destloc + i] = srcline[srcloc + i];
            }
          for (; i < 4; i++)
            {
            destline[destloc + i] = 0xFF;
            }
          }
        lastsrcline = srcline;
        }
      }
    }
}

//----------------------------------------------------------------------------
// A neat trick to quickly divide all 4 of the bytes in an integer by 2.
#define VTK_VEC_DIV_2(intvector)    (((intvector) >> 1) & 0x7F7F7F7F)

void vtkParallelRenderManager::MagnifyImageLinear(
                                             vtkUnsignedCharArray *fullImage,
                                             const int fullImageSize[2],
                                             vtkUnsignedCharArray *reducedImage,
                                             const int reducedImageSize[2],
                                             const int fullImageViewport[4],
                                             const int reducedImageViewport[4])
{
  int xmag, ymag;
  int x, y;
  int srcComp = reducedImage->GetNumberOfComponents();;

  //Allocate full image so all pixels are on 4-byte integer boundaries.
  fullImage->SetNumberOfComponents(4);
  fullImage->SetNumberOfTuples(fullImageSize[0]*fullImageSize[1]);

  int destLeft, destRight, destBottom, destTop, destWidth, destHeight;
  if (fullImageViewport)
    {
    destLeft   = fullImageViewport[0];
    destBottom = fullImageViewport[1];
    destRight  = fullImageViewport[2];
    destTop    = fullImageViewport[3];
    destWidth  = fullImageViewport[2] - fullImageViewport[0];
    destHeight = fullImageViewport[3] - fullImageViewport[1];
    }
  else
    {
    destLeft = destBottom = destRight = destTop = 0;
    destWidth = fullImageSize[0];  destHeight = fullImageSize[1];
    }

  int srcLeft, srcRight, srcBottom, srcTop, srcWidth, srcHeight;
  if (reducedImageViewport)
    {
    srcLeft   = reducedImageViewport[0];
    srcBottom = reducedImageViewport[1];
    srcRight  = reducedImageViewport[2];
    srcTop    = reducedImageViewport[3];
    srcWidth  = reducedImageViewport[2] - reducedImageViewport[0];
    srcHeight = reducedImageViewport[3] - reducedImageViewport[1];
    }
  else
    {
    srcLeft = srcBottom = srcRight = srcTop = 0;
    srcWidth = reducedImageSize[0];  srcHeight = reducedImageSize[1];
    }

  // Guess x and y magnification.  Round up to ensure we do not try to
  // read data from the image data that does not exist.
  xmag = (destWidth +srcWidth -1)/srcWidth;
  ymag = (destHeight+srcHeight-1)/srcHeight;

  // For speed, we only magnify by powers of 2.  Round up to the nearest
  // power of 2 to ensure that the reduced image is large enough.
  int powOf2;
  for (powOf2 = 1; powOf2 < xmag; powOf2 <<= 1);
  xmag = powOf2;
  for (powOf2 = 1; powOf2 < ymag; powOf2 <<= 1);
  ymag = powOf2;

  unsigned char *srcline = reducedImage->GetPointer(
                                         srcComp*srcBottom*reducedImageSize[0]);
  unsigned char *destline = fullImage->GetPointer(
                                                 4*destBottom*fullImageSize[0]);
  for (y = 0; y < destHeight; y += ymag)
    {
    unsigned char *srcval = srcline + srcComp*srcLeft;
    unsigned char *destval = destline + 4*destLeft;
    for (x = 0; x < destWidth; x += xmag)
      {
      destval[0] = srcval[0];
      destval[1] = srcval[1];
      destval[2] = srcval[2];
      destval[3] = 0xFF;    //Hope we don't need the alpha value.
      srcval += srcComp;
      destval += 4*xmag;
      }
    srcline += srcComp*reducedImageSize[0];
    destline += 4*fullImageSize[0]*ymag;
    }

  // Now that we have everything on 4-byte boundaries, we will treat
  // everything as integers for much faster computation.
  unsigned int *image =
    (unsigned int *)fullImage->GetPointer(0)
    + destBottom*fullImageSize[0] + destLeft;

  // Fill in scanlines.
  for (; xmag > 1; xmag >>= 1)
    {
    int halfXMag = xmag/2;
    for (y = 0; y < destHeight; y += ymag)
      {
      unsigned int *scanline = image + y*fullImageSize[0];
      int maxX = destWidth - halfXMag;    //Don't access bad memory.
      for (x = halfXMag; x < maxX; x += xmag)
        {
        scanline[x] =
          VTK_VEC_DIV_2(scanline[x-halfXMag]) +
          VTK_VEC_DIV_2(scanline[x+halfXMag]);
        }
      }
    }

  // Add blank scanlines.
  for (; ymag > 1; ymag >>= 1)
    {
    int halfYMag = ymag/2;
    int maxY = destHeight - halfYMag;    //Don't access bad memory.
    for (y = halfYMag; y < maxY; y += ymag)
      {
      unsigned int *destline2 = image + y*fullImageSize[0];
      unsigned int *srcline1 = image + (y-halfYMag)*fullImageSize[0];
      unsigned int *srcline2 = image + (y+halfYMag)*fullImageSize[0];
      for (x = 0; x < destWidth; x++)
        {
        destline2[x] = VTK_VEC_DIV_2(srcline1[x]) + VTK_VEC_DIV_2(srcline2[x]);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkParallelRenderManager::MagnifyImage(vtkUnsignedCharArray *fullImage,
                                            const int fullImageSize[2],
                                            vtkUnsignedCharArray *reducedImage,
                                            const int reducedImageSize[2],
                                            const int fullImageViewport[4],
                                            const int reducedImageViewport[4])
{
  switch (this->MagnifyImageMethod)
    {
    case vtkParallelRenderManager::NEAREST:
      this->MagnifyImageNearest(fullImage, fullImageSize,
                                reducedImage, reducedImageSize,
                                fullImageViewport, reducedImageViewport);
      break;
    case LINEAR:
      this->MagnifyImageLinear(fullImage, fullImageSize,
                               reducedImage, reducedImageSize,
                               fullImageViewport, reducedImageViewport);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::MagnifyReducedImage()
{
  if ((this->FullImageUpToDate))
    {
    return;
    }

  this->ReadReducedImage();

  if (this->FullImage->GetPointer(0) != this->ReducedImage->GetPointer(0))
    {
    this->Timer->StartTimer();
    this->MagnifyImage(this->FullImage, this->FullImageSize,
                       this->ReducedImage, this->ReducedImageSize);
    this->Timer->StopTimer();
    // We log the image inflation under render time because it is inversely
    // proportional to the image size.  This makes the auto image reduction
    // calculation work better.
    this->RenderTime += this->Timer->GetElapsedTime();
    }

  this->FullImageUpToDate = 1;
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::WriteFullImage()
{
  if (this->RenderWindowImageUpToDate || !this->WriteBackImages)
    {
    return;
    }

  if (   this->MagnifyImages
      && (   (this->FullImageSize[0] != this->ReducedImageSize[0])
          || (this->FullImageSize[1] != this->ReducedImageSize[1]) ) )
    {
    this->MagnifyReducedImage();
    this->SetRenderWindowPixelData(this->FullImage, this->FullImageSize);
    }
  else
    {
    // Only write back image if it has already been read and potentially
    // changed.
    if (this->ReducedImageUpToDate)
      {
      this->SetRenderWindowPixelData(this->ReducedImage,
                     this->ReducedImageSize);
      }
    }

  this->RenderWindowImageUpToDate = 1;
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::SetRenderWindowPixelData(
  vtkUnsignedCharArray *pixels, const int pixelDimensions[2])
{
  if (pixels->GetNumberOfComponents() == 4)
    {
    this->RenderWindow->SetRGBACharPixelData(0, 0,
                                             pixelDimensions[0]-1,
                                             pixelDimensions[1]-1,
                                             pixels,
                                             this->ChooseBuffer());
    }
  else
    {
    this->RenderWindow->SetPixelData(0, 0,
                                     pixelDimensions[0]-1,
                                     pixelDimensions[1]-1,
                                     pixels,
                                     this->ChooseBuffer());
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::ReadReducedImage()
{
  if (this->ReducedImageUpToDate)
    {
    return;
    }

  this->Timer->StartTimer();

  if (this->ImageReductionFactor > 1)
    {
    if (this->UseRGBA)
      {
      this->RenderWindow->GetRGBACharPixelData(0, 0, this->ReducedImageSize[0]-1,
                                             this->ReducedImageSize[1]-1,
                                             this->ChooseBuffer(),
                                             this->ReducedImage);
      }
    else
      {
      this->RenderWindow->GetPixelData(0, 0, this->ReducedImageSize[0]-1,
                                             this->ReducedImageSize[1]-1,
                                             this->ChooseBuffer(),
                                             this->ReducedImage);
      }
    }
  else
    {
    if (this->UseRGBA)
      {
      this->RenderWindow->GetRGBACharPixelData(0, 0, this->FullImageSize[0]-1,
                                             this->FullImageSize[1]-1,
                                             this->ChooseBuffer(),
                                             this->FullImage);
      }
    else
      {
      this->RenderWindow->GetPixelData(0, 0, this->FullImageSize[0]-1,
                                             this->FullImageSize[1]-1,
                                             this->ChooseBuffer(),
                                             this->FullImage);
      }
    this->FullImageUpToDate = 1;
    this->ReducedImage
      ->SetNumberOfComponents(this->FullImage->GetNumberOfComponents());
    this->ReducedImage->SetArray(this->FullImage->GetPointer(0),
                                 this->FullImage->GetSize(), 1);
    this->ReducedImage->SetNumberOfTuples(this->FullImage->GetNumberOfTuples());
    }

  this->Timer->StopTimer();
  this->ImageProcessingTime += this->Timer->GetElapsedTime();

  this->ReducedImageUpToDate = 1;
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::GetPixelData(vtkUnsignedCharArray *data)
{
  if (!this->RenderWindow)
    {
    vtkErrorMacro("Tried to read pixel data from non-existent RenderWindow");
    return;
    }

  // Read image from RenderWindow and magnify if necessary.
  this->MagnifyReducedImage();

  data->SetNumberOfComponents(this->FullImage->GetNumberOfComponents());
  data->SetArray(this->FullImage->GetPointer(0),
                 this->FullImage->GetSize(), 1);
  data->SetNumberOfTuples(this->FullImage->GetNumberOfTuples());
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::GetPixelData(int x1, int y1, int x2, int y2,
                                            vtkUnsignedCharArray *data)
{
  if (!this->RenderWindow)
    {
    vtkErrorMacro("Tried to read pixel data from non-existent RenderWindow");
    return;
    }

  this->MagnifyReducedImage();

  if (x1 > x2)
    {
    int tmp = x1;
    x1 = x2;
    x2 = tmp;
    }
  if (y1 > y2)
    {
    int tmp = y1;
    y1 = y2;
    y2 = tmp;
    }

  if ( (x1 < 0) || (x2 >= this->FullImageSize[0]) ||
       (y1 < 0) || (y2 >= this->FullImageSize[1]) )
    {
    vtkErrorMacro("Requested pixel data out of RenderWindow bounds");
    return;
    }

  vtkIdType width = x2 - x1 + 1;
  vtkIdType height = y2 - y1 + 1;

  int numComp = this->FullImage->GetNumberOfComponents();

  data->SetNumberOfComponents(numComp);
  data->SetNumberOfTuples(width*height);

  const unsigned char *src = this->FullImage->GetPointer(0);
  unsigned char *dest = data->WritePointer(0, width*height*numComp);

  for (int row = 0; row < height; row++)
    {
    memcpy(dest + row*width*numComp,
           src + (row+y1)*this->FullImageSize[0]*numComp + x1*numComp,
           width*numComp);
    }
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::GetReducedPixelData(vtkUnsignedCharArray *data)
{
  if (!this->RenderWindow)
    {
    vtkErrorMacro("Tried to read pixel data from non-existent RenderWindow");
    return;
    }

  // Read image from RenderWindow and magnify if necessary.
  this->ReadReducedImage();

  data->SetNumberOfComponents(this->ReducedImage->GetNumberOfComponents());
  data->SetArray(this->ReducedImage->GetPointer(0),
                 this->ReducedImage->GetSize(), 1);
  data->SetNumberOfTuples(this->ReducedImage->GetNumberOfTuples());
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::GetReducedPixelData(int x1, int y1,
                                                   int x2, int y2,
                                                   vtkUnsignedCharArray *data)
{
  if (!this->RenderWindow)
    {
    vtkErrorMacro("Tried to read pixel data from non-existent RenderWindow");
    return;
    }

  this->ReadReducedImage();

  if (x1 > x2)
    {
    int tmp = x1;
    x1 = x2;
    x2 = tmp;
    }
  if (y1 > y2)
    {
    int tmp = y1;
    y1 = y2;
    y2 = tmp;
    }

  if ( (x1 < 0) || (x2 >= this->ReducedImageSize[0]) ||
       (y1 < 0) || (y2 >= this->ReducedImageSize[1]) )
    {
    vtkErrorMacro("Requested pixel data out of RenderWindow bounds");
    return;
    }

  vtkIdType width = x2 - x1 + 1;
  vtkIdType height = y2 - y1 + 1;

  int numComp = this->ReducedImage->GetNumberOfComponents();

  data->SetNumberOfComponents(numComp);
  data->SetNumberOfTuples(width*height);

  const unsigned char *src = this->ReducedImage->GetPointer(0);
  unsigned char *dest = data->WritePointer(0, width*height*numComp);

  for (int row = 0; row < height; row++)
    {
    memcpy(dest + row*width*numComp,
           src + (row+y1)*this->ReducedImageSize[0]*numComp + x1*numComp,
           width*numComp);
    }
}

// Static function prototypes --------------------------------------------

static void AbortRenderCheck(vtkObject *vtkNotUsed(caller), 
                             unsigned long vtkNotUsed(event),
                             void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  self->CheckForAbortRender();
}

static void StartRender(vtkObject *vtkNotUsed(caller), 
                        unsigned long vtkNotUsed(event),
                        void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  self->StartRender();
}

static void EndRender(vtkObject *vtkNotUsed(caller), 
                      unsigned long vtkNotUsed(event),
                      void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  self->EndRender();
}

static void SatelliteStartRender(vtkObject *vtkNotUsed(caller),
                                 unsigned long vtkNotUsed(event),
                                 void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  self->SatelliteStartRender();
}

static void SatelliteEndRender(vtkObject *vtkNotUsed(caller),
                               unsigned long vtkNotUsed(event),
                               void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  self->SatelliteEndRender();
}

/*
static void ResetCamera(vtkObject *caller,
                        unsigned long vtkNotUsed(event),
                        void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  vtkRenderer *ren = (vtkRenderer *)caller;
  self->ResetCamera(ren);
}

static void ResetCameraClippingRange(vtkObject *caller,
                                     unsigned long vtkNotUsed(event),
                                     void *clientData, void *)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)clientData;
  vtkRenderer *ren = (vtkRenderer *)caller;
  self->ResetCameraClippingRange(ren);
}
*/

static void RenderRMI(void *arg, void *, int, int)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)arg;
  self->RenderRMI();
}

static void ComputeVisiblePropBoundsRMI(void *arg, void *, int, int)
{
  vtkParallelRenderManager *self = (vtkParallelRenderManager *)arg;
  self->ComputeVisiblePropBoundsRMI();
}



//----------------------------------------------------------------------------
// the variables such as winInfoInt are initialzed prior to use  
#if defined(_MSC_VER) && !defined(VTK_DISPLAY_WIN32_WARNINGS)
#pragma warning ( disable : 4701 )
#endif

void vtkParallelRenderManager::SatelliteStartRender()
{
  vtkParallelRenderManager::RenderWindowInfoInt winInfoInt;
  vtkParallelRenderManager::RenderWindowInfoDouble winInfoDouble;
  vtkParallelRenderManager::RendererInfoInt renInfoInt;
  vtkParallelRenderManager::RendererInfoDouble renInfoDouble;
  vtkParallelRenderManager::LightInfoDouble lightInfoDouble;
  int i, j;

  vtkDebugMacro("SatelliteStartRender");

  this->FullImageUpToDate = 0;
  this->ReducedImageUpToDate = 0;
  this->RenderWindowImageUpToDate = 0;

  if (this->FullImage->GetPointer(0) == this->ReducedImage->GetPointer(0))
    {
    // "Un-share" pointer for full/reduced images in case we need separate
    // arrays this run.
    this->ReducedImage->Initialize();
    }

//if (!this->ParallelRendering)
//  {
//  return;
//  }

  this->InvokeEvent(vtkCommand::StartEvent, NULL);

  if (!this->Controller->Receive((int *)(&winInfoInt), 
                                 vtkParallelRenderManager::WIN_INFO_INT_SIZE,
                                 this->RootProcessId,
                                 vtkParallelRenderManager::WIN_INFO_INT_TAG))
    {
    return;
    }
  
  if (!this->Controller->Receive((double *)(&winInfoDouble),
                                 vtkParallelRenderManager::WIN_INFO_DOUBLE_SIZE,
                                 this->RootProcessId,
                                 vtkParallelRenderManager::WIN_INFO_DOUBLE_TAG))
    {
    return;
    }

  this->RenderWindow->SetDesiredUpdateRate(winInfoDouble.DesiredUpdateRate);
  this->SetUseCompositing(winInfoInt.UseCompositing);
  if (this->MaxImageReductionFactor < winInfoDouble.ImageReductionFactor)
    {
    this->SetMaxImageReductionFactor(winInfoDouble.ImageReductionFactor);
    }
  this->SetImageReductionFactor(winInfoDouble.ImageReductionFactor);
  this->FullImageSize[0] = winInfoInt.FullSize[0];
  this->FullImageSize[1] = winInfoInt.FullSize[1];
  this->ReducedImageSize[0] = winInfoInt.ReducedSize[0];
  this->ReducedImageSize[1] = winInfoInt.ReducedSize[1];
  
  this->ReceiveWindowInformation();

  this->SetRenderWindowSize();

  vtkCollectionSimpleIterator rsit;
  vtkRendererCollection *rens = this->GetRenderers();

  this->Viewports->SetNumberOfTuples(rens->GetNumberOfItems());

  rens->InitTraversal(rsit);
  for (i = 0; i < winInfoInt.NumberOfRenderers; i++)
    {
    if (!this->Controller->Receive((int *)(&renInfoInt), 
                                   vtkParallelRenderManager::REN_INFO_INT_SIZE,
                                   this->RootProcessId,
                                   vtkParallelRenderManager::REN_INFO_INT_TAG))
      {
      continue;
      }
    if (!this->Controller->Receive(
                                 (double *)(&renInfoDouble),
                                 vtkParallelRenderManager::REN_INFO_DOUBLE_SIZE,
                                 this->RootProcessId,
                                 vtkParallelRenderManager::REN_INFO_DOUBLE_TAG))
      {
      continue;
      }
    vtkLightCollection *lc = NULL;
    vtkCollectionSimpleIterator lsit;
    vtkRenderer *ren = rens->GetNextRenderer(rsit);
    if (ren == NULL)
      {
      vtkErrorMacro("Not enough renderers");
      }
    else
      {
      this->Viewports->SetTuple(i, ren->GetViewport());
      ren->SetViewport(renInfoDouble.Viewport);
      ren->SetBackground(renInfoDouble.Background[0],
                         renInfoDouble.Background[1],
                         renInfoDouble.Background[2]);
      vtkCamera *cam = ren->GetActiveCamera();
      cam->SetPosition(renInfoDouble.CameraPosition);
      cam->SetFocalPoint(renInfoDouble.CameraFocalPoint);
      cam->SetViewUp(renInfoDouble.CameraViewUp);
      cam->SetClippingRange(renInfoDouble.CameraClippingRange);
      cam->SetViewAngle(renInfoDouble.CameraViewAngle);
      cam->SetWindowCenter(renInfoDouble.WindowCenter[0],
                           renInfoDouble.WindowCenter[1]);
      if (renInfoDouble.ParallelScale != 0.0)
        {
        cam->ParallelProjectionOn();
        cam->SetParallelScale(renInfoDouble.ParallelScale);
        }
      else
        {
        cam->ParallelProjectionOff();
        }
      ren->SetDraw(renInfoInt.Draw);
      lc = ren->GetLights();
      lc->InitTraversal(lsit);
      }

    for (j = 0; j < renInfoInt.NumberOfLights; j++)
      {
      if (ren != NULL && lc != NULL)
        {
        vtkLight *light = lc->GetNextLight(lsit);
        if (light == NULL)
          {
          // Not enough lights?  Just create them.
          vtkDebugMacro("Adding light");
          light = vtkLight::New();
          ren->AddLight(light);
          light->Delete();
          }

        this->Controller->Receive((double *)(&lightInfoDouble),
                                  vtkParallelRenderManager::LIGHT_INFO_DOUBLE_SIZE,
                                  this->RootProcessId,
                                  vtkParallelRenderManager::LIGHT_INFO_DOUBLE_TAG);
        light->SetLightType((int)(lightInfoDouble.Type));
        light->SetPosition(lightInfoDouble.Position);
        light->SetFocalPoint(lightInfoDouble.FocalPoint);
        }
      }

    if (ren != NULL)
      {
      vtkLight *light;
      while ((light = lc->GetNextLight(lsit)))
        {
        // To many lights?  Just remove the extras.
        ren->RemoveLight(light);
        }
      }

    this->ReceiveRendererInformation(ren);
    }

  if (rens->GetNextRenderer(rsit))
    {
    vtkErrorMacro("Too many renderers.");
    }

  this->PreRenderProcessing();
}

//----------------------------------------------------------------------------
void vtkParallelRenderManager::TileWindows(int xsize, int ysize, int ncolumn)
{
  if (!this->RenderWindow || !this->Controller)
    {
    return;
    }

  int procId = this->Controller->GetLocalProcessId();

  int row = procId / ncolumn;
  int column = procId % ncolumn;

  this->RenderWindow->SetPosition(xsize*column, ysize*row);
}

//----------------------------------------------------------------------------

// Disable warnings about qualifiers on return types.
#if defined(_COMPILER_VERSION)
# pragma set woff 3303
#endif
#if defined(__INTEL_COMPILER)
# pragma warning (disable:858)
#endif

#ifndef VTK_LEGACY_REMOVE
# ifdef VTK_WORKAROUND_WINDOWS_MANGLE
#  undef StartService
void const vtkParallelRenderManager::StartServiceA()
{
  VTK_LEGACY_REPLACED_BODY(vtkParallelRenderManager::StartService, "VTK 5.0",
                           vtkParallelRenderManager::StartServices);
  this->StartServices();
}
void const vtkParallelRenderManager::StartServiceW()
{
  VTK_LEGACY_REPLACED_BODY(vtkParallelRenderManager::StartService, "VTK 5.0",
                           vtkParallelRenderManager::StartServices);
  this->StartServices();
}
# endif
void const vtkParallelRenderManager::StartService()
{
  VTK_LEGACY_REPLACED_BODY(vtkParallelRenderManager::StartService, "VTK 5.0",
                           vtkParallelRenderManager::StartServices);
  this->StartServices();
}
#endif
