/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkMapper.h"
#include "vtkLookupTable.h"

// Initialize static member that controls global immediate mode rendering
static int vtkMapperGlobalImmediateModeRendering = 0;

// Initialize static member that controls global coincidence resolution
static int vtkMapperGlobalResolveCoincidentTopology = VTK_RESOLVE_OFF;
static double vtkMapperGlobalResolveCoincidentTopologyZShift = 0.01;
static float vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor = 1.0;
static float vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetUnits = 1.0;

// Construct with initial range (0,1).
vtkMapper::vtkMapper()
{
  this->Colors = NULL;
  this->Scalars = NULL;

  this->LookupTable = NULL;

  this->ScalarVisibility = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;
  this->UseLookupTableScalarRange = 0;

  this->ImmediateModeRendering = 0;

  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
  this->ScalarMode = VTK_SCALAR_MODE_DEFAULT;
  
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = -1.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;

  this->RenderTime = 0.0;
  
  strcpy(this->ArrayName, "");
  this->ArrayId = -1;
  this->ArrayComponent = 0;
  this->ArrayAccessMode = VTK_GET_ARRAY_BY_ID;
}

vtkMapper::~vtkMapper()
{
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  if ( this->Colors != NULL )
    {
    this->Colors->UnRegister(this);
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
    this->Update();
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

void vtkMapper::SetResolveCoincidentTopology(int val)
{
  if (val == vtkMapperGlobalResolveCoincidentTopology)
    {
    return;
    }
  vtkMapperGlobalResolveCoincidentTopology = val;
}

int vtkMapper::GetResolveCoincidentTopology()
{
  return vtkMapperGlobalResolveCoincidentTopology;
}

void vtkMapper::SetResolveCoincidentTopologyToDefault()
{
  vtkMapperGlobalResolveCoincidentTopology = VTK_RESOLVE_OFF;
}

void vtkMapper::SetResolveCoincidentTopologyZShift(double val)
{
  if (val == vtkMapperGlobalResolveCoincidentTopologyZShift)
    {
    return;
    }
  vtkMapperGlobalResolveCoincidentTopologyZShift = val;
}

double vtkMapper::GetResolveCoincidentTopologyZShift()
{
  return vtkMapperGlobalResolveCoincidentTopologyZShift;
}

void vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(
                                            float factor, float units)
{
  if (factor == vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor &&
      units == vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetUnits )
    {
    return;
    }
  vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor = factor;
  vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor = units;
}

void vtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(
                           float& factor, float& units)
{
  factor = vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetFactor;
  units = vtkMapperGlobalResolveCoincidentTopologyPolygonOffsetUnits;
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

void vtkMapper::ShallowCopy(vtkAbstractMapper *mapper)
{
  vtkMapper *m = vtkMapper::SafeDownCast(mapper);
  if ( m != NULL )
    {
    this->SetLookupTable(m->GetLookupTable());
    this->SetScalarVisibility(m->GetScalarVisibility());
    this->SetScalarRange(m->GetScalarRange());
    this->SetColorMode(m->GetColorMode());
    this->SetScalarMode(m->GetScalarMode());
    this->SetImmediateModeRendering(m->GetImmediateModeRendering());
    this->SetUseLookupTableScalarRange(m->GetUseLookupTableScalarRange());
    this->ColorByArrayComponent(m->GetArrayName(),m->GetArrayComponent());
    this->ColorByArrayComponent(m->GetArrayId(),m->GetArrayComponent());
    }

  // Now do superclass
  this->vtkAbstractMapper3D::ShallowCopy(mapper);

}

vtkScalars *vtkMapper::GetColors()
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

// a side effect of this is that this->Colors is also set
// to the return value
vtkUnsignedCharArray *vtkMapper::MapScalars(float alpha)
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

void vtkMapper::ColorByArrayComponent(int arrayNum, int component)
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

void vtkMapper::ColorByArrayComponent(char* arrayName, int component)
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
const char *vtkMapper::GetColorModeAsString(void)
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

// Return the method for obtaining scalar data.
const char *vtkMapper::GetScalarModeAsString(void)
{
  if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_DATA )
    {
    return "UseCellData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_DATA ) 
    {
    return "UsePointData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA )
    {
    return "UsePointFieldData";
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    return "UseCellFieldData";
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

  os << indent << "UseLookupTableScalarRange: " 
     << this->UseLookupTableScalarRange << "\n";

  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;

  os << indent << "Scalar Mode: " << this->GetScalarModeAsString() << endl;

  os << indent << "RenderTime: " << this->RenderTime << endl;

  os << indent << "Resolve Coincident Topology: ";
  if ( vtkMapperGlobalResolveCoincidentTopology == VTK_RESOLVE_OFF )
    {
    os << "Off" << endl;
    }
  else if ( vtkMapperGlobalResolveCoincidentTopology == VTK_RESOLVE_POLYGON_OFFSET )
    {
    os << "Polygon Offset" << endl;
    }
  else
    {
    os << "Shift Z-Buffer" << endl;
    }
}
