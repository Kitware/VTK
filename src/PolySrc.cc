/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolySrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

#include "PolySrc.hh"

void vtkPolySource::Modified()
{
  this->vtkPolyData::Modified();
  this->vtkSource::_Modified();
}

unsigned long int vtkPolySource::GetMTime()
{
  unsigned long dtime = this->vtkPolyData::GetMTime();
  unsigned long ftime = this->vtkSource::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkPolySource::Update()
{
  this->UpdateFilter();
}

void vtkPolySource::DebugOn()
{
  vtkPolyData::DebugOn();
  vtkSource::_DebugOn();
}

void vtkPolySource::DebugOff()
{
  vtkPolyData::DebugOff();
  vtkSource::_DebugOff();
}

int vtkPolySource::GetDataReleased()
{
  return this->DataReleased;
}

void vtkPolySource::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkPolySource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyData::PrintSelf(os,indent);
  vtkSource::_PrintSelf(os,indent);
}
