// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGenericSubdivisionErrorMetric.h"

#include "vtkGenericAdaptorCell.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericDataSet.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include <cassert>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkGenericSubdivisionErrorMetric::vtkGenericSubdivisionErrorMetric()
{
  this->GenericCell = nullptr;
  this->DataSet = nullptr;
}

//------------------------------------------------------------------------------
vtkGenericSubdivisionErrorMetric::~vtkGenericSubdivisionErrorMetric() = default;

//------------------------------------------------------------------------------
// Avoid reference loop
void vtkGenericSubdivisionErrorMetric::SetGenericCell(vtkGenericAdaptorCell* c)
{
  this->GenericCell = c;
  this->Modified();
}

//------------------------------------------------------------------------------
// Avoid reference loop
void vtkGenericSubdivisionErrorMetric::SetDataSet(vtkGenericDataSet* ds)
{
  this->DataSet = ds;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkGenericSubdivisionErrorMetric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "GenericCell: " << this->GenericCell << endl;
  os << indent << "DataSet: " << this->DataSet << endl;
}
VTK_ABI_NAMESPACE_END
