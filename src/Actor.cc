/*=========================================================================

  Program:   OSCAR 
  Module:    Actor.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#include <stdlib.h>
#include <math.h>
#include "Actor.h"
#include <kgl.h>

Actor::Actor()
{
  this->Origin[0] = 0.0;
  this->Origin[1] = 0.0;
  this->Origin[2] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 0.0;

  this->Orientation[0] = 0.0;
  this->Orientation[1] = 0.0;
  this->Orientation[2] = 0.0;

  this->Scale[0] = 1.0;
  this->Scale[1] = 1.0;
  this->Scale[2] = 1.0;

  this->Visibility = 1;
}

int Actor::GetVisibility()
{
  return this->Visibility;
}

drawcyl()
{
  double dy = .2;
  double theta, dtheta = 2*M_PI/20;
  double x, y, z;
  float n[3], v[3];
  int i, j;

  for (i = 0, y = -1;  i < 10;  i++, y += dy)  {
    kgl_StartPrimitive(KGL_TMESH);
    for (j = 0, theta = 0;  j <= 20;  j++, theta += dtheta)  {
      if (j == 20)  theta = 0;
      x = cos(theta);
      z = sin(theta);
      n[0] = x;  n[1] = 0;  n[2] = z;
      kgl_SetNormal3(n);
      v[0] = x;  v[1] = y;  v[2] = z;
      kgl_SetVertex3(v);
      v[1] = y + dy;
      kgl_SetVertex3(v);
      }
    kgl_EndPrimitive();
    }
  return 0;
}

void Actor::Render(Renderer *ren)
{
  /* render my property */
  this->MyProperty->Render(ren);

  /* send a render to the modeller */
  
  /* some test junk */
  drawcyl();
}
  
void Actor::GetCompositeMatrix(float mat[4][4])
{
  mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0; mat[0][3] = 0;
  mat[1][0] = 0; mat[1][1] = 1; mat[1][2] = 0; mat[1][3] = 0;
  mat[2][0] = 0; mat[2][1] = 0; mat[2][2] = 1; mat[2][3] = 0;
  mat[3][0] = 0; mat[3][1] = 0; mat[3][2] = 0; mat[3][3] = 1;
}

ActorCollection::ActorCollection()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
}

void ActorCollection::AddMember(Actor *actor)
{
  ActorListElement *elem;

  elem = new ActorListElement;
  
  if (!this->Top)
    {
    this->Top = elem;
    }
  else
    {
    this->Bottom->Next = elem;
    }
  this->Bottom = elem;

  elem->Actor = actor;
  elem->Next = NULL;

  this->NumberOfItems++;
}


int ActorCollection::GetNumberOfMembers()
{
  return this->NumberOfItems;
}

Actor *ActorCollection::GetMember(int num)
{
  int i;
  ActorListElement *elem;

  if (num > this->NumberOfItems)
    {
    fprintf(stderr,"Actor.cc Requesting illegal index\n");
    return this->Top->Actor;
    }

  elem = this->Top;
  for (i = 1; i < num; i++)
    {
    elem = elem->Next;
    }
  
  return (elem->Actor);
}
