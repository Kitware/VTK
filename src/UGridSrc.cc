/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UGridSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "UGridSrc.hh"

void vtkUnstructuredGridSource::Modified()
{
  this->vtkUnstructuredGrid::Modified();
  this->vtkSource::_Modified();
}

unsigned long int vtkUnstructuredGridSource::GetMTime()
{
  unsigned long dtime = this->vtkUnstructuredGrid::GetMTime();
  unsigned long ftime = this->vtkSource::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkUnstructuredGridSource::Update()
{
  this->UpdateFilter();
}

void vtkUnstructuredGridSource::DebugOn()
{
  vtkUnstructuredGrid::DebugOn();
  vtkSource::_DebugOn();
}

void vtkUnstructuredGridSource::DebugOff()
{
  vtkUnstructuredGrid::DebugOff();
  vtkSource::_DebugOff();
}

int vtkUnstructuredGridSource::GetDataReleased()
{
  return this->DataReleased;
}

void vtkUnstructuredGridSource::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkUnstructuredGridSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkUnstructuredGrid::PrintSelf(os,indent);
  vtkSource::_PrintSelf(os,indent);
}
