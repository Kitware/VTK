/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkMapper.h"

// Initialize static member that controls global immediate mode rendering
static int vtkMapperGlobalImmediateModeRendering = 0;

// Construct with initial range (0,1).
vtkMapper::vtkMapper()
{
  this->Input = NULL;
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
  if ( this->Colors != NULL ) this->Colors->Delete();
  if (this->Input)
    {
    this->Input->UnRegister(this);
    this->Input = NULL;
    }
}

void vtkMapper::SetGlobalImmediateModeRendering(int val)
{
  if (val == vtkMapperGlobalImmediateModeRendering) return;
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

void vtkMapper::operator=(const vtkMapper& m)
{
  this->SetLookupTable(m.LookupTable);

  this->SetScalarVisibility(m.ScalarVisibility);
  this->SetScalarRange(m.ScalarRange[0], m.ScalarRange[1]);
}

// a side effect of this is that this->Colors is also set
// to the return value
vtkScalars *vtkMapper::GetColors()
{
  vtkScalars *scalars;
  
  // make sure we have an input
  if (!this->Input) return NULL;
    
  // get and scalar data according to scalar mode
  if ( this->ScalarMode == VTK_SCALAR_MODE_DEFAULT )
    {
    scalars = this->Input->GetPointData()->GetScalars();
    if (!scalars) scalars = this->Input->GetCellData()->GetScalars();
    }
  else if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_DATA )
    {
    scalars = this->Input->GetPointData()->GetScalars();
    }
  else
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
      if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();
      this->LookupTable->Build();
      }

    // Setup mapper/scalar object for color generation
    this->LookupTable->SetTableRange(this->ScalarRange);
    if (this->Colors) this->Colors->Delete();
    this->Colors = scalars;
    this->Colors->Register(this);
    this->Colors->InitColorTraversal(1.0, this->LookupTable, this->ColorMode);
    }

  else //scalars not visible
    {
    if ( this->Colors ) this->Colors->Delete();
    this->Colors = NULL;
    }
  
  return this->Colors;
}

// Specify a lookup table for the mapper to use.
void vtkMapper::SetLookupTable(vtkLookupTable *lut)
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

vtkLookupTable *vtkMapper::GetLookupTable()
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();
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

// Get the bounds for this Prop as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
void vtkMapper::GetBounds(float bounds[6])
{
  this->GetBounds();
  for (int i=0; i<6; i++) bounds[i] = this->Bounds[i];
}

float *vtkMapper::GetCenter()
{
  this->GetBounds();
  for (int i=0; i<3; i++) 
    {
    this->Center[i] = (this->Bounds[2*i+1] + this->Bounds[2*i]) / 2.0;
    }
  return this->Center;
}

float vtkMapper::GetLength()
{
  double diff, l=0.0;
  int i;

  this->GetBounds();
  for (i=0; i<3; i++)
    {
    diff = this->Bounds[2*i+1] - this->Bounds[2*i];
    l += diff * diff;
    }
 
  return (float)sqrt(l);
}

// Update the network connected to this mapper.
void vtkMapper::Update()
{
  if ( this->Input )
    {
    this->Input->Update();
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
  vtkProcessObject::PrintSelf(os,indent);

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

