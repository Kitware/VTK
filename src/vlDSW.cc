/*=========================================================================

  Program:   Visualization Library
  Module:    vlDSW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlDSW.hh"

void vlDataSetWriter::WriteData()
{
}

void vlDataSetWriter::Modified()
{
  this->vlDataWriter::Modified();
  this->vlDataSetFilter::_Modified();
}

unsigned long int vlDataSetWriter::GetMTime()
{
  unsigned long dtime = this->vlDataWriter::GetMTime();
  unsigned long ftime = this->vlDataSetFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlDataSetWriter::DebugOn()
{
  vlDataWriter::DebugOn();
  vlDataSetFilter::_DebugOn();
}

void vlDataSetWriter::DebugOff()
{
  vlDataWriter::DebugOff();
  vlDataSetFilter::_DebugOff();
}

void vlDataSetWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataWriter::PrintSelf(os,indent);
  vlDataSetFilter::_PrintSelf(os,indent);
}
