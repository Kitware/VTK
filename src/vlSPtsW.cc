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
  FILE *fp;
  vlStructuredPoints *input=(vlStructuredPoints *)this->Input;
  int dim[3];
  float ar[3], origin[3];

  vlDebugMacro(<<"Writing vl structured points...");

  if ( !(fp=this->OpenVLFile(this->Filename)) || !this->WriteHeader(fp) )
      return;
//
// Write structured points specific stuff
//
  fprintf(fp,"DATASET STRUCTURED_POINTS\n");

  input->GetDimensions(dim);
  fprintf(fp,"DIMENSIONS %d %d %d\n", dim[0], dim[1], dim[2]);

  input->GetAspectRatio(ar);
  fprintf(fp,"ASPECT_RATIO %f %f %f\n", ar[0], ar[1], ar[2]);

  input->GetOrigin(origin);
  fprintf(fp,"ORIGIN %f %f %f\n", origin[0], origin[1], origin[2]);

  this->WritePointData(fp, input);
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
