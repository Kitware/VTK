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

// Description:
// Specify the input data or filter.
void vlPolyWriter::SetInput(vlPolyData *input)
{
  if ( this->Input != input )
    {
    vlDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vlDataSet *) input;
    this->Modified();
    }
}

void vlPolyWriter::WriteData()
{
  FILE *fp;
  vlPolyData *input=(vlPolyData *)this->Input;

  vlDebugMacro(<<"Writing vl polygonal data...");

  if ( !(fp=this->OpenVLFile()) || !this->WriteHeader(fp) )
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

  this->CloseVLFile(fp);
}

void vlPolyWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataWriter::PrintSelf(os,indent);
}
