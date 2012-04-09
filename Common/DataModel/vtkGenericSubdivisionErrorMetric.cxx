/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericSubdivisionErrorMetric.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericSubdivisionErrorMetric.h"

#include "vtkObjectFactory.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericDataSet.h"
#include "vtkMath.h"
#include <assert.h>


//-----------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::vtkGenericSubdivisionErrorMetric()
{
  this->GenericCell = NULL;
  this->DataSet = 0;
}

//-----------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::~vtkGenericSubdivisionErrorMetric()
{
}

//-----------------------------------------------------------------------------
// Avoid reference loop
void vtkGenericSubdivisionErrorMetric::SetGenericCell(vtkGenericAdaptorCell *c)
{
  this->GenericCell = c;
  this->Modified();
}

//-----------------------------------------------------------------------------
// Avoid reference loop
void vtkGenericSubdivisionErrorMetric::SetDataSet(vtkGenericDataSet *ds)
{
  this->DataSet = ds;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkGenericSubdivisionErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GenericCell: "  << this->GenericCell << endl;
  os << indent << "DataSet: "  << this->DataSet << endl;
}
