/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SPt2Poly.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SPt2Poly.hh"

void vtkStructuredPointsToPolyDataFilter::Modified()
{
  this->vtkPolyData::Modified();
  this->vtkStructuredPointsFilter::_Modified();
}

unsigned long int vtkStructuredPointsToPolyDataFilter::GetMTime()
{
  unsigned long dtime = this->vtkPolyData::GetMTime();
  unsigned long ftime = this->vtkStructuredPointsFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkStructuredPointsToPolyDataFilter::DebugOn()
{
  vtkPolyData::DebugOn();
  vtkStructuredPointsFilter::_DebugOn();
}

void vtkStructuredPointsToPolyDataFilter::DebugOff()
{
  vtkPolyData::DebugOff();
  vtkStructuredPointsFilter::_DebugOff();
}

void vtkStructuredPointsToPolyDataFilter::Update()
{
  this->UpdateFilter();
}

int vtkStructuredPointsToPolyDataFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkStructuredPointsToPolyDataFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkStructuredPointsToPolyDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyData::PrintSelf(os,indent);
  vtkStructuredPointsFilter::_PrintSelf(os,indent);
}
