/*=========================================================================

  Program:   Visualization Library
  Module:    vlSGridR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlSGridR.hh"

vlStructuredGridReader::vlStructuredGridReader()
{
}

vlStructuredGridReader::~vlStructuredGridReader()
{
}

unsigned long int vlStructuredGridReader::GetMTime()
{
  unsigned long dtime = this->vlStructuredGridSource::GetMTime();
  unsigned long rtime = this->Reader.GetMTime();
  return (dtime > rtime ? dtime : rtime);
}

// Description:
// Specify file name of vl polygonal data file to read.
void vlStructuredGridReader::SetFilename(char *name) 
{
  this->Reader.SetFilename(name);
}
char *vlStructuredGridReader::GetFilename() 
{
  return this->Reader.GetFilename();
}

// Description:
// Get the type of file (ASCII or BINARY)
int vlStructuredGridReader::GetFileType() 
{
  return this->Reader.GetFileType();
}

// Description:
// Set the name of the scalar data to extract. If not specified, first 
// scalar data encountered is extracted.
void vlStructuredGridReader::SetScalarsName(char *name) 
{
  this->Reader.SetScalarsName(name);
}
char *vlStructuredGridReader::GetScalarsName() 
{
  return this->Reader.GetScalarsName();
}

// Description:
// Set the name of the vector data to extract. If not specified, first 
// vector data encountered is extracted.
void vlStructuredGridReader::SetVectorsName(char *name) 
{
  this->Reader.SetVectorsName(name);
}
char *vlStructuredGridReader::GetVectorsName() 
{
  return this->Reader.GetVectorsName();
}

// Description:
// Set the name of the tensor data to extract. If not specified, first 
// tensor data encountered is extracted.
void vlStructuredGridReader::SetTensorsName(char *name) 
{
  this->Reader.SetTensorsName(name);
}
char *vlStructuredGridReader::GetTensorsName() 
{
  return this->Reader.GetTensorsName();
}

// Description:
// Set the name of the normal data to extract. If not specified, first 
// normal data encountered is extracted.
void vlStructuredGridReader::SetNormalsName(char *name) 
{
  this->Reader.SetNormalsName(name);
}
char *vlStructuredGridReader::GetNormalsName() 
{
  return this->Reader.GetNormalsName();
}

// Description:
// Set the name of the texture coordinate data to extract. If not specified,
// first texture coordinate data encountered is extracted.
void vlStructuredGridReader::SetTCoordsName(char *name) 
{
  this->Reader.SetTCoordsName(name);
}
char *vlStructuredGridReader::GetTCoordsName() 
{
  return this->Reader.GetTCoordsName();
}

// Description:
// Set the name of the lookup table data to extract. If not specified, uses 
// lookup table named by scalar. Otherwise, this specification supersedes.
void vlStructuredGridReader::SetLookupTableName(char *name) 
{
  this->Reader.SetLookupTableName(name);
}
char *vlStructuredGridReader::GetLookupTableName() 
{
  return this->Reader.GetLookupTableName();
}

void vlStructuredGridReader::Execute()
{
  FILE *fp;
  int numPts=0, npts;
  int retStat;
  char line[257];

  vlDebugMacro(<<"Reading vl structured grid file...");
  this->Initialize();

  if ( !(fp=this->Reader.OpenVLFile(this->Debug)) ||
  ! this->Reader.ReadHeader(fp,this->Debug) )
      return;
//
// Read structured grid specific stuff
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    goto PREMATURE;

  if ( !strncmp(this->Reader.LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
      goto PREMATURE;

    if ( strncmp(this->Reader.LowerCase(line),"structured_grid",17) )
      {
      vlErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
//
// Read keyword and number of points
//
    while (1)
      {
      if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
        goto PREMATURE;

      if ( ! strncmp(this->Reader.LowerCase(line),"dimensions",10) )
        {
        int dim[3];
        if ( (retStat=fscanf(fp,"%d %d %d",dim, dim+1, dim+2)) == EOF
        || retStat < 3 ) 
          goto PREMATURE;

        numPts = dim[0] * dim[1] * dim[2];
        this->SetDimensions(dim);
        }

      else if ( ! strncmp(line,"points",6) )
        {
        if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
          goto PREMATURE;

        this->Reader.ReadPoints(fp, (vlPointSet *)this, npts);
        }

      else if ( ! strncmp(line, "point_data", 10) )
        {
        if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
          goto PREMATURE;
        
        if ( npts != numPts )
          {
          vlErrorMacro(<<"Number of points don't match!");
          return;
          }

        break; //out of this loop
        }

      else
        goto UNRECOGNIZED;

      }
    }

  else if ( !strncmp(line, "point_data", 10) )
    {
    if ( (retStat=fscanf(fp,"%d", &npts)) == EOF || retStat < 1 ) 
      goto PREMATURE;

    numPts = 0;
    vlWarningMacro(<<"Not reading any dataset geometry...");
    }

  else 
    goto UNRECOGNIZED;

//
// Now read the point data
//
  this->Reader.ReadPointData(fp, (vlDataSet *)this, numPts, this->Debug);
  return;

  PREMATURE:
    vlErrorMacro(<< "Premature EOF");
    return;

  UNRECOGNIZED:
    vlErrorMacro(<< "Unrecognized keyord: " << line);
    return;

}

void vlStructuredGridReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredGridSource::PrintSelf(os,indent);
  this->Reader.PrintSelf(os,indent);
}
