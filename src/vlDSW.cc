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
#include "vlPolyW.hh"
#include "vlSPtsW.hh"
#include "vlSGridW.hh"
#include "vlUGridW.hh"

// Description:
// Specify the input data or filter.
void vlDataSetWriter::SetInput(vlDataSet *input)
{
  if ( this->Input != input )
    {
    vlDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = input;
    this->Modified();
    }
}

void vlDataSetWriter::WriteData()
{
  FILE *fp;
  char *type;
  vlPolyWriter pwriter;
  vlStructuredPointsWriter spwriter;
  vlStructuredGridWriter sgwriter;
  vlUnstructuredGridWriter ugwriter;
  vlDataWriter *writer;

  vlDebugMacro(<<"Writing vl dataset...");

  type = this->Input->GetDataType();
  if ( ! strcmp(type,"vlPolyData") )
    {
    pwriter.SetInput((vlPolyData *)this->Input);
    writer = (vlDataWriter *)&pwriter;
    }

  else if ( ! strcmp(type,"vlStructuredPoints") )
    {
    spwriter.SetInput((vlStructuredPoints *)this->Input);
    writer = (vlDataWriter *)&spwriter;
    }

  else if ( ! strcmp(type,"vlStructuredGrid") )
    {
    sgwriter.SetInput((vlStructuredGrid *)this->Input);
    writer = (vlDataWriter *)&sgwriter;
    }

  else if ( ! strcmp(type,"vlUnstructuredGrid") )
    {
    ugwriter.SetInput((vlUnstructuredGrid *)this->Input);
    writer = (vlDataWriter *)&ugwriter;
    }

  else
    {
    vlErrorMacro(<< "Cannot write dataset type: " << type);
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

void vlDataSetWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataWriter::PrintSelf(os,indent);
}
