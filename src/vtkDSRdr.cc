/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDSRdr.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vtkDSRead.hh"
#include "vtkPolyR.hh"
#include "vtkSPtsR.hh"
#include "vtkSGridR.hh"
#include "vtkUGridR.hh"


vtkDataSetReader::vtkDataSetReader()
{
}

vtkDataSetReader::~vtkDataSetReader()
{
}

// Description:
// Specify file name of vtk data file to read.
void vtkDataSetReader::SetFilename(char *name) 
{
  this->Reader.SetFilename(name);
}
char *vtkDataSetReader::GetFilename() 
{
  return this->Reader.GetFilename();
}

// Description:
// Get the type of file (ASCII or BINARY)
int vtkDataSetReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vtkDataSetReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vtkDataSetReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vtkDataSetReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vtkDataSetReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vtkDataSetReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vtkDataSetReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vtkDataSetReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vtkDataSetReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vtkDataSetReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vtkDataSetReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vtkDataSetReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vtkDataSetReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}


void vtkDataSetReader::Execute()
{
  FILE *fp;
  int retStat;
  char line[257];
  vtkDataSet *reader;

  vtkDebugMacro(<<"Reading vtk dataset...");
  this->Initialize();
  if ( this->Debug ) this->Reader.DebugOn();
  else this->Reader.DebugOff();

  if ( !(fp=this->Reader.OpenVLFile()) || !this->Reader.ReadHeader(fp) )
      return;
//
// Determine dataset type
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    {
    vtkErrorMacro(<< "Premature EOF reading dataset keyword");
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// See if type is recognized.
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      {
      vtkErrorMacro(<< "Premature EOF reading type");
      return;
      }

    rewind(fp);
    if ( ! strncmp(this->Reader.LowerCase(line),"polydata",8) )
      {
      vtkPolyReader *preader = new vtkPolyReader;
      preader->SetFilename(this->Reader.GetFilename());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      reader = (vtkDataSet *)preader;
      }

    else if ( ! strncmp(line,"structured_points",17) )
      {
      vtkStructuredPointsReader *preader = new vtkStructuredPointsReader;
      preader->SetFilename(this->Reader.GetFilename());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      reader = (vtkDataSet *)preader;
      }

    else if ( ! strncmp(line,"structured_grid",15) )
      {
      vtkStructuredGridReader *preader = new vtkStructuredGridReader;
      preader->SetFilename(this->Reader.GetFilename());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      reader = (vtkDataSet *)preader;
      }

    else if ( ! strncmp(line,"unstructured_grid",17) )
      {
      vtkUnstructuredGridReader *preader = new vtkUnstructuredGridReader;
      preader->SetFilename(this->Reader.GetFilename());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      reader = (vtkDataSet *)preader;
      }

    else
      {
      vtkErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
    }
//
// Create appropriate dataset
//
  if ( this->DataSet ) delete this->DataSet;
  this->DataSet = reader;
  this->PointData = *(reader->GetPointData());

  return;
}

void vtkDataSetReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
