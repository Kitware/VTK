/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PtS2PtSF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PtS2PtSF.hh"
#include "PolyData.hh"

vtkPointSetToPointSetFilter::vtkPointSetToPointSetFilter()
{
  // prevents dangling reference to PointSet
  this->PointSet = new vtkPolyData;
}

vtkPointSetToPointSetFilter::~vtkPointSetToPointSetFilter()
{
  this->PointSet->Delete();
}

vtkDataSet* vtkPointSetToPointSetFilter::MakeObject()
{
  vtkPointSetToPointSetFilter *o = new vtkPointSetToPointSetFilter();
  o->PointSet = this->PointSet;
  o->SetPoints(this->GetPoints());
  return o;
}

void vtkPointSetToPointSetFilter::Modified()
{
  this->vtkPointSet::Modified();
  this->vtkPointSetFilter::_Modified();
}

unsigned long int vtkPointSetToPointSetFilter::GetMTime()
{
  unsigned long dtime = this->vtkPointSet::GetMTime();
  unsigned long ftime = this->vtkPointSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkPointSetToPointSetFilter::DebugOn()
{
  vtkPointSet::DebugOn();
  vtkPointSetFilter::_DebugOn();
}

void vtkPointSetToPointSetFilter::DebugOff()
{
  vtkPointSet::DebugOff();
  vtkPointSetFilter::_DebugOff();
}

int vtkPointSetToPointSetFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkPointSetToPointSetFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkPointSetToPointSetFilter::Update()
{
  this->UpdateFilter();
}

void vtkPointSetToPointSetFilter::Initialize()
{
  if ( this->Input != NULL )
    {
    // copies input geometry to internal data set
    vtkDataSet *ds=this->Input->MakeObject();
    this->PointSet->Delete();
    this->PointSet = ds;
    }
  else
    {
    return;
    }
}

void vtkPointSetToPointSetFilter::ComputeBounds()
{
  if ( this->Points != NULL )
    {
    this->Points->ComputeBounds();
    float *bounds=this->Points->GetBounds();
    for (int i=0; i < 6; i++) this->Bounds[i] = bounds[i];
    }
};

void vtkPointSetToPointSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPointSet::PrintSelf(os,indent);
  vtkPointSetFilter::_PrintSelf(os,indent);

  if ( this->PointSet )
    {
    os << indent << "PointSet: (" << this->PointSet << ")\n";
    os << indent << "PointSet type: " << this->PointSet->GetClassName() <<"\n";
    }
  else
    {
    os << indent << "PointSet: (none)\n";
    }
}
