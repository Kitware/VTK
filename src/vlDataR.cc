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
// Construct object.
vlDataReader::vlDataReader()
{
  this->ScalarsName = NULL;
  this->VectorsName = NULL;
  this->TensorsName = NULL;
  this->NormalsName = NULL;
  this->TCoordsName = NULL;
  this->LookupTableName = NULL;
  this->ScalarLut = NULL;
}  

vlDataReader::~vlDataReader()
{
  if (this->ScalarsName) delete [] this->ScalarsName;
  if (this->VectorsName) delete [] this->VectorsName;
  if (this->TensorsName) delete [] this->TensorsName;
  if (this->NormalsName) delete [] this->NormalsName;
  if (this->TCoordsName) delete [] this->TCoordsName;
  if (this->LookupTableName) delete [] this->LookupTableName;
  if (this->ScalarLut) delete [] this->ScalarLut;
}

// Description:
// Open a vl data file. Returns NULL if error.
FILE *vlDataReader::OpenVLFile(char *filename, int debug)
{
  FILE *fptr=NULL;

  if ( debug )
    {
    vlDebugMacro(<< "Opening vl file");
    }

  if ( !filename || (fptr=fopen(filename, "rb")) == NULL )
    {
    vlErrorMacro(<< "Unable to open file: "<< filename);
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
    vlDebugMacro(<< "Reading vl file header");
    }
//
// read header
//
  if ( fgets (line, 256, fp) == NULL ) goto PREMATURE;
  line[256] = '\0';
  if ( strncmp ("# vl DataSet Version", line, 20) )
    {
    vlErrorMacro(<< "Unrecognized header: "<< line);
    return 0;
    }
//
// read title
//
  if ( fgets (line, 256, fp) == NULL ) goto PREMATURE;
  line[256] = '\0';
  if ( debug )
    {
    vlDebugMacro(<< "Reading vl file entitled: " << line);
    }
//
// read type
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    goto PREMATURE;

  if ( !strncmp(this->LowerCase(line),"ascii",5) ) this->FileType = ASCII;
  else if ( !strncmp(line,"binary",6) ) this->FileType = BINARY;
  else
    {
    vlErrorMacro(<< "Unrecognized file type: "<< line);
    this->FileType = NULL;
    return 0;
    }

  return 1;

  PREMATURE:
    vlErrorMacro(<< "Premature EOF");
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
    vlDebugMacro(<< "Reading vl point data");
    }
//
// Read keywords until end-of-file
//
  while ( (retStat=fscanf(fp, "%256s", line)) != EOF && retStat == 1 ) 
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
      vlErrorMacro(<< "Unsupported point attribute type: " << line);
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

  if ((retStat=fscanf(fp, "%256s", line)) ==  EOF || retStat < 1) 
    goto PREMATURE;

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

  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading points");
    return 0;

  UNSUPPORTED:
    vlErrorMacro(<< "Unsupported points type: " << line);
    return 0;
}

// Description:
// Read scalar point attributes. Return 0 if error.
int vlDataReader::ReadScalarData(FILE *fp, vlDataSet *ds, int numPts)
{
  char line[257], name[257], key[128], tableName[257];
  int retStat, i, skipScalar=0;

  if ( (retStat=fscanf(fp, "%256s %256s", name, line)) == EOF || retStat < 2 ||
  ((retStat=fscanf(fp, "%256s %256s", key, tableName)) == EOF || retStat < 2) )
    goto PREMATURE;

  if ( strcmp(this->LowerCase(key), "lookup_table") )
    goto BAD_DATA;

  //
  // See whether scalar has been already read or scalar name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetScalars() != NULL || 
  (this->ScalarsName && strcmp(name,this->ScalarsName)) )
    skipScalar = 1;
  else
    this->SetScalarLut(tableName); //may be "default"


  if ( ! strncmp(this->LowerCase(line), "bit", 3) )
    {
    vlBitScalars *scalars = new vlBitScalars(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),(numPts+1)/8,fp) != ((numPts+1)/8) ) goto PREMATURE;
      scalars->WrotePtr();
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
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else if ( ! strncmp(line, "char", 4) )
    {
    vlCharScalars *scalars = new vlCharScalars(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),numPts,fp) != numPts ) goto PREMATURE;
      scalars->WrotePtr();
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
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else if ( ! strncmp(line, "short", 5) )
    {
    vlShortScalars *scalars = new vlShortScalars(numPts);
    short *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(short),numPts,fp) != numPts ) goto PREMATURE;
      scalars->WrotePtr();
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
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else if ( ! strncmp(line, "int", 3) )
    {
    vlIntScalars *scalars = new vlIntScalars(numPts);
    int *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(int),numPts,fp) != numPts ) goto PREMATURE;
      scalars->WrotePtr();
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
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else if ( ! strncmp(line, "float", 5) )
    {
    vlFloatScalars *scalars = new vlFloatScalars(numPts);
    float *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),numPts,fp) != numPts ) goto PREMATURE;
      scalars->WrotePtr();
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
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else 
    goto UNSUPPORTED;

  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading scalar data");
    return 0;

  BAD_DATA:
    vlErrorMacro(<< "Bad data while reading scalar data");
    return 0;

  UNSUPPORTED:
    vlErrorMacro(<< "Unsupported scalar data type: " << line);
    return 0;
}

// Description:
// Read vector point attributes. Return 0 if error.
int vlDataReader::ReadVectorData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, skipVector=0;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %256s", name, line)) == EOF || retStat < 2) 
    goto PREMATURE;

  //
  // See whether vector has been already read or vector name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetVectors() != NULL || 
  (this->VectorsName && strcmp(name,this->VectorsName)) )
    {
    skipVector = 1;
    }

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vlFloatVectors *vectors = new vlFloatVectors(numPts);
    float *ptr = vectors->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),3*numPts,fp) != (3*numPts) ) goto PREMATURE;
      vectors->WrotePtr();
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
    if ( skipVector ) delete vectors;
    else ds->GetPointData()->SetVectors(vectors);
    }

  else 
    goto UNSUPPORTED;

  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading vector data");
    return 0;

  UNSUPPORTED:
    vlErrorMacro(<< "Unsupported vector data type: " << line);
    return 0;
}

// Description:
// Read normal point attributes. Return 0 if error.
int vlDataReader::ReadNormalData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, skipNormal;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %256s", name, line)) == EOF || retStat < 2) 
    goto PREMATURE;
  //
  // See whether normal has been already read or normal name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetNormals() != NULL || 
  (this->NormalsName && strcmp(name,this->NormalsName)) )
    {
    skipNormal = 1;
    }

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vlFloatNormals *normals = new vlFloatNormals(numPts);
    float *ptr = normals->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),3*numPts,fp) != (3*numPts) ) goto PREMATURE;
      normals->WrotePtr();
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
    if ( skipNormal ) delete normals;
    else ds->GetPointData()->SetNormals(normals);
    }

  else 
    goto UNSUPPORTED;

  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading normal data");
    return 0;

  UNSUPPORTED:
    vlErrorMacro(<< "Unsupported normal data type: " << line);
    return 0;
}

// Description:
// Read tensor point attributes. Return 0 if error.
int vlDataReader::ReadTensorData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, skipTensor;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %256s", name, line)) == EOF || retStat < 2) 
    goto PREMATURE;
  //
  // See whether tensor has been already read or tensor name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetTensors() != NULL || 
  (this->TensorsName && strcmp(name,this->TensorsName)) )
    {
    skipTensor = 1;
    }

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vlFloatTensors *tensors = new vlFloatTensors(numPts);
    tensors->SetDimension(3);
    float *ptr = tensors->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),9*numPts,fp) != (9*numPts) ) goto PREMATURE;
      tensors->WrotePtr();
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
    if ( skipTensor ) delete tensors;
    else ds->GetPointData()->SetTensors(tensors);
    }

  else 
    goto UNSUPPORTED;

  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading tensor data");
    return 0;

  UNSUPPORTED:
    vlErrorMacro(<< "Unsupported tensor data type: " << line);
    return 0;
}

// Description:
// Read color scalar point attributes. Return 0 if error.
int vlDataReader::ReadCoScalarData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, nValues, skipScalar;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %d", name, &nValues)) ==  EOF || retStat < 2) 
    goto PREMATURE;

  //
  // See whether scalar has been already read or scalar name (if specified) 
  // matches name in file. 
  //
  if ( ds->GetPointData()->GetScalars() != NULL || 
  (this->ScalarsName && strcmp(name,this->ScalarsName)) )
    {
    skipScalar = 1;
    }

  if ( nValues == 1 )
    {
    vlGraymap *scalars = new vlGraymap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),numPts,fp) != numPts ) goto PREMATURE;
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f;
      unsigned char rgba[4];
      rgba[1] = rgba[2] = rgba[3] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f",&f)) == EOF || retStat < 1) goto PREMATURE;
        rgba[0] = (unsigned char)((float)f*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else if ( nValues == 2 )
    {
    vlAGraymap *scalars = new vlAGraymap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),2*numPts,fp) != (2*numPts) ) goto PREMATURE;
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f[2];
      unsigned char rgba[4];
      rgba[1] = rgba[2] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f",f,f+1)) == EOF || retStat < 2) goto PREMATURE;
        rgba[0] = (unsigned char)((float)f[0]*255.0);
        rgba[3] = (unsigned char)((float)f[1]*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else if ( nValues == 3 )
    {
    vlPixmap *scalars = new vlPixmap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),3*numPts,fp) != (3*numPts) ) goto PREMATURE;
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f[3];
      unsigned char rgba[4];
      rgba[3] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",f,f+1,f+2)) == EOF || retStat < 3) goto PREMATURE;
        rgba[0] = (unsigned char)((float)f[0]*255.0);
        rgba[1] = (unsigned char)((float)f[1]*255.0);
        rgba[2] = (unsigned char)((float)f[2]*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else if ( nValues == 4 )
    {
    vlAPixmap *scalars = new vlAPixmap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(unsigned char),4*numPts,fp) != (4*numPts) ) goto PREMATURE;
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f[4];
      unsigned char rgba[4];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f %f",f,f+1,f+2,f+3)) == EOF || retStat < 4) goto PREMATURE;
        rgba[0] = (unsigned char)((float)f[0]*255.0);
        rgba[1] = (unsigned char)((float)f[1]*255.0);
        rgba[2] = (unsigned char)((float)f[2]*255.0);
        rgba[3] = (unsigned char)((float)f[3]*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( skipScalar ) delete scalars;
    else ds->GetPointData()->SetScalars(scalars);
    }

  else
    goto UNSUPPORTED;

  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading color scalars data");
    return 0;

  UNSUPPORTED:
    vlErrorMacro(<< "Do not support " << nValues << " per scalar");
    return 0;
}

// Description:
// Read texture coordinates point attributes. Return 0 if error.
int vlDataReader::ReadTCoordsData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, dim, skipTCoord;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %d %256s", name, &dim, line)) == EOF || retStat < 3) 
    goto PREMATURE;

  if ( dim < 1 || dim > 3 ) goto UNSUPPORTED_DIMENSION;

  //
  // See whether texture coords have been already read or texture coords name
  // (if specified) matches name in file. 
  //
  if ( ds->GetPointData()->GetTCoords() != NULL || 
  (this->TCoordsName && strcmp(name,this->TCoordsName)) )
    {
    skipTCoord = 1;
    }

  if ( ! strncmp(this->LowerCase(line), "float", 5) )
    {
    vlFloatTCoords *tcoords = new vlFloatTCoords(numPts);
    tcoords->SetDimension(dim);
    float *ptr = tcoords->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
      if ( fread(ptr,sizeof(float),dim*numPts,fp) != (dim*numPts) ) goto PREMATURE;
      tcoords->WrotePtr();
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
    if ( skipTCoord ) delete tcoords;
    else ds->GetPointData()->SetTCoords(tcoords);
    }

  else 
    goto UNSUPPORTED;

  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading texture coordinates data");
    return 0;

  UNSUPPORTED:
    vlErrorMacro(<< "Unsupported texture coordinates data type: " << line);
    return 0;

  UNSUPPORTED_DIMENSION:
    vlErrorMacro(<< "Unsupported texture coordinates dimension: " << dim);
    return 0;

}

// Description:
// Read lookup table. Return 0 if error.
int vlDataReader::ReadLutData(FILE *fp, vlDataSet *ds, int numPts)
{
  int retStat, i, size, skipTable;
  vlLookupTable *lut;
  unsigned char *ptr;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %d", name, &size)) ==  EOF || retStat < 2) 
    goto PREMATURE;

  if ( ds->GetPointData()->GetScalars() == NULL ||
  (this->LookupTableName && strcmp(name,this->LookupTableName)) ||
  (this->ScalarLut && strcmp(name,this->ScalarLut)) )
    {
    skipTable = 1;
    }

  lut = new vlLookupTable(size);
  ptr = lut->WritePtr(0,size);

  if ( this->FileType == BINARY)
    {
    if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
    if ( fread(ptr,sizeof(unsigned char),4*size,fp) != (4*size) ) goto PREMATURE;
    lut->WrotePtr();
    }
  else // ascii
    {
    float rgba[4];
    for (i=0; i<size; i++)
      {
      if ((retStat=fscanf(fp,"%f %f %f %f",rgba,rgba+1,rgba+2,rgba+3)) == EOF || retStat < 4) goto PREMATURE;
      lut->SetTableValue(i, rgba[0], rgba[1], rgba[2], rgba[3]);
      }
    }

  if ( skipTable ) delete lut;
  else ds->GetPointData()->GetScalars()->SetLookupTable(lut);

  return 1;
//
// It's great knowing there are goto's in this code!
//
  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading lookup table");
    return 0;
}


// Description:
// Read lookup table. Return 0 if error.
int vlDataReader::ReadCells(FILE *fp, int size, int *data)
{
  char line[257];
  int i, retStat;

  if ( this->FileType == BINARY)
    {
    if ( fgets(line,256,fp) == NULL ) goto PREMATURE; //suck up newline
    if ( fread(data,sizeof(int),size,fp) != size ) goto PREMATURE;
    }
  else // ascii
    {
    for (i=0; i<size; i++)
      {
      if ((retStat=fscanf(fp,"%d",data+i)) == EOF || retStat < 1) 
        goto PREMATURE;
      }
    }

  return 1;

  PREMATURE:
    vlErrorMacro(<< "Premature EOF while reading cell data");
    return 0;
}



char *vlDataReader::LowerCase(char *str)
{
  for ( char *s=str; *s != '\0'; s++) *s = tolower(*s);
  return str;
}

void vlDataReader::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

  if ( this->FileType == BINARY )
    os << indent << "File Type: BINARY\n";
  else
    os << indent << "File Type: ASCII\n";

  if ( this->ScalarsName )
    os << indent << "Scalars Name: " << this->ScalarsName << "\n";
  else
    os << indent << "Scalars Name: (None)\n";

  if ( this->VectorsName )
    os << indent << "Vectors Name: " << this->VectorsName << "\n";
  else
    os << indent << "Vectors Name: (None)\n";

  if ( this->NormalsName )
    os << indent << "Normals Name: " << this->NormalsName << "\n";
  else
    os << indent << "Normals Name: (None)\n";

  if ( this->TensorsName )
    os << indent << "Tensors Name: " << this->TensorsName << "\n";
  else
    os << indent << "Tensors Name: (None)\n";

  if ( this->TCoordsName )
    os << indent << "Texture Coords Name: " << this->TCoordsName << "\n";
  else
    os << indent << "Texture Coordinates Name: (None)\n";

  if ( this->LookupTableName )
    os << indent << "Lookup Table Name: " << this->LookupTableName << "\n";
  else
    os << indent << "Lookup Table Name: (None)\n";

}
