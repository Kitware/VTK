/*=========================================================================

  Program:   OSCAR 
  Module:    Light.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#include <stdlib.h>
#include "Light.h"

Light::Light()
{
  this->FocalPoint[0] = 0.0;
  this->FocalPoint[1] = 0.0;
  this->FocalPoint[2] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 1.0;

  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;

  this->OnStatus = 1;

  this->Intensity = 1.0;
}

void Light::SetPosition(float X, float Y, float Z)
{
  this->Position[0] = X;
  this->Position[1] = Y;
  this->Position[2] = Z;
}
void Light::SetPosition(float a[3])
{
  this->Position[0] = a[0];
  this->Position[1] = a[1];
  this->Position[2] = a[2];
}
float *Light::GetPosition()
{
  return (this->Position);
}

void Light::SetFocalPoint(float X, float Y, float Z)
{
  this->FocalPoint[0] = X; 
  this->FocalPoint[1] = Y; 
  this->FocalPoint[2] = Z;
}
void Light::SetFocalPoint(float a[3])
{
  this->FocalPoint[0] = a[0];
  this->FocalPoint[1] = a[1];
  this->FocalPoint[2] = a[2];
}
float *Light::GetFocalPoint()
{
  return (this->FocalPoint);
}

int Light::GetStatus()
{
  return this->OnStatus;
}

LightCollection::LightCollection()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
}

void LightCollection::AddMember(Light *light)
{
  LightListElement *elem;

  elem = new LightListElement;
  
  if (!this->Top)
    {
    this->Top = elem;
    }
  else
    {
    this->Bottom->Next = elem;
    }
  this->Bottom = elem;

  elem->Light = light;
  elem->Next = NULL;

  this->NumberOfItems++;
}

int LightCollection::GetNumberOfMembers()
{
  return this->NumberOfItems;
}

Light *LightCollection::GetMember(int num)
{
  int i;
  LightListElement *elem;

  if (num > this->NumberOfItems)
    {
    fprintf(stderr,"Light.cc Requesting illegal index\n");
    return this->Top->Light;
    }

  elem = this->Top;
  for (i = 1; i < num; i++)
    {
    elem = elem->Next;
    }
  
  return (elem->Light);
}
