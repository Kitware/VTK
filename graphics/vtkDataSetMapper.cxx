/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDataSetMapper.h"
#include "vtkPolyMapper.h"

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
void vtkDataSetMapper::Render(vtkRenderer *ren, vtkActor *act)
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
    pm->SetInput(gf->GetOutput());

    this->GeometryExtractor = gf;
    this->PolyMapper = pm;
    }
//
// For efficiency: if input type is vtkPolyData, there's no need to pass it thru
// the geometry filter.
//
  if ( ! strcmp(this->Input->GetDataType(),"vtkPolyData") )
    {
    this->PolyMapper->SetInput((vtkPolyData *)this->Input);
    }
  else
    {
    this->GeometryExtractor->SetInput(this->Input);
    this->PolyMapper->SetInput(this->GeometryExtractor->GetOutput());
    }
  
  // update ourselves in case something has changed
  this->PolyMapper->SetLookupTable(this->GetLookupTable());
  this->PolyMapper->SetScalarsVisible(this->GetScalarsVisible());
  this->PolyMapper->SetScalarRange(this->GetScalarRange());

  this->PolyMapper->Render(ren,act);
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
