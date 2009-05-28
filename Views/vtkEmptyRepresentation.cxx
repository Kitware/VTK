/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEmptyRepresentation.cxx

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

#include "vtkEmptyRepresentation.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkEmptyRepresentation, "1.1");
vtkStandardNewMacro(vtkEmptyRepresentation);


vtkEmptyRepresentation::vtkEmptyRepresentation()
{
  this->SetNumberOfInputPorts(0);
}

vtkEmptyRepresentation::~vtkEmptyRepresentation()
{
}


void vtkEmptyRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
