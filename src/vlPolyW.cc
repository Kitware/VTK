/*=========================================================================

  Program:   Visualization Library
  Module:    vlPolyW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlPolyW.hh"

void vlPolyWriter::WriteData()
{
  FILE *fp;
  vlPolyData *input=(vlPolyData *)this->Input;

  vlDebugMacro(<<"Writing vl polygonal data...");

  if ( !(fp=this->OpenVLFile(this->Filename)) || !this->WriteHeader(fp) )
      return;
//
// Write polygonal data specific stuff
//
  fprintf(fp,"DATASET POLYDATA\n");
  this->WritePoints(fp, input->GetPoints());

  if (input->GetVerts()) this->WriteCells(fp, input->GetVerts(),"VERTICES");
  if (input->GetLines()) this->WriteCells(fp, input->GetLines(),"LINES");
  if (input->GetPolys()) this->WriteCells(fp, input->GetPolys(),"POLYGONS");
  if (input->GetStrips()) this->WriteCells(fp, input->GetStrips(),"TRIANGLE_STRIPS");

  this->WritePointData(fp, input);

}

void vlPolyWriter::Modified()
{
  this->vlDataWriter::Modified();
  this->vlPolyFilter::_Modified();
}

unsigned long int vlPolyWriter::GetMTime()
{
  unsigned long dtime = this->vlDataWriter::GetMTime();
  unsigned long ftime = this->vlPolyFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlPolyWriter::DebugOn()
{
  vlDataWriter::DebugOn();
  vlPolyFilter::_DebugOn();
}

void vlPolyWriter::DebugOff()
{
  vlDataWriter::DebugOff();
  vlPolyFilter::_DebugOff();
}

void vlPolyWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataWriter::PrintSelf(os,indent);
  vlPolyFilter::_PrintSelf(os,indent);
}
