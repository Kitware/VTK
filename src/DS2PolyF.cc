/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DS2PolyF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DS2PolyF.hh"

void vtkDataSetToPolyFilter::Modified()
{
  this->vtkPolyData::Modified();
  this->vtkDataSetFilter::_Modified();
}

unsigned long int vtkDataSetToPolyFilter::GetMTime()
{
  unsigned long dtime = this->vtkPolyData::GetMTime();
  unsigned long ftime = this->vtkDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkDataSetToPolyFilter::DebugOn()
{
  vtkPolyData::DebugOn();
  vtkDataSetFilter::_DebugOn();
}

void vtkDataSetToPolyFilter::DebugOff()
{
  vtkPolyData::DebugOff();
  vtkDataSetFilter::_DebugOff();
}

void vtkDataSetToPolyFilter::Update()
{
  this->UpdateFilter();
}

int vtkDataSetToPolyFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkDataSetToPolyFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkDataSetToPolyFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyData::PrintSelf(os,indent);
  vtkDataSetFilter::_PrintSelf(os,indent);
}
