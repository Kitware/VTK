/*=========================================================================

  Program:   Visualization Library
  Module:    SG2PolyF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SG2PolyF.hh"

void vlStructuredGridToPolyFilter::Modified()
{
  this->vlPolyData::Modified();
  this->vlStructuredGridFilter::_Modified();
}

unsigned long int vlStructuredGridToPolyFilter::GetMTime()
{
  unsigned long dtime = this->vlPolyData::GetMTime();
  unsigned long ftime = this->vlStructuredGridFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlStructuredGridToPolyFilter::DebugOn()
{
  vlPolyData::DebugOn();
  vlStructuredGridFilter::_DebugOn();
}

void vlStructuredGridToPolyFilter::DebugOff()
{
  vlPolyData::DebugOff();
  vlStructuredGridFilter::_DebugOff();
}

void vlStructuredGridToPolyFilter::Update()
{
  this->UpdateFilter();
}

int vlStructuredGridToPolyFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vlStructuredGridToPolyFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlStructuredGridToPolyFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyData::PrintSelf(os,indent);
  vlStructuredGridFilter::_PrintSelf(os,indent);
}
