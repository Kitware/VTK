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

// Description:
// Specify the input data or filter.
void vlStructuredPointsWriter::SetInput(vlStructuredPoints *input)
{
  if ( this->Input != input )
    {
    vlDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vlDataSet *) input;
    this->Modified();
    }
}

void vlStructuredPointsWriter::WriteData()
{
  FILE *fp;
  vlStructuredPoints *input=(vlStructuredPoints *)this->Input;
  int dim[3];
  float ar[3], origin[3];

  vlDebugMacro(<<"Writing vl structured points...");

  if ( !(fp=this->OpenVLFile()) || !this->WriteHeader(fp) )
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

void vlStructuredPointsWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataWriter::PrintSelf(os,indent);
}
