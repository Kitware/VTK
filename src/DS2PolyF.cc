/*=========================================================================

  Program:   Visualization Library
  Module:    DS2PolyF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DS2PolyF.hh"

void vlDataSetToPolyFilter::Modified()
{
  this->vlPolyData::Modified();
  this->vlDataSetFilter::_Modified();
}

unsigned long int vlDataSetToPolyFilter::GetMTime()
{
  unsigned long dtime = this->vlPolyData::GetMTime();
  unsigned long ftime = this->vlDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlDataSetToPolyFilter::Update()
{
  this->UpdateFilter();
}

void vlDataSetToPolyFilter::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyData::PrintSelf(os,indent);
  vlDataSetFilter::_PrintSelf(os,indent);
}
