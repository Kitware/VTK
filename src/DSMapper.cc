/*=========================================================================

  Program:   Visualization Library
  Module:    DSMapper.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Methods for polygon mapper
//
#include "DSMapper.hh"

vlDataSetMapper::vlDataSetMapper()
{
  this->Input = 0;
  this->Mapper = 0;
}

vlDataSetMapper::~vlDataSetMapper()
{
  if ( this->Input )
    {
    this->Input->UnRegister(this);
    }

  if ( this->Mapper )
    {
    this->Mapper->UnRegister(this);
    }
}

void vlDataSetMapper::SetInput(vlDataSet *in)
{
  if (in != this->Input )
    {
    this->Input = in;
    this->Input->Register(this);
    this->Modified();
    }
}
vlDataSet* vlDataSetMapper::GetInput()
{
  return this->Input;
}

//
// Return bounding box of data
//
float *vlDataSetMapper::GetBounds()
{
  static float bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  if ( ! this->Input ) 
    return bounds;
  else
    {
    this->Input->Update();
    return this->Input->GetBounds();
    }
}
//
// Receives from Actor -> maps data to primitives
//
void vlDataSetMapper::Render(vlRenderer *ren)
{
//
// make sure that we've been properly initialized
//
  if ( !this->Input )
    {
    vlErrorMacro(<< "No input!\n");
    return;
    }
  else
    {
//
// Now can create appropriate mapper
//
    if ( this->CreateMapper() )
      this->Mapper->Render(ren);
    }
}

int vlDataSetMapper::CreateMapper()
{
  vlMapper *mapper;

  if ( !(mapper = this->Input->MakeMapper()) )
    {
    vlErrorMacro(<< "Cannot map type: " << this->Input->GetClassName() <<"\n");
    return 0;
    }
  if ( mapper != this->Mapper ) 
    {
    *mapper = *this; // Update lookup table, etc.
    if (this->Mapper) this->Mapper->UnRegister(this);
    this->Mapper = mapper;
    this->Mapper->Register(this);
    }
  return 1;
} 

void vlDataSetMapper::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlDataSetMapper::GetClassName()))
    {
    vlMapper::PrintSelf(os,indent);

    if ( this->Input )
      {
      os << indent << "Input: (" << this->Input << ")\n";
      }
    else
      {
      os << indent << "Input: (none)\n";
      }

    if ( this->Mapper )
      {
      os << indent << "Mapper: (" << this->Mapper << ")\n";
      os << indent << "Mapper type: " << this->Mapper->GetClassName() << "\n";
      }
    else
      {
      os << indent << "Mapper: (none)\n";
      }
   }
}
