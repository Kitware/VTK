/*=========================================================================

  Program:   Visualization Library
  Module:    vlSGridW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlSGridW.hh"

void vlStructuredGridWriter::WriteData()
{
  FILE *fp;
  vlStructuredGrid *input=(vlStructuredGrid *)this->Input;
  int dim[3];

  vlDebugMacro(<<"Writing vl structured grid...");

  if ( !(fp=this->OpenVLFile(this->Filename)) || !this->WriteHeader(fp) )
      return;
//
// Write structured grid specific stuff
//
  fprintf(fp,"DATASET STRUCTURED_GRID\n");

  input->GetDimensions(dim);
  fprintf(fp,"DIMENSIONS %d %d %d\n", dim[0], dim[1], dim[2]);

  this->WritePoints(fp, input->GetPoints());

  this->WritePointData(fp, input);
}

void vlStructuredGridWriter::Modified()
{
  this->vlDataWriter::Modified();
  this->vlStructuredGridFilter::_Modified();
}

unsigned long int vlStructuredGridWriter::GetMTime()
{
  unsigned long dtime = this->vlDataWriter::GetMTime();
  unsigned long ftime = this->vlStructuredGridFilter::_GetMTime();
  return (dtime > ftime ? dtime : ftime);
}

void vlStructuredGridWriter::DebugOn()
{
  vlDataWriter::DebugOn();
  vlStructuredGridFilter::_DebugOn();
}

void vlStructuredGridWriter::DebugOff()
{
  vlDataWriter::DebugOff();
  vlStructuredGridFilter::_DebugOff();
}

void vlStructuredGridWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataWriter::PrintSelf(os,indent);
  vlStructuredGridFilter::_PrintSelf(os,indent);
}
