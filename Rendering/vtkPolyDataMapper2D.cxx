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
  this->Scalars = NULL;

  this->LookupTable = NULL;

  this->ScalarVisibility = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;
  this->UseLookupTableScalarRange = 0;

  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
  
  this->TransformCoordinate = NULL;

  strcpy(this->ArrayName, "");
  this->ArrayId = -1;
  this->ArrayComponent = 0;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
}

void vtkPolyDataMapper2D::ShallowCopy(vtkAbstractMapper *mapper)
{
  vtkPolyDataMapper2D *m = vtkPolyDataMapper2D::SafeDownCast(mapper);
  if ( m != NULL )
    {
    this->SetLookupTable(m->GetLookupTable());
    this->SetScalarVisibility(m->GetScalarVisibility());
    this->SetScalarRange(m->GetScalarRange());
    this->SetColorMode(m->GetColorMode());
    this->SetScalarMode(m->GetScalarMode());
    this->SetUseLookupTableScalarRange(m->GetUseLookupTableScalarRange());
    this->ColorByArrayComponent(m->GetArrayName(),m->GetArrayComponent());
    this->ColorByArrayComponent(m->GetArrayId(),m->GetArrayComponent());
    this->SetTransformCoordinate(m->GetTransformCoordinate());
    }

  // Now do superclass
  this->vtkMapper2D::ShallowCopy(mapper);

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
vtkUnsignedCharArray *vtkPolyDataMapper2D::MapScalars(float alpha)
{
  // Get rid of old colors
  if ( this->Colors )
    {
    this->Colors->UnRegister(this);
    this->Colors = NULL;
    }
  
  // map scalars if necessary
  if ( this->ScalarVisibility )
    {
    vtkDataArray *scalars = vtkAbstractMapper::
      GetScalars(this->GetInput(), this->ScalarMode, this->ArrayAccessMode,
                 this->ArrayId, this->ArrayName, this->ArrayComponent);
    if ( scalars )
      {
      if ( scalars->GetLookupTable() )
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
      if ( !this->UseLookupTableScalarRange )
        {
        this->LookupTable->SetRange(this->ScalarRange);
        }
      this->LookupTable->SetAlpha(alpha);
      this->Colors = this->LookupTable->
        MapScalars(scalars, this->ColorMode, this->ArrayComponent);
      }
    }

  return this->Colors;
}

// a side effect of this is that this->Colors is also set
// to the return value
vtkScalars *vtkPolyDataMapper2D::GetColors()
{
  VTK_LEGACY_METHOD("GetColors", "4.0");
  vtkUnsignedCharArray *scalars = this->MapScalars(1.0);

  if ( scalars == NULL )
    {
    return NULL;
    }

  if ( this->Scalars == NULL )
    {
    this->Scalars = vtkScalars::New();
    }
  int numScalars = scalars->GetNumberOfTuples();
  this->Scalars->SetNumberOfScalars(numScalars);
  this->Scalars->SetNumberOfComponents(4);
  this->Scalars->SetData(scalars);
  this->Scalars->InitColorTraversal(1.0,this->LookupTable,this->ColorMode);
  
  return this->Scalars;
}

void vtkPolyDataMapper2D::ColorByArrayComponent(int arrayNum, int component)
{
  if (this->ArrayId == arrayNum && component == this->ArrayComponent &&
      this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
    return;
    }
  this->Modified();
  
  this->ArrayId = arrayNum;
  this->ArrayComponent = component;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
}

void vtkPolyDataMapper2D::ColorByArrayComponent(char* arrayName, int component)
{
  if (strcmp(this->ArrayName, arrayName) == 0 &&
      component == this->ArrayComponent &&
      this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
    {
    return;
    }
  this->Modified();
  
  strcpy(this->ArrayName, arrayName);
  this->ArrayComponent = component;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_NAME;
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
  if ( this->ColorMode == VTK_COLOR_MODE_MAP_SCALARS ) 
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






