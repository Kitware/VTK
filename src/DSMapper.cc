/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DSMapper.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DSMapper.hh"
#include "PolyMap.hh"

vtkDataSetMapper::vtkDataSetMapper()
{
  this->GeometryExtractor = NULL;
  this->PolyMapper = NULL;
}

vtkDataSetMapper::~vtkDataSetMapper()
{
  // delete internally created objects.
  if ( this->GeometryExtractor ) this->GeometryExtractor->Delete();
  if ( this->PolyMapper ) this->PolyMapper->Delete();
}

void vtkDataSetMapper::SetInput(vtkDataSet *in)
{
  if (in != this->Input )
    {
    this->Input = in;
    this->Modified();
    }
}

//
// Return bounding box of data
//
float *vtkDataSetMapper::GetBounds()
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
void vtkDataSetMapper::Render(vtkRenderer *ren)
{
//
// make sure that we've been properly initialized
//
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input!\n");
    return;
    }
//
// Need a lookup table
//
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();
  this->LookupTable->Build();
//
// Now can create appropriate mapper
//
  if ( this->PolyMapper == NULL ) 
    {
    vtkGeometryFilter *gf = new vtkGeometryFilter();
    vtkPolyMapper *pm = new vtkPolyMapper;
    pm->SetInput(gf);

    this->GeometryExtractor = gf;
    this->PolyMapper = pm;
    }
  
  // update ourselves in case something has changed
  this->PolyMapper->SetLookupTable(this->GetLookupTable());
  this->PolyMapper->SetScalarsVisible(this->GetScalarsVisible());
  this->PolyMapper->SetScalarRange(this->GetScalarRange());

  this->GeometryExtractor->SetInput(this->Input);
  this->PolyMapper->Render(ren);
}

void vtkDataSetMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMapper::PrintSelf(os,indent);

  if ( this->PolyMapper )
    {
    os << indent << "Poly Mapper: (" << this->PolyMapper << ")\n";
    }
  else
    {
    os << indent << "Poly Mapper: (none)\n";
    }

  if ( this->GeometryExtractor )
    {
    os << indent << "Geometry Extractor: (" << this->GeometryExtractor << ")\n";
    }
  else
    {
    os << indent << "Geometry Extractor: (none)\n";
    }
}
