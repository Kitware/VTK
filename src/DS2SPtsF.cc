/*=========================================================================

  Program:   Visualization Library
  Module:    DS2SPtsF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DS2SPtsF.hh"

void vlDataSetToStructuredPointsFilter::Modified()
{
  this->vlStructuredPoints::Modified();
  this->vlDataSetFilter::_Modified();
}

unsigned long int vlDataSetToStructuredPointsFilter::GetMTime()
{
  unsigned long dtime = this->vlStructuredPoints::GetMTime();
  unsigned long ftime = this->vlDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlDataSetToStructuredPointsFilter::DebugOn()
{
  vlStructuredPoints::DebugOn();
  vlDataSetFilter::_DebugOn();
}

void vlDataSetToStructuredPointsFilter::DebugOff()
{
  vlStructuredPoints::DebugOff();
  vlDataSetFilter::_DebugOff();
}

void  vlDataSetToStructuredPointsFilter::Update()
{
  this->UpdateFilter();
}

int vlDataSetToStructuredPointsFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vlDataSetToStructuredPointsFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlDataSetToStructuredPointsFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredPoints::PrintSelf(os,indent);
  vlDataSetFilter::_PrintSelf(os,indent);
}
