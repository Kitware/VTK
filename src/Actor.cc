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

#include "Actor.hh"

vlActor::vlActor()
{
  this->Mapper = 0;
  this->MyProperty = 0;

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

int vlActor::GetVisibility()
{
  return this->Visibility;
}

void vlActor::Render(vlRenderer *ren)
{
  /* render my property */
  this->MyProperty->Render(ren);

  /* send a render to the modeller */
  this->Mapper->Render(ren);

}
  
void vlActor::GetCompositeMatrix(float mat[4][4])
{
  mat[0][0] = 1; mat[0][1] = 0; mat[0][2] = 0; mat[0][3] = 0;
  mat[1][0] = 0; mat[1][1] = 1; mat[1][2] = 0; mat[1][3] = 0;
  mat[2][0] = 0; mat[2][1] = 0; mat[2][2] = 1; mat[2][3] = 0;
  mat[3][0] = 0; mat[3][1] = 0; mat[3][2] = 0; mat[3][3] = 1;
}

void vlActor::SetMapper(vlMapper *m)
{
  if ( this->Mapper != m )
  {
    if ( this->Mapper ) this->Mapper->UnRegister((void *)this);
    this->Mapper = m;
    this->Mapper->Register((void *)this);
    this->Modified();
  }
}

vlMapper *vlActor::GetMapper()
{
  return this->Mapper;
}

