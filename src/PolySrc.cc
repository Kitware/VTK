/*=========================================================================

  Program:   Visualization Library
  Module:    PolySrc.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/

#include "PolySrc.hh"

void vlPolySource::Modified()
{
  this->vlPolyData::Modified();
  this->vlSource::_Modified();
}

unsigned long int vlPolySource::GetMTime()
{
  unsigned long dtime = this->vlPolyData::GetMTime();
  unsigned long ftime = this->vlSource::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlPolySource::Update()
{
  this->UpdateFilter();
}

void vlPolySource::DebugOn()
{
  vlPolyData::DebugOn();
  vlSource::_DebugOn();
}

void vlPolySource::DebugOff()
{
  vlPolyData::DebugOff();
  vlSource::_DebugOff();
}

int vlPolySource::GetDataReleased()
{
  return this->DataReleased;
}

void vlPolySource::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlPolySource::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyData::PrintSelf(os,indent);
  vlSource::_PrintSelf(os,indent);
}
