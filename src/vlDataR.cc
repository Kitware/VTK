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
#include "IScalars.hh"
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

  this->Lut = NULL;

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
      if ( ! this->ReadScalarData(fp, ds, numPts) ) return 0;
      }
//
// read vector data
//
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(fp, ds, numPts) ) return 0;
      }
//
// read 3x3 tensor data
//
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(fp, ds, numPts) ) return 0;
      }
//
// read normals data
//
    else if ( ! strncmp(line, "normals", 7) )
      {
      if ( ! this->ReadNormalData(fp, ds, numPts) ) return 0;
      }
//
// read texture coordinates data
//
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(fp, ds, numPts) ) return 0;
      }
//
// read color scalars data
//
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(fp, ds, numPts) ) return 0;
      }
//
// read lookup table. Associate with scalar data.
//
    else if ( ! strncmp(line, "lookup_table", 12) )
      {
      if ( ! this->ReadLutData(fp, ds, numPts) ) return 0;
      }

    else
      {
      vlReadErrorMacro(<< "Unsupported point attribute type: " << line);
      return 0;
      }
    }

  return 1;

}

// Description:
// Read point coordinates. Return 0 if error.
int vlDataReader::ReadPoints(FILE *fp, vlPointSet *ps, int numPts)
{
  int retStat, i;
  char line[257];

  if ((retStat=fscanf(fp, "%s", line)) ==  EOF || retStat < 1) goto PREMATURE;

  if ( ! strncmp(this->LowerCase(line), "int", 3) )
    {
    vlIntPoints *points = new vlIntPoints(numPts);
    int *ptr = points->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(int),3*numPts,fp) != (3*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      int p[3];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%d %d %d",p,p+1,p+2)) == EOF || retStat < 3) goto PREMATURE;
        points->SetPoint(i,p);
        }
      }
    ps->SetPoints(points);
    }

  else if ( ! strncmp(line, "float", 5) )
    {
    vlFloatPoints *points = new vlFloatPoints(numPts);
    float *ptr = points->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),3*numPts,fp) != (3*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      float p[3];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",p,p+1,p+2)) == EOF || retStat < 3) goto PREMATURE;
        points->SetPoint(i,p);
        }
      }
    ps->SetPoints(points);
    }

  else 
    goto UNSUPPORTED;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF while reading points");
    return 0;

  UNSUPPORTED:
    vlReadErrorMacro(<< "Unsupported points type: " << line);
    return 0;
}

// Description:
// Read scalar point attributes. Return 0 if error.
int vlDataReader::ReadScalarData(FILE *fp, vlDataSet *ds, int numPts)
{
  char line[257];
  int retStat, i;

  if ((retStat=fscanf(fp, "%s", line)) ==  EOF || retStat < 1) goto PREMATURE;
  if ( ! strncmp(this->LowerCase(line), "bit", 3) )
    {
    vlBitScalars *scalars = new vlBitScalars(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),(numPts+1)/8,fp) != ((numPts+1)/8) ) goto PREMATURE;
      }
    else // ascii
      {
      int iv;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%d",&iv)) == EOF || retStat < 1) goto PREMATURE;
        scalars->SetScalar(i,iv);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else if ( ! strncmp(line, "char", 4) )
    {
    vlCharScalars *scalars = new vlCharScalars(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),numPts,fp) != numPts ) goto PREMATURE;
      }
    else // ascii
      {
      unsigned char c;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%c",&c)) == EOF || retStat < 1) goto PREMATURE;
        scalars->SetScalar(i,c);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else if ( ! strncmp(line, "short", 5) )
    {
    vlShortScalars *scalars = new vlShortScalars(numPts);
    short *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(short),numPts,fp) != numPts ) goto PREMATURE;
      }
    else // ascii
      {
      short s;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%hd",&s)) == EOF || retStat < 1) goto PREMATURE;
        scalars->SetScalar(i,s);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else if ( ! strncmp(line, "int", 3) )
    {
    vlIntScalars *scalars = new vlIntScalars(numPts);
    int *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(int),numPts,fp) != numPts ) goto PREMATURE;
      }
    else // ascii
      {
      int iv;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%d",&iv)) == EOF || retStat < 1) goto PREMATURE;
        scalars->SetScalar(i,iv);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else if ( ! strncmp(line, "float", 5) )
    {
    vlFloatScalars *scalars = new vlFloatScalars(numPts);
    float *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),numPts,fp) != numPts ) goto PREMATURE;
      }
    else // ascii
      {
      float f;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f",&f)) == EOF || retStat < 1) goto PREMATURE;
        scalars->SetScalar(i,f);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else 
    goto UNSUPPORTED;
//
// Assign lookup table if previously read
//
  if (this->Lut) ds->GetPointData()->GetScalars()->SetLookupTable(this->Lut);
  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF while reading scalar data");
    return 0;

  UNSUPPORTED:
    vlReadErrorMacro(<< "Unsupported scalar data type: " << line);
    return 0;
}

// Description:
// Read vector point attributes. Return 0 if error.
int vlDataReader::ReadVectorData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i;
  char line[257];

  if ((retStat=fscanf(fp, "%s", line)) ==  EOF || retStat < 1) goto PREMATURE;
  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vlFloatVectors *vectors = new vlFloatVectors(numPts);
    float *ptr = vectors->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),3*numPts,fp) != (3*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      float v[3];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",v,v+1,v+2)) == EOF || retStat < 3) goto PREMATURE;
        vectors->SetVector(i,v);
        }
      }
    ds->GetPointData()->SetVectors(vectors);
    }

  else 
    goto UNSUPPORTED;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF while reading vector data");
    return 0;

  UNSUPPORTED:
    vlReadErrorMacro(<< "Unsupported vector data type: " << line);
    return 0;
}

// Description:
// Read normal point attributes. Return 0 if error.
int vlDataReader::ReadNormalData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i;
  char line[257];

  if ((retStat=fscanf(fp, "%s", line)) ==  EOF || retStat < 1) goto PREMATURE;
  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vlFloatNormals *normals = new vlFloatNormals(numPts);
    float *ptr = normals->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),3*numPts,fp) != (3*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      float n[3];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",n,n+1,n+2)) == EOF || retStat < 3) goto PREMATURE;
        normals->SetNormal(i,n);
        }
      }
    ds->GetPointData()->SetNormals(normals);
    }

  else 
    goto UNSUPPORTED;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF while reading normal data");
    return 0;

  UNSUPPORTED:
    vlReadErrorMacro(<< "Unsupported normal data type: " << line);
    return 0;
}

// Description:
// Read tensor point attributes. Return 0 if error.
int vlDataReader::ReadTensorData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i;
  char line[257];

  if ((retStat=fscanf(fp, "%s", line)) ==  EOF || retStat < 1) goto PREMATURE;
  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vlFloatTensors *tensors = new vlFloatTensors(numPts);
    tensors->SetDimension(3);
    float *ptr = tensors->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),9*numPts,fp) != (9*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      vlTensor tensor;
      float t[9];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",t,t+1,t+2,t+3,t+4,t+5,t+6,t+7,t+8)) == EOF || retStat < 9) goto PREMATURE;
        tensor = t;
        tensors->SetTensor(i,tensor);
        }
      }
    ds->GetPointData()->SetTensors(tensors);
    }

  else 
    goto UNSUPPORTED;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF while reading tensor data");
    return 0;

  UNSUPPORTED:
    vlReadErrorMacro(<< "Unsupported tensor data type: " << line);
    return 0;
}

// Description:
// Read color scalar point attributes. Return 0 if error.
int vlDataReader::ReadCoScalarData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, nBytes;
  char line[257];

  if ((retStat=fscanf(fp, "%d", &nBytes)) ==  EOF || retStat < 1) goto PREMATURE;
  if ( nBytes == 1 )
    {
    vlGraymap *scalars = new vlGraymap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),numPts,fp) != numPts ) goto PREMATURE;
      }
    else // ascii
      {
      unsigned char rgba[4];
      rgba[1] = rgba[2] = rgba[3] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%c",rgba)) == EOF || retStat < 1) goto PREMATURE;
        scalars->SetColor(i,rgba);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else if ( nBytes == 2 )
    {
    vlAGraymap *scalars = new vlAGraymap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),2*numPts,fp) != (2*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      unsigned char rgba[4];
      rgba[1] = rgba[2] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%c %c",rgba,rgba+3)) == EOF || retStat < 2) goto PREMATURE;
        scalars->SetColor(i,rgba);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else if ( nBytes == 3 )
    {
    vlPixmap *scalars = new vlPixmap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),3*numPts,fp) != (3*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      unsigned char rgba[4];
      rgba[3] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%c %c %c",rgba,rgba+1,rgba+2)) == EOF || retStat < 3) goto PREMATURE;
        scalars->SetColor(i,rgba);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else if ( nBytes == 4 )
    {
    vlAPixmap *scalars = new vlAPixmap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),4*numPts,fp) != (4*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      unsigned char rgba[4];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%c %c %c %c",rgba,rgba+1,rgba+2,rgba+3)) == EOF || retStat < 4) goto PREMATURE;
        scalars->SetColor(i,rgba);
        }
      }
    ds->GetPointData()->SetScalars(scalars);
    }

  else
    goto UNSUPPORTED;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF while reading color scalars data");
    return 0;

  UNSUPPORTED:
    vlReadErrorMacro(<< "Do not support " << nBytes << " per scalar");
    return 0;
}

// Description:
// Read texture coordinates point attributes. Return 0 if error.
int vlDataReader::ReadTCoordsData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, dim;
  char line[257];

  if ((retStat=fscanf(fp, "%d %s", &dim, line)) ==  EOF || retStat < 2) goto PREMATURE;

  if ( dim < 1 || dim > 3 ) goto UNSUPPORTED_DIMENSION;

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vlFloatTCoords *tcoords = new vlFloatTCoords(numPts);
    tcoords->SetDimension(dim);
    float *ptr = tcoords->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),dim*numPts,fp) != (dim*numPts) ) goto PREMATURE;
      }
    else // ascii
      {
      float tc[3];
      int j;
      for (i=0; i<numPts; i++)
        {
        for (j=0; j<dim; j++)
          if ((retStat=fscanf(fp,"%f",tc+j)) == EOF || retStat < 1) goto PREMATURE;
        tcoords->SetTCoord(i,tc);
        }
      }
    ds->GetPointData()->SetTCoords(tcoords);
    }

  else 
    goto UNSUPPORTED;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF while reading texture coordinates data");
    return 0;

  UNSUPPORTED:
    vlReadErrorMacro(<< "Unsupported texture coordinates data type: " << line);
    return 0;

  UNSUPPORTED_DIMENSION:
    vlReadErrorMacro(<< "Unsupported texture coordinates dimension: " << dim);
    return 0;

}

// Description:
// Read lookup table. Return 0 if error.
int vlDataReader::ReadLutData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, size;
  vlLookupTable *lut;
  unsigned char *ptr;
  char line[257];

  if ((retStat=fscanf(fp, "%d", &size)) ==  EOF || retStat < 1) goto PREMATURE;

  lut = new vlLookupTable(size);
  ptr = lut->WritePtr(0,size);

  if ( this->FileType == BINARY)
    {
    if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
    if ( fread(ptr,sizeof(unsigned char),4*size,fp) != (4*size) ) goto PREMATURE;
    }
  else // ascii
    {
    unsigned char rgba[4];
    for (i=0; i<size; i++)
      {
      if ((retStat=fscanf(fp,"%c %c %d %c",rgba,rgba+1,rgba+2,rgba+3)) == EOF || retStat < 4) goto PREMATURE;
      lut->SetTableValue(i,rgba);
      }
    }
  this->Lut == lut;
  if ( ds->GetPointData()->GetScalars() != NULL )
    ds->GetPointData()->GetScalars()->SetLookupTable(lut);
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlReadErrorMacro(<< "Premature EOF while lookup table");
    return 0;
}


char *vlDataReader::LowerCase(char *str)
{
  for ( char *s=str; *s != '\0'; s++) *s = tolower(*s);
  return str;
}

