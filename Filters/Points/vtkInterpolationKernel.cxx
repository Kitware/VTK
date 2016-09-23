/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolationKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInterpolationKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"

//----------------------------------------------------------------------------
vtkInterpolationKernel::vtkInterpolationKernel()
{
  this->RequiresInitialization = true;

  this->Locator = NULL;
  this->DataSet = NULL;
  this->PointData = NULL;
}

//----------------------------------------------------------------------------
vtkInterpolationKernel::~vtkInterpolationKernel()
{
  this->FreeStructures();
}

//----------------------------------------------------------------------------
void vtkInterpolationKernel::
FreeStructures()
{
  if ( this->Locator )
  {
    this->Locator->Delete();
    this->Locator = NULL;
  }

  if ( this->DataSet )
  {
    this->DataSet->Delete();
    this->DataSet = NULL;
  }

  if ( this->PointData )
  {
    this->PointData->Delete();
    this->PointData = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkInterpolationKernel::
Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds, vtkPointData *attr)
{
  this->FreeStructures();

  if ( loc )
  {
    this->Locator = loc;
    this->Locator->Register(this);
  }

  if ( ds )
  {
    this->DataSet = ds;
    this->DataSet->Register(this);
  }

  if ( attr )
  {
    this->PointData = attr;
    this->PointData->Register(this);
  }
}

//----------------------------------------------------------------------------
void vtkInterpolationKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Requires Initialization: "
     << (this->GetRequiresInitialization() ? "On\n" : "Off\n");

  if ( this->Locator )
  {
    os << indent << "Locator:\n";
    this->Locator->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Locator: (None)\n";
  }

  if ( this->DataSet )
  {
    os << indent << "DataSet:\n";
    this->DataSet->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "DataSet: (None)\n";
  }

  if ( this->PointData )
  {
    os << indent << "PointData:\n";
    this->PointData->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "PointData: (None)\n";
  }
}
