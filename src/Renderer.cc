/*=========================================================================

  Program:   OSCAR 
  Module:    Renderer.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#include <stdlib.h>
#include <string.h>
#include "Renderer.hh"

vlRenderer::vlRenderer()
{
  this->ActiveCamera = NULL;

  this->Ambient[0] = 1;
  this->Ambient[1] = 1;
  this->Ambient[2] = 1;

  this->Background[0] = 0;
  this->Background[1] = 0;
  this->Background[2] = 0;

  this->BackLight = 1;
}

void vlRenderer::SetActiveCamera(vlCamera *cam)
{
  this->ActiveCamera = cam;
}

void vlRenderer::AddLights(vlLight *light)
{
  this->Lights.AddMember(light);
}

void vlRenderer::AddActors(vlActor *actor)
{
  this->Actors.AddMember(actor);
}
