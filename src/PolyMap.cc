/*=========================================================================

  Program:   Visualization Library
  Module:    PolyMap.cc
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
#include "PolyMap.hh"

vlPolyMapper::vlPolyMapper()
{
  this->Input = 0;
  this->Verts = 0;
  this->Lines = 0;
  this->Polys = 0;
  this->Strips = 0;
  this->VertsVisibility = 1;
  this->LinesVisibility = 1;
  this->PolysVisibility = 1;
  this->StripsVisibility = 1;
}

vlPolyMapper::~vlPolyMapper()
{
  if ( this->Input != 0 )
    {
    this->Input->UnRegister((void *)this);
    }
}

void vlPolyMapper::SetInput(vlPolyData *in)
{
  if (in != this->Input )
    {
    this->Input = in;
    this->Input->Register((void *)this);
    this->Modified();
    }
}
vlPolyData* vlPolyMapper::GetInput()
{
  return this->Input;
}

//
// Receives from Actor -> maps data to primitives
//
void vlPolyMapper::Render(vlRenderer *ren)
{
  vlPointData *pd;
  vlRGBArray *colors;
  vlScalars *scalars;
  int i;
  char forceBuild = 0;
//
// make sure that we've been properly initialized
//
  if ( ! this->Input ) 
    return;
  else
    this->Input->Update();

  if ( ! this->LookupTable ) this->LookupTable = new vlLookupTable;
  this->LookupTable->Build();

  if ( ! this->Polys )
    {
    forceBuild = 1;
    this->Verts = ren->GetPrimitive("points");
    this->Lines = ren->GetPrimitive("lines");
    this->Polys = ren->GetPrimitive("polygons");
    this->Strips = ren->GetPrimitive("triangle_strips");
    }
//
// Now send data down to primitives and draw it
//
  if ( forceBuild || this->Input->GetMtime() > this->BuildTime || 
  this->LookupTable->GetMtime() > this->BuildTime )
    {
//
// create colors
//
    if ( this->ScalarsVisible && (pd=this->Input->GetPointData()) && 
    (scalars=pd->GetScalars()) )
      {
      colors = new vlRGBArray;
      colors->Initialize (this->Input->NumPoints());

      this->LookupTable->SetTableRange(this->ScalarRange);
      for (i=0; i<this->Input->NumPoints(); i++)
        {
        colors->SetColor(i,this->LookupTable->MapValue(scalars->GetScalar(i)));
        }
      }
    else
      {
      colors = 0;
      }
//
//  Cause primitives to build themselves
//
    this->Verts->Build(this->Input,colors);
    this->Lines->Build(this->Input,colors);
    this->Polys->Build(this->Input,colors);
    this->Strips->Build(this->Input,colors);
    this->BuildTime.Modified();
    }

  if ( this->VertsVisibility ) this->Verts->Draw(ren);
  if ( this->LinesVisibility ) this->Lines->Draw(ren);
  if ( this->PolysVisibility ) this->Polys->Draw(ren);
  if ( this->StripsVisibility ) this->Strips->Draw(ren);

}

void vlPolyMapper::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPolyMapper::GetClassName()))
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

    if ( this->VertsVisibility )
      {
      os << indent << "Verts are visible.\n";
      }
    else
      {
      os << indent << "Verts are not visible.\n";
      }

    if ( this->LinesVisibility )
      {
      os << indent << "Lines are visible.\n";
      }
    else
      {
      os << indent << "Lines are not visible.\n";
      }

    if ( this->PolysVisibility )
      {
      os << indent << "Polygons are visible.\n";
      }
    else
      {
      os << indent << "Polygons are not visible.\n";
      }

    if ( this->StripsVisibility )
      {
      os << indent << "Triangle strips are visible.\n";
      }
    else
      {
      os << indent << "Triangle strips are not visible.\n";
      }
   }
}
