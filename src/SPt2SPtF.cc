/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SPt2SPtF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SPt2SPtF.hh"

void vtkStructuredPointsToStructuredPointsFilter::Modified()
{
  this->vtkStructuredPoints::Modified();
  this->vtkStructuredPointsFilter::_Modified();
}

unsigned long int vtkStructuredPointsToStructuredPointsFilter::GetMTime()
{
  unsigned long dtime = this->vtkStructuredPoints::GetMTime();
  unsigned long ftime = this->vtkStructuredPointsFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkStructuredPointsToStructuredPointsFilter::DebugOn()
{
  vtkStructuredPoints::DebugOn();
  vtkStructuredPointsFilter::_DebugOn();
}

void vtkStructuredPointsToStructuredPointsFilter::DebugOff()
{
  vtkStructuredPoints::DebugOff();
  vtkStructuredPointsFilter::_DebugOff();
}

int vtkStructuredPointsToStructuredPointsFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkStructuredPointsToStructuredPointsFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkStructuredPointsToStructuredPointsFilter::Update()
{
  this->UpdateFilter();
}

void vtkStructuredPointsToStructuredPointsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPoints::PrintSelf(os,indent);
  vtkStructuredPointsFilter::_PrintSelf(os,indent);
}
