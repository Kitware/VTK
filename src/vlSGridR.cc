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
  this->Filename = NULL;
}

vlStructuredGridReader::~vlStructuredGridReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlStructuredGridReader::Execute()
{
  FILE *fp;
  int numPts=0;
  int retStat;
  char line[257];
  int npts;

  vlDebugMacro(<<"Reading vl structured grid file...");
  this->Initialize();

  if ( !(fp=this->OpenVLFile(this->Filename, this->Debug)) ||
  ! this->ReadHeader(fp,this->Debug) )
      return;
//
// Read structured points specific stuff
//
  if ( (retStat=fscanf(fp,"%s",line)) == EOF || retStat < 1 ) goto PREMATURE;

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
    {
//
// Make sure we're reading right type of geometry
//
    if ( (retStat=fscanf(fp,"%s",line)) == EOF || retStat < 1 ) 
      goto PREMATURE;

    if ( strncmp(this->LowerCase(line),"structured_grid",17) )
      {
      vlReadErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
//
// Read keyword and number of points
//
    while (1)
      {
      if ( (retStat=fscanf(fp,"%s",line)) == EOF || retStat < 1 ) 
        goto PREMATURE;

      if ( ! strncmp(this->LowerCase(line),"dimensions",10) )
        {
        int dim[3];
        if ( (retStat=fscanf(fp,"%d %d %d",dim, dim+1, dim+2)) == EOF
        || retStat < 3 ) 
          goto PREMATURE;

        numPts = dim[0] * dim[1] * dim[2];
        this->SetDimensions(dim);
        }

      else if ( ! strncmp(line,"points",12) )
        {
        int npts;
        if ( (retStat=fscanf(fp,"%d",npts)) == EOF || retStat < 1 ) 
          goto PREMATURE;

        if ( npts > 0 && npts != numPts )

        this->ReadPoints(fp, (vlPointSet *)this, npts);
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
  this->ReadPointData(fp, (vlDataSet *)this, numPts, this->Debug);
  return;

  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF");
    return;

  UNRECOGNIZED:
    vlReadErrorMacro(<< "Unrecognized keyord: " << line);
    return;

}

void vlStructuredGridReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredGridSource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}
