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
#include <iostream.h>
#include "Light.hh"

vlLight::vlLight()
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

void vlLight::SetPosition(float X, float Y, float Z)
{
  this->Position[0] = X;
  this->Position[1] = Y;
  this->Position[2] = Z;
}
void vlLight::SetPosition(float a[3])
{
  this->Position[0] = a[0];
  this->Position[1] = a[1];
  this->Position[2] = a[2];
}
float *vlLight::GetPosition()
{
  return (this->Position);
}

void vlLight::SetFocalPoint(float X, float Y, float Z)
{
  this->FocalPoint[0] = X; 
  this->FocalPoint[1] = Y; 
  this->FocalPoint[2] = Z;
}
void vlLight::SetFocalPoint(float a[3])
{
  this->FocalPoint[0] = a[0];
  this->FocalPoint[1] = a[1];
  this->FocalPoint[2] = a[2];
}
float *vlLight::GetFocalPoint()
{
  return (this->FocalPoint);
}

int vlLight::GetStatus()
{
  return this->OnStatus;
}



