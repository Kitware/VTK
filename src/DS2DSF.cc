/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2DSF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DS2DSF.hh"
#include "PolyData.hh"

vtkDataSetToDataSetFilter::vtkDataSetToDataSetFilter()
{
  // prevents dangling reference to DataSet
  this->DataSet = new vtkPolyData;
}

vtkDataSetToDataSetFilter::~vtkDataSetToDataSetFilter()
{
  this->DataSet->Delete();
}

// Description:
// Initialize method is fancy: creates an internal dataset that holds
// geometry representation. All methods directed at geometry are 
// forwarded to internal dataset.
void vtkDataSetToDataSetFilter::Initialize()
{
  if ( this->Input )
    {
    // copies input geometry to internal data set
    this->DataSet->Delete();
    this->DataSet = this->Input->MakeObject(); 
    }
  else
    {
    return;
    }
}

void vtkDataSetToDataSetFilter::ComputeBounds()
{
  float *bounds = this->DataSet->GetBounds();
  for (int i=0; i<6; i++) this->Bounds[i] = bounds[i];
}

void vtkDataSetToDataSetFilter::Modified()
{
  this->vtkDataSet::Modified();
  this->vtkDataSetFilter::_Modified();
}

void vtkDataSetToDataSetFilter::DebugOn()
{
  vtkDataSet::DebugOn();
  vtkDataSetFilter::_DebugOn();
}

void vtkDataSetToDataSetFilter::DebugOff()
{
  vtkDataSet::DebugOff();
  vtkDataSetFilter::_DebugOff();
}

unsigned long int vtkDataSetToDataSetFilter::GetMTime()
{
  unsigned long dtime = this->vtkDataSet::GetMTime();
  unsigned long ftime = this->vtkDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

int vtkDataSetToDataSetFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkDataSetToDataSetFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkDataSetToDataSetFilter::Update()
{
  this->UpdateFilter();
}

void vtkDataSetToDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSet::PrintSelf(os,indent);
  vtkDataSetFilter::_PrintSelf(os,indent);

  if ( this->DataSet )
    {
    os << indent << "DataSet: (" << this->DataSet << ")\n";
    os << indent << "DataSet type: " << this->DataSet->GetClassName() << "\n";
    }
  else
    {
    os << indent << "DataSet: (none)\n";
    }
}
