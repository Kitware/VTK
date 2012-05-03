/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignCoordinatesLayoutStrategy.cxx

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

#include "vtkAssignCoordinatesLayoutStrategy.h"

#include "vtkAssignCoordinates.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkAssignCoordinatesLayoutStrategy);

vtkAssignCoordinatesLayoutStrategy::vtkAssignCoordinatesLayoutStrategy()
{
  this->AssignCoordinates = vtkSmartPointer<vtkAssignCoordinates>::New();
}

vtkAssignCoordinatesLayoutStrategy::~vtkAssignCoordinatesLayoutStrategy()
{
}

void vtkAssignCoordinatesLayoutStrategy::SetXCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetXCoordArrayName(name);
}

const char* vtkAssignCoordinatesLayoutStrategy::GetXCoordArrayName()
{
  return this->AssignCoordinates->GetXCoordArrayName();
}

void vtkAssignCoordinatesLayoutStrategy::SetYCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetYCoordArrayName(name);
}

const char* vtkAssignCoordinatesLayoutStrategy::GetYCoordArrayName()
{
  return this->AssignCoordinates->GetYCoordArrayName();
}

void vtkAssignCoordinatesLayoutStrategy::SetZCoordArrayName(const char* name)
{
  this->AssignCoordinates->SetZCoordArrayName(name);
}

const char* vtkAssignCoordinatesLayoutStrategy::GetZCoordArrayName()
{
  return this->AssignCoordinates->GetZCoordArrayName();
}

void vtkAssignCoordinatesLayoutStrategy::Layout()
{
  this->AssignCoordinates->SetInputData(this->Graph);
  this->AssignCoordinates->Update();
  this->Graph->ShallowCopy(this->AssignCoordinates->GetOutput());
}

void vtkAssignCoordinatesLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
