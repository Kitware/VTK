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

void vlDataSetReader::Execute()
{
  FILE *fp;
  int retStat;
  char line[257];
  vlDataSet *reader;

  vlDebugMacro(<<"Reading vl dataset...");
  this->Initialize();

  if ( !(fp=this->Reader.OpenVLFile(this->Debug)) ||
  ! this->Reader.ReadHeader(fp,this->Debug) )
      return;
//
// Determine dataset type
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    goto PREMATURE;

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// See if type is recognized.
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      goto PREMATURE;

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
  return;

  PREMATURE:
    vlErrorMacro(<< "Premature EOF");
    return;
}

void vlDataSetReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlDataSetSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
