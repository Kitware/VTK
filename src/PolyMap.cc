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
  this->Input = NULL;
  this->Verts = NULL;
  this->Lines = NULL;
  this->Polys = NULL;
  this->Strips = NULL;
  this->VertsVisibility = 1;
  this->LinesVisibility = 1;
  this->PolysVisibility = 1;
  this->StripsVisibility = 1;
}

vlPolyMapper::~vlPolyMapper()
{
  if ( this->Input != NULL )
    {
    this->Input->UnRegister(this);
    }
}

void vlPolyMapper::SetInput(vlPolyData *in)
{
  if (in != this->Input )
    {
    this->Input = in;
    this->Input->Register(this);
    this->Modified();
    }
}
vlPolyData* vlPolyMapper::GetInput()
{
  return this->Input;
}
//
// Return bounding box of data
//
float *vlPolyMapper::GetBounds()
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
    {
    vlErrorMacro(<< "No input!\n");
    return;
    }
  else
    this->Input->Update();

  if ( ! this->LookupTable ) this->LookupTable = new vlLookupTable;
  this->LookupTable->Build();

//
// Now send data down to primitives and draw it
//
  if ( forceBuild || this->Input->GetMTime() > this->BuildTime || 
  this->LookupTable->GetMTime() > this->BuildTime )
    {
//
// create colors
//
    if ( this->ScalarsVisible && (pd=this->Input->GetPointData()) && 
    (scalars=pd->GetScalars()) )
      {
      colors = new vlRGBArray;
      colors->Allocate (this->Input->GetNumberOfPoints());

      this->LookupTable->SetTableRange(this->ScalarRange);
      for (i=0; i<this->Input->GetNumberOfPoints(); i++)
        {
        colors->SetColor(i,this->LookupTable->MapValue(scalars->GetScalar(i)));
        }
      }
    else
      {
      colors = NULL;
      }

    if (this->VertsVisibility && this->Input->GetNumberOfVerts())
      {
      if (!this->Verts) this->Verts = ren->GetPrimitive("points");
      this->Verts->Build(this->Input,colors);
      }
    if ( this->LinesVisibility && this->Input->GetNumberOfLines())
      {
      if (!this->Lines) this->Lines = ren->GetPrimitive("lines");
      this->Lines->Build(this->Input,colors);
      }
    if ( this->PolysVisibility && this->Input->GetNumberOfPolys())
      {
      if (!this->Polys) this->Polys = ren->GetPrimitive("polygons");
      this->Polys->Build(this->Input,colors);
      }
    if ( this->StripsVisibility && this->Input->GetNumberOfStrips())
      {
      if (!this->Strips) this->Strips = ren->GetPrimitive("triangle_strips");
      this->Strips->Build(this->Input,colors);
      }

    this->BuildTime.Modified();
    }

  // draw the primitives
  if (this->VertsVisibility && this->Input->GetNumberOfVerts())
    {
    this->Verts->Draw(ren);
    }
  if ( this->LinesVisibility && this->Input->GetNumberOfLines())
    {
    this->Lines->Draw(ren);
    }
  if ( this->PolysVisibility && this->Input->GetNumberOfPolys())
    {
    this->Polys->Draw(ren);
    }
  if ( this->StripsVisibility && this->Input->GetNumberOfStrips())
    {
    this->Strips->Draw(ren);
    }

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

    os << indent << "Vertex Visibility: " << (this->VertsVisibility ? "On\n" : "Off\n");
    os << indent << "Line Visibility: " << (this->LinesVisibility ? "On\n" : "Off\n");
    os << indent << "Polygon Visibility: " << (this->PolysVisibility ? "On\n" : "Off\n");
    os << indent << "Triangle Strip Visibility: " << (this->StripsVisibility ? "On\n" : "Off\n");

   }
}
