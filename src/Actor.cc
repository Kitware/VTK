/*=========================================================================

  Program:   Visualization Library
  Module:    Actor.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/

#include <stdlib.h>
#include <iostream.h>
#include <math.h>

#include "Actor.hh"

vlActor::vlActor()
{
  this->Mapper = 0;
  this->Property = 0;

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
  this->Pickable   = 1;
  this->Dragable   = 1;
}

vlActor::~vlActor()
{
  if ( this->Mapper ) this->Mapper->UnRegister((void *)this);
}

void vlActor::Render(vlRenderer *ren)
{
  /* render the property */
  this->Property->Render(ren);

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

void vlActor::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlActor::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);
    
    os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
    if ( this->Mapper )
      {
      os << indent << "Mapper:\n";
      this->Mapper->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Mapper: (none)\n";
      }
    os << indent << "Orientation: (" << this->Orientation[0] << ", " 
      << this->Orientation[1] << ", " << this->Orientation[2] << ")\n";
    os << indent << "Origin: (" << this->Origin[0] << ", " 
      << this->Origin[1] << ", " << this->Origin[2] << ")\n";
    os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");
    os << indent << "Position: (" << this->Position[0] << ", " 
      << this->Position[1] << ", " << this->Position[2] << ")\n";
    if ( this->Property )
      {
      os << indent << "Property:\n";
      this->Property->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Property: (none)\n";
      }
    os << indent << "Scale: (" << this->Scale[0] << ", " 
      << this->Scale[1] << ", " << this->Scale[2] << ")\n";
    os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");
    }
}


