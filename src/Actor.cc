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
#include <iostream.h>
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

void Actor::Render(Renderer *ren)
{
  /* render my property */
  this->MyProperty->Render(ren);

  /* send a render to the modeller */
  this->mapper->Render(ren);

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
    cerr << "Actor: Requesting illegal index\n";
    return this->Top->Actor;
    }

  elem = this->Top;
  for (i = 1; i < num; i++)
    {
    elem = elem->Next;
    }
  
  return (elem->Actor);
}
