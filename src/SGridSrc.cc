/*=========================================================================

  Program:   Visualization Library
  Module:    SGridSrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SGridSrc.hh"

void vlStructuredGridSource::Modified()
{
  this->vlStructuredGrid::Modified();
  this->vlSource::_Modified();
}

unsigned long int vlStructuredGridSource::GetMTime()
{
  unsigned long dtime = this->vlStructuredGrid::GetMTime();
  unsigned long ftime = this->vlSource::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlStructuredGridSource::Update()
{
  this->UpdateFilter();
}

void vlStructuredGridSource::DebugOn()
{
  vlStructuredGrid::DebugOn();
  vlSource::_DebugOn();
}

void vlStructuredGridSource::DebugOff()
{
  vlStructuredGrid::DebugOff();
  vlSource::_DebugOff();
}

int vlStructuredGridSource::GetDataReleased()
{
  return this->DataReleased;
}

void vlStructuredGridSource::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlStructuredGridSource::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredGrid::PrintSelf(os,indent);
  vlSource::_PrintSelf(os,indent);
}
