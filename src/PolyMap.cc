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
#include "PolyMap.hh"

// Description:
// Construct mapper with vertices, lines, polygons, and triangle strips
// turned on.
vlPolyMapper::vlPolyMapper()
{
  this->Verts = NULL;
  this->Lines = NULL;
  this->Polys = NULL;
  this->Strips = NULL;
  this->Colors = NULL;

  this->VertsVisibility = 1;
  this->LinesVisibility = 1;
  this->PolysVisibility = 1;
  this->StripsVisibility = 1;
}

vlPolyMapper::~vlPolyMapper()
{
  //delete internally created objects
  if ( this->Verts != NULL ) delete this->Verts;
  if ( this->Lines != NULL ) delete this->Lines;
  if ( this->Polys != NULL ) delete this->Polys;
  if ( this->Strips != NULL ) delete this->Strips;
  if ( this->Colors != NULL ) delete this->Colors;
}

void vlPolyMapper::SetInput(vlPolyData *in)
{
  if (in != this->Input )
    {
    vlDebugMacro(<<" setting Input to " << (void *)in);
    this->Input = (vlDataSet *) in;
    this->Modified();
    }
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
  vlScalars *scalars;
  int i;
  vlPolyData *input=(vlPolyData *)this->Input;
  vlColorScalars *colors;
//
// make sure that we've been properly initialized
//
  if ( input == NULL ) 
    {
    vlErrorMacro(<< "No input!");
    return;
    }
  else
    input->Update();

  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();
  this->LookupTable->Build();
//
// Now send data down to primitives and draw it
//
  if ( this->GetMTime() > this->BuildTime || 
  input->GetMTime() > this->BuildTime || 
  this->LookupTable->GetMTime() > this->BuildTime )
    {
//
// create colors
//
    if ( this->ScalarsVisible && (pd=input->GetPointData()) && 
    (scalars=pd->GetScalars()) )
      {
      if ( scalars->GetNumberOfValuesPerScalar() == 1 )
        {
        if ( this->Colors == NULL ) 
          {
          this->Colors = colors = (vlColorScalars *) 
                                new vlAPixmap(input->GetNumberOfPoints());
          }
        else
          {
          colors = this->Colors;
          }

        this->LookupTable->SetTableRange(this->ScalarRange);
        for (i=0; i<input->GetNumberOfPoints(); i++)
          {
          colors->SetColor(i,this->LookupTable->MapValue(scalars->GetScalar(i)));
          }
        }
      else
        {
        colors = (vlColorScalars *)scalars;
        }
      }
    else
      {
      if ( this->Colors ) delete this->Colors;
      this->Colors = colors = NULL;
      }

    if (this->VertsVisibility && input->GetNumberOfVerts())
      {
      if (!this->Verts) this->Verts = ren->GetPrimitive("points");
      this->Verts->Build(input,colors);
      }
    if ( this->LinesVisibility && input->GetNumberOfLines())
      {
      if (!this->Lines) this->Lines = ren->GetPrimitive("lines");
      this->Lines->Build(input,colors);
      }
    if ( this->PolysVisibility && input->GetNumberOfPolys())
      {
      if (!this->Polys) this->Polys = ren->GetPrimitive("polygons");
      this->Polys->Build(input,colors);
      }
    if ( this->StripsVisibility && input->GetNumberOfStrips())
      {
      if (!this->Strips) this->Strips = ren->GetPrimitive("triangle_strips");
      this->Strips->Build(input,colors);
      }

    this->BuildTime.Modified();
    }

  // draw the primitives
  if (this->VertsVisibility && input->GetNumberOfVerts())
    {
    this->Verts->Draw(ren);
    }
  if ( this->LinesVisibility && input->GetNumberOfLines())
    {
    this->Lines->Draw(ren);
    }
  if ( this->PolysVisibility && input->GetNumberOfPolys())
    {
    this->Polys->Draw(ren);
    }
  if ( this->StripsVisibility && input->GetNumberOfStrips())
    {
    this->Strips->Draw(ren);
    }

}

void vlPolyMapper::PrintSelf(ostream& os, vlIndent indent)
{
  vlMapper::PrintSelf(os,indent);

  os << indent << "Vertex Visibility: " << (this->VertsVisibility ? "On\n" : "Off\n");
  os << indent << "Line Visibility: " << (this->LinesVisibility ? "On\n" : "Off\n");
  os << indent << "Polygon Visibility: " << (this->PolysVisibility ? "On\n" : "Off\n");
  os << indent << "Triangle Strip Visibility: " << (this->StripsVisibility ? "On\n" : "Off\n");

}
