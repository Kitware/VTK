/*=========================================================================

  Program:   Visualization Library
  Module:    SPt2SPtF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SPt2SPtF.hh"

void vlStructuredPointsToStructuredPointsFilter::Modified()
{
  this->vlStructuredPoints::Modified();
  this->vlStructuredPointsFilter::_Modified();
}

unsigned long int vlStructuredPointsToStructuredPointsFilter::GetMTime()
{
  unsigned long dtime = this->vlStructuredPoints::GetMTime();
  unsigned long ftime = this->vlStructuredPointsFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlStructuredPointsToStructuredPointsFilter::DebugOn()
{
  vlStructuredPoints::DebugOn();
  vlStructuredPointsFilter::_DebugOn();
}

void vlStructuredPointsToStructuredPointsFilter::DebugOff()
{
  vlStructuredPoints::DebugOff();
  vlStructuredPointsFilter::_DebugOff();
}

int vlStructuredPointsToStructuredPointsFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vlStructuredPointsToStructuredPointsFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlStructuredPointsToStructuredPointsFilter::Update()
{
  this->UpdateFilter();
}

void vlStructuredPointsToStructuredPointsFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredPoints::PrintSelf(os,indent);
  vlStructuredPointsFilter::_PrintSelf(os,indent);
}
