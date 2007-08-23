/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLDatabase.cxx

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

#include "vtkSQLDatabase.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSQLDatabase, "1.1");

vtkSQLDatabase::vtkSQLDatabase()
{
}

vtkSQLDatabase::~vtkSQLDatabase()
{
}

void vtkSQLDatabase::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

