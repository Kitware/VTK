/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkLookupTable.h"
#include "vtkCellArray.h"
#include "vtkByteSwap.h"
#include "vtkBitArray.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDoubleArray.h"

// Description:
// Created object with default header, ASCII format, and default names for 
// scalars, vectors, tensors, normals, and texture coordinates.
vtkDataWriter::vtkDataWriter()
{
  this->FileName = NULL;

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

  this->FieldDataName = new char[10];
  strcpy(this->FieldDataName,"FieldData");
}

vtkDataWriter::~vtkDataWriter()
{
  if ( this->FileName ) delete [] this->FileName;
  if ( this->Header ) delete [] this->Header;
  if ( this->ScalarsName ) delete [] this->ScalarsName;
  if ( this->VectorsName ) delete [] this->VectorsName;
  if ( this->TensorsName ) delete [] this->TensorsName;
  if ( this->NormalsName ) delete [] this->NormalsName;
  if ( this->TCoordsName ) delete [] this->TCoordsName;
  if ( this->LookupTableName ) delete [] this->LookupTableName;
  if ( this->FieldDataName ) delete [] this->FieldDataName;
}

// Description:
// Open a vtk data file. Returns NULL if error.
FILE *vtkDataWriter::OpenVTKFile()
{
  FILE *fptr;

  vtkDebugMacro(<<"Opening vtk file for writing...");

  if ( !this->FileName )
    {
    vtkErrorMacro(<< "No FileName specified! Can't write!");
    return NULL;
    }

  if ( (fptr=fopen(this->FileName, "wb")) == NULL )
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    return NULL;
    }

  return fptr;
}

// Description:
// Write the header of a vtk data file. Returns 0 if error.
int vtkDataWriter::WriteHeader(FILE *fp)
{
  vtkDebugMacro(<<"Writing header...");

  fprintf (fp, "# vtk DataFile Version 2.0\n");
  fprintf (fp, "%s\n", this->Header);

  if ( this->FileType == VTK_ASCII )
    fprintf (fp, "ASCII\n");
  else
    fprintf (fp, "BINARY\n");

  return 1;
}

// Description:
// Write the cell data (e.g., scalars, vectors, ...) of a vtk dataset.
// Returns 0 if error.
int vtkDataWriter::WriteCellData(FILE *fp, vtkDataSet *ds)
{
  int numCells;
  vtkScalars *scalars;
  vtkVectors *vectors;
  vtkNormals *normals;
  vtkTCoords *tcoords;
  vtkTensors *tensors;
  vtkFieldData *field;
  vtkCellData *cd=ds->GetCellData();

  vtkDebugMacro(<<"Writing cell data...");

  numCells = ds->GetNumberOfCells();
  scalars = cd->GetScalars();
  vectors = cd->GetVectors();
  normals = cd->GetNormals();
  tcoords = cd->GetTCoords();
  tensors = cd->GetTensors();
  field = cd->GetFieldData();

  if ( numCells <= 0 || !(scalars || vectors || normals || tcoords || tensors || field) )
    {
    vtkDebugMacro(<<"No cell data to write!");
    return 1;
    }

  fprintf (fp, "CELL_DATA %d\n", numCells);
//
// Write scalar data
//
  if ( scalars && scalars->GetNumberOfScalars() > 0 )
    {
    if ( ! this->WriteScalarData(fp, scalars, numCells) ) return 0;
    }
//
// Write vector data
//
  if ( vectors && vectors->GetNumberOfVectors() > 0 )
    {
    if ( ! this->WriteVectorData(fp, vectors, numCells) ) return 0;
    }
//
// Write normals
//
  if ( normals && normals->GetNumberOfNormals() > 0 )
    {
    if ( ! this->WriteNormalData(fp, normals, numCells) ) return 0;
    }
//
// Write texture coords
//
  if ( tcoords && tcoords->GetNumberOfTCoords() > 0 )
    {
    if ( ! this->WriteTCoordData(fp, tcoords, numCells) ) return 0;
    }
//
// Write tensors
//
  if ( tensors && tensors->GetNumberOfTensors() > 0 )
    {
    if ( ! this->WriteTensorData(fp, tensors, numCells) ) return 0;
    }
//
// Write field
//
  if ( field && field->GetNumberOfTuples() > 0 )
    {
    if ( ! this->WriteFieldData(fp, field, numCells) ) return 0;
    }

  return 1;
}

// Description:
// Write the point data (e.g., scalars, vectors, ...) of a vtk dataset.
// Returns 0 if error.
int vtkDataWriter::WritePointData(FILE *fp, vtkDataSet *ds)
{
  int numPts;
  vtkScalars *scalars;
  vtkVectors *vectors;
  vtkNormals *normals;
  vtkTCoords *tcoords;
  vtkTensors *tensors;
  vtkFieldData *field;
  vtkPointData *pd=ds->GetPointData();

  vtkDebugMacro(<<"Writing point data...");

  numPts = ds->GetNumberOfPoints();
  scalars = pd->GetScalars();
  vectors = pd->GetVectors();
  normals = pd->GetNormals();
  tcoords = pd->GetTCoords();
  tensors = pd->GetTensors();
  field = pd->GetFieldData();

  if ( numPts <= 0 || !(scalars || vectors || normals || tcoords || tensors || field) )
    {
    vtkDebugMacro(<<"No point data to write!");
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

//
// Write field
//
  if ( field && field->GetNumberOfTuples() > 0 )
    {
    if ( ! this->WriteFieldData(fp, field, numPts) ) return 0;
    }

  return 1;
}

// Template to handle writing data in ascii or binary
template <class T>
static void WriteDataArray(FILE *fp, T *data, int fileType, char *format, int num, int numComp)
{
  int i, j, idx;

  if ( fileType == VTK_ASCII )
    {
    for (j=0; j<num; j++)
      {
      for (i=0; i<numComp; i++)
	{
	idx = i + j*numComp;
	fprintf (fp, format, *data++);
	if ( !((idx+1)%9) ) fprintf (fp,"\n");
	}
      }
    }
  else
    {
    fwrite (data, sizeof(T), num*numComp, fp);
    }
  fprintf (fp,"\n");
}

// Write out data to file specified.
int vtkDataWriter::WriteArray(FILE *fp, int dataType, vtkDataArray *data, char *format, 
			      int num, int numComp)
{
  int i, j, idx;
  
  switch (dataType)
    {
    case VTK_BIT:
      {
      fprintf (fp, format, "int");
      if ( this->FileType == VTK_ASCII )
	{
	int s;
	for (j=0; j<num; j++)
	  {
	  for (i=0; i<numComp; i++)
	    {
	    idx = i + j*numComp;
	    s = ((vtkBitArray *)data)->GetValue(i);
	    fprintf (fp, "%d ", (s!=0.0?1:0));
	    if ( !((idx+1)%6) ) fprintf (fp,"\n");
	    }
	  }
	}
      else
	{
        unsigned char *cptr=((vtkUnsignedCharArray *)data)->GetPointer(0);
	fwrite (cptr,sizeof(unsigned char),(num-1)/8+1,fp);
	}
      fprintf (fp,"\n");
      }
    break;

    case VTK_CHAR:
      {
      fprintf (fp, format, "char");
      char *s=((vtkCharArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%i ", num, numComp);
      }
    break;

    case VTK_UNSIGNED_CHAR:
      {
      fprintf (fp, format, "unsigned_char");
      unsigned char *s=((vtkUnsignedCharArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%i ", num, numComp);
      }
    break;
    
    case VTK_SHORT:
      {
      fprintf (fp, format, "short");
      short *s=((vtkShortArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%hd ", num, numComp);
      }
    break;

    case VTK_UNSIGNED_SHORT:
      {
      fprintf (fp, format, "unsigned_short");
      unsigned short *s=((vtkUnsignedShortArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%hu ", num, numComp);
      }
    break;

    case VTK_INT:
      {
      fprintf (fp, format, "int");
      int *s=((vtkIntArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%d ", num, numComp);
      }
    break;

    case VTK_UNSIGNED_INT:
      {
      fprintf (fp, format, "unsigned_int");
      unsigned int *s=((vtkUnsignedIntArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%d ", num, numComp);
      }
    break;

    case VTK_LONG:
      {
      fprintf (fp, format, "long");
      long *s=((vtkLongArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%d ", num, numComp);
      }
    break;

    case VTK_UNSIGNED_LONG:
      {
      fprintf (fp, format, "unsigned_long");
      unsigned long *s=((vtkUnsignedLongArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%d ", num, numComp);
      }
    break;

    case VTK_FLOAT:
      {
      fprintf (fp, format, "float");
      float *s=((vtkFloatArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%g ", num, numComp);
      }
    break;

    case VTK_DOUBLE:
      {
      fprintf (fp, format, "double");
      double *s=((vtkDoubleArray *)data)->GetPointer(0);
      WriteDataArray(fp, s, this->FileType, "%g ", num, numComp);
      }
    break;

    default:
      {
      vtkErrorMacro(<<"Type currently not supported");
      return 0;
      }
    }
  
  return 1;
}

int vtkDataWriter::WritePoints(FILE *fp, vtkPoints *points)
{
  int numPts=points->GetNumberOfPoints();
  
  fprintf (fp, "POINTS %d ", numPts);
  return this->WriteArray(fp, points->GetDataType(), points->GetData(), "%s\n", numPts, 3);
}

// Description:
// Write out coordinates for rectilinear grids.
int vtkDataWriter::WriteCoordinates(FILE *fp, vtkScalars *coords, int axes)
{
  int ncoords=coords->GetNumberOfScalars();
  
  if ( axes == 0 ) fprintf (fp, "X_COORDINATES %d ", ncoords);
  else if ( axes == 1) fprintf (fp, "Y_COORDINATES %d ", ncoords);
  else fprintf (fp, "Z_COORDINATES %d ", ncoords);

  return this->WriteArray(fp, coords->GetDataType(), coords->GetData(), "%s\n", ncoords, 1);
}

// Description:
// Write out scalar data.
int vtkDataWriter::WriteScalarData(FILE *fp, vtkScalars *scalars, int num)
{
  int i, j, size=0;
  char *name;
  vtkLookupTable *lut;
  int dataType = scalars->GetDataType();

  if ( (lut=scalars->GetLookupTable()) == NULL || (size = lut->GetNumberOfColors()) <= 0 )
    name = "default";
  else 
    name = this->LookupTableName;

  if ( dataType != VTK_UNSIGNED_CHAR )
    {
    char format[1024];
    fprintf (fp, "SCALARS ");
    sprintf(format,"%s %s %s\n",this->ScalarsName, "%s\nLOOKUP_TABLE", name);
    if (this->WriteArray(fp, scalars->GetDataType(), scalars->GetData(), format, num, 1) == 0)
      {
      return 0;
      }
    }

  else //color scalars
    {
    int nvs = scalars->GetNumberOfComponents();
    unsigned char *data=((vtkUnsignedCharArray *)scalars->GetData())->GetPointer(0);
    fprintf (fp, "COLOR_SCALARS %s %d\n", this->ScalarsName, nvs);

    if ( this->FileType == VTK_ASCII )
      {
      for (i=0; i<num; i++)
        {
	for (j=0; j<nvs; j++)
	  {
	  fprintf (fp, "%g ", (float)data[nvs*i+j]/255.0);
	  }
        if ( i != 0 && !(i%2) ) fprintf (fp,"\n");
        }
      }
    else // binary type
      {
      fwrite (data,sizeof(unsigned char),nvs*num,fp);
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
        fprintf (fp, "%g %g %g %g\n", c[0], c[1], c[2], c[3]);
        }
      }
    else
      {
      unsigned char *colors=lut->GetPointer(0);
      fwrite(colors,sizeof(unsigned char),4*size,fp);
      }
    fprintf (fp, "\n");
    }

  return 1;
}

int vtkDataWriter::WriteVectorData(FILE *fp, vtkVectors *vectors, int num)
{
  char format[1024];

  fprintf (fp, "VECTORS ");
  sprintf(format, "%s %s\n", this->VectorsName, "%s");
  return this->WriteArray(fp, vectors->GetDataType(), vectors->GetData(), format, num, 3);
}

int vtkDataWriter::WriteNormalData(FILE *fp, vtkNormals *normals, int num)
{
  char format[1024];

  fprintf (fp, "NORMALS ");
  sprintf(format, "%s %s\n", this->NormalsName, "%s");
  return this->WriteArray(fp, normals->GetDataType(), normals->GetData(), format, num, 3);
}

int vtkDataWriter::WriteTCoordData(FILE *fp, vtkTCoords *tcoords, int num)
{
  int dim=tcoords->GetNumberOfComponents();
  char format[1024];

  fprintf (fp, "TEXTURE_COORDINATES ");
  sprintf(format, "%s %d %s\n", this->TCoordsName, dim, "%s");
  return this->WriteArray(fp, tcoords->GetDataType(), tcoords->GetData(), format, num, dim);
}

int vtkDataWriter::WriteTensorData(FILE *fp, vtkTensors *tensors, int num)
{
  char format[1024];

  fprintf (fp, "TENSORS ");
  sprintf(format, "%s %s\n", this->TensorsName, "%s");
  return this->WriteArray(fp, tensors->GetDataType(), tensors->GetData(), format, num, 9);
}

int vtkDataWriter::WriteFieldData(FILE *fp, vtkFieldData *f, int num)
{
  char format[1024];
  int i, numArrays=f->GetNumberOfArrays();
  int numComp, numTuples, numFieldComp=f->GetNumberOfComponents();
  vtkDataArray *array;

  if ( numArrays < 1 ) return 1;
  fprintf (fp, "FIELD %s %d\n", this->FieldDataName, numArrays);
  
  for (i=0; i < numArrays; i++)
    {
    array = f->GetArray(i);
    if ( array != NULL )
      {
      numComp = array->GetNumberOfComponents();
      numTuples = array->GetNumberOfTuples();
      sprintf(format, "%s %d %d %s\n", f->GetArrayName(i), numComp, numTuples, "%s");
      this->WriteArray(fp, array->GetDataType(), array, format, numTuples, numComp);
      }
    else
      {
      fprintf (fp, "NULL_ARRAY");
      }
    }
  
  return 1;
}

int vtkDataWriter::WriteCells(FILE *fp, vtkCellArray *cells, char *label)
{
  int ncells=cells->GetNumberOfCells();
  int size=cells->GetNumberOfConnectivityEntries();

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
    vtkByteSwap::SwapWrite4BERange(cells->GetPointer(),size,fp);
    }

  fprintf (fp,"\n");
  return 1;
}

void vtkDataWriter::WriteData()
{
  vtkErrorMacro(<<"WriteData() should be implemented in concrete subclass");
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

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

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

  if ( this->FieldDataName )
    os << indent << "Field Data Name: " << this->FieldDataName << "\n";
  else
    os << indent << "Field Data Name: (None)\n";

}
