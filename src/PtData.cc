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
#include "PtData.hh"
#include "Cell.hh"
#include "FScalars.hh"
#include "FVectors.hh"
#include "FNormals.hh"
#include "FTCoords.hh"

// Description:
// Construct with copying turned on for all data.
vlPointData::vlPointData()
{
  this->Scalars = NULL;
  this->Vectors = NULL;
  this->Normals = NULL;
  this->TCoords = NULL;

  this->CopyScalars = 1;
  this->CopyVectors = 1;
  this->CopyNormals = 1;
  this->CopyTCoords = 1;
}

vlPointData::vlPointData (const vlPointData& pd)
{
  this->Scalars = pd.Scalars;
  if (this->Scalars) this->Scalars->Register(this);

  this->Vectors = pd.Vectors;
  if (this->Vectors) this->Vectors->Register(this);

  this->Normals = pd.Normals;
  if (this->Normals) this->Normals->Register(this);

  this->TCoords = pd.TCoords;
  if(this->TCoords) this->TCoords->Register(this);

  this->CopyScalars = 1;
  this->CopyVectors = 1;
  this->CopyNormals = 1;
  this->CopyTCoords = 1;
}

vlPointData::~vlPointData()
{
  vlPointData::Initialize();
}

// Description:
// Shallow copy of data.
vlPointData& vlPointData::operator=(vlPointData& pd)
{
  this->SetScalars(pd.GetScalars());
  this->SetVectors(pd.GetVectors());
  this->SetNormals(pd.GetNormals());
  this->SetTCoords(pd.GetTCoords());

  this->CopyScalars = pd.CopyScalars;
  this->CopyVectors = pd.CopyVectors;
  this->CopyNormals = pd.CopyNormals;
  this->CopyTCoords = pd.CopyTCoords;

  return *this;
}

// Description:
// Copy the point data from one point to another.
void vlPointData::CopyData(vlPointData* fromPd, int fromId, int toId)
{
  if ( this->CopyScalars && fromPd->Scalars && this->Scalars )
    {
    this->Scalars->InsertScalar(toId,fromPd->Scalars->GetScalar(fromId));
    }

  if ( this->CopyVectors && fromPd->Vectors && this->Vectors )
    {
    this->Vectors->InsertVector(toId,fromPd->Vectors->GetVector(fromId));
    }

  if ( this->CopyNormals && fromPd->Normals && this->Normals )
    {
    this->Normals->InsertNormal(toId,fromPd->Normals->GetNormal(fromId));
    }

  if ( this->CopyTCoords && fromPd->TCoords && this->TCoords )
    {
    this->TCoords->InsertTCoord(toId,fromPd->TCoords->GetTCoord(fromId));
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
  if ( this->Scalars != NULL )
    {
    this->Scalars->UnRegister(this);
    this->Scalars = NULL;
    }

  if ( this->Vectors != NULL )
    {
    this->Vectors->UnRegister(this);
    this->Vectors = NULL;
    }

  if ( this->Normals != NULL )
    {
    this->Normals->UnRegister(this);
    this->Normals = NULL;
    }

  if ( this->TCoords != NULL )
    {
    this->TCoords->UnRegister(this);
    this->TCoords = NULL;
    }
};

// Description:
// Pass entire arrays of input data through to output. Obey the "copy"
// flags.
void vlPointData::PassData(vlPointData* pd)
{
  if ( this->CopyScalars ) this->SetScalars(pd->GetScalars());
  if ( this->CopyVectors ) this->SetVectors(pd->GetVectors());
  if ( this->CopyNormals ) this->SetNormals(pd->GetNormals());
  if ( this->CopyTCoords ) this->SetTCoords(pd->GetTCoords());
}

// Description:
// Allocates point data for point-by-point copy operation.  If sze=0, then 
// use the input PointData to create (i.e., find initial size of) new 
// objects; otherwise use the sze variable.
void vlPointData::CopyAllocate(vlPointData* pd, int sze, int ext)
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

  if ( this->CopyScalars && (s = pd->GetScalars()) ) 
    {
    if ( sze > 0 ) newScalars = s->MakeObject(sze,ext);
    else newScalars = s->MakeObject(s->GetNumberOfScalars());
    this->SetScalars(newScalars);
    }

  if ( this->CopyVectors && (v = pd->GetVectors()) ) 
    {
    if ( sze > 0 ) newVectors = v->MakeObject(sze,ext);
    else newVectors = v->MakeObject(v->GetNumberOfVectors());
    this->SetVectors(newVectors);
    }

  if ( this->CopyNormals && (n = pd->GetNormals()) ) 
    {
    if ( sze > 0 ) newNormals = n->MakeObject(sze,ext);
    else newNormals = n->MakeObject(n->GetNumberOfNormals());
    this->SetNormals(newNormals);
    }

  if ( this->CopyTCoords && (t = pd->GetTCoords()) ) 
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
static vlFloatScalars cellScalars(MAX_CELL_SIZE);
static vlFloatVectors cellVectors(MAX_CELL_SIZE);
static vlFloatNormals cellNormals(MAX_CELL_SIZE);
static vlFloatTCoords cellTCoords(MAX_CELL_SIZE,3);

// Description:
// Initialize point interpolation.
void vlPointData::InterpolateAllocate(vlPointData* pd, int sze, int ext)
{
  this->CopyAllocate(pd, sze, ext);

  if ( pd->TCoords )
    {
    cellTCoords.SetDimension(pd->TCoords->GetDimension());
    }
}

// Description:
// Interpolate data from points and interpolation weights.
void vlPointData::InterpolatePoint(vlPointData *fromPd, int toId, vlIdList *ptIds, float *weights)
{
  int i, j;
  float s, *pv, v[3], *pn, n[3], *ptc, tc[3];

  if ( this->CopyScalars && fromPd->Scalars && this->Scalars )
    {
    fromPd->Scalars->GetScalars(*ptIds, cellScalars);
    for (s=0.0, i=0; i < ptIds->GetNumberOfIds(); i++)
      {
      s += cellScalars.GetScalar(i) * weights[i];
      }
    this->Scalars->InsertScalar(toId,s);
    }

  if ( this->CopyVectors && fromPd->Vectors && this->Vectors )
    {
    fromPd->Vectors->GetVectors(*ptIds, cellVectors);
    for (v[0]=v[1]=v[2]=0.0, i=0; i < ptIds->GetNumberOfIds(); i++)
      {
      pv = cellVectors.GetVector(i);
      v[0] += pv[0]*weights[i];
      v[1] += pv[1]*weights[i];
      v[2] += pv[2]*weights[i];
      }
    this->Vectors->InsertVector(toId,v);
    }

  if ( this->CopyNormals && fromPd->Normals && this->Normals )
    {
    fromPd->Normals->GetNormals(*ptIds, cellNormals);
    for (n[0]=n[1]=n[2]=0.0, i=0; i < ptIds->GetNumberOfIds(); i++)
      {
      pn = cellNormals.GetNormal(i);
      n[0] += pn[0]*weights[i];
      n[1] += pn[1]*weights[i];
      n[2] += pn[2]*weights[i];
      }
    this->Normals->InsertNormal(toId,n);
    }

  if ( this->CopyTCoords && fromPd->TCoords && this->TCoords )
    {
    fromPd->TCoords->GetTCoords(*ptIds, cellTCoords);
    for (tc[0]=tc[1]=tc[2]=0.0, i=0; i < ptIds->GetNumberOfIds(); i++)
      {
      ptc = cellTCoords.GetTCoord(i);
      for (j=0; j<cellTCoords.GetDimension(); j++) tc[j] += ptc[0]*weights[i];
      }
    this->TCoords->InsertTCoord(toId,tc);
    }
}

void vlPointData::NullPoint (int ptId)
{
  static float null[3] = {0.0, 0.0, 0.0};

  if ( this->Scalars )
    {
    this->Scalars->InsertScalar(ptId, 0.0);
    }

  if ( this->Vectors )
    {
    this->Vectors->InsertVector(ptId,null);
    }

  if ( this->Normals )
    {
    this->Normals->InsertNormal(ptId,null);
    }

  if ( this->TCoords )
    {
    this->TCoords->InsertTCoord(ptId,null);
    }

}

void vlPointData::Squeeze()
{
  if ( this->Scalars ) this->Scalars->Squeeze();
  if ( this->Vectors ) this->Vectors->Squeeze();
  if ( this->Normals ) this->Normals->Squeeze();
  if ( this->TCoords ) this->TCoords->Squeeze();
}

// Description:
// Turn on copying of all data.
void vlPointData::CopyAllOn()
{
  this->CopyScalarsOn();
  this->CopyVectorsOn();
  this->CopyNormalsOn();
  this->CopyTCoordsOn();
}

// Description:
// Turn off copying of all data.
void vlPointData::CopyAllOff()
{
  this->CopyScalarsOff();
  this->CopyVectorsOff();
  this->CopyNormalsOff();
  this->CopyTCoordsOff();
}

