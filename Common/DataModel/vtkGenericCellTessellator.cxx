/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCellTessellator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericCellTessellator.h"
#include "vtkObjectFactory.h"

#include "vtkPoints.h"
#include "vtkIdList.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkMergePoints.h"
#include "vtkCellArray.h"
#include "vtkCollection.h"
#include "vtkGenericSubdivisionErrorMetric.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkGenericCellIterator.h"


#include <cassert>

#include "vtkMath.h"

vtkCxxSetObjectMacro(vtkGenericCellTessellator, ErrorMetrics, vtkCollection);

//-----------------------------------------------------------------------------
// Create the tessellator helper with a default of 0.25 for threshold
vtkGenericCellTessellator::vtkGenericCellTessellator()
{
  this->ErrorMetrics = vtkCollection::New();
  this->MaxErrorsCapacity = 0;
  this->MaxErrors = 0;
  this->Measurement = 0;
}

//-----------------------------------------------------------------------------
vtkGenericCellTessellator::~vtkGenericCellTessellator()
{
  this->SetErrorMetrics( 0 );
  delete[] this->MaxErrors;
}


//-----------------------------------------------------------------------------
void vtkGenericCellTessellator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Measurement: "  << this->Measurement << endl;
  os << indent << "ErrorMetrics: " << this->ErrorMetrics << endl;
  /* this->MaxErrorsCapacity */
  /* this->MaxErrors */
}

//-----------------------------------------------------------------------------
// Description:
// Does the edge need to be subdivided according to at least one error
// metric? The edge is defined by its `leftPoint' and its `rightPoint'.
// `leftPoint', `midPoint' and `rightPoint' have to be initialized before
// calling RequiresEdgeSubdivision().
// Their format is global coordinates, parametric coordinates and
// point centered attributes: xyx rst abc de...
// `alpha' is the normalized abscissa of the midpoint along the edge.
// (close to 0 means close to the left point, close to 1 means close to the
// right point)
// \pre leftPoint_exists: leftPoint!=0
// \pre midPoint_exists: midPoint!=0
// \pre rightPoint_exists: rightPoint!=0
// \pre clamped_alpha: alpha>0 && alpha<1
// \pre valid_size: sizeof(leftPoint)=sizeof(midPoint)=sizeof(rightPoint)
//          =GetAttributeCollection()->GetNumberOfPointCenteredComponents()+6
int vtkGenericCellTessellator::RequiresEdgeSubdivision(double *leftPoint,
                                                   double *midPoint,
                                                   double *rightPoint,
                                                   double alpha)
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
  assert("pre: clamped_alpha" && alpha>0 && alpha<1);

  int result = 0;
  this->ErrorMetrics->InitTraversal();
  vtkGenericSubdivisionErrorMetric *e =
    static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());

  // Once we found at least one error metric that need subdivision,
  // the subdivision has to be done and there is no need to check for other
  // error metrics.
  while(!result && e != 0 )
  {
    result = e->RequiresEdgeSubdivision(leftPoint,midPoint,rightPoint,alpha);
    e = static_cast<vtkGenericSubdivisionErrorMetric *>
      (this->ErrorMetrics->GetNextItemAsObject());
  }

  return result;
}

//-----------------------------------------------------------------------------
// Description:
// Update the max error of each error metric according to the error at the
// mid-point. The type of error depends on the state
// of the concrete error metric. For instance, it can return an absolute
// or relative error metric.
// See RequiresEdgeSubdivision() for a description of the arguments.
// \pre leftPoint_exists: leftPoint!=0
// \pre midPoint_exists: midPoint!=0
// \pre rightPoint_exists: rightPoint!=0
// \pre clamped_alpha: alpha>0 && alpha<1
// \pre valid_size: sizeof(leftPoint)=sizeof(midPoint)=sizeof(rightPoint)
//          =GetAttributeCollection()->GetNumberOfPointCenteredComponents()+6
void vtkGenericCellTessellator::UpdateMaxError(double *leftPoint,
                                               double *midPoint,
                                               double *rightPoint,
                                               double alpha)
{
  assert("pre: leftPoint_exists" && leftPoint!=0);
  assert("pre: midPoint_exists" && midPoint!=0);
  assert("pre: rightPoint_exists" && rightPoint!=0);
  assert("pre: clamped_alpha" && alpha>0 && alpha<1);

  this->ErrorMetrics->InitTraversal();
  vtkGenericSubdivisionErrorMetric *e =
    static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());

  // Once we found at least one error metric that need subdivision,
  // the subdivision has to be done and there is no need to check for other
  // error metrics.
  for(int i = 0; e!=0; ++i)
  {
    double error = e->GetError(leftPoint,midPoint,rightPoint,alpha);
    assert("check: positive_error" && error>=0);
    if(error > this->MaxErrors[i])
    {
      this->MaxErrors[i] = error;
    }
    e = static_cast<vtkGenericSubdivisionErrorMetric *>
      (this->ErrorMetrics->GetNextItemAsObject());
  }
}

//-----------------------------------------------------------------------------
// Description:
// Init the error metric with the dataset. Should be called in each filter
// before any tessellation of any cell.
void vtkGenericCellTessellator::InitErrorMetrics(vtkGenericDataSet *ds)
{
  this->Initialize(ds);
  this->ErrorMetrics->InitTraversal();
  vtkGenericSubdivisionErrorMetric *e =
    static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());

  while(e!=0)
  {
    e->SetDataSet(ds);
    e = static_cast<vtkGenericSubdivisionErrorMetric *>
      (this->ErrorMetrics->GetNextItemAsObject());
  }

  if(this->Measurement)
  {
    this->ResetMaxErrors();
  }
}

//-----------------------------------------------------------------------------
// Description:
// Reset the maximal error of each error metric. The purpose of the maximal
// error is to measure the quality of a fixed subdivision.
void vtkGenericCellTessellator::ResetMaxErrors()
{
  int c = this->ErrorMetrics->GetNumberOfItems();

  // Allocate the array.
  if(c>this->MaxErrorsCapacity)
  {
    this->MaxErrorsCapacity = c;
    delete [] this->MaxErrors;
    this->MaxErrors = new double[this->MaxErrorsCapacity];
  }

  for(int i = 0; i<c; ++i)
  {
    this->MaxErrors[i] = 0;
  }
}

//-----------------------------------------------------------------------------
// Description:
// Get the maximum error measured after the fixed subdivision.
// \pre errors_exists: errors!=0
// \pre valid_size: sizeof(errors)==GetErrorMetrics()->GetNumberOfItems()
void vtkGenericCellTessellator::GetMaxErrors(double *errors)
{
  assert("pre: errors_exists" && errors!=0);

  int c = this->ErrorMetrics->GetNumberOfItems();
  for(int i = 0; i<c; ++i)
  {
    errors[i] = this->MaxErrors[i];
  }
}

//-----------------------------------------------------------------------------
// Description:
// Send the current cell to error metrics. Should be called at the beginning
// of the implementation of Tessellate(), Triangulate()
// or TessellateFace()
// \pre cell_exists: cell!=0
void vtkGenericCellTessellator::SetGenericCell(vtkGenericAdaptorCell *cell)
{
  assert("pre: cell_exists" && cell!=0);

  this->ErrorMetrics->InitTraversal();
  vtkGenericSubdivisionErrorMetric *e=static_cast<vtkGenericSubdivisionErrorMetric *>(this->ErrorMetrics->GetNextItemAsObject());

  while(e!=0)
  {
    e->SetGenericCell(cell);
    e = static_cast<vtkGenericSubdivisionErrorMetric *>
      (this->ErrorMetrics->GetNextItemAsObject());
  }
}
