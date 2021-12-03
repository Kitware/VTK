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

#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGenericCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

//------------------------------------------------------------------------------
vtkCompositeInterpolatedVelocityField::vtkCompositeInterpolatedVelocityField()
{
  this->LastDataSetIndex = 0;
  this->DataSets = new vtkCompositeInterpolatedVelocityFieldDataSetsType;
}

//------------------------------------------------------------------------------
vtkCompositeInterpolatedVelocityField::~vtkCompositeInterpolatedVelocityField()
{
  delete this->DataSets;
  this->DataSets = nullptr;
}

//------------------------------------------------------------------------------
// Copy the list of datasets to copy from.
void vtkCompositeInterpolatedVelocityField::CopyParameters(
  vtkAbstractInterpolatedVelocityField* from)
{
  this->Superclass::CopyParameters(from);

  // See if we need to copy our parameters
  vtkCompositeInterpolatedVelocityField* obj =
    vtkCompositeInterpolatedVelocityField::SafeDownCast(from);
  if (!obj)
  {
    return;
  }
  *(this->DataSets) = *(obj->DataSets);

  // The weights must be copied as well
  this->WeightsSize = obj->WeightsSize;
  delete[] this->Weights;
  this->Weights = new double[obj->WeightsSize];
}

//------------------------------------------------------------------------------
void vtkCompositeInterpolatedVelocityField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DataSets: " << this->DataSets << endl;
  os << indent << "Last Dataset Index: " << this->LastDataSetIndex << endl;
}
