/*=========================================================================

  Program:   Visualization Library
  Module:    Interact.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Interact.hh"
#include "Actor.hh"
#include "CellPick.hh"

// Description:
// Construct object so that light follows camera motion.
vlRenderWindowInteractor::vlRenderWindowInteractor()
{
  this->RenderWindow    = NULL;
  this->CurrentCamera   = NULL;
  this->CurrentLight    = NULL;
  this->CurrentRenderer = NULL;

  this->LightFollowCamera = 1;
  this->Initialized = 0;

  this->SelfCreatedPicker = 0;
  this->Picker = this->CreateDefaultPicker();
  this->OutlineActor = NULL;
  this->OutlineMapper.SetInput(this->Outline);
  this->PickedRenderer = NULL;
  this->CurrentActor = NULL;

  this->StartPickMethod = NULL;
  this->StartPickMethodArgDelete = NULL;
  this->StartPickMethodArg = NULL;
  this->EndPickMethod = NULL;
  this->EndPickMethodArgDelete = NULL;
  this->EndPickMethodArg = NULL;
  this->UserMethod = NULL;
  this->UserMethodArgDelete = NULL;
  this->UserMethodArg = NULL;
}

vlRenderWindowInteractor::~vlRenderWindowInteractor()
{
  if ( this->OutlineActor ) delete this->OutlineActor;
  if ( this->SelfCreatedPicker && this->Picker) delete this->Picker;
}

void vlRenderWindowInteractor::FindPokedRenderer(int x,int y)
{
  vlRendererCollection *rc;
  vlRenderer *aren;

  this->CurrentRenderer = NULL;

  rc = this->RenderWindow->GetRenderers();
  
  for (rc->InitTraversal(); 
       ((aren = rc->GetNextItem())&&(!this->CurrentRenderer));)
    {
    if (aren->IsInViewport(x,y))
      {
      this->CurrentRenderer = aren;
      }
    }
  
  // we must have a value 
  if (this->CurrentRenderer == NULL)
    {
    rc->InitTraversal();
    aren = rc->GetNextItem();
    this->CurrentRenderer = aren;
    }
}

void  vlRenderWindowInteractor::FindPokedCamera(int x,int y)
{
  float *vp;
  vlLightCollection *lc;

  this->FindPokedRenderer(x,y);
  vp = this->CurrentRenderer->GetViewport();

  this->CurrentCamera = this->CurrentRenderer->GetActiveCamera();  
  memcpy(this->Center,this->CurrentRenderer->GetCenter(),sizeof(int)*2);
  this->DeltaElevation = -20.0/((vp[3] - vp[1])*this->Size[1]);
  this->DeltaAzimuth = -20.0/((vp[2] - vp[0])*this->Size[0]);

  // as a side effect also set the light 
  // in case they are using light follow camera 
  lc = this->CurrentRenderer->GetLights();
  lc->InitTraversal();
  this->CurrentLight = lc->GetNextItem();
}

// Description:
// When pick action successfully selects actor, this method highlights the 
// actor appropriately.
void vlRenderWindowInteractor::HighlightActor(vlActor *actor)
{
  if ( ! this->OutlineActor )
    {
    // have to defer creation to get right type
    this->OutlineActor = new vlActor;
    this->OutlineActor->PickableOff();
    this->OutlineActor->DragableOff();
    this->OutlineActor->SetMapper(this->OutlineMapper);
    this->OutlineActor->GetProperty()->SetColor(1.0,1.0,1.0);
    this->OutlineActor->GetProperty()->SetAmbient(1.0);
    this->OutlineActor->GetProperty()->SetDiffuse(0.0);
    }

  if ( this->PickedRenderer ) 
    this->PickedRenderer->RemoveActors(OutlineActor);

  if ( ! actor )
    {
    this->PickedRenderer = NULL;
    }
  else 
    {
    this->PickedRenderer = this->CurrentRenderer;
    this->CurrentRenderer->AddActors(OutlineActor);
    this->Outline.SetBounds(actor->GetBounds());
    this->CurrentActor = actor;
    }
  this->RenderWindow->Render();
}

// Description:
// Specify a method to be executed prior to the pick operation.
void vlRenderWindowInteractor::SetStartPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartPickMethod || arg != this->StartPickMethodArg )
    {
    this->StartPickMethod = f;
    this->StartPickMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Specify a method to be executed after the pick operation.
void vlRenderWindowInteractor::SetEndPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndPickMethod || arg != this->EndPickMethodArg )
    {
    this->EndPickMethod = f;
    this->EndPickMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Set the object used to perform pick operations. You can use this to 
// control what type of data is picked.
void vlRenderWindowInteractor::SetPicker(vlPicker *picker)
{
  if ( this->Picker != picker ) 
    {
    if ( this->SelfCreatedPicker ) delete this->Picker;
    this->SelfCreatedPicker = 0;
    this->Picker = picker;
    this->Modified();
    }
}

vlPicker *vlRenderWindowInteractor::CreateDefaultPicker()
{
  if ( this->SelfCreatedPicker ) delete this->Picker;
  this->SelfCreatedPicker = 1;
  return new vlCellPicker;
}

// Description:
// Set the user method. This method is invokedon a ctrl-u.
void vlRenderWindowInteractor::SetUserMethod(void (*f)(void *), void *arg)
{
  if ( f != this->UserMethod || arg != this->UserMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->UserMethodArg)&&(this->UserMethodArgDelete))
      {
      (*this->UserMethodArgDelete)(this->UserMethodArg);
      }
    this->UserMethod = f;
    this->UserMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Called when a void* argument is being discarded.  Lets the user free it.
void vlRenderWindowInteractor::SetUserMethodArgDelete(void (*f)(void *))
{
  if ( f != this->UserMethodArgDelete)
    {
    this->UserMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Called when a void* argument is being discarded.  Lets the user free it.
void vlRenderWindowInteractor::SetStartPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartPickMethodArgDelete)
    {
    this->StartPickMethodArgDelete = f;
    this->Modified();
    }
}
// Description:
// Called when a void* argument is being discarded.  Lets the user free it.
void vlRenderWindowInteractor::SetEndPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndPickMethodArgDelete)
    {
    this->EndPickMethodArgDelete = f;
    this->Modified();
    }
}

void vlRenderWindowInteractor::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

  os << indent << "RenderWindow:    " << this->RenderWindow << "\n";
  os << indent << "CurrentCamera:   " << this->CurrentCamera << "\n";
  os << indent << "CurrentLight:    " << this->CurrentLight << "\n";
  os << indent << "CurrentRenderer: " << this->CurrentRenderer << "\n";
  os << indent << "LightFollowCamera: " << (this->LightFollowCamera ? "On\n" : "Off\n");
}

