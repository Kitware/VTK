/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDSW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vtkDSW.hh"
#include "vtkPolyW.hh"
#include "vtkSPtsW.hh"
#include "vtkSGridW.hh"
#include "vtkUGridW.hh"

// Description:
// Specify the input data or filter.
void vtkDataSetWriter::SetInput(vtkDataSet *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = input;
    this->Modified();
    }
}

void vtkDataSetWriter::WriteData()
{
  FILE *fp;
  char *type;
  vtkPolyWriter pwriter;
  vtkStructuredPointsWriter spwriter;
  vtkStructuredGridWriter sgwriter;
  vtkUnstructuredGridWriter ugwriter;
  vtkDataWriter *writer;

  vtkDebugMacro(<<"Writing vtk dataset...");

  type = this->Input->GetDataType();
  if ( ! strcmp(type,"vtkPolyData") )
    {
    pwriter.SetInput((vtkPolyData *)this->Input);
    writer = (vtkDataWriter *)&pwriter;
    }

  else if ( ! strcmp(type,"vtkStructuredPoints") )
    {
    spwriter.SetInput((vtkStructuredPoints *)this->Input);
    writer = (vtkDataWriter *)&spwriter;
    }

  else if ( ! strcmp(type,"vtkStructuredGrid") )
    {
    sgwriter.SetInput((vtkStructuredGrid *)this->Input);
    writer = (vtkDataWriter *)&sgwriter;
    }

  else if ( ! strcmp(type,"vtkUnstructuredGrid") )
    {
    ugwriter.SetInput((vtkUnstructuredGrid *)this->Input);
    writer = (vtkDataWriter *)&ugwriter;
    }

  else
    {
    vtkErrorMacro(<< "Cannot write dataset type: " << type);
    return;
    }

  writer->SetFilename(this->Filename);
  writer->SetScalarsName(this->ScalarsName);
  writer->SetVectorsName(this->VectorsName);
  writer->SetNormalsName(this->NormalsName);
  writer->SetTensorsName(this->TensorsName);
  writer->SetTCoordsName(this->TCoordsName);
  writer->SetLookupTableName(this->LookupTableName);
  writer->Write();

}

void vtkDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataWriter::PrintSelf(os,indent);
}
