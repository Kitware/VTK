/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSGrdW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vtkSGrdW.hh"

// Description:
// Specify the input data or filter.
void vtkStructuredGridWriter::SetInput(vtkStructuredGrid *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

void vtkStructuredGridWriter::WriteData()
{
  FILE *fp;
  vtkStructuredGrid *input=(vtkStructuredGrid *)this->Input;
  int dim[3];

  vtkDebugMacro(<<"Writing vtk structured grid...");

  if ( !(fp=this->OpenVLFile()) || !this->WriteHeader(fp) )
      return;
//
// Write structured grid specific stuff
//
  fprintf(fp,"DATASET STRUCTURED_GRID\n");

  input->GetDimensions(dim);
  fprintf(fp,"DIMENSIONS %d %d %d\n", dim[0], dim[1], dim[2]);

  this->WritePoints(fp, input->GetPoints());

  this->WritePointData(fp, input);

  this->CloseVLFile(fp);
}

void vtkStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
