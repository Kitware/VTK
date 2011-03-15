/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageProperty.h"

#include "vtkObjectFactory.h"
#include "vtkLookupTable.h"

vtkStandardNewMacro(vtkImageProperty);

vtkCxxSetObjectMacro(vtkImageProperty, LookupTable, vtkScalarsToColors);

//----------------------------------------------------------------------------
// Construct a new vtkImageProperty with default values
vtkImageProperty::vtkImageProperty()
{
  this->ColorWindow = 255.0;
  this->ColorLevel = 127.5;

  this->LookupTable = NULL;
  this->UseLookupTableScalarRange = 0;

  this->Opacity = 1.0;
  this->Ambient = 1.0;
  this->Diffuse = 0.0;

  this->InterpolationType = VTK_LINEAR_INTERPOLATION;
}

//----------------------------------------------------------------------------
// Destruct a vtkImageProperty
vtkImageProperty::~vtkImageProperty()
{
  if (this->LookupTable != NULL)
    {
    this->LookupTable->Delete();
    }
}

//----------------------------------------------------------------------------
const char *vtkImageProperty::GetInterpolationTypeAsString()
{
  switch (this->InterpolationType)
    {
    case VTK_NEAREST_INTERPOLATION:
      return "Nearest";
    case VTK_LINEAR_INTERPOLATION:
      return "Linear";
    case VTK_CUBIC_INTERPOLATION:
      return "Cubic";
    }
  return "";
}

//----------------------------------------------------------------------------
void vtkImageProperty::DeepCopy(vtkImageProperty *p)
{
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned long int vtkImageProperty::GetMTime()
{
  unsigned long mTime = this->vtkObject::GetMTime();
  unsigned long time;

  if (this->LookupTable)
    {
    time = this->LookupTable->GetMTime();
    mTime = (mTime > time ? mTime : time);
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImageProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ColorWindow: " << this->ColorWindow << "\n";
  os << indent << "ColorLevel: " << this->ColorLevel << "\n";
  os << indent << "UseLookupTableScalarRange: "
     << (this->UseLookupTableScalarRange ? "On\n" : "Off\n");
  os << indent << "LookupTable: " << this->LookupTable << "\n";
  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Ambient: " << this->Ambient << "\n";
  os << indent << "Diffuse: " << this->Diffuse << "\n";
  os << indent << "InterpolationType: "
     << this->GetInterpolationTypeAsString() << "\n";
}
