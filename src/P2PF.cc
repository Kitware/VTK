/*=========================================================================

  Program:   Visualization Library
  Module:    P2PF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "P2PF.hh"

void vlPolyToPolyFilter::Modified()
{
  this->vlPolyData::Modified();
  this->vlPolyFilter::_Modified();
}

unsigned long int vlPolyToPolyFilter::GetMTime()
{
  unsigned long dtime = this->vlPolyData::GetMTime();
  unsigned long ftime = this->vlPolyFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlPolyToPolyFilter::Update()
{
  this->UpdateFilter();
}

void vlPolyToPolyFilter::DebugOn()
{
  vlPolyData::DebugOn();
  vlPolyFilter::_DebugOn();
}

void vlPolyToPolyFilter::DebugOff()
{
  vlPolyData::DebugOff();
  vlPolyFilter::_DebugOff();
}

int vlPolyToPolyFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vlPolyToPolyFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlPolyToPolyFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyData::PrintSelf(os,indent);
  vlPolyFilter::_PrintSelf(os,indent);
}


