/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPolyDataMapper2D.h"
#include "vtkImagingFactory.h"

#include "vtkLookupTable.h"

vtkPolyDataMapper2D::vtkPolyDataMapper2D()
{
  this->Input = NULL;
  this->Colors = NULL;

  this->LookupTable = NULL;

  this->ScalarVisibility = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;
  this->UseLookupTableScalarRange = 0;

  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  
  this->TransformCoordinate = NULL;
}

void vtkPolyDataMapper2D::ShallowCopy(vtkPolyDataMapper2D *m)
{
  this->SetLookupTable(m->GetLookupTable());
  this->SetClippingPlanes(m->GetClippingPlanes());
  this->SetColorMode(m->GetColorMode());
  this->SetScalarVisibility(m->GetScalarVisibility());
  this->SetScalarRange(m->GetScalarRange());
  this->SetTransformCoordinate(m->GetTransformCoordinate());
}

vtkPolyDataMapper2D::~vtkPolyDataMapper2D()
{  
  if (this->TransformCoordinate)
    {
    this->TransformCoordinate->UnRegister(this);
    }
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  if ( this->Colors != NULL )
    {
    this->Colors->Delete();
    }
  
  this->SetInput(NULL);
}

vtkPolyDataMapper2D *vtkPolyDataMapper2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkImagingFactory::CreateInstance("vtkPolyDataMapper2D");
  return (vtkPolyDataMapper2D*)ret;
}


// Overload standard modified time function. If lookup table is modified,
// then this object is modified as well.
unsigned long vtkPolyDataMapper2D::GetMTime()
{
  unsigned long mTime = this->MTime;
  unsigned long lutMTime;

  if ( this->LookupTable != NULL )
    {
    lutMTime = this->LookupTable->GetMTime();
    mTime = ( lutMTime > mTime ? lutMTime : mTime );
    }

  return mTime;
}

// a side effect of this is that this->Colors is also set
// to the return value
vtkScalars *vtkPolyDataMapper2D::GetColors()
{
  vtkScalars *scalars;
  
  // make sure we have an input
  if (!this->Input)
    {
    return NULL;
    }
    
  // get point data and scalars
  scalars = this->Input->GetPointData()->GetScalars();
  // if we don;t have point data scalars, try cell data
  if (!scalars)
    {
    scalars = this->Input->GetCellData()->GetScalars();
    }
  
  // do we have any scalars ?
  if (scalars && this->ScalarVisibility)
    {
    // if the scalars have a lookup table use it instead
    if (scalars->GetLookupTable())
      {
      this->SetLookupTable(scalars->GetLookupTable());
      }
    else
      {
      // make sure we have a lookup table
      if ( this->LookupTable == NULL )
      	{
      	this->CreateDefaultLookupTable();
      	}
      this->LookupTable->Build();
      }

    if (!this->UseLookupTableScalarRange)
      {
      this->LookupTable->SetRange(this->ScalarRange);
      }

    if (this->Colors)
      {
      this->Colors->Delete();
      }
    this->Colors = scalars;
    this->Colors->Register(this);
    this->Colors->InitColorTraversal(1.0, this->LookupTable, this->ColorMode);
    }

  else //scalars not visible
    {
    if ( this->Colors )
      {
      this->Colors->Delete();
      }
    this->Colors = NULL;
    }
  
  return this->Colors;
}

// Specify a lookup table for the mapper to use.
void vtkPolyDataMapper2D::SetLookupTable(vtkScalarsToColors *lut)
{
  if ( this->LookupTable != lut ) 
    {
    if (lut)
      {
      lut->Register(this);
      }
    if ( this->LookupTable ) 
      {
      this->LookupTable->UnRegister(this);
      }
    this->LookupTable = lut;
    this->Modified();
    }
}

vtkScalarsToColors *vtkPolyDataMapper2D::GetLookupTable()
{
  if ( this->LookupTable == NULL )
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

void vtkPolyDataMapper2D::CreateDefaultLookupTable()
{
  if ( this->LookupTable ) 
    {
    this->LookupTable->UnRegister(this);
    }
  this->LookupTable = vtkLookupTable::New();
}

// Return the method of coloring scalar data.
const char *vtkPolyDataMapper2D::GetColorModeAsString(void)
{
  if ( this->ColorMode == VTK_COLOR_MODE_LUMINANCE )
    {
    return "Luminance";
    }
  else if ( this->ColorMode == VTK_COLOR_MODE_MAP_SCALARS ) 
    {
    return "MapScalars";
    }
  else 
    {
    return "Default";
    }
}

void vtkPolyDataMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMapper2D::PrintSelf(os,indent);

  if ( this->Input )
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }

  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
    }
  os << indent << "Scalar Visibility: " 
    << (this->ScalarVisibility ? "On\n" : "Off\n");

  float *range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";
  os << indent << "UseLookupTableScalarRange: " << this->UseLookupTableScalarRange << "\n";
  
  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;

  if ( this->TransformCoordinate )
    {
    os << indent << "Transform Coordinate: " 
       << this->TransformCoordinate << "\n";
    this->TransformCoordinate->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "No Transform Coordinate\n";
    }
}






