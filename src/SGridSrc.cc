/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SGridSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SGridSrc.hh"

void vtkStructuredGridSource::Modified()
{
  this->vtkStructuredGrid::Modified();
  this->vtkSource::_Modified();
}

unsigned long int vtkStructuredGridSource::GetMTime()
{
  unsigned long dtime = this->vtkStructuredGrid::GetMTime();
  unsigned long ftime = this->vtkSource::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkStructuredGridSource::Update()
{
  this->UpdateFilter();
}

void vtkStructuredGridSource::DebugOn()
{
  vtkStructuredGrid::DebugOn();
  vtkSource::_DebugOn();
}

void vtkStructuredGridSource::DebugOff()
{
  vtkStructuredGrid::DebugOff();
  vtkSource::_DebugOff();
}

int vtkStructuredGridSource::GetDataReleased()
{
  return this->DataReleased;
}

void vtkStructuredGridSource::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkStructuredGridSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredGrid::PrintSelf(os,indent);
  vtkSource::_PrintSelf(os,indent);
}
