/*=========================================================================

  Program:   Visualization Library
  Module:    UGridSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "UGridSrc.hh"

void vlUnstructuredGridSource::Modified()
{
  this->vlUnstructuredGrid::Modified();
  this->vlSource::_Modified();
}

unsigned long int vlUnstructuredGridSource::GetMTime()
{
  unsigned long dtime = this->vlUnstructuredGrid::GetMTime();
  unsigned long ftime = this->vlSource::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlUnstructuredGridSource::Update()
{
  this->UpdateFilter();
}

void vlUnstructuredGridSource::DebugOn()
{
  vlUnstructuredGrid::DebugOn();
  vlSource::_DebugOn();
}

void vlUnstructuredGridSource::DebugOff()
{
  vlUnstructuredGrid::DebugOff();
  vlSource::_DebugOff();
}

int vlUnstructuredGridSource::GetDataReleased()
{
  return this->DataReleased;
}

void vlUnstructuredGridSource::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlUnstructuredGridSource::PrintSelf(ostream& os, vlIndent indent)
{
  vlUnstructuredGrid::PrintSelf(os,indent);
  vlSource::_PrintSelf(os,indent);
}
