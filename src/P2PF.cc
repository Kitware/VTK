/*=========================================================================

  Program:   Visualization Toolkit
  Module:    P2PF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "P2PF.hh"

void vtkPolyToPolyFilter::Modified()
{
  this->vtkPolyData::Modified();
  this->vtkPolyFilter::_Modified();
}

unsigned long int vtkPolyToPolyFilter::GetMTime()
{
  unsigned long dtime = this->vtkPolyData::GetMTime();
  unsigned long ftime = this->vtkPolyFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vtkPolyToPolyFilter::Update()
{
  this->UpdateFilter();
}

void vtkPolyToPolyFilter::DebugOn()
{
  vtkPolyData::DebugOn();
  vtkPolyFilter::_DebugOn();
}

void vtkPolyToPolyFilter::DebugOff()
{
  vtkPolyData::DebugOff();
  vtkPolyFilter::_DebugOff();
}

int vtkPolyToPolyFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkPolyToPolyFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vtkPolyToPolyFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyData::PrintSelf(os,indent);
  vtkPolyFilter::_PrintSelf(os,indent);
}


