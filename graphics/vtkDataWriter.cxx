/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkDataWriter.h"
#include "vtkBitScalars.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkFloatScalars.h"
#include "vtkShortScalars.h"
#include "vtkIntScalars.h"
#include "vtkFloatPoints.h"
#include "vtkIntPoints.h"
#include "vtkFloatNormals.h"
#include "vtkFloatTensors.h"
#include "vtkFloatTCoords.h"
#include "vtkGraymap.h"
#include "vtkAGraymap.h"
#include "vtkPixmap.h"
#include "vtkAPixmap.h"
#include "vtkLookupTable.h"
#include "vtkCellArray.h"
#include "vtkByteSwap.h"

// Description:
// Created object with default header, ASCII format, and default names for 
// scalars, vectors, tensors, normals, and texture coordinates.
vtkDataWriter::vtkDataWriter()
{
  this->Filename = NULL;

  this->Header = new char[257];
  strcpy(this->Header,"vtk output");
  this->FileType = VTK_ASCII;

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

vtkDataWriter::~vtkDataWriter()
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
// Open a vtk data file. Returns NULL if error.
FILE *vtkDataWriter::OpenVTKFile()
{
  FILE *fptr;

  vtkDebugMacro(<<"Opening vtk file for writing...");

  if ( !this->Filename )
    {
    vtkErrorMacro(<< "No filename specified! Can't write!");
    return NULL;
    }

  if ( (fptr=fopen(this->Filename, "wb")) == NULL )
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->Filename);
    return NULL;
    }

  return fptr;
}

// Description:
// Write the header of a vtk data file. Returns 0 if error.
int vtkDataWriter::WriteHeader(FILE *fp)
{
  vtkDebugMacro(<<"Writing header...");

  fprintf (fp, "# vtk DataFile Version 1.0\n");
  fprintf (fp, "%s\n", this->Header);

  if ( this->FileType == VTK_ASCII )
    fprintf (fp, "ASCII\n");
  else
    fprintf (fp, "BINARY\n");

  return 1;
}

// Description:
// Write the point data (e.g., scalars, vectors, ...) of a vtk data file. 
// Returns 0 if error.
int vtkDataWriter::WritePointData(FILE *fp, vtkDataSet *ds)
{
  int numPts;
  vtkScalars *scalars;
  vtkVectors *vectors;
  vtkNormals *normals;
  vtkTCoords *tcoords;
  vtkTensors *tensors;
  vtkPointData *pd=ds->GetPointData();

  vtkDebugMacro(<<"Writing point data...");

  numPts = ds->GetNumberOfPoints();
  scalars = pd->GetScalars();
  vectors = pd->GetVectors();
  normals = pd->GetNormals();
  tcoords = pd->GetTCoords();
  tensors = pd->GetTensors();

  if ( numPts <= 0 || !(scalars || vectors || normals || tcoords || tensors) )
    {
    vtkWarningMacro(<<"No point data to write!");
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

int vtkDataWriter::WritePoints(FILE *fp, vtkPoints *points)
{
  int i, numPts=points->GetNumberOfPoints();
  char *type;
  vtkByteSwap swap;
  
  fprintf (fp, "POINTS %d ", numPts);
  type = points->GetDataType();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "float\n");
    if ( this->FileType == VTK_ASCII )
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
      vtkFloatPoints *fpoints = (vtkFloatPoints *)points;
      float *fptr=fpoints->GetPtr(0);
      // swap the bytes if necc
      swap.SwapWrite4BERange(fptr,3*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else if ( !strcmp(type,"int") )
    {
    vtkIntPoints *ipoints = (vtkIntPoints *)points;
    fprintf (fp, "int\n");
    if ( this->FileType == VTK_ASCII )
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
      vtkIntPoints *ipoints = (vtkIntPoints *)points;
      int *iptr=ipoints->GetPtr(0);
      // swap the bytes if necc
      swap.SwapWrite4BERange(iptr,3*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vtkErrorMacro(<<"Point type: " << type << " currently not supported");
    return 0;
    }

  return 1;
}

int vtkDataWriter::WriteScalarData(FILE *fp, vtkScalars *scalars, int numPts)
{
  int i, size = 0;
  char *type, *name;
  vtkLookupTable *lut;
  vtkByteSwap swap;

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
      if ( this->FileType == VTK_ASCII )
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
        vtkBitScalars *bscalars = (vtkBitScalars *)scalars;
        unsigned char *cptr=bscalars->GetPtr(0);
        fwrite (cptr,sizeof(char),(numPts-1)/8+1,fp);
        }
      fprintf (fp,"\n");
      }

    else if ( !strcmp(type,"unsigned char") )
      {
      fprintf (fp, "%s unsigned_char\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == VTK_ASCII )
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
        vtkUnsignedCharScalars *cscalars = (vtkUnsignedCharScalars *)scalars;
        unsigned char *cptr=cscalars->GetPtr(0);
        fwrite (cptr,sizeof(unsigned char),numPts,fp);
        }
      fprintf (fp,"\n");
      }

    else if ( !strcmp(type,"short") )
      {
      fprintf (fp, "%s short\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == VTK_ASCII )
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
        vtkShortScalars *sscalars = (vtkShortScalars *)scalars;
        short *sptr=sscalars->GetPtr(0);
	// swap the bytes if necc
	swap.SwapWrite2BERange(sptr,numPts,fp);
        }
      fprintf (fp,"\n");
      }

    else if ( !strcmp(type,"int") )
      {
      fprintf (fp, "%s int\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == VTK_ASCII )
        {
        float s;
        for (i=0; i<numPts; i++)
          {
          s = scalars->GetScalar(i);
          fprintf (fp, "%d ", (int)s);
          if ( !((i+1)%6) ) fprintf (fp,"\n");
          }
        }
      else
        {
        vtkIntScalars *iscalars = (vtkIntScalars *)scalars;
        int *iptr=iscalars->GetPtr(0);
	// swap the bytes if necc
	swap.SwapWrite4BERange(iptr,numPts,fp);
        }
      fprintf (fp,"\n");
      }

    else if ( !strcmp(type,"float") )
      {
      fprintf (fp, "%s float\nLOOKUP_TABLE %s\n", this->ScalarsName, name);
      if ( this->FileType == VTK_ASCII )
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
        vtkFloatScalars *fscalars = (vtkFloatScalars *)scalars;
        float *fptr=fscalars->GetPtr(0);
	// swap the bytes if necc
	swap.SwapWrite4BERange(fptr,numPts,fp);
        }
      fprintf (fp,"\n");
      }

    else
      {
      vtkErrorMacro(<<"Scalar type: " << type << " currently not supported");
      return 0;
      }
    }

  else //color scalars
    {
    int nvs = scalars->GetNumberOfValuesPerScalar();
    vtkColorScalars *coscalars = (vtkColorScalars *)scalars;

    fprintf (fp, "COLOR_SCALARS %s %d\n", this->ScalarsName, nvs);

    if ( this->FileType == VTK_ASCII )
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
    if ( this->FileType == VTK_ASCII )
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

  return 1;
}

int vtkDataWriter::WriteVectorData(FILE *fp, vtkVectors *vectors, int numPts)
{
  int i;
  char *type;
  vtkByteSwap swap;


  fprintf (fp, "VECTORS ");
  type = vectors->GetDataType();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "%s float\n", this->VectorsName);
    if ( this->FileType == VTK_ASCII )
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
      vtkFloatVectors *fvectors = (vtkFloatVectors *)vectors;
      float *fptr=fvectors->GetPtr(0);
      // swap the bytes if necc
      swap.SwapWrite4BERange(fptr,3*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vtkErrorMacro(<<"Vector type: " << type << " currently not supported");
    return 0;
    }

  return 1;
}

int vtkDataWriter::WriteNormalData(FILE *fp, vtkNormals *normals, int numPts)
{
  int i;
  char *type;
  vtkByteSwap swap;

  fprintf (fp, "NORMALS ");
  type = normals->GetDataType();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "%s float\n", this->NormalsName);
    if ( this->FileType == VTK_ASCII )
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
      vtkFloatNormals *fnormals = (vtkFloatNormals *)normals;
      float *fptr=fnormals->GetPtr(0);
      // swap the bytes if necc
      swap.SwapWrite4BERange(fptr,3*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vtkErrorMacro(<<"Normal type: " << type << " currently not supported");
    return 0;
    }

  return 1;
}

int vtkDataWriter::WriteTCoordData(FILE *fp, vtkTCoords *tcoords, int numPts)
{
  int i, j, dim;
  char *type;
  vtkByteSwap swap;

  fprintf (fp, "TEXTURE_COORDINATES ");
  type = tcoords->GetDataType();
  dim = tcoords->GetDimension();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "%s %d float\n", this->TCoordsName, dim);
    if ( this->FileType == VTK_ASCII )
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
      vtkFloatTCoords *ftcoords = (vtkFloatTCoords *)tcoords;
      float *fptr=ftcoords->GetPtr(0);
      // swap the bytes if necc
      swap.SwapWrite4BERange(fptr,dim*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vtkErrorMacro(<<"Texture coord type: " << type << " currently not supported");
    return 0;
    }

  return 1;
}

int vtkDataWriter::WriteTensorData(FILE *fp, vtkTensors *tensors, int numPts)
{
  int i, j, k, dim;
  char *type;
  vtkByteSwap swap;

  fprintf (fp, "TENSORS ");
  type = tensors->GetDataType();
  dim = tensors->GetDimension();

  if ( !strcmp(type,"float") )
    {
    fprintf (fp, "%s float\n", this->TensorsName);
    if ( this->FileType == VTK_ASCII )
      {
      vtkTensor t;
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
      vtkFloatTensors *ftensors = (vtkFloatTensors *)tensors;
      float *fptr=ftensors->GetPtr(0);
      // swap the bytes if necc
      swap.SwapWrite4BERange(fptr,dim*dim*numPts,fp);
      }
    fprintf (fp,"\n");
    }

  else
    {
    vtkErrorMacro(<<"Tensor type: " << type << " currently not supported");
    return 0;
    }

  return 1;
}

int vtkDataWriter::WriteCells(FILE *fp, vtkCellArray *cells, char *label)
{
  int ncells=cells->GetNumberOfCells();
  int size=cells->GetNumberOfConnectivityEntries();
  vtkByteSwap swap;

  if ( ncells < 1 ) return 1;

  fprintf (fp, "%s %d %d\n", label, ncells, size);

  if ( this->FileType == VTK_ASCII )
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
    // swap the bytes if necc
    swap.SwapWrite4BERange(cells->GetPtr(),size,fp);
    }

  fprintf (fp,"\n");
  return 1;
}

// Description:
// Close a vtk file.
void vtkDataWriter::CloseVTKFile(FILE *fp)
{
  vtkDebugMacro(<<"Closing vtk file\n");
  if ( fp != NULL ) fclose(fp);
}

void vtkDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);

  os << indent << "Filename: " 
     << (this->Filename ? this->Filename : "(none)") << "\n";

  if ( this->FileType == VTK_BINARY )
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
