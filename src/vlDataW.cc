/*=========================================================================

  Program:   Visualization Library
  Module:    vlDataW.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "vlDataW.hh"
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
#include "CellArr.hh"

// Description:
// Created object with default header, ASCII format, and default names for 
// scalars, vectors, tensors, normals, and texture coordinates.
vlDataWriter::vlDataWriter()
{
  this->Filename = NULL;

  this->Header = new char[257];
  strcpy(this->Header,"vl output");
  this->FileType = ASCII;

  this->ScalarsName = new char[8];
  strcpy(this->ScalarsName,"scalars");

  this->VectorsName = new char[8];
  strcpy(this->VectorsName,"vectors");

  this->TensorsName = new char[8];
  strcpy(this->TensorsName,"tensors");

  this->NormalsName = new char[8];
  strcpy(this->NormalsName,"normals");

  this->TCoordsName = new char[14];
  strcpy(this->TCoordsName,"textureCoords");

  this->LookupTableName = new char[13];
  strcpy(this->LookupTableName,"lookup_table");
}

vlDataWriter::~vlDataWriter()
{
  if ( this->Filename ) delete [] this->Filename;
  if ( this->Header ) delete [] this->Header;
  if ( this->ScalarsName ) delete [] this->ScalarsName;
  if ( this->VectorsName ) delete [] this->VectorsName;
  if ( this->TensorsName ) delete [] this->TensorsName;
  if ( this->NormalsName ) delete [] this->NormalsName;
  if ( this->TCoordsName ) delete [] this->TCoordsName;
  if ( this->LookupTableName ) delete [] this->LookupTableName;
}

// Description:
// Open a vl data file. Returns NULL if error.
FILE *vlDataWriter::OpenVLFile()
{
  FILE *fptr;

  vlDebugMacro(<<"Opening vl file for writing...");

  if ( !this->Filename )
    {
    vlErrorMacro(<< "No filename specified! Can't write!");
    return NULL;
    }

  if ( (fptr=fopen(this->Filename, "wb")) == NULL )
    {
    vlErrorMacro(<< "Unable to open file: "<< this->Filename);
    return NULL;
    }

  return fptr;
}

// Description:
// Write the header of a vl data file. Returns 0 if error.
int vlDataWriter::WriteHeader(FILE *fp)
{
  vlDebugMacro(<<"Writing header...");

  fprintf (fp, "# vl DataSet Version 1.0\n");
  fprintf (fp, "%s\n", this->Header);

  if ( this->FileType == ASCII )
    fprintf (fp, "ASCII\n");
  else
    fprintf (fp, "BINARY\n");

  return 1;
}

// Description:
// Write the point data (e.g., scalars, vectors, ...) of a vl data file. 
// Returns 0 if error.
int vlDataWriter::WritePointData(FILE *fp, vlDataSet *ds)
{
  int numPts;
  vlScalars *scalars;
  vlVectors *vectors;
  vlNormals *normals;
  vlTCoords *tcoords;
  vlTensors *tensors;
  vlPointData *pd=ds->GetPointData();

  vlDebugMacro(<<"Writing point data...");

  numPts = ds->GetNumberOfPoints();
  scalars = pd->GetScalars();
  vectors = pd->GetVectors();
  normals = pd->GetNormals();
  tcoords = pd->GetTCoords();
  tensors = pd->GetTensors();

  if ( numPts <= 0 || !(scalars || vectors || normals || tcoords || tensors) )
    {
    vlWarningMacro(<<"No point data to write!");
    return 1;
    }

  fprintf (fp, "POINT_DATA %d\n", numPts);
//
// Write scalar data
//
  if ( scalars && scalars->GetNumberOfScalars() > 0 )
    {
    if ( ! this->WriteScalarData(fp, scalars, numPts) ) return 0;
    }
//
// Write vector data
//
  if ( vectors && vectors->GetNumberOfVectors() > 0 )
    {
    if ( ! this->WriteVectorData(fp, vectors, numPts) ) return 0;
    }
//
// Write normals
//
  if ( normals && normals->GetNumberOfNormals() > 0 )
    {
    if ( ! this->WriteNormalData(fp, normals, numPts) ) return 0;
    }
//
// Write texture coords
//
  if ( tcoords && tcoords->GetNumberOfTCoords() > 0 )
    {
    if ( ! this->WriteTCoordData(fp, tcoords, numPts) ) return 0;
    }
//
// Write tensors
//
  if ( tensors && tensors->GetNumberOfTensors() > 0 )
    {
    if ( ! this->WriteTensorData(fp, tensors, numPts) ) return 0;
    }

  return 1;
}

int vlDataWriter::WritePoints(FILE *fp, vlPoints *points)
{
  int i, numPts=points->GetNumberOfPoints();
  char *type;
  
  fprintf (fp, "POINTS %d ", numPts);
  type = points->GetDataType();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "float\n");
    if ( this->FileType == ASCII )
      {
      float *p;
      for (i=0; i<numPts; i++)
        {
        p = points->GetPoint(i);
        fprintf (fp, "%f %f %f ", p[0], p[1], p[2]);
        if ( (i%2) ) fprintf (fp,"\n");
        }
      }
    else
      {
      vlFloatPoints *fpoints = (vlFloatPoints *)points;
      float *fptr=fpoints->GetPtr(0);
      fwrite (fptr,sizeof(float),3*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else if ( !strcmp(type,"int") )
    {
    vlIntPoints *ipoints = (vlIntPoints *)points;
    fprintf (fp, "int\n");
    if ( this->FileType == ASCII )
      {
      int *p;
      for (i=0; i<numPts; i++)
        {
        p = ipoints->GetPtr(3*i);
        fprintf (fp, "%d %d %d", p[0], p[1], p[2]);
        if ( (i%2) ) fprintf (fp,"\n");
        }
      }
    else
      {
      vlIntPoints *ipoints = (vlIntPoints *)points;
      int *iptr=ipoints->GetPtr(0);
      fwrite (iptr,sizeof(int),3*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vlErrorMacro(<<"Point type: " << type << " currently not supported");
    return 0;
    }
}

int vlDataWriter::WriteScalarData(FILE *fp, vlScalars *scalars, int numPts)
{
  int i, size;
  char *type, *name;
  vlLookupTable *lut;

  if ( (lut=scalars->GetLookupTable()) == NULL || (size = lut->GetNumberOfColors()) > 0 )
    name = "default";
  else 
    name = this->LookupTableName;

  if ( ! strcmp(scalars->GetScalarType(),"SingleValued") )
    {
    fprintf (fp, "SCALARS ");
    type = scalars->GetDataType();

    if ( !strcmp(type,"bit") )
      {
      fprintf (fp, "%s bit\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == ASCII )
        {
        float s;
        for (i=0; i<numPts; i++)
          {
          s = scalars->GetScalar(i);
          fprintf (fp, "%d ", (s!=0.0?1:0));
          if ( !((i+1)%6) ) fprintf (fp,"\n");
          }
        }
      else
        {
        vlBitScalars *bscalars = (vlBitScalars *)scalars;
        unsigned char *cptr=bscalars->GetPtr(0);
        fwrite (cptr,sizeof(char),(numPts-1)/8+1,fp);
        }
      fprintf (fp,"\n");
      }

    else if ( !strcmp(type,"char") )
      {
      fprintf (fp, "%s char\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == ASCII )
        {
        unsigned char s;
        for (i=0; i<numPts; i++)
          {
          s = (unsigned char) scalars->GetScalar(i);
          fprintf (fp, "%c ", s);
          if ( !((i+1)%6) ) fprintf (fp,"\n");
          }
        }
      else
        {
        vlCharScalars *cscalars = (vlCharScalars *)scalars;
        unsigned char *cptr=cscalars->GetPtr(0);
        fwrite (cptr,sizeof(unsigned char),numPts,fp);
        }
      fprintf (fp,"\n");
      }

    else if ( !strcmp(type,"short") )
      {
      fprintf (fp, "%s short\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == ASCII )
        {
        short s;
        for (i=0; i<numPts; i++)
          {
          s = (short) scalars->GetScalar(i);
          fprintf (fp, "%d ", s);
          if ( !((i+1)%6) ) fprintf (fp,"\n");
          }
        }
      else
        {
        vlShortScalars *sscalars = (vlShortScalars *)scalars;
        short *sptr=sscalars->GetPtr(0);
        fwrite (sptr,sizeof(short),numPts,fp);
        }
      fprintf (fp,"\n");
      }

    else if ( !strcmp(type,"int") )
      {
      fprintf (fp, "%s int\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == ASCII )
        {
        float s;
        for (i=0; i<numPts; i++)
          {
          s = scalars->GetScalar(i);
          fprintf (fp, "%d ", s);
          if ( !((i+1)%6) ) fprintf (fp,"\n");
          }
        }
      else
        {
        vlIntScalars *iscalars = (vlIntScalars *)scalars;
        int *iptr=iscalars->GetPtr(0);
        fwrite (iptr,sizeof(int),numPts,fp);
        }
      fprintf (fp,"\n");
      }

    else if ( !strcmp(type,"float") )
      {
      fprintf (fp, "%s float\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == ASCII )
        {
        float s;
        for (i=0; i<numPts; i++)
          {
          s = scalars->GetScalar(i);
          fprintf (fp, "%f ", s);
          if ( !((i+1)%6) ) fprintf (fp,"\n");
          }
        }
      else
        {
        vlFloatScalars *fscalars = (vlFloatScalars *)scalars;
        float *fptr=fscalars->GetPtr(0);
        fwrite (fptr,sizeof(float),numPts,fp);
        }
      fprintf (fp,"\n");
      }

    else
      {
      vlErrorMacro(<<"Scalar type: " << type << " currently not supported");
      return 0;
      }
    }

  else //color scalars
    {
    int nvs = scalars->GetNumberOfValuesPerScalar();
    vlColorScalars *coscalars = (vlColorScalars *)scalars;

    fprintf (fp, "COLOR_SCALARS %s %d\n", this->ScalarsName, nvs);

    if ( this->FileType == ASCII )
      {
      if ( nvs == 1) //graymap
        {
        unsigned char *c;
        for (i=0; i<numPts; i++)
          {
          c = coscalars->GetColor(i);
          fprintf (fp, "%f ", (float)c[0]/255.0);
          if ( i != 0 && !(i%6) ) fprintf (fp,"\n");
          }
        }

      else if ( nvs == 2) //agraymap
        {
        unsigned char *c;
        for (i=0; i<numPts; i++)
          {
          c = coscalars->GetColor(i);
          fprintf (fp, "%f %f  ", (float)c[0]/255.0, (float)c[3]/255.0);
          if ( i != 0 && !(i%3) ) fprintf (fp,"\n");
          }
        }

      else if ( nvs == 3) //pixmap
        {
        unsigned char *c;
        for (i=0; i<numPts; i++)
          {
          c = coscalars->GetColor(i);
          fprintf (fp, "%f %f %f  ", (float)c[0]/255.0, (float)c[1]/255.0, (float)c[2]/255.0);
          if ( i != 0 && !(i%2) ) fprintf (fp,"\n");
          }
        }

      else if ( nvs == 4) //apixmap
        {
        unsigned char *c;
        for (i=0; i<numPts; i++)
          {
          c = coscalars->GetColor(i);
          fprintf (fp, "%f %f %f %f\n", (float)c[0]/255.0, (float)c[1]/255.0, 
                  (float)c[2]/255.0, (float)c[3]/255.0);
          }
        }
      }
    else // binary type
      {
      unsigned char *cptr=coscalars->GetPtr(0);
      fwrite (cptr,sizeof(unsigned char),nvs*numPts,fp);
      }

    fprintf (fp,"\n");
    }

  //if lookup table, write it out
  if ( lut && size > 0 )
    {
    fprintf (fp, "LOOKUP_TABLE %s %d\n", this->LookupTableName, size);
    if ( this->FileType == ASCII )
      {
      float *c;
      for (i=0; i<size; i++)
        {
        c = lut->GetTableValue(i);
        fprintf (fp, "%f %f %f %f\n", c[0], c[1], c[2], c[3]);
        }
      }
    else
      {
      unsigned char *colors=lut->GetPtr(0);
      fwrite(colors,sizeof(unsigned char),4*size,fp);
      }
    fprintf (fp, "\n");
    }
}

int vlDataWriter::WriteVectorData(FILE *fp, vlVectors *vectors, int numPts)
{
  int i;
  char *type;

  fprintf (fp, "VECTORS ");
  type = vectors->GetDataType();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "%s float\n", this->VectorsName);
    if ( this->FileType == ASCII )
      {
      float *v;
      for (i=0; i<numPts; i++)
        {
        v = vectors->GetVector(i);
        fprintf (fp, "%f %f %f ", v[0], v[1], v[2]);
        if ( (i%2) ) fprintf (fp,"\n");
        }
      }
    else
      {
      vlFloatVectors *fvectors = (vlFloatVectors *)vectors;
      float *fptr=fvectors->GetPtr(0);
      fwrite (fptr,sizeof(float),3*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vlErrorMacro(<<"Vector type: " << type << " currently not supported");
    return 0;
    }
}

int vlDataWriter::WriteNormalData(FILE *fp, vlNormals *normals, int numPts)
{
  int i;
  char *type;

  fprintf (fp, "NORMALS ");
  type = normals->GetDataType();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "%s float\n", this->NormalsName);
    if ( this->FileType == ASCII )
      {
      float *n;
      for (i=0; i<numPts; i++)
        {
        n = normals->GetNormal(i);
        fprintf (fp, "%f %f %f ", n[0], n[1], n[2]);
        if ( (i%2) ) fprintf (fp,"\n");
        }
      }
    else
      {
      vlFloatNormals *fnormals = (vlFloatNormals *)normals;
      float *fptr=fnormals->GetPtr(0);
      fwrite (fptr,sizeof(float),3*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vlErrorMacro(<<"Normal type: " << type << " currently not supported");
    return 0;
    }
}

int vlDataWriter::WriteTCoordData(FILE *fp, vlTCoords *tcoords, int numPts)
{
  int i, j, dim;
  char *type;

  fprintf (fp, "TCOORDS ");
  type = tcoords->GetDataType();
  dim = tcoords->GetDimension();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "%s float\n", this->TCoordsName);
    if ( this->FileType == ASCII )
      {
      float *tc;
      for (i=0; i<numPts; i++)
        {
        tc = tcoords->GetTCoord(i);
        for (j=0; j<dim; j++) fprintf (fp, "%f ", tc[j]);
        if ( !((i+1)%3) ) fprintf (fp,"\n");
        }
      }
    else
      {
      vlFloatTCoords *ftcoords = (vlFloatTCoords *)tcoords;
      float *fptr=ftcoords->GetPtr(0);
      fwrite (fptr,sizeof(float),dim*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vlErrorMacro(<<"Texture coord type: " << type << " currently not supported");
    return 0;
    }
}

int vlDataWriter::WriteTensorData(FILE *fp, vlTensors *tensors, int numPts)
{
  int i, j, k, dim;
  char *type;

  fprintf (fp, "TENSORS ");
  type = tensors->GetDataType();
  dim = tensors->GetDimension();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "%s float\n", this->TensorsName);
    if ( this->FileType == ASCII )
      {
      vlTensor t;
      for (i=0; i<numPts; i++)
        {
        tensors->GetTensor(i,t);
        for (j=0; j<dim; j++)
          {
          for (k=0; k<dim; k++) fprintf (fp, "%f ", t.GetComponent(j,k));
          fprintf (fp, "\n");
          }
        }
      }
    else
      {
      vlFloatTensors *ftensors = (vlFloatTensors *)tensors;
      float *fptr=ftensors->GetPtr(0);
      fwrite (fptr,sizeof(float),dim*dim*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vlErrorMacro(<<"Tensor type: " << type << " currently not supported");
    return 0;
    }

  return 1;
}

int vlDataWriter::WriteCells(FILE *fp, vlCellArray *cells, char *label)
{
  int ncells=cells->GetNumberOfCells();
  int size=cells->GetSize();

  fprintf (fp, "%s %d %d\n", label, ncells, size);

  if ( this->FileType == ASCII )
    {
    int npts, *pts, j;
    for (cells->InitTraversal(); cells->GetNextCell(npts,pts); )
      {
      fprintf (fp, "%d ", npts);
      for (j=0; j<npts; j++)
        {
        fprintf (fp, "%d ", pts[j]);
        }
      fprintf (fp, "\n");
      }
    }
  else
    {
    fwrite (cells->GetPtr(),sizeof(int),size,fp);
    }

  fprintf (fp,"\n");
}

void vlDataWriter::PrintSelf(ostream& os, vlIndent indent)
{
  vlWriter::PrintSelf(os,indent);

  if ( this->Filename )
    os << indent << "Filename: " << this->Filename << "\n";
  else
    os << indent << "Filename: (None)\n";

  if ( this->FileType == BINARY )
    os << indent << "File Type: BINARY\n";
  else
    os << indent << "File Type: ASCII\n";

  if ( this->Header )
    os << indent << "Header: " << this->Header << "\n";
  else
    os << indent << "Header: (None)\n";

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
