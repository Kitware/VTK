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

  this->OutlineActor = NULL;
  this->OutlineMapper.SetInput(this->Outline);
  this->PickedRenderer = NULL;
  this->CurrentActor = NULL;
}

vlRenderWindowInteractor::~vlRenderWindowInteractor()
{
}

void vlRenderWindowInteractor::FindPokedRenderer(int x,int y)
{
  vlRendererCollection *rc;
  vlRenderer *aren;
  float *vp;

  this->CurrentRenderer = NULL;

  rc = this->RenderWindow->GetRenderers();
  
  for (rc->InitTraversal(); 
       ((aren = rc->GetNextItem())&&(!this->CurrentRenderer));)
    {
    vp = aren->GetViewport();
    if ((vp[0]*this->Size[0] <= x)&&
	(vp[2]*this->Size[0] >= x)&&
	(vp[1]*this->Size[1] <= y)&&
	(vp[3]*this->Size[1] >= y))
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
  this->Center[0] = (int)((vp[2]+vp[0])/2.0*(float)this->Size[0]);
  this->Center[1] = (int)((vp[3]+vp[1])/2.0*(float)this->Size[1]);
  this->DeltaElevation = 20.0/((vp[3] - vp[1])*this->Size[1]);
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
void vlRenderWindowInteractor::PickActor(vlActor *actor)
{
  if ( ! this->OutlineActor )
    {
    // have to defer creation to get right type
    this->OutlineActor = this->RenderWindow->MakeActor();
    this->OutlineActor->PickableOff();
    this->OutlineActor->DragableOff();
    this->OutlineActor->SetMapper(this->OutlineMapper);
    this->OutlineActor->GetProperty()->SetColor(1.0,1.0,1.0);
    this->OutlineActor->GetProperty()->SetWireframe();
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

