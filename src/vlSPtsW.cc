/*=========================================================================

  Program:   Visualization Library
  Module:    vlSPtsW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlSPtsW.hh"

void vlStructuredPointsWriter::WriteData()
{
}

void vlStructuredPointsWriter::Modified()
{
  this->vlDataWriter::Modified();
  this->vlStructuredPointsFilter::_Modified();
}

unsigned long int vlStructuredPointsWriter::GetMTime()
{
  unsigned long dtime = this->vlDataWriter::GetMTime();
  unsigned long ftime = this->vlStructuredPointsFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlStructuredPointsWriter::DebugOn()
{
  vlDataWriter::DebugOn();
  vlStructuredPointsFilter::_DebugOn();
}

void vlStructuredPointsWriter::DebugOff()
{
  vlDataWriter::DebugOff();
  vlStructuredPointsFilter::_DebugOff();
}

void vlStructuredPointsWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataWriter::PrintSelf(os,indent);
  vlStructuredPointsFilter::_PrintSelf(os,indent);
}
