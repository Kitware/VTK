/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPtsW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vtkSPtsW.hh"

// Description:
// Specify the input data or filter.
void vtkStructuredPointsWriter::SetInput(vtkStructuredPoints *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

void vtkStructuredPointsWriter::WriteData()
{
  FILE *fp;
  vtkStructuredPoints *input=(vtkStructuredPoints *)this->Input;
  int dim[3];
  float ar[3], origin[3];

  vtkDebugMacro(<<"Writing vtk structured points...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
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

  this->CloseVTKFile(fp);
}

void vtkStructuredPointsWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
