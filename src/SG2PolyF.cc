/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SG2PolyF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SG2PolyF.hh"

void vtkStructuredGridToPolyFilter::Modified()
{
  this->vtkPolyData::Modified();
  this->vtkStructuredGridFilter::_Modified();
}

unsigned long int vtkStructuredGridToPolyFilter::GetMTime()
{
  unsigned long dtime = this->vtkPolyData::GetMTime();
  unsigned long ftime = this->vtkStructuredGridFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkStructuredGridToPolyFilter::DebugOn()
{
  vtkPolyData::DebugOn();
  vtkStructuredGridFilter::_DebugOn();
}

void vtkStructuredGridToPolyFilter::DebugOff()
{
  vtkPolyData::DebugOff();
  vtkStructuredGridFilter::_DebugOff();
}

void vtkStructuredGridToPolyFilter::Update()
{
  this->UpdateFilter();
}

int vtkStructuredGridToPolyFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkStructuredGridToPolyFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkStructuredGridToPolyFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyData::PrintSelf(os,indent);
  vtkStructuredGridFilter::_PrintSelf(os,indent);
}
