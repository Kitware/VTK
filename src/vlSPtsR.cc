/*=========================================================================

  Program:   Visualization Library
  Module:    vlSPtsR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlSPtsR.hh"

vlStructuredPointsReader::vlStructuredPointsReader()
{
}

vlStructuredPointsReader::~vlStructuredPointsReader()
{
}

unsigned long int vlStructuredPointsReader::GetMTime()
{
  unsigned long dtime = this->vlStructuredPointsSource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Description:
// Specify file name of vl polygonal data file to read.
void vlStructuredPointsReader::SetFilename(char *name) 
{
  this->Reader.SetFilename(name);
}
char *vlStructuredPointsReader::GetFilename() 
{
  return this->Reader.GetFilename();
}

// Description:
// Get the type of file (ASCII or BINARY)
int vlStructuredPointsReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vlStructuredPointsReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vlStructuredPointsReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vlStructuredPointsReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vlStructuredPointsReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vlStructuredPointsReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vlStructuredPointsReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vlStructuredPointsReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vlStructuredPointsReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vlStructuredPointsReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vlStructuredPointsReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vlStructuredPointsReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vlStructuredPointsReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}

void vlStructuredPointsReader::Execute()
{
  FILE *fp;
  int numPts=0;
  int retStat;
  char line[257];
  int npts;
  int dimsRead=0, arRead=0, originRead=0;

  vlDebugMacro(<<"Reading vl structured points file...");
  this->Initialize();

  if ( !(fp=this->Reader.OpenVLFile(this->Debug)) ||
  ! this->Reader.ReadHeader(fp,this->Debug) )
      return;
//
// Read structured points specific stuff
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    {
    vlErrorMacro(<<"Data file ends prematurely!");
    return;
    }

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      {
      vlErrorMacro(<<"Data file ends prematurely!");
      return;
      } 

    if ( strncmp(this->Reader.LowerCase(line),"structured_points",17) )
      {
      vlErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
//
// Read keyword and number of points
//
    numPts = this->GetNumberOfPoints(); // get default
    while (1)
      {
      if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) break;

      if ( ! strncmp(this->Reader.LowerCase(line),"dimensions",10) )
        {
        int dim[3];
        if ( (retStat=fscanf(fp,"%d %d %d",dim, dim+1, dim+2)) == EOF
        || retStat < 3 ) 
          {
          vlErrorMacro(<<"Error reading dimensions!");
          return;
          }

        numPts = dim[0] * dim[1] * dim[2];
        this->SetDimensions(dim);
        dimsRead = 1;
        }

      else if ( ! strncmp(line,"aspect_ratio",12) )
        {
        float ar[3];
        if ( (retStat=fscanf(fp,"%f %f %f",ar, ar+1, ar+2)) == EOF
        || retStat < 3 ) 
          {
          vlErrorMacro(<<"Error reading aspect ratio!");
          return;
          }

        this->SetAspectRatio(ar);
        arRead = 1;
        }

      else if ( ! strncmp(line,"origin",6) )
        {
        float origin[3];
        if ( (retStat=fscanf(fp,"%f %f %f",origin, origin+1, origin+2)) == EOF
        || retStat < 3 ) 
          {
          vlErrorMacro(<<"Error reading origin!");
          return;
          }

        this->SetOrigin(origin);
        originRead = 1;
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
          {
          vlErrorMacro(<<"Cannot read point data!");
          return;
          }
        
        if ( npts != numPts )
          {
          vlErrorMacro(<<"Number of points don't match data values!");
          return;
          }

        this->Reader.ReadPointData(fp, (vlDataSet *)this, npts, this->Debug);
        break; //out of this loop
        }

      else
        {
        vlErrorMacro(<< "Unrecognized keyord: " << line);
        return;
        }
      }

      if ( !dimsRead ) vlWarningMacro(<<"No dimensions read.");
      if ( !arRead ) vlWarningMacro(<<"No aspect ratio read.");
      if ( !originRead ) vlWarningMacro(<<"No origin read.");
    }

  else if ( !strncmp(line,"point_data",10) )
    {
    vlWarningMacro(<<"No geometry defined in data file!");
    if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
      {
      vlErrorMacro(<<"Cannot read point data!");
      return;
      }
    this->Reader.ReadPointData(fp, (vlDataSet *)this, numPts, this->Debug);
    }

  else 
    {
    vlErrorMacro(<< "Unrecognized keyord: " << line);
    }
}


void vlStructuredPointsReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredPointsSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
