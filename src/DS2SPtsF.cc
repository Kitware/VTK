/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2SPtsF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DS2SPtsF.hh"

void vtkDataSetToStructuredPointsFilter::Modified()
{
  this->vtkStructuredPoints::Modified();
  this->vtkDataSetFilter::_Modified();
}

unsigned long int vtkDataSetToStructuredPointsFilter::GetMTime()
{
  unsigned long dtime = this->vtkStructuredPoints::GetMTime();
  unsigned long ftime = this->vtkDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkDataSetToStructuredPointsFilter::DebugOn()
{
  vtkStructuredPoints::DebugOn();
  vtkDataSetFilter::_DebugOn();
}

void vtkDataSetToStructuredPointsFilter::DebugOff()
{
  vtkStructuredPoints::DebugOff();
  vtkDataSetFilter::_DebugOff();
}

void  vtkDataSetToStructuredPointsFilter::Update()
{
  this->UpdateFilter();
}

int vtkDataSetToStructuredPointsFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkDataSetToStructuredPointsFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkDataSetToStructuredPointsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPoints::PrintSelf(os,indent);
  vtkDataSetFilter::_PrintSelf(os,indent);
}
