/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataReader.cxx
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
#include <ctype.h>
#include "vtkDataReader.h"
#include "vtkRectilinearGrid.h"
#include "vtkPointSet.h"
#include "vtkFieldData.h"
#include "vtkLookupTable.h"
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
// Construct object.
vtkDataReader::vtkDataReader()
{
  this->FileType = VTK_ASCII;
  this->FileName = NULL;
  this->ScalarsName = NULL;
  this->VectorsName = NULL;
  this->TensorsName = NULL;
  this->NormalsName = NULL;
  this->TCoordsName = NULL;
  this->LookupTableName = NULL;
  this->FieldDataName = NULL;
  this->ScalarLut = NULL;
  this->InputString = NULL;
  this->InputStringPos = 0;
  this->ReadFromInputString = 0;
  this->IS = NULL;
  this->Source = NULL;
}  

vtkDataReader::~vtkDataReader()
{
  if (this->FileName) delete [] this->FileName;
  if (this->ScalarsName) delete [] this->ScalarsName;

  if (this->VectorsName) delete [] this->VectorsName;
  if (this->TensorsName) delete [] this->TensorsName;
  if (this->NormalsName) delete [] this->NormalsName;
  if (this->TCoordsName) delete [] this->TCoordsName;
  if (this->LookupTableName) delete [] this->LookupTableName;
  if (this->FieldDataName) delete [] this->FieldDataName;
  if (this->ScalarLut) delete [] this->ScalarLut;
  if (this->InputString) delete [] this->InputString;
}

void vtkDataReader::SetInputString(char* _arg, int len)
{ 
  if (this->Debug)
    {
    vtkDebugMacro(<< "setting InputString to " << _arg );
    }

  if (this->InputString && _arg && strncmp(_arg, this->InputString, len) == 0)
    {
    return;
    }
  
  if (this->InputString) delete [] this->InputString; 
  if (_arg) 
    { 
    this->InputString = new char[len]; 
    memcpy(this->InputString,_arg,len); 
    } 
   else 
    { 
    this->InputString = NULL; 
    } 
  this->Modified(); 
} 

// Description:
// Internal function used to consume whitespace when reading in
// an InputString.
void vtkDataReader::EatWhiteSpace()
{
  char c;
  while (this->IS->get(c)) 
    {
    if (c == '\n' || isspace(c)==0)
      {
      this->IS->putback(c);
      break;
      }
    }
}

// Description:
// Internal function to read in a line up to 256 characters.
// Returns zero if there was an error.
int vtkDataReader::ReadLine(char result[256])
{
  this->IS->getline(result,256);
  if (this->IS->eof()) return 0;
  return 1;
}

// Description:
// Internal function to read in a string up to 256 characters.
// Returns zero if there was an error.
int vtkDataReader::ReadString(char result[256])
{
  *this->IS >> result;
  if (this->IS->fail()) return 0;
  return 1;
}

// Description:
// Internal function to read in an integer value.
// Returns zero if there was an error.
int vtkDataReader::Read(char *result)
{
  int intData;
  *this->IS >> intData;
  if (this->IS->fail()) return 0;

  *result = (char) intData;
  return 1;
}

int vtkDataReader::Read(unsigned char *result)
{
  int intData;
  *this->IS >> intData;
  if (this->IS->fail()) return 0;

  *result = (unsigned char) intData;
  return 1;
}

int vtkDataReader::Read(short *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

int vtkDataReader::Read(unsigned short *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

int vtkDataReader::Read(int *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

int vtkDataReader::Read(unsigned int *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

int vtkDataReader::Read(long *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

int vtkDataReader::Read(unsigned long *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

int vtkDataReader::Read(float *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}

int vtkDataReader::Read(double *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) return 0;
  return 1;
}


// Description:
// Open a vtk data file. Returns zero if error.
int vtkDataReader::OpenVTKFile()
{
  if (this->ReadFromInputString)
    {
    if (this->InputString)
      {
      vtkDebugMacro(<< "Reading from InputString");
      this->IS = new istrstream(this->InputString,strlen(this->InputString));
      return 1;
      }
    }
  else
    {
    vtkDebugMacro(<< "Opening vtk file");

    if ( !this->FileName )
      {
      vtkErrorMacro(<< "No file specified!");
      return 0;
      }
    this->IS = new ifstream(this->FileName, ios::in);
    if (this->IS->fail())
      {
      vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    return 1;
    }

  return 0;
}

// Description:
// Read the header of a vtk data file. Returns 0 if error.
int vtkDataReader::ReadHeader()
{
  char line[256];

  vtkDebugMacro(<< "Reading vtk file header");
  //
  // read header
  //
  if (!this->ReadLine(line))
    {
    vtkErrorMacro(<<"Premature EOF reading first line! " << " for file: " 
                  << this->FileName);
    return 0;
    }
  if ( strncmp ("# vtk DataFile Version", line, 20) )
    {
    vtkErrorMacro(<< "Unrecognized file type: "<< line << " for file: " 
                  << this->FileName);
    return 0;
    }
  //
  // read title
  //
  if (!this->ReadLine(line))
    {
    vtkErrorMacro(<<"Premature EOF reading title! " << " for file: " 
                  << this->FileName);
    return 0;
    }
  vtkDebugMacro(<< "Reading vtk file entitled: " << line);
  //
  // read type
  //
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Premature EOF reading file type!" << " for file: " 
                  << this->FileName);
    return 0;
    }

  if ( !strncmp(this->LowerCase(line),"ascii",5) ) this->FileType = VTK_ASCII;
  else if ( !strncmp(line,"binary",6) ) this->FileType = VTK_BINARY;
  else
    {
    vtkErrorMacro(<< "Unrecognized file type: "<< line << " for file: " 
                  << this->FileName);
    this->FileType = 0;
    return 0;
    }

  // if this is a binary file we need to make sure that we opened it 
  // as a binary file.
  if (this->FileType == VTK_BINARY)
    {
    vtkDebugMacro(<< "Opening vtk file as binary");
    delete this->IS;
#ifdef _WIN32
    this->IS = new ifstream(this->FileName, ios::in | ios::binary);
#else
    this->IS = new ifstream(this->FileName, ios::in);
#endif
    if (this->IS->fail())
      {
      vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
      delete this->IS;
      this->IS = NULL;
      return 0;
      }
    // read up to the same point in the file
    this->ReadLine(line);
    this->ReadLine(line);
    this->ReadString(line);
    }

  if (this->Source)
    {
    float progress=this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }
  
  return 1;
}

// Description:
// Read the cell data of a vtk data file. The number of cells (from the 
// dataset) must match the number of cells defined in cell attributes (unless
// no geometry was defined).
int vtkDataReader::ReadCellData(vtkDataSet *ds, int numCells)
{
  char line[256];
  vtkDataSetAttributes *a=ds->GetCellData();

  vtkDebugMacro(<< "Reading vtk cell data");
    
  //
  // Read keywords until end-of-file
  //
  while (this->ReadString(line))
    {
    //
    // read scalar data
    //
    if ( ! strncmp(this->LowerCase(line), "scalars", 7) )
      {
      if ( ! this->ReadScalarData(a, numCells) ) return 0;
      }
    //
    // read vector data
    //
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(a, numCells) ) return 0;
      }
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(a, numCells) ) return 0;
      }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
      {
      if ( ! this->ReadNormalData(a, numCells) ) return 0;
      }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(a, numCells) ) return 0;
      }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(a, numCells) ) return 0;
      }
    //
    // read lookup table. Associate with scalar data.
    //
    else if ( ! strncmp(line, "lookup_table", 12) )
      {
      if ( ! this->ReadLutData(a) ) return 0;
      }
    //
    // read field of data
    //
    else if ( ! strncmp(line, "field", 5) )
      {
      vtkFieldData *f;
      if ( ! (f=this->ReadFieldData(numCells)) ) return 0;
      a->SetFieldData(f);
      f->Delete();
      }
    //
    // maybe bumped into point data
    //
    else if ( ! strncmp(line, "point_data", 10) )
      {
      int npts;
      if (!this->Read(&npts))
        {
        vtkErrorMacro(<<"Cannot read point data!");
        return 0;
        }

      this->ReadPointData(ds, npts);
      }

    else
      {
      vtkErrorMacro(<< "Unsupported cell attribute type: " << line 
                    << " for file: " << this->FileName);
      return 0;
      }
    }

  return 1;
}


// Description:
// Read the point data of a vtk data file. The number of points (from the 
// dataset) must match the number of points defined in point attributes (unless
// no geometry was defined).
int vtkDataReader::ReadPointData(vtkDataSet *ds, int numPts)
{
  char line[256];
  vtkDataSetAttributes *a=ds->GetPointData();

  vtkDebugMacro(<< "Reading vtk point data");
    
  //
  // Read keywords until end-of-file
  //
  while (this->ReadString(line))
    {
    //
    // read scalar data
    //
    if ( ! strncmp(this->LowerCase(line), "scalars", 7) )
      {
      if ( ! this->ReadScalarData(a, numPts) ) return 0;
      }
    //
    // read vector data
    //
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(a, numPts) ) return 0;
      }
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(a, numPts) ) return 0;
      }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
      {

      if ( ! this->ReadNormalData(a, numPts) ) return 0;
      }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(a, numPts) ) return 0;
      }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(a, numPts) ) return 0;
      }
    //
    // read lookup table. Associate with scalar data.
    //
    else if ( ! strncmp(line, "lookup_table", 12) )
      {
      if ( ! this->ReadLutData(a) ) return 0;
      }
    //
    // read field of data
    //
    else if ( ! strncmp(line, "field", 5) )
      {
      vtkFieldData *f;
      if ( ! (f=this->ReadFieldData(numPts)) ) return 0;
      a->SetFieldData(f);
      f->Delete();
      }
    //
    // maybe bumped into cell data
    //
    else if ( ! strncmp(line, "cell_data", 9) )
      {
      int ncells;
      if (!this->Read(&ncells))
        {
        vtkErrorMacro(<<"Cannot read cell data!");
        return 0;
        }

      this->ReadCellData(ds, ncells);
      }

    else
      {
      vtkErrorMacro(<< "Unsupported point attribute type: " << line 
                    << " for file: " << this->FileName);
      return 0;
      }
    }

  return 1;
}

// General templated function to read data of various types.
template <class T>
static int ReadBinaryData(istream *IS, T *data, int numTuples, int numComp)
{
  char line[256];

  // suck up newline
  IS->getline(line,256);
  IS->read((char *)data, sizeof(T)*numComp*numTuples);
  if (IS->eof())
    {
    vtkGenericWarningMacro(<<"Error reading binary data!");
    return 0;
    }
  return 1;
}

// General templated function to read data of various types.
template <class T>
static int ReadASCIIData(vtkDataReader *self, T *data, int numTuples, int numComp)
{
  int i, j;

  for (i=0; i<numTuples; i++)
    {
    for (j=0; j<numComp; j++)
      {
      if ( !self->Read(data++) )
        {
        vtkGenericWarningMacro(<<"Error reading ascii data!");
        return 0;
        }
      }
    }
  return 1;
}

// Decription:
// Read data array. Return pointer to array object if successful read; 
// otherwise return NULL. Note: this method instantiates a reference counted
// object with initial count of one; proper protocol is for you to assign 
// the data object and then invoke Delete() it to restore proper reference
// count.
vtkDataArray *vtkDataReader::ReadArray(char *dataType, int numTuples, int numComp)
{
  char *type=this->LowerCase(dataType);
  vtkDataArray *array;

  if ( ! strncmp(type, "bit", 3) )
    {
    array = new vtkBitArray(numComp);
    unsigned char *ptr=((vtkBitArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      char line[256];
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned char)*(numTuples*numComp+7)/8);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary bit array!");
        return NULL;
        }
      }
    else 
      {
      int b;
      for (int i=0; i<numTuples; i++)
        {

        for (int j=0; j<numComp; j++)
          {
          if ( !this->Read(&b) )
            {
            vtkErrorMacro(<<"Error reading binary bit array!");
            return NULL;
            }
          else
            {
            ((vtkBitArray *)array)->SetValue(i*numComp+j,b);
            }
          }
        }
      }
    }
  
  else if ( ! strncmp(type, "char", 4) )
    {
    array = new vtkCharArray(numComp);
    char *ptr = ((vtkCharArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "unsigned_char", 13) )
    {
    array = new vtkUnsignedCharArray(numComp);
    unsigned char *ptr = ((vtkUnsignedCharArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "short", 5) )
    {
    array = new vtkShortArray(numComp);
    short *ptr = ((vtkShortArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap2BERange(ptr,numTuples*numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "unsigned_short", 14) )
    {
    array = new vtkUnsignedShortArray(numComp);
    unsigned short *ptr = ((vtkUnsignedShortArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap2BERange((short *)ptr,numTuples*numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "int", 3) )
    {
    array = new vtkIntArray(numComp);
    int *ptr = ((vtkIntArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange(ptr,numTuples*numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "unsigned_int", 12) )
    {
    array = new vtkUnsignedIntArray(numComp);
    unsigned int *ptr = ((vtkUnsignedIntArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange((int *)ptr,numTuples*numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "long", 4) )
    {
    array = new vtkLongArray(numComp);
    long *ptr = ((vtkLongArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange((int *)ptr,numTuples*numComp);
      }

    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "unsigned_long", 13) )
    {
    array = new vtkUnsignedLongArray(numComp);
    unsigned long *ptr = ((vtkUnsignedLongArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange((int *)ptr,numTuples*numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "float", 5) )
    {
    array = new vtkFloatArray(numComp);
    float *ptr = ((vtkFloatArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange(ptr,numTuples*numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "double", 6) )
    {
    array = new vtkDoubleArray(numComp);
    double *ptr = ((vtkDoubleArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      ReadBinaryData(this->IS, ptr, numTuples, numComp);
      //      vtkByteSwap::Swap4BERange(ptr,numTuples*numComp);
      }
    else 
      {
      ReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else 
    {
    vtkErrorMacro(<< "Unsupported data type: " << type);
    return NULL;
    }

  return array;
}

// Description:
// Read point coordinates. Return 0 if error.
int vtkDataReader::ReadPoints(vtkPointSet *ps, int numPts)
{
  char line[256];
  vtkDataArray *data;

  if (!this->ReadString(line)) 
    {
    vtkErrorMacro(<<"Cannot read points type!" << " for file: " << this->FileName);
    return 0;
    }

  data = this->ReadArray(line, numPts, 3);
  if ( data != NULL )
    {
    vtkPoints *points=vtkPoints::New();
    points->SetData(data);
    data->Delete();
    ps->SetPoints(points);
    points->Delete();
    }
  else
    {
    return 0;
    }

  vtkDebugMacro(<<"Read " << ps->GetNumberOfPoints() << " points");
  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

// Description:
// Read the coordinates for a rectilinear grid. The axes parameter specifies
// which coordinate axes (0,1,2) is being read.
int vtkDataReader::ReadCoordinates(vtkRectilinearGrid *rg, int axes, 
                                   int numCoords)
{
  char line[256];
  vtkScalars *coords;
  vtkDataArray *data;

  if (!this->ReadString(line)) 
    {
    vtkErrorMacro(<<"Cannot read coordinates type!" << " for file: " 
                  << this->FileName);
    return 0;
    }

  data = this->ReadArray(line, numCoords, 1);
  if ( data != NULL )
    {
    coords=vtkScalars::New();
    coords->SetData(data);
    data->Delete();
    }
  else
    {
    return 0;
    }

  if ( axes == 0 ) rg->SetXCoordinates(coords);
  else if ( axes == 1 ) rg->SetYCoordinates(coords);
  else rg->SetZCoordinates(coords);


  coords->Delete();

  vtkDebugMacro(<<"Read " << coords->GetNumberOfScalars() << " coordinates");
  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

// Description:
// Read scalar point attributes. Return 0 if error.
int vtkDataReader::ReadScalarData(vtkDataSetAttributes *a, int numPts)
{
  char line[256], name[256], key[256], tableName[256];
  int skipScalar=0;
  vtkDataArray *data;

  if (!(this->ReadString(name) && this->ReadString(line) && 
        this->ReadString(key) && this->ReadString(tableName)))
    {
    vtkErrorMacro(<<"Cannot read scalar header!" << " for file: " 
                  << this->FileName);
    return 0;
    }

  if (strcmp(this->LowerCase(key), "lookup_table"))
    {
    vtkErrorMacro(<<"Lookup table must be specified with scalar.\n" <<
    "Use \"LOOKUP_TABLE default\" to use default table.");
    return 0;
    }

  //
  // See whether scalar has been already read or scalar name (if specified) 
  // matches name in file. 
  //
  if ( a->GetScalars() != NULL || (this->ScalarsName && strcmp(name,this->ScalarsName)) )
    skipScalar = 1;
  else
    this->SetScalarLut(tableName); //may be "default"

  // Read the data
  data = this->ReadArray(line, numPts, 1);
  if ( data != NULL )
    {
    vtkScalars *scalars=vtkScalars::New();
    scalars->SetData(data);
    data->Delete();
    if ( ! skipScalar ) a->SetScalars(scalars);
    scalars->Delete();
    }
  else
    {
    return 0;
    }
  
  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

// Description:
// Read vector point attributes. Return 0 if error.
int vtkDataReader::ReadVectorData(vtkDataSetAttributes *a, int numPts)
{
  int skipVector=0;
  char line[256], name[256];
  vtkDataArray *data;

  if (!(this->ReadString(name) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read vector data!" << " for file: " << this->FileName);
    return 0;
    }

  //
  // See whether vector has been already read or vector name (if specified) 
  // matches name in file. 
  //
  if ( a->GetVectors() != NULL || (this->VectorsName && strcmp(name,this->VectorsName)) )
    {
    skipVector = 1;
    }

  data = this->ReadArray(line, numPts, 3);
  if ( data != NULL )
    {
    vtkVectors *vectors=vtkVectors::New();
    vectors->SetData(data);
    data->Delete();
    if ( ! skipVector ) a->SetVectors(vectors);
    vectors->Delete();
    }
  else
    {
    return 0;
    }

  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

// Description:
// Read normal point attributes. Return 0 if error.
int vtkDataReader::ReadNormalData(vtkDataSetAttributes *a, int numPts)
{
  int skipNormal=0;
  char line[256], name[256];
  vtkDataArray *data;

  if (!(this->ReadString(name) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read normal data!" << " for file: " << this->FileName);
    return 0;

    }

  //
  // See whether normal has been already read or normal name (if specified) 
  // matches name in file. 
  //
  if ( a->GetNormals() != NULL || (this->NormalsName && strcmp(name,this->NormalsName)) )
    {
    skipNormal = 1;
    }

  data = this->ReadArray(line, numPts, 3);
  if ( data != NULL )
    {
    vtkNormals *normals=vtkNormals::New();
    normals->SetData(data);
    data->Delete();
    if ( ! skipNormal ) a->SetNormals(normals);
    normals->Delete();
    }
  else
    {
    return 0;
    }

  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

// Description:
// Read tensor point attributes. Return 0 if error.
int vtkDataReader::ReadTensorData(vtkDataSetAttributes *a, int numPts)
{
  int skipTensor=0;
  char line[256], name[256];
  vtkDataArray *data;

  if (!(this->ReadString(name) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read tensor data!" << " for file: " << this->FileName);
    return 0;
    }
  //
  // See whether tensor has been already read or tensor name (if specified) 
  // matches name in file. 
  //
  if ( a->GetTensors() != NULL || (this->TensorsName && strcmp(name,this->TensorsName)) )
    {
    skipTensor = 1;
    }

  data = this->ReadArray(line, numPts, 9);
  if ( data != NULL )
    {
    vtkTensors *tensors=vtkTensors::New();
    tensors->SetData(data);
    data->Delete();
    if ( ! skipTensor ) a->SetTensors(tensors);
    tensors->Delete();
    }
  else
    {
    return 0;
    }

  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

// Description:
// Read color scalar point attributes. Return 0 if error.
int vtkDataReader::ReadCoScalarData(vtkDataSetAttributes *a, int numPts)
{
  int i, j, idx, numComp, skipScalar=0;
  char name[256];
  vtkFloatArray *data;

  if (!(this->ReadString(name) && this->Read(&numComp)))
    {
    vtkErrorMacro(<<"Cannot read color scalar data!" << " for file: " 
                  << this->FileName);
    return 0;
    }
  //
  // See whether scalar has been already read or scalar name (if specified) 
  // matches name in file. 
  //
  if ( a->GetScalars() != NULL ||
  (this->ScalarsName && strcmp(name,this->ScalarsName)) )
    {
    skipScalar = 1;
    }

  char type[6] = "float";
  data = (vtkFloatArray *)this->ReadArray(type, numPts, numComp);
  if ( data != NULL )
    {
    if ( ! skipScalar ) 
      {
      vtkScalars *scalars=vtkScalars::New(VTK_UNSIGNED_CHAR,numComp);
      vtkUnsignedCharArray *ucharData=(vtkUnsignedCharArray *)scalars->GetData();
      ucharData->SetNumberOfTuples(numPts);
      for (i=0; i<numPts; i++)
        {
        for (j=0; j<numComp; j++)
          {
          idx = i*numComp + j;
          ucharData->SetValue(idx, (unsigned char) (255.0*data->GetValue(idx)));
          }
        }
      a->SetScalars(scalars);
      scalars->Delete();
      }
    data->Delete();
    }
  else
    {
    return 0;
    }

  if ( this->Source )
    {

    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

// Description:
// Read texture coordinates point attributes. Return 0 if error.
int vtkDataReader::ReadTCoordsData(vtkDataSetAttributes *a, int numPts)
{
  int dim;
  int skipTCoord = 0;
  char line[256], name[256];
  vtkDataArray *data;

  if (!(this->ReadString(name) && this->Read(&dim) && 
        this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read texture data!" << " for file: " << this->FileName);
    return 0;
    }

  if ( dim < 1 || dim > 3 )
    {
    vtkErrorMacro(<< "Unsupported texture coordinates dimension: " << dim 
                  << " for file: " << this->FileName);
    return 0;
    }

  //
  // See whether texture coords have been already read or texture coords name
  // (if specified) matches name in file. 
  //
  if ( a->GetTCoords() != NULL || 
  (this->TCoordsName && strcmp(name,this->TCoordsName)) )
    {
    skipTCoord = 1;
    }

  data = this->ReadArray(line, numPts, dim);
  if ( data != NULL )
    {
    vtkTCoords *tcoords=vtkTCoords::New();
    tcoords->SetData(data);
    data->Delete();
    if ( ! skipTCoord ) a->SetTCoords(tcoords);
    tcoords->Delete();
    }
  else
    {
    return 0;
    }

  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

// Description:
// Read lookup table. Return 0 if error.
int vtkDataReader::ReadLutData(vtkDataSetAttributes *a)
{
  int i;
  int size, skipTable=0;
  vtkLookupTable *lut;
  unsigned char *ptr;
  char line[256], name[256];

  if (!(this->ReadString(name) && this->Read(&size)))
    {
    vtkErrorMacro(<<"Cannot read lookup table data!" << " for file: " 
                  << this->FileName);
    return 0;
    }

  if ( a->GetScalars() == NULL ||
  (this->LookupTableName && strcmp(name,this->LookupTableName)) ||
  (this->ScalarLut && strcmp(name,this->ScalarLut)) )
    {
    skipTable = 1;
    }

  lut = vtkLookupTable::New();
  lut->Allocate(size);
  ptr = lut->WritePointer(0,size);

  if ( this->FileType == VTK_BINARY)
    {
    // suck up newline
    this->IS->getline(line,256);
    this->IS->read((char *)ptr,sizeof(unsigned char)*4*size);
    if (this->IS->eof())
      {
      vtkErrorMacro(<<"Error reading binary lookup table!" << " for file: " 
                    << this->FileName);
      return 0;
      }
    }
  else // ascii
    {
    float rgba[4];
    for (i=0; i<size; i++)
      {
      if (!(this->Read(rgba) && this->Read(rgba+1) &&
            this->Read(rgba+2) && this->Read(rgba+3)))
        {
        vtkErrorMacro(<<"Error reading lookup table!" << " for file: " 
                      << this->FileName);
        return 0;
        }
      lut->SetTableValue(i, rgba[0], rgba[1], rgba[2], rgba[3]);
      }
    }

  if ( ! skipTable ) a->GetScalars()->SetLookupTable(lut);
  lut->Delete();

  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }


  return 1;
}


// Description:
// Read lookup table. Return 0 if error.
int vtkDataReader::ReadCells(int size, int *data)
{
  char line[256];
  int i;

  if ( this->FileType == VTK_BINARY)
    {
    // suck up newline
    this->IS->getline(line,256);
    this->IS->read((char *)data,sizeof(int)*size);
    if (this->IS->eof())
      {
      vtkErrorMacro(<<"Error reading binary cell data!" << " for file: " 
                    << this->FileName);
      return 0;
      }
    vtkByteSwap::Swap4BERange(data,size);
    }
  else // ascii
    {
    for (i=0; i<size; i++)
      {
      if (!this->Read(data+i))
        {
        vtkErrorMacro(<<"Error reading ascii cell data!" << " for file: " 
                      << this->FileName);
        return 0;
        }
      }
    }

  if ( this->Source )
    {
    float progress = this->Source->GetProgress();
    this->Source->UpdateProgress(progress + 0.5*(1.0 - progress));
    }

  return 1;
}

vtkFieldData *vtkDataReader::ReadFieldData(int num)
{
  int i, numArrays, skipField=0;
  vtkFieldData *f;
  char name[256], type[256];
  int numComp, numTuples;
  vtkDataArray *data;

  if ( !(this->ReadString(name) && this->Read(&numArrays)) )
    {
    vtkErrorMacro(<<"Cannot read field header!" << " for file: " 
                  << this->FileName);
    return NULL;
    }
  
  // See whether field data name (if specified) 
  if ( (this->FieldDataName && strcmp(name,this->FieldDataName)) )
    {
    skipField = 1;
    }

  f = vtkFieldData::New();
  f->SetNumberOfArrays(numArrays);
  
  // Read the number of arrays specified
  for (i=0; i<numArrays; i++)
    {
    this->ReadString(name);
    this->Read(&numComp);
    this->Read(&numTuples);
    this->ReadString(type);
    data = this->ReadArray(type, numTuples, numComp);
    if ( data != NULL )
      {
      if ( ! skipField )
        {
        f->SetArray(i,data);
        f->SetArrayName(i,name);
        }
      data->Delete();
      }
    else
      {
      f->Delete();
      return NULL;
      }
    }

  return f;
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
void vtkDataReader::CloseVTKFile()
{
  vtkDebugMacro(<<"Closing vtk file");
  if ( this->IS != NULL ) delete this->IS;
  this->IS = NULL;
}

void vtkDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";

  if ( this->FileType == VTK_BINARY )
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

  if ( this->FieldDataName )
    os << indent << "Field Data Name: " << this->FieldDataName << "\n";
  else
    os << indent << "Field Data Name: (None)\n";

}
