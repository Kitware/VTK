/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkPolyDataMapper2D.h"

#ifdef _WIN32
  #include "vtkWin32PolyDataMapper2D.h"
#else
  #include "vtkXPolyDataMapper2D.h"
#endif

vtkPolyDataMapper2D::vtkPolyDataMapper2D()
{
  this->Input = NULL;
  this->Colors = NULL;

  this->LookupTable = NULL;

  this->ScalarVisibility = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;

  this->ColorMode = VTK_COLOR_MODE_DEFAULT;
}

vtkPolyDataMapper2D::~vtkPolyDataMapper2D()
{  
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister(this);
    }
  if ( this->Colors != NULL ) this->Colors->Delete();
}

vtkPolyDataMapper2D *vtkPolyDataMapper2D::New()
{
#ifdef _WIN32
    return vtkWin32PolyDataMapper2D::New();
#else
    return vtkXPolyDataMapper2D::New();
#endif

}


// Description:
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
  if (!this->Input) return NULL;
    
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

// Description:
// Specify a lookup table for the mapper to use.
void vtkPolyDataMapper2D::SetLookupTable(vtkLookupTable *lut)
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

vtkLookupTable *vtkPolyDataMapper2D::GetLookupTable()
{
  if ( this->LookupTable == NULL ) this->CreateDefaultLookupTable();
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

// Description:
// Return the method of coloring scalar data.
char *vtkPolyDataMapper2D::GetColorModeAsString(void)
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
  
  os << indent << "Color Mode: " << this->GetColorModeAsString() << endl;
}






