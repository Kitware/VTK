/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2UGrid.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DS2UGrid.hh"

void vtkDataSetToUnstructuredGridFilter::Modified()
{
  this->vtkUnstructuredGrid::Modified();
  this->vtkDataSetFilter::_Modified();
}

unsigned long int vtkDataSetToUnstructuredGridFilter::GetMTime()
{
  unsigned long dtime = this->vtkUnstructuredGrid::GetMTime();
  unsigned long ftime = this->vtkDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

int vtkDataSetToUnstructuredGridFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkDataSetToUnstructuredGridFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkDataSetToUnstructuredGridFilter::Update()
{
  this->UpdateFilter();
}

void vtkDataSetToUnstructuredGridFilter::DebugOn()
{
  vtkUnstructuredGrid::DebugOn();
  vtkDataSetFilter::_DebugOn();
}

void vtkDataSetToUnstructuredGridFilter::DebugOff()
{
  vtkUnstructuredGrid::DebugOff();
  vtkDataSetFilter::_DebugOff();
}

void vtkDataSetToUnstructuredGridFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkUnstructuredGrid::PrintSelf(os,indent);
  vtkDataSetFilter::_PrintSelf(os,indent);
}
