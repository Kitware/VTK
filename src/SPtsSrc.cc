/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SPtsSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SPtsSrc.hh"

void vtkStructuredPointsSource::Modified()
{
  this->vtkStructuredPoints::Modified();
  this->vtkSource::_Modified();
}

unsigned long int vtkStructuredPointsSource::GetMTime()
{
  unsigned long dtime = this->vtkStructuredPoints::GetMTime();
  unsigned long ftime = this->vtkSource::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkStructuredPointsSource::Update()
{
  this->UpdateFilter();
}

void vtkStructuredPointsSource::DebugOn()
{
  vtkStructuredPoints::DebugOn();
  vtkSource::_DebugOn();
}

void vtkStructuredPointsSource::DebugOff()
{
  vtkStructuredPoints::DebugOff();
  vtkSource::_DebugOff();
}

int vtkStructuredPointsSource::GetDataReleased()
{
  return this->DataReleased;
}

void vtkStructuredPointsSource::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkStructuredPointsSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPoints::PrintSelf(os,indent);
  vtkSource::_PrintSelf(os,indent);
}
