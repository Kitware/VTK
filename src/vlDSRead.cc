/*=========================================================================

  Program:   Visualization Library
  Module:    vlDSRead.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlDSRead.hh"
#include "vlPolyR.hh"
#include "vlSPtsR.hh"
#include "vlSGridR.hh"
#include "vlUGridR.hh"


vlDataSetReader::vlDataSetReader()
{
}

vlDataSetReader::~vlDataSetReader()
{
}

// Description:
// Specify file name of vl data file to read.
void vlDataSetReader::SetFilename(char *name) 
{
  this->Reader.SetFilename(name);
}
char *vlDataSetReader::GetFilename() 
{
  return this->Reader.GetFilename();
}

// Description:
// Get the type of file (ASCII or BINARY)
int vlDataSetReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vlDataSetReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vlDataSetReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vlDataSetReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vlDataSetReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vlDataSetReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vlDataSetReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vlDataSetReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vlDataSetReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vlDataSetReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vlDataSetReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vlDataSetReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vlDataSetReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}


void vlDataSetReader::Execute()
{
  FILE *fp;
  int retStat;
  char line[257];
  vlDataSet *reader;

  vlDebugMacro(<<"Reading vl dataset...");
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
    vlErrorMacro(<< "Premature EOF reading dataset keyword");
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// See if type is recognized.
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      {
      vlErrorMacro(<< "Premature EOF reading type");
      return;
      }

    rewind(fp);
    if ( ! strncmp(this->Reader.LowerCase(line),"polydata",8) )
      {
      vlPolyReader *preader = new vlPolyReader;
      preader->SetFilename(this->Reader.GetFilename());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      reader = (vlDataSet *)preader;
      }

    else if ( ! strncmp(line,"structured_points",17) )
      {
      vlStructuredPointsReader *preader = new vlStructuredPointsReader;
      preader->SetFilename(this->Reader.GetFilename());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      reader = (vlDataSet *)preader;
      }

    else if ( ! strncmp(line,"structured_grid",15) )
      {
      vlStructuredGridReader *preader = new vlStructuredGridReader;
      preader->SetFilename(this->Reader.GetFilename());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      reader = (vlDataSet *)preader;
      }

    else if ( ! strncmp(line,"unstructured_grid",17) )
      {
      vlUnstructuredGridReader *preader = new vlUnstructuredGridReader;
      preader->SetFilename(this->Reader.GetFilename());
      preader->SetScalarsName(this->Reader.GetScalarsName());
      preader->SetVectorsName(this->Reader.GetVectorsName());
      preader->SetNormalsName(this->Reader.GetNormalsName());
      preader->SetTensorsName(this->Reader.GetTensorsName());
      preader->SetTCoordsName(this->Reader.GetTCoordsName());
      preader->SetLookupTableName(this->Reader.GetLookupTableName());
      preader->Update();
      reader = (vlDataSet *)preader;
      }

    else
      {
      vlErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
    }
//
// Create appropriate dataset
//
  if ( this->DataSet ) delete this->DataSet;
  this->DataSet = reader;
  *(this->DataSet->GetPointData()) = *(reader->GetPointData());

  return;
}

void vlDataSetReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
