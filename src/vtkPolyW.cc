/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vtkPolyW.hh"

// Description:
// Specify the input data or filter.
void vtkPolyWriter::SetInput(vtkPolyData *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vtkDataSet *) input;
    this->Modified();
    }
}

void vtkPolyWriter::WriteData()
{
  FILE *fp;
  vtkPolyData *input=(vtkPolyData *)this->Input;

  vtkDebugMacro(<<"Writing vtk polygonal data...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
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

  this->CloseVTKFile(fp);
}

void vtkPolyWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
