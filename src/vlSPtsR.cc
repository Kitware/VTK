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
  this->Filename = NULL;
}

vlStructuredPointsReader::~vlStructuredPointsReader()
{
  if ( this->Filename ) delete [] this->Filename;
}

void vlStructuredPointsReader::Execute()
{
  FILE *fp;
  int numPts=0;
  int retStat;
  char line[257];
  int npts;

  vlDebugMacro(<<"Reading vl structured points file...");
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

    if ( strncmp(this->LowerCase(line),"structured_points",17) )
      {
      vlReadErrorMacro(<< "Cannot read dataset type: " << line);
      return;
      }
//
// Read keyword and number of points
//
    numPts = this->GetNumberOfPoints(); // get default
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

      else if ( ! strncmp(line,"aspect_ratio",12) )
        {
        float ar[3];
        if ( (retStat=fscanf(fp,"%f %f %f",ar, ar+1, ar+2)) == EOF
        || retStat < 3 ) 
          goto PREMATURE;

        this->SetAspectRatio(ar);
        }

      else if ( ! strncmp(line,"origin",6) )
        {
        float origin[3];
        if ( (retStat=fscanf(fp,"%f %f %f",origin, origin+1, origin+2)) == EOF
        || retStat < 3 ) 
          goto PREMATURE;

        this->SetOrigin(origin);
        }

      else if ( ! strncmp(line, "point_data", 16) )
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

  else if ( !strncmp(line,"point_attributes",16) )
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


void vlStructuredPointsReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Filename: " << this->Filename << "\n";
}
