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
#include "PolyMap.hh"
#include "UGridMap.hh"
#include "UPtsMap.hh"

vlDataSetMapper::vlDataSetMapper()
{
  this->Input = NULL;
  this->Mapper = NULL;
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
  char *InputType;
  char *OldMapperType = NULL;

  InputType = this->Input->GetDataType();

  // if we have an old mapper get its type otherwise set it to none
  if (this->Mapper)
    {
    OldMapperType = this->Mapper->GetClassName();
    }
  else
    {
    OldMapperType = "none";
    }

  if (!strcmp("vlPolyData",InputType))
    {
    if (strcmp(OldMapperType,"vlPolyMapper"))
      {
      if (this->Mapper) this->Mapper->UnRegister(this);
      this->Mapper = new vlPolyMapper;
      this->Mapper->Register(this);
      }
    }
  
  if (!strcmp("vlStructuredPoints",InputType))
    {
    vlErrorMacro(<< "Structured Points Mapper not supported");
    }

  if (!strcmp("vlStructuredGrid",InputType))
    {
    vlErrorMacro(<< "Structured Grid Mapper not supported");
    }

  if (!strcmp("vlUnstructuredGrid",InputType))
    {
    if (strcmp(OldMapperType,"vlUnstructuredGridMapper"))
      {
      if (this->Mapper) this->Mapper->UnRegister(this);
      this->Mapper = new vlUnstructuredGridMapper;
      this->Mapper->Register(this);
      }
    }

  if (!strcmp("vlUnstructuredPoints",InputType))
    {
    if (strcmp(OldMapperType,"vlUnstructuredPointsMapper"))
      {
      if (this->Mapper) this->Mapper->UnRegister(this);
      this->Mapper = new vlUnstructuredPointsMapper;
      this->Mapper->Register(this);
      }
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
