/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkMapper.h"
#include "vtkLookupTable.h"

// Initialize static member that controls global immediate mode rendering
static int vtkMapperGlobalImmediateModeRendering = 0;

// Construct with initial range (0,1).
vtkMapper::vtkMapper()
{
  this->Colors = NULL;

  this->LookupTable = NULL;

  this->ScalarVisibility = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;

  this->ImmediateModeRendering = 0;

  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
  
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->RenderTime = 0.0;
}

vtkMapper::~vtkMapper()
{
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  if ( this->Colors != NULL )
    {
    this->Colors->Delete();
    }
}

// Get the bounds for the input of this mapper as 
// (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkMapper::GetBounds()
{
  static float bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  if ( ! this->GetInput() ) 
    {
    return bounds;
    }
  else
    {
    this->GetInput()->Update();
    this->GetInput()->GetBounds(this->Bounds);
    return this->Bounds;
    }
}

vtkDataSet *vtkMapper::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}

void vtkMapper::SetGlobalImmediateModeRendering(int val)
{
  if (val == vtkMapperGlobalImmediateModeRendering)
    {
    return;
    }
  vtkMapperGlobalImmediateModeRendering = val;
}

int vtkMapper::GetGlobalImmediateModeRendering()
{
  return vtkMapperGlobalImmediateModeRendering;
}

// Overload standard modified time function. If lookup table is modified,
// then this object is modified as well.
unsigned long vtkMapper::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long lutMTime;

  if ( this->LookupTable != NULL )
    {
    lutMTime = this->LookupTable->GetMTime();
    mTime = ( lutMTime > mTime ? lutMTime : mTime );
    }

  return mTime;
}

void vtkMapper::ShallowCopy(vtkMapper *m)
{
  this->SetLookupTable(m->GetLookupTable());

  this->SetScalarVisibility(m->GetScalarVisibility());
  this->SetScalarRange(m->GetScalarRange());
  this->SetColorMode(m->GetColorMode());
  this->SetScalarMode(m->GetScalarMode());
  this->SetImmediateModeRendering(m->GetImmediateModeRendering());
}

// a side effect of this is that this->Colors is also set
// to the return value
vtkScalars *vtkMapper::GetColors()
{
  vtkScalars *scalars;
  
  // make sure we have an input
  if (!this->GetInput())
    {
    return NULL;
    }
    
  // get and scalar data according to scalar mode
  if ( this->ScalarMode == VTK_SCALAR_MODE_DEFAULT )
    {
    scalars = this->GetInput()->GetPointData()->GetScalars();
    if (!scalars)
      {
      scalars = this->GetInput()->GetCellData()->GetScalars();
      }
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_DATA )
    {
    scalars = this->GetInput()->GetPointData()->GetScalars();
    }
  else
    {
    scalars = this->GetInput()->GetCellData()->GetScalars();
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

    // Setup mapper/scalar object for color generation
    this->LookupTable->SetRange(this->ScalarRange);
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
void vtkMapper::SetLookupTable(vtkScalarsToColors *lut)
{
  if ( this->LookupTable != lut ) 
    {
    if ( this->LookupTable) 
      {
      this->LookupTable->UnRegister(this);
      }
    this->LookupTable = lut;
    if (lut)
      {
      lut->Register(this);
      }
    this->Modified();
    }
}

vtkScalarsToColors *vtkMapper::GetLookupTable()
{
  if ( this->LookupTable == NULL )
    {
    this->CreateDefaultLookupTable();
    }
  return this->LookupTable;
}

void vtkMapper::CreateDefaultLookupTable()
{
  if ( this->LookupTable) 
    {
    this->LookupTable->UnRegister(this);
    }
  this->LookupTable = vtkLookupTable::New();
}


// Update the network connected to this mapper.
void vtkMapper::Update()
{
  if ( this->GetInput() )
    {
    this->GetInput()->Update();
    }
}


// Return the method of coloring scalar data.
char *vtkMapper::GetColorModeAsString(void)
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

// Return the method for obtaining scalar data.
char *vtkMapper::GetScalarModeAsString(void)
{
  if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
    {
    return "UseCellData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_DATA ) 
    {
    return "UsePointData";
    }
  else 
    {
    return "Default";
    }
}


void vtkMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkAbstractMapper3D::PrintSelf(os,indent);

  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
    }

  os << indent << "Immediate Mode Rendering: " 
    << (this->ImmediateModeRendering ? "On\n" : "Off\n");
  os << indent << "Global Immediate Mode Rendering: " << 
    (vtkMapperGlobalImmediateModeRendering ? "On\n" : "Off\n");
  os << indent << "Scalar Visibility: " 
    << (this->ScalarVisibility ? "On\n" : "Off\n");

  float *range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";
  
  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;

  os << indent << "Scalar Mode: " << this->GetScalarModeAsString() << endl;

  os << indent << "RenderTime: " << this->RenderTime << endl;

}

