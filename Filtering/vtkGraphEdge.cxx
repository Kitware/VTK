/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphEdge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkGraphEdge.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkGraphEdge, "1.1");
vtkStandardNewMacro(vtkGraphEdge);
//----------------------------------------------------------------------------
vtkGraphEdge::vtkGraphEdge()
{
  this->Source = 0;
  this->Target = 0;
  this->Id = 0;
}

//----------------------------------------------------------------------------
vtkGraphEdge::~vtkGraphEdge()
{
}

//----------------------------------------------------------------------------
void vtkGraphEdge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << this->Source << endl;
  os << indent << "Target: " << this->Target << endl;
  os << indent << "Id: " << this->Id << endl;
}
