/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ProbeF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ProbeF.hh"

vtkProbeFilter::vtkProbeFilter()
{
  this->Source = NULL;
}

void vtkProbeFilter::Execute()
{
  int cellId, ptId;
  float *x, tol2;
  vtkCell *cell;
  vtkPointData *pd;
  int numPts, subId;
  float pcoords[3], weights[MAX_CELL_SIZE];
  vtkDataSet *input=this->Input, *source=this->Source;

  vtkDebugMacro(<<"Probing data");
  this->Initialize();

  pd = input->GetPointData();
  numPts = source->GetNumberOfPoints();
//
// Allocate storage for output PointData
//
  this->PointData.InterpolateAllocate(pd);
//
// Use tolerance as a function of size of input data
//
  tol2 = input->GetLength();
  tol2 = tol2*tol2 / 1000.0;
//
// Loop over all source points, interpolating input data
//
  for (ptId=0; ptId < numPts; ptId++)
    {
    x = source->GetPoint(ptId);
    cellId = input->FindCell(x,NULL,tol2,subId,pcoords,weights);
    if ( cellId >= 0 )
      {
      cell = input->GetCell(cellId);
      this->PointData.InterpolatePoint(pd,ptId,&(cell->PointIds),weights);
      }
    else
      {
      this->PointData.NullPoint(ptId);
      }
    }

  this->Modified(); //make sure something's changed
}

void vtkProbeFilter::Initialize()
{
  if ( this->Source )
    {
    // copies SOURCE geometry to internal data set
    if (this->DataSet) this->DataSet->Delete();
    this->DataSet = this->Source->MakeObject(); 
    }
}

// Description:
// Override update method because execution can branch two ways (Input 
// and Source)
void vtkProbeFilter::Update()
{
  // make sure input is available
  if ( this->Input == NULL || this->Source == NULL )
    {
    vtkErrorMacro(<< "No input!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Source->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->GetMTime() || 
  this->Source->GetMTime() > this->GetMTime() || 
  this->GetMTime() > this->ExecuteTime || this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
  if ( this->Source->ShouldIReleaseData() ) this->Source->ReleaseData();
}

void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Source: " << this->Source << "\n";
}
