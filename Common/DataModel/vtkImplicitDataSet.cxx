/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitDataSet.h"

#include "vtkCell.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkImplicitDataSet);
vtkCxxSetObjectMacro(vtkImplicitDataSet,DataSet,vtkDataSet);

// Construct an vtkImplicitDataSet with no initial dataset; the OutValue
// set to a large negative number; and the OutGradient set to (0,0,1).
vtkImplicitDataSet::vtkImplicitDataSet()
{
  this->DataSet = NULL;

  this->OutValue = -VTK_DOUBLE_MAX;

  this->OutGradient[0] = 0.0;
  this->OutGradient[1] = 0.0;
  this->OutGradient[2] = 1.0;

  this->Weights = NULL;
  this->Size = 0;
}

vtkImplicitDataSet::~vtkImplicitDataSet()
{
  this->SetDataSet(NULL);
  delete [] this->Weights;
}

// Evaluate the implicit function. This returns the interpolated scalar value
// at x[3].
double vtkImplicitDataSet::EvaluateFunction(double x[3])
{
  vtkDataArray *scalars;
  vtkCell *cell;
  vtkIdType id;
  int subId, i, numPts;
  double pcoords[3], s;

  if ( this->DataSet->GetMaxCellSize() > this->Size )
    {
    delete [] this->Weights;
    this->Weights = new double[this->DataSet->GetMaxCellSize()];
    this->Size = this->DataSet->GetMaxCellSize();
    }

  // See if a dataset has been specified
  if ( !this->DataSet ||
       !(scalars = this->DataSet->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"Can't evaluate dataset!");
    return this->OutValue;
    }

  // Find the cell that contains xyz and get it
  cell = this->DataSet->FindAndGetCell(x,NULL,-1,VTK_DBL_EPSILON,subId,pcoords,this->Weights);

  if (cell)
    { // Interpolate the point data
    numPts = cell->GetNumberOfPoints();
    for (s=0.0, i=0; i < numPts; i++)
      {
      id = cell->PointIds->GetId(i);
      s += scalars->GetComponent(id,0) * this->Weights[i];
      }
    return s;
    }
  else
    { // use outside value
    return this->OutValue;
    }
}

unsigned long vtkImplicitDataSet::GetMTime()
{
  unsigned long mTime=this->vtkImplicitFunction::GetMTime();
  unsigned long DataSetMTime;

  if ( this->DataSet != NULL )
    {
    DataSetMTime = this->DataSet->GetMTime();
    mTime = ( DataSetMTime > mTime ? DataSetMTime : mTime );
    }

  return mTime;
}


// Evaluate implicit function gradient.
void vtkImplicitDataSet::EvaluateGradient(double x[3], double n[3])
{
  vtkDataArray *scalars;
  vtkCell *cell;
  vtkIdType id;
  int subId, i, numPts;
  double pcoords[3];

  if ( this->DataSet->GetMaxCellSize() > this->Size )
    {
    delete [] this->Weights;
    this->Weights = new double[this->DataSet->GetMaxCellSize()];
    this->Size = this->DataSet->GetMaxCellSize();
    }

  // See if a dataset has been specified
  if ( !this->DataSet ||
       !(scalars = this->DataSet->GetPointData()->GetScalars()) )
    {
    vtkErrorMacro(<<"Can't evaluate gradient!");
    for ( i=0; i < 3; i++ )
      {
      n[i] = this->OutGradient[i];
      }
    return;
    }

  // Find the cell that contains xyz and get it
  cell = this->DataSet->FindAndGetCell(x,NULL,-1,VTK_DBL_EPSILON,subId,pcoords,this->Weights);

  if (cell)
    { // Interpolate the point data
    numPts = cell->GetNumberOfPoints();

    for ( i=0; i < numPts; i++ ) //Weights used to hold scalar values
      {
      id = cell->PointIds->GetId(i);
      this->Weights[i] = scalars->GetComponent(id,0);
      }
    cell->Derivatives(subId, pcoords, this->Weights, 1, n);
    }

  else
    { // use outside value
    for ( i=0; i < 3; i++ )
      {
      n[i] = this->OutGradient[i];
      }
    }
}

void vtkImplicitDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Out Value: " << this->OutValue << "\n";
  os << indent << "Out Gradient: (" << this->OutGradient[0] << ", "
     << this->OutGradient[1] << ", " << this->OutGradient[2] << ")\n";

  if ( this->DataSet )
    {
    os << indent << "Data Set: " << this->DataSet << "\n";
    }
  else
    {
    os << indent << "Data Set: (none)\n";
    }
}

//----------------------------------------------------------------------------
void vtkImplicitDataSet::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->DataSet, "DataSet");
}
