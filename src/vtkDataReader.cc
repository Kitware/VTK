/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataReader.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkDataReader.hh"
#include <ctype.h>

#include "vtkBitScalars.hh"
#include "vtkUnsignedCharScalars.hh"
#include "vtkFloatScalars.hh"
#include "vtkShortScalars.hh"
#include "vtkIntScalars.hh"
#include "vtkFloatPoints.hh"
#include "vtkIntPoints.hh"
#include "vtkFloatNormals.hh"
#include "vtkFloatTensors.hh"
#include "vtkFloatTCoords.hh"
#include "vtkGraymap.hh"
#include "vtkAGraymap.hh"
#include "vtkPixmap.hh"
#include "vtkAPixmap.hh"
#include "vtkLookupTable.hh"

// Description:
// Construct object.
vtkDataReader::vtkDataReader()
{
  this->Filename = NULL;
  this->ScalarsName = NULL;
  this->VectorsName = NULL;
  this->TensorsName = NULL;
  this->NormalsName = NULL;
  this->TCoordsName = NULL;
  this->LookupTableName = NULL;
  this->ScalarLut = NULL;
}  

vtkDataReader::~vtkDataReader()
{
  if (this->Filename) delete [] this->Filename;
  if (this->ScalarsName) delete [] this->ScalarsName;
  if (this->VectorsName) delete [] this->VectorsName;
  if (this->TensorsName) delete [] this->TensorsName;
  if (this->NormalsName) delete [] this->NormalsName;
  if (this->TCoordsName) delete [] this->TCoordsName;
  if (this->LookupTableName) delete [] this->LookupTableName;
  if (this->ScalarLut) delete [] this->ScalarLut;
}

// Description:
// Open a vtk data file. Returns NULL if error.
FILE *vtkDataReader::OpenVTKFile()
{
  FILE *fptr;

  vtkDebugMacro(<< "Opening vtk file");

  if ( !this->Filename )
    {
    vtkErrorMacro(<< "No file specified!");
    return NULL;
    }

  if ( (fptr=fopen(this->Filename, "rb")) == NULL )
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->Filename);
    return NULL;
    }

  return fptr;
}

// Description:
// Read the header of a vtk data file. Returns 0 if error.
int vtkDataReader::ReadHeader(FILE *fp)
{
  char line[257];
  int retStat;

  vtkDebugMacro(<< "Reading vtk file header");
//
// read header
//
  if ( fgets (line, 256, fp) == NULL )
    {
    vtkErrorMacro(<<"Premature EOF reading first line!");
    return 0;
    }
  if ( strncmp ("# vtk DataFile Version", line, 20) )
    {
    vtkErrorMacro(<< "Unrecognized file type: "<< line);
    return 0;
    }
//
// read title
//
  if ( fgets (line, 256, fp) == NULL )
    {
    vtkErrorMacro(<<"Premature EOF reading title!");
    return 0;
    }
  line[256] = '\0';
  vtkDebugMacro(<< "Reading vtk file entitled: " << line);
//
// read type
//
  if ( (retStat=fscanf(fp,"%256s",line)) == EOF || retStat < 1 ) 
    {
    vtkErrorMacro(<<"Premature EOF reading file type!");
    return 0;
    }

  if ( !strncmp(this->LowerCase(line),"ascii",5) ) this->FileType = ASCII;
  else if ( !strncmp(line,"binary",6) ) this->FileType = BINARY;
  else
    {
    vtkErrorMacro(<< "Unrecognized file type: "<< line);
    this->FileType = NULL;
    return 0;
    }

  return 1;
}

// Description:
// Read the point data of a vtk data file. The number of points (from the 
// dataset) must match the number of points defined in point attributes (unless
// no geometry was defined).
int vtkDataReader::ReadPointData(FILE *fp, vtkDataSet *ds, int numPts)
{
  int retStat;
  char line[257];

  vtkDebugMacro(<< "Reading vtk point data");
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
      if ( ! this->ReadLutData(fp, ds) ) return 0;
      }

    else
      {
      vtkErrorMacro(<< "Unsupported point attribute type: " << line);
      return 0;
      }
    }

  return 1;
}

// Description:
// Read point coordinates. Return 0 if error.
int vtkDataReader::ReadPoints(FILE *fp, vtkPointSet *ps, int numPts)
{
  int retStat, i;
  char line[257];

  if ((retStat=fscanf(fp, "%256s", line)) ==  EOF || retStat < 1) 
    {
    vtkErrorMacro(<<"Cannot read points type!");
    return 0;
    }

  if ( ! strncmp(this->LowerCase(line), "int", 3) )
    {
    vtkIntPoints *points = new vtkIntPoints(numPts);
    int *ptr = points->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(int),3*numPts,fp) != (3*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary points!");
        return 0;
        }
      points->WrotePtr();
      }
    else // ascii
      {
      int p[3];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%d %d %d",p,p+1,p+2)) == EOF || retStat < 3)
          {
          vtkErrorMacro(<<"Error reading points!");
          return 0;
          }
        points->SetPoint(i,p);
        }
      }
    ps->SetPoints(points);
    points->Delete();
    }

  else if ( ! strncmp(line, "float", 5) )
    {
    vtkFloatPoints *points = new vtkFloatPoints(numPts);
    float *ptr = points->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(float),3*numPts,fp) != (3*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary points!");
        return 0;
        }
      points->WrotePtr();
      }
    else // ascii
      {
      float p[3];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",p,p+1,p+2)) == EOF || retStat < 3)
          {
          vtkErrorMacro(<<"Error reading points!");
          return 0;
          }
        points->SetPoint(i,p);
        }
      }
    ps->SetPoints(points);
    points->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported points type: " << line);
    return 0;
    }

  vtkDebugMacro(<<"Read " << ps->GetNumberOfPoints() << " points");
  return 1;
}

// Description:
// Read scalar point attributes. Return 0 if error.
int vtkDataReader::ReadScalarData(FILE *fp, vtkDataSet *ds, int numPts)
{
  char line[257], name[257], key[128], tableName[257];
  int retStat, i, skipScalar=0;

  if ( (retStat=fscanf(fp, "%256s %256s", name, line)) == EOF || retStat < 2 ||
  ((retStat=fscanf(fp, "%256s %256s", key, tableName)) == EOF || retStat < 2) )
    {
    vtkErrorMacro(<<"Cannot read scalar header!");
    return 0;
    }

  if ( strcmp(this->LowerCase(key), "lookup_table") )
    {
    vtkErrorMacro(<<"Lookup table must be specified with scalar.\n" <<
                   "Use \"LOOKUP_TABLE default\" to use default table.");
    return 0;
    }
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
    vtkBitScalars *scalars = new vtkBitScalars(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(unsigned char),(numPts+1)/8,fp) != ((numPts+1)/8)) )
        {
        vtkErrorMacro(<<"Error reading binary bit scalars!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      int iv;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%d",&iv)) == EOF || retStat < 1)
          {
          vtkErrorMacro(<<"Error reading bit scalars!");
          return 0;
          }
        scalars->SetScalar(i,iv);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( ! strncmp(line, "unsigned_char", 13) )
    {
    vtkUnsignedCharScalars *scalars = new vtkUnsignedCharScalars(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(unsigned char),numPts,fp) != numPts) )
        {
        vtkErrorMacro(<<"Error reading binary char scalars!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      unsigned char c;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%c",&c)) == EOF || retStat < 1)
          {
          vtkErrorMacro(<<"Error reading char scalars!");
          return 0;
          }
        scalars->SetScalar(i,c);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( ! strncmp(line, "short", 5) )
    {
    vtkShortScalars *scalars = new vtkShortScalars(numPts);
    short *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(short),numPts,fp) != numPts) )
        {
        vtkErrorMacro(<<"Error reading binary short scalars!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      short s;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%hd",&s)) == EOF || retStat < 1)
          {
          vtkErrorMacro(<<"Error reading short scalars!");
          return 0;
          }
        scalars->SetScalar(i,s);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( ! strncmp(line, "int", 3) )
    {
    vtkIntScalars *scalars = new vtkIntScalars(numPts);
    int *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(int),numPts,fp) != numPts) )
        {
        vtkErrorMacro(<<"Error reading binary int scalars!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      int iv;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%d",&iv)) == EOF || retStat < 1)
          {
          vtkErrorMacro(<<"Error reading int scalars!");
          return 0;
          }
        scalars->SetScalar(i,iv);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( ! strncmp(line, "float", 5) )
    {
    vtkFloatScalars *scalars = new vtkFloatScalars(numPts);
    float *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(int),numPts,fp) != numPts) )
        {
        vtkErrorMacro(<<"Error reading binary int scalars!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f",&f)) == EOF || retStat < 1)
          {
          vtkErrorMacro(<<"Error reading float scalars!");
          return 0;
          }
        scalars->SetScalar(i,f);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported scalar data type: " << line);
    return 0;
    }

  return 1;
}

// Description:
// Read vector point attributes. Return 0 if error.
int vtkDataReader::ReadVectorData(FILE *fp, vtkDataSet *ds, int numPts)
{
  int retStat, i, skipVector=0;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %256s", name, line)) == EOF || retStat < 2) 
    {
    vtkErrorMacro(<<"Cannot read vector data!");
    return 0;
    }

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
    vtkFloatVectors *vectors = new vtkFloatVectors(numPts);
    float *ptr = vectors->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(float),3*numPts,fp) != (3*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary vectors!");
        return 0;
        }
      vectors->WrotePtr();
      }
    else // ascii
      {
      float v[3];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",v,v+1,v+2)) == EOF || retStat < 3)
          {
          vtkErrorMacro(<<"Error reading vectors!");
          return 0;
          }
        vectors->SetVector(i,v);
        }
      }
    if ( ! skipVector ) ds->GetPointData()->SetVectors(vectors);
    vectors->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported vector type: " << line);
    return 0;
    }

  return 1;
}

// Description:
// Read normal point attributes. Return 0 if error.
int vtkDataReader::ReadNormalData(FILE *fp, vtkDataSet *ds, int numPts)
{
  int retStat, i, skipNormal;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %256s", name, line)) == EOF || retStat < 2) 
    {
    vtkErrorMacro(<<"Cannot read normal data!");
    return 0;
    }
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
    vtkFloatNormals *normals = new vtkFloatNormals(numPts);
    float *ptr = normals->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(float),3*numPts,fp) != (3*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary normals!");
        return 0;
        }
      normals->WrotePtr();
      }
    else // ascii
      {
      float n[3];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",n,n+1,n+2)) == EOF || retStat < 3)
          {
          vtkErrorMacro(<<"Error reading normals!");
          return 0;
          }
        normals->SetNormal(i,n);
        }
      }
    if ( ! skipNormal ) ds->GetPointData()->SetNormals(normals);
    normals->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported normals type: " << line);
    return 0;
    }

  return 1;
}

// Description:
// Read tensor point attributes. Return 0 if error.
int vtkDataReader::ReadTensorData(FILE *fp, vtkDataSet *ds, int numPts)
{
  int retStat, i, skipTensor;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %256s", name, line)) == EOF || retStat < 2) 
    {
    vtkErrorMacro(<<"Cannot read tensor data!");
    return 0;
    }
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
    vtkFloatTensors *tensors = new vtkFloatTensors(numPts);
    tensors->SetDimension(3);
    float *ptr = tensors->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(float),9*numPts,fp) != (9*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary tensors!");
        return 0;
        }
      tensors->WrotePtr();
      }
    else // ascii
      {
      vtkTensor tensor;
      float t[9];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f %f %f %f %f %f %f",
			    t,t+1,t+2,t+3,t+4,t+5,t+6,t+7,t+8)) 
	    == EOF || retStat < 9)
          {
          vtkErrorMacro(<<"Error reading tensors!");
          return 0;
          }
        tensor = t;
        tensors->SetTensor(i,&tensor);
        }
      }
    if ( ! skipTensor ) ds->GetPointData()->SetTensors(tensors);
    tensors->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported tensors type: " << line);
    return 0;
    }

  return 1;
}

// Description:
// Read color scalar point attributes. Return 0 if error.
int vtkDataReader::ReadCoScalarData(FILE *fp, vtkDataSet *ds, int numPts)
{
  int retStat, i, nValues, skipScalar=0;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %d", name, &nValues)) ==  EOF || retStat < 2)
    {
    vtkErrorMacro(<<"Cannot read color scalar data!");
    return 0;
    }
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
    vtkGraymap *scalars = new vtkGraymap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(unsigned char),numPts,fp) != numPts) )
        {
        vtkErrorMacro(<<"Error reading binary graymap!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f;
      unsigned char rgba[4];
      rgba[1] = rgba[2] = rgba[3] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f",&f)) == EOF || retStat < 1)
          {
          vtkErrorMacro(<<"Error reading graymap!");
          return 0;
          }
        rgba[0] = (unsigned char)((float)f*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( nValues == 2 )
    {
    vtkAGraymap *scalars = new vtkAGraymap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(unsigned char),2*numPts,fp) != (2*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary alpha-graymap!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f[2];
      unsigned char rgba[4];
      rgba[1] = rgba[2] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f",f,f+1)) == EOF || retStat < 2)
          {
          vtkErrorMacro(<<"Error reading alpha-graymap!");
          return 0;
          }
        rgba[0] = (unsigned char)((float)f[0]*255.0);
        rgba[3] = (unsigned char)((float)f[1]*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( nValues == 3 )
    {
    vtkPixmap *scalars = new vtkPixmap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(unsigned char),3*numPts,fp) != (3*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary pixmap!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f[3];
      unsigned char rgba[4];
      rgba[3] = 0;
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f",f,f+1,f+2)) == EOF || retStat < 3) 
          {
          vtkErrorMacro(<<"Error reading pixmap!");
          return 0;
          }
        rgba[0] = (unsigned char)((float)f[0]*255.0);
        rgba[1] = (unsigned char)((float)f[1]*255.0);
        rgba[2] = (unsigned char)((float)f[2]*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else if ( nValues == 4 )
    {
    vtkAPixmap *scalars = new vtkAPixmap(numPts);
    unsigned char *ptr = scalars->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(unsigned char),4*numPts,fp) != (4*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary alpha-pixmap!");
        return 0;
        }
      scalars->WrotePtr();
      }
    else // ascii
      {
      float f[4];
      unsigned char rgba[4];
      for (i=0; i<numPts; i++)
        {
        if ((retStat=fscanf(fp,"%f %f %f %f",f,f+1,f+2,f+3)) == EOF || retStat < 4)
          {
          vtkErrorMacro(<<"Error reading alpha-pixmap!");
          return 0;
          }
        rgba[0] = (unsigned char)((float)f[0]*255.0);
        rgba[1] = (unsigned char)((float)f[1]*255.0);
        rgba[2] = (unsigned char)((float)f[2]*255.0);
        rgba[3] = (unsigned char)((float)f[3]*255.0);
        scalars->SetColor(i,rgba);
        }
      }
    if ( ! skipScalar ) ds->GetPointData()->SetScalars(scalars);
    scalars->Delete();
    }

  else
    {
    vtkErrorMacro(<< "Unsupported number values per scalar: " << nValues);
    return 0;
    }

  return 1;
}

// Description:
// Read texture coordinates point attributes. Return 0 if error.
int vtkDataReader::ReadTCoordsData(FILE *fp, vtkDataSet *ds, int numPts)
{
  int retStat, i, dim, skipTCoord;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %d %256s", name, &dim, line)) == EOF || retStat < 3) 
    {
    vtkErrorMacro(<<"Cannot read texture data!");
    return 0;
    }

  if ( dim < 1 || dim > 3 )
    {
    vtkErrorMacro(<< "Unsupported texture coordinates dimension: " << dim);
    return 0;
    }

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
    vtkFloatTCoords *tcoords = new vtkFloatTCoords(numPts);
    tcoords->SetDimension(dim);
    float *ptr = tcoords->WritePtr(0,numPts);
    if ( this->FileType == BINARY)
      {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(float),dim*numPts,fp) != (dim*numPts)) )
        {
        vtkErrorMacro(<<"Error reading binary tensors!");
        return 0;
        }
      tcoords->WrotePtr();
      }
    else // ascii
      {
      float tc[3];
      int j;
      for (i=0; i<numPts; i++)
        {
        for (j=0; j<dim; j++)
          {
          if ((retStat=fscanf(fp,"%f",tc+j)) == EOF || retStat < 1)
            {
            vtkErrorMacro(<<"Error reading texture coordinates!");
            return 0;
            }
          }
        tcoords->SetTCoord(i,tc);
        }
      }
    if ( ! skipTCoord ) ds->GetPointData()->SetTCoords(tcoords);
    tcoords->Delete();
    }

  else 
    {
    vtkErrorMacro(<< "Unsupported texture coordinates data type: " << line);
    return 0;
  }

  return 1;
}

// Description:
// Read lookup table. Return 0 if error.
int vtkDataReader::ReadLutData(FILE *fp, vtkDataSet *ds)
{
  int retStat, i, size, skipTable;
  vtkLookupTable *lut;
  unsigned char *ptr;
  char line[257], name[257];

  if ((retStat=fscanf(fp, "%256s %d", name, &size)) ==  EOF || retStat < 2) 
    {
    vtkErrorMacro(<<"Cannot read lookup table data!");
    return 0;
    }

  if ( ds->GetPointData()->GetScalars() == NULL ||
  (this->LookupTableName && strcmp(name,this->LookupTableName)) ||
  (this->ScalarLut && strcmp(name,this->ScalarLut)) )
    {
    skipTable = 1;
    }

  lut = new vtkLookupTable(size);
  ptr = lut->WritePtr(0,size);

  if ( this->FileType == BINARY)
    {
      if ( (fgets(line,256,fp) == NULL) || //suck up newline
      (fread(ptr,sizeof(unsigned char),4*size,fp) != (4*size)) )
        {
        vtkErrorMacro(<<"Error reading binary lookup table!");
        return 0;
        }
    lut->WrotePtr();
    }
  else // ascii
    {
    float rgba[4];
    for (i=0; i<size; i++)
      {
      if ((retStat=fscanf(fp,"%f %f %f %f",rgba,rgba+1,rgba+2,rgba+3)) == EOF || retStat < 4) 
        {
        vtkErrorMacro(<<"Error reading lookup table!");
        return 0;
        }
      lut->SetTableValue(i, rgba[0], rgba[1], rgba[2], rgba[3]);
      }
    }

  if ( ! skipTable ) ds->GetPointData()->GetScalars()->SetLookupTable(lut);
  lut->Delete();

  return 1;
}


// Description:
// Read lookup table. Return 0 if error.
int vtkDataReader::ReadCells(FILE *fp, int size, int *data)
{
  char line[257];
  int i, retStat;

  if ( this->FileType == BINARY)
    {
    if ( (fgets(line,256,fp) == NULL) || //suck up newline
    (fread(data,sizeof(int),size,fp) != size) )
      {
      vtkErrorMacro(<<"Error reading binary cell data!");
      return 0;
      }
    }
  else // ascii
    {
    for (i=0; i<size; i++)
      {
      if ((retStat=fscanf(fp,"%d",data+i)) == EOF || retStat < 1) 
        {
        vtkErrorMacro(<<"Error reading texture coordinates!");
        return 0;
        }
      }
    }

  return 1;
}

char *vtkDataReader::LowerCase(char *str)
{
  int i;
  char *s;

  for ( i=0, s=str; *s != '\0' && i<256; s++,i++) 
    *s = tolower(*s);
  return str;
}

// Description:
// Close a vtk file.
void vtkDataReader::CloseVTKFile(FILE *fp)
{
  vtkDebugMacro(<<"Closing vtk file");
  if ( fp != NULL ) fclose(fp);
}

void vtkDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Filename )
    os << indent << "Filename: " << this->Filename << "\n";
  else
    os << indent << "Filename: (None)\n";

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
