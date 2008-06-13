/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoRepresentation.cxx

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
#include "vtkGeoRepresentation.h"

#include "vtkGeoTerrain.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkGeoRepresentation, "1.1");
vtkStandardNewMacro(vtkGeoRepresentation);
vtkCxxSetObjectMacro(vtkGeoRepresentation, Terrain, vtkGeoTerrain);
//----------------------------------------------------------------------------
vtkGeoRepresentation::vtkGeoRepresentation()
{
  this->Terrain = 0;
}

//----------------------------------------------------------------------------
vtkGeoRepresentation::~vtkGeoRepresentation()
{
  this->SetTerrain(0);
}

//----------------------------------------------------------------------------
void vtkGeoRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Terrain: " << (this->Terrain ? "" : "(null)") << endl;
  if (this->Terrain)
    {
    this->Terrain->PrintSelf(os, indent.GetNextIndent());
    }
}
