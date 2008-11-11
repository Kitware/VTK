/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeRingLayoutStrategy.cxx

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

#include "vtkTreeRingLayoutStrategy.h"

vtkCxxRevisionMacro(vtkTreeRingLayoutStrategy, "1.3");

vtkTreeRingLayoutStrategy::vtkTreeRingLayoutStrategy()
{
  this->InteriorRadius = 6.0;
  this->RingThickness = 1.0;
  this->RootStartAngle = 0.;
  this->RootEndAngle = 360.;
  this->UseRectangularCoordinates = false;
}

vtkTreeRingLayoutStrategy::~vtkTreeRingLayoutStrategy()
{
}

void vtkTreeRingLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
 os << "InteriorRadius: " << this->InteriorRadius << endl;
 os << "RingThickness: " << this->RingThickness << endl;
 os << "RootStartAngle: " << this->RootStartAngle << endl;
 os << "RootEndAngle: " << this->RootEndAngle << endl;
 os << "UseRectangularCoordinates: " << this->UseRectangularCoordinates << endl;
}


