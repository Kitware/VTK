/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextDevice2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkContextDevice2D.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkContextDevice2D, "1.1");
//-----------------------------------------------------------------------------
//vtkStandardNewMacro(vtkContextDevice2D);

vtkContextDevice2D::vtkContextDevice2D()
{
  this->Geometry[0] = 0;
  this->Geometry[1] = 0;
}

//-----------------------------------------------------------------------------
vtkContextDevice2D::~vtkContextDevice2D()
{
}

//-----------------------------------------------------------------------------
void vtkContextDevice2D::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
