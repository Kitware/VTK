/*=========================================================================

  Program:   Visualization Library
  Module:    vlUGridW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlUGridW.hh"

void vlUnstructuredGridWriter::WriteData()
{
}

void vlUnstructuredGridWriter::Modified()
{
  this->vlDataWriter::Modified();
  this->vlUnstructuredGridFilter::_Modified();
}

unsigned long int vlUnstructuredGridWriter::GetMTime()
{
  unsigned long dtime = this->vlDataWriter::GetMTime();
  unsigned long ftime = this->vlUnstructuredGridFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlUnstructuredGridWriter::DebugOn()
{
  vlDataWriter::DebugOn();
  vlUnstructuredGridFilter::_DebugOn();
}

void vlUnstructuredGridWriter::DebugOff()
{
  vlDataWriter::DebugOff();
  vlUnstructuredGridFilter::_DebugOff();
}

void vlUnstructuredGridWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataWriter::PrintSelf(os,indent);
  vlUnstructuredGridFilter::_PrintSelf(os,indent);
}
