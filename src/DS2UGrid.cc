/*=========================================================================

  Program:   Visualization Library
  Module:    DS2UGrid.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DS2UGrid.hh"

void vlDataSetToUnstructuredGridFilter::Modified()
{
  this->vlUnstructuredGrid::Modified();
  this->vlDataSetFilter::_Modified();
}

unsigned long int vlDataSetToUnstructuredGridFilter::GetMTime()
{
  unsigned long dtime = this->vlUnstructuredGrid::GetMTime();
  unsigned long ftime = this->vlDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlDataSetToUnstructuredGridFilter::Update()
{
  this->UpdateFilter();
}

void vlDataSetToUnstructuredGridFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlUnstructuredGrid::PrintSelf(os,indent);
  vlDataSetFilter::_PrintSelf(os,indent);
}
