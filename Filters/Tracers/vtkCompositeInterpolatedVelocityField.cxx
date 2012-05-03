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


const double vtkCompositeInterpolatedVelocityField::TOLERANCE_SCALE = 1.0E-8;


vtkCompositeInterpolatedVelocityField::vtkCompositeInterpolatedVelocityField()
{
  this->LastDataSetIndex = 0;
  this->DataSets = new vtkCompositeInterpolatedVelocityFieldDataSetsType;
}


vtkCompositeInterpolatedVelocityField::~vtkCompositeInterpolatedVelocityField()
{
  if ( this->DataSets )
    {
    delete this->DataSets;
    this->DataSets = NULL;
    }
}

void vtkCompositeInterpolatedVelocityField::PrintSelf( ostream & os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );

  os << indent << "DataSets: "           << this->DataSets         << endl;
  os << indent << "Last Dataset Index: " << this->LastDataSetIndex << endl;
}
