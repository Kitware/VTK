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
#include "Renderer.h"

Renderer::Renderer()
{
  this->ActiveCamera = NULL;

  this->Ambient[0] = 1;
  this->Ambient[1] = 1;
  this->Ambient[2] = 1;

  this->Background[0] = 0;
  this->Background[1] = 0;
  this->Background[2] = 0;

  this->BackLight = 1;

  strcpy(this->name,"oscar");
}

void Renderer::SetBackground(float R, float G, float B)
{
  this->Background[0] = R;
  this->Background[1] = G;
  this->Background[2] = B;
}

void Renderer::SetActiveCamera(Camera *cam)
{
  this->ActiveCamera = cam;
}

void Renderer::AddLights(Light *light)
{
  this->Lights.AddMember(light);
}

void Renderer::AddActors(Actor *actor)
{
  this->Actors.AddMember(actor);
}

void Renderer::GetBackground(float *result)
{
  result[0] = this->Background[0];
  result[1] = this->Background[1];
  result[2] = this->Background[2];
}
