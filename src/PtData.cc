/*=========================================================================

  Program:   Visualization Library
  Module:    PtData.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// PointData methods
//
#include "PtData.hh"

vlPointData::vlPointData (const vlPointData& pd)
{
  this->Scalars = pd.Scalars;
  if (this->Scalars) this->Scalars->Register((void *)this);

  this->Vectors = pd.Vectors;
  if (this->Vectors) this->Vectors->Register((void *)this);

  this->Normals = pd.Normals;
  if (this->Normals) this->Normals->Register((void *)this);

  this->TCoords = pd.TCoords;
  if(this->TCoords) this->TCoords->Register((void *)this);

  this->CopyScalars = 1;
  this->CopyVectors = 1;
  this->CopyNormals = 1;
  this->CopyTCoords = 1;
}

vlPointData::~vlPointData()
{
  vlPointData::Initialize();
}

vlPointData& vlPointData::operator=(vlPointData& pd)
{
  vlScalars *s;
  vlVectors *v;
  vlNormals *n;
  vlTCoords *t;

  if ( (s = pd.GetScalars()) ) 
    {
    this->SetScalars(s);
    }

  if ( (v = pd.GetVectors()) ) 
    {
    this->SetVectors(v);
    }

  if ( (n = pd.GetNormals()) ) 
    {
    this->SetNormals(n);
    }

  if ( (t = pd.GetTCoords()) ) 
    {
    this->SetTCoords(t);
    }

  this->CopyScalars = pd.CopyScalars;
  this->CopyVectors = pd.CopyVectors;
  this->CopyNormals = pd.CopyNormals;
  this->CopyTCoords = pd.CopyTCoords;

  return *this;
}

//
// Copy the point data from one point to another
//
void vlPointData::CopyData(vlPointData* from_pd, int from_id, int to_id)
{
  if ( this->CopyScalars && from_pd->Scalars && this->Scalars )
    {
    this->Scalars->InsertScalar(to_id,from_pd->Scalars->GetScalar(from_id));
    }

  if ( this->CopyVectors && from_pd->Vectors && this->Vectors )
    {
    this->Vectors->InsertVector(to_id,from_pd->Vectors->GetVector(from_id));
    }

  if ( this->CopyNormals && from_pd->Normals && this->Normals )
    {
    this->Normals->InsertNormal(to_id,from_pd->Normals->GetNormal(from_id));
    }

  if ( this->CopyTCoords && from_pd->TCoords && this->TCoords )
    {
    this->TCoords->InsertTCoord(to_id,from_pd->TCoords->GetTCoord(from_id));
    }
}


void vlPointData::Initialize()
{
//
// Modify ourselves
//
  this->Modified();
//
// First free up any memory
//
  if ( this->Scalars )
    {
    this->Scalars->UnRegister((void *)this);
    this->Scalars = 0;
    }

  if ( this->Vectors )
    {
    this->Vectors->UnRegister((void *)this);
    this->Vectors = 0;
    }

  if ( this->Normals )
    {
    this->Normals->UnRegister((void *)this);
    this->Normals = 0;
    }

  if ( this->TCoords )
    {
    this->TCoords->UnRegister((void *)this);
    this->TCoords = 0;
    }
};

//
// Initializes point data for point-by-point copy operation.  If sze=0, then use 
// the input PointData to create (i.e., find initial size of) new objects; otherwise
// use the sze variable.
//
void vlPointData::CopyInitialize(vlPointData* pd, int sFlg, int vFlg, int nFlg, int tFlg, int sze, int ext)
{
  vlScalars *s, *newScalars;
  vlVectors *v, *newVectors;
  vlNormals *n, *newNormals;
  vlTCoords *t, *newTCoords;

  vlPointData::Initialize();
//
// Now create various point data depending upon input
//
  if ( !pd ) return;

  if ( (this->CopyScalars = sFlg) && (s = pd->GetScalars()) ) 
    {
    if ( sze > 0 ) newScalars = s->MakeObject(sze,ext);
    else newScalars = s->MakeObject(s->GetNumberOfScalars());
    this->SetScalars(newScalars);
    }

  if ( (this->CopyVectors = vFlg) && (v = pd->GetVectors()) ) 
    {
    if ( sze > 0 ) newVectors = v->MakeObject(sze,ext);
    else newVectors = v->MakeObject(v->GetNumberOfVectors());
    this->SetVectors(newVectors);
    }

  if ( (this->CopyNormals = nFlg) && (n = pd->GetNormals()) ) 
    {
    if ( sze > 0 ) newNormals = n->MakeObject(sze,ext);
    else newNormals = n->MakeObject(n->GetNumberOfNormals());
    this->SetNormals(newNormals);
    }

  if ( (this->CopyTCoords = tFlg) && (t = pd->GetTCoords()) ) 
    {
    if ( sze > 0 ) newTCoords = t->MakeObject(sze,t->GetDimension(),ext);
    else newTCoords = t->MakeObject(t->GetNumberOfTCoords(),t->GetDimension());
    this->SetTCoords(newTCoords);
    }
};

void vlPointData::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPointData::GetClassName()))
    {
    vlObject::PrintSelf(os,indent);

    if ( this->Scalars )
      {
      os << indent << "Scalars:\n";
      this->Scalars->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Scalar: (none)\n";
      }

    if ( this->Vectors )
      {
      os << indent << "Vectors:\n";
      this->Vectors->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Vectors: (none)\n";
      }

    if ( this->Normals )
      {
      os << indent << "Normals:\n";
      this->Normals->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Normals: (none)\n";
      }

    if ( this->TCoords )
      {
      os << indent << "Texture Coordinates:\n";
      this->TCoords->PrintSelf(os,indent.GetNextIndent());
      }
    else
      {
      os << indent << "Texture Coordinates: (none)\n";
      }
    os << indent << "Copy Scalars: " << (this->CopyScalars ? "On\n" : "Off\n");
    os << indent << "Copy Vectors: " << (this->CopyVectors ? "On\n" : "Off\n");
    os << indent << "Copy Normals: " << (this->CopyNormals ? "On\n" : "Off\n");
    os << indent << "Copy Texture Coordinates: " << (this->CopyTCoords ? "On\n" : "Off\n");


    }
}
