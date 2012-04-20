/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphGeodesicPath.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphGeodesicPath.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkExecutive.h"


//-----------------------------------------------------------------------------
vtkGraphGeodesicPath::vtkGraphGeodesicPath()
{
  this->StartVertex = 0;
  this->EndVertex   = 0;
}

//-----------------------------------------------------------------------------
vtkGraphGeodesicPath::~vtkGraphGeodesicPath()
{
}

//-----------------------------------------------------------------------------
void vtkGraphGeodesicPath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "StartVertex: " << this->StartVertex << endl;
  os << indent << "EndVertex: "   << this->EndVertex   << endl;
}

