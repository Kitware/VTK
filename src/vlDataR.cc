/*=========================================================================

  Program:   Visualization Library
  Module:    vlDataR.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlDataR.hh"
#include <ctype.h>

#include "BScalars.hh"
#include "CScalars.hh"
#include "FScalars.hh"
#include "SScalars.hh"
#include "FPoints.hh"
#include "IPoints.hh"
#include "FNormals.hh"
#include "FTensors.hh"
#include "FTCoords.hh"
#include "Graymap.hh"
#include "AGraymap.hh"
#include "Pixmap.hh"
#include "APixmap.hh"
#include "Lut.hh"

// Description:
// Open a vl data file. Returns NULL if error.
FILE *vlDataReader::OpenVLFile(char *filename, int debug)
{
  FILE *fptr=NULL;

  if ( debug )
    {
    vlReadDebugMacro(<< "Opening vl file");
    }

  if ( !filename || (fptr=fopen(filename, "rb")) == NULL )
    {
    vlReadErrorMacro(<< "Unable to open file: "<< filename);
    }

  return fptr;
}

// Description:
// Read the header of a vl data file. Returns 0 if error.
int vlDataReader::ReadHeader(FILE *fp, int debug)
{
  char line[257];
  int retStat;

  if ( debug )
    {
    vlReadDebugMacro(<< "Reading vl file header");
    }
//
// read header
//
  if ( fgets (line, 256, fp) == NULL ) goto PREMATURE;
  line[256] = '\0';
  if ( strncmp ("# vl DataSet Version", line, 20) )
    {
    vlReadErrorMacro(<< "Unrecognized header: "<< line);
    return 0;
    }
//
// read title
//
  if ( fgets (line, 256, fp) == NULL ) goto PREMATURE;
  line[256] = '\0';
  if ( debug )
    {
    vlReadDebugMacro(<< "Reading vl file entitled: " << line);
    }
//
// read type
//
  if ( (retStat=fscanf(fp,"%s",line)) == EOF || retStat < 1 ) goto PREMATURE;

  if ( !strncmp(this->LowerCase(line),"ascii",5) ) this->FileType = ASCII;
  else if ( !strncmp(line,"binary",6) ) this->FileType = BINARY;
  else
    {
    vlReadErrorMacro(<< "Unrecognized file type: "<< line);
    this->FileType == NULL;
    return 0;
    }

  return 1;

  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF");
    return 0;
}

// Description:
// Read the point data of a vl data file. The number of points (from the 
// dataset) must match the number of points defined in point attributes unless
// numPts<=0, which means no points were defined in the dataset. Returns 0 if 
// error.
int vlDataReader::ReadPointData(FILE *fp, vlDataSet *ds, int numPts, int debug)
{
  int retStat;
  char line[257];

  if ( debug )
    {
    vlReadDebugMacro(<< "Reading vl point data");
    }
//
// Read keywords until end-of-file
//
  while ( (retStat=fscanf(fp, "%s", line)) != EOF && retStat == 1 ) 
    {
//
// read scalar data
//
    if ( ! strncmp(this->LowerCase(line), "scalars", 7) )
      {
      fscanf(fp, "%s", line);
      if ( ! strncmp(line, "bit", 3) )
        {
        }

      else if ( ! strncmp(line, "char", 4) )
        {
        vlCharScalars *scalars = new vlCharScalars(numPts);
        unsigned char *ptr = scalars->WritePtr(0,numPts);
        if ( this->FileType == BINARY)
          {
          fgets(line,256,fp); //suck up newline
          fread(ptr,sizeof(unsigned char),numPts,fp);
          }
        else // ascii
          {
          }
        ds->GetPointData()->SetScalars(scalars);
        }

      else if ( ! strncmp(line, "short", 5) )
        {
        }

      else if ( ! strncmp(line, "int", 3) )
        {
        }

      else if ( ! strncmp(line, "float", 5) )
        {
        }

      else if ( ! strncmp(line, "double", 6) )
        {
        }

      else 
        goto UNSUPPORTED;

      }
//
// read vector data
//
    else if ( ! strncmp(line, "vectors", 7) )
      {
      }
//
// read 3x3 tensor data
//
    else if ( ! strncmp(line, "tensors", 7) )
      {
      }
//
// read normals data
//
    else if ( ! strncmp(line, "normals", 7) )
      {
      }
//
// read texture coordinates data
//
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      }
//
// read color scalars data
//
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      }
//
// read lookup table. Associate with scalar data.
//
    else if ( ! strncmp(line, "lookup_table", 12) )
      {
      }

    else
      {
      vlReadErrorMacro(<< "Unsupported point attribute type: " << line);
      return 0;
      }
    }

  return 1;

  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF");
    return 0;

  UNSUPPORTED:
    vlReadErrorMacro(<< "Unsupported data type: " << line);
    return 0;
}

char *vlDataReader::LowerCase(char *str)
{
  for ( char *s=str; *s != '\0'; s++) *s = tolower(*s);
  return str;
}
