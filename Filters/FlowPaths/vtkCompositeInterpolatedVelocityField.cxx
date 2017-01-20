/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeInterpolatedVelocityField.h"

#include "vtkMath.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkGenericCell.h"
#include "vtkObjectFactory.h"


vtkCompositeInterpolatedVelocityField::vtkCompositeInterpolatedVelocityField()
{
  this->LastDataSetIndex = 0;
  this->DataSets = new vtkCompositeInterpolatedVelocityFieldDataSetsType;
}


vtkCompositeInterpolatedVelocityField::~vtkCompositeInterpolatedVelocityField()
{
  delete this->DataSets;
  this->DataSets = NULL;
}

void vtkCompositeInterpolatedVelocityField::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "DataSets: "           << this->DataSets         << endl;
  os << indent << "Last Dataset Index: " << this->LastDataSetIndex << endl;
}
