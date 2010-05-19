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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkGraphEdge.h"

#include "vtkObjectFactory.h"

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
