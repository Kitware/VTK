/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataReader.h"

#include "vtkBitArray.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkShortArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include <ctype.h>
#include <sys/stat.h>

vtkCxxRevisionMacro(vtkDataReader, "1.134");
vtkStandardNewMacro(vtkDataReader);

vtkCxxSetObjectMacro(vtkDataReader, InputArray, vtkCharArray);

// this undef is required on the hp. vtkMutexLock ends up including
// /usr/inclue/dce/cma_ux.h which has the gall to #define read as cma_read

#ifdef read
#undef read
#endif

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
  this->InputStringLength = 0;
  this->InputStringPos = 0;
  this->ReadFromInputString = 0;
  this->IS = NULL;
  this->Header = NULL;

  this->InputArray = 0;

  this->NumberOfScalarsInFile = 0;
  this->ScalarsNameInFile = NULL;
  this->ScalarsNameAllocSize = 0;
  this->NumberOfVectorsInFile = 0;
  this->VectorsNameInFile = NULL;
  this->VectorsNameAllocSize = 0;
  this->NumberOfTensorsInFile = 0;
  this->TensorsNameInFile = NULL;
  this->TensorsNameAllocSize = 0;
  this->NumberOfTCoordsInFile = 0;
  this->TCoordsNameInFile = NULL;
  this->TCoordsNameAllocSize = 0;
  this->NumberOfNormalsInFile = 0;
  this->NormalsNameInFile = NULL;
  this->NormalsNameAllocSize = 0;
  this->NumberOfFieldDataInFile = 0;
  this->FieldDataNameInFile = NULL;
  this->FieldDataNameAllocSize = 0;

  this->ReadAllScalars = 0;
  this->ReadAllVectors = 0;
  this->ReadAllNormals = 0;
  this->ReadAllTensors = 0;
  this->ReadAllColorScalars = 0;
  this->ReadAllTCoords = 0;
  this->ReadAllFields = 0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}  

vtkDataReader::~vtkDataReader()
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  if (this->ScalarsName)
    {
    delete [] this->ScalarsName;
    }
  if (this->VectorsName)
    {
    delete [] this->VectorsName;
    }
  if (this->TensorsName)
    {
    delete [] this->TensorsName;
    }
  if (this->NormalsName)
    {
    delete [] this->NormalsName;
    }
  if (this->TCoordsName)
    {
    delete [] this->TCoordsName;
    }
  if (this->LookupTableName)
    {
    delete [] this->LookupTableName;
    }
  if (this->FieldDataName)
    {
    delete [] this->FieldDataName;
    }
  if (this->ScalarLut)
    {
    delete [] this->ScalarLut;
    }
  if (this->InputString)
    {
    delete [] this->InputString;
    }
  if (this->Header)
    {
    delete [] this->Header;
    }

  this->SetInputArray(0);
  this->InitializeCharacteristics();
  if ( this->IS )
    {
    delete this->IS;
    }
}

void vtkDataReader::SetInputString(const char *in)
{ 
  if (in != NULL)
    {
    this->SetInputString(in, static_cast<int>(strlen(in)));
    }
  else
    {
    if (this->InputString)
      {
      delete [] this->InputString; 
      }
    this->InputString = NULL;
    }
}

void vtkDataReader::SetBinaryInputString(const char *in, int len)
{
    this->SetInputString(in,len);
}

void vtkDataReader::SetInputString(const char *in, int len)
{ 
  if (this->Debug)
    {
    vtkDebugMacro(<< "setting InputString to " << in );
    }

  if (this->InputString && in && strncmp(in, this->InputString, len) == 0)
    {
    return;
    }
  
  if (this->InputString)
    {
    delete [] this->InputString; 
    }
  if (in) 
    { 
    this->InputString = new char[len]; 
    memcpy(this->InputString,in,len); 
    this->InputStringLength = len;
    } 
   else 
    { 
    this->InputString = NULL; 
    this->InputStringLength = 0;
    } 
  this->Modified(); 
} 

// Internal function to read in a line up to 256 characters.
// Returns zero if there was an error.
int vtkDataReader::ReadLine(char result[256])
{
  this->IS->getline(result,256);
  if (this->IS->fail())
    {
    if (this->IS->eof()) 
      {
      return 0;
      }
    if (this->IS->gcount() == 255)
      {
      // Read 256 chars; ignoring the rest of the line.
      this->IS->clear();
      this->IS->ignore(VTK_INT_MAX, '\n');
      }
    }
  return 1;
}

// Internal function to read in a string up to 256 characters.
// Returns zero if there was an error.
int vtkDataReader::ReadString(char result[256])
{
  this->IS->width(256);
  *this->IS >> result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

// Internal function to read in an integer value.
// Returns zero if there was an error.
int vtkDataReader::Read(char *result)
{
  int intData;
  *this->IS >> intData;
  if (this->IS->fail())
    {
    return 0;
    }

  *result = (char) intData;
  return 1;
}

int vtkDataReader::Read(unsigned char *result)
{
  int intData;
  *this->IS >> intData;
  if (this->IS->fail())
    {
    return 0;
    }

  *result = (unsigned char) intData;
  return 1;
}

int vtkDataReader::Read(short *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(unsigned short *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(int *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(unsigned int *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(long *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(unsigned long *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(float *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(double *result)
{
  *this->IS >> *result;
  if (this->IS->fail()) 
    {
    return 0;
    }
  return 1;
}


// Open a vtk data file. Returns zero if error.
int vtkDataReader::OpenVTKFile()
{
  if (this->ReadFromInputString)
    {
    if (this->InputArray)
      {
      vtkDebugMacro(<< "Reading from InputArray");
      this->IS = new istrstream(this->InputArray->GetPointer(0), 
                                this->InputArray->GetNumberOfTuples()*
        this->InputArray->GetNumberOfComponents());
      return 1;
      }
    else if (this->InputString)
      {
      vtkDebugMacro(<< "Reading from InputString");
      this->IS = new istrstream(this->InputString, this->InputStringLength);
      return 1;
      }
    }
  else
    {
    vtkDebugMacro(<< "Opening vtk file");

    if ( !this->FileName || (strlen(this->FileName) == 0))
      {
      vtkErrorMacro(<< "No file specified!");
      this->SetErrorCode( vtkErrorCode::NoFileNameError );
      return 0;
      }

    // first make sure the file exists, this prevents an empty file from
    // being created on older compilers
    struct stat fs;
    if (stat(this->FileName, &fs) != 0) 
      {
      vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
      this->SetErrorCode( vtkErrorCode::CannotOpenFileError );
      return 0;
      }
    this->IS = new ifstream(this->FileName, ios::in);
    if (this->IS->fail())
      {
      vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
      delete this->IS;
      this->IS = NULL;
      this->SetErrorCode( vtkErrorCode::CannotOpenFileError );
      return 0;
      }
    return 1;
    }

  return 0;
}

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
                  << (this->FileName?this->FileName:"(Null FileName)"));
    this->SetErrorCode( vtkErrorCode::PrematureEndOfFileError );
    return 0;
    }
  if ( strncmp ("# vtk DataFile Version", line, 20) )
    {
    vtkErrorMacro(<< "Unrecognized file type: "<< line << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    
    this->SetErrorCode( vtkErrorCode::UnrecognizedFileTypeError );
    return 0;
    }
  //
  // read title
  //
  if (!this->ReadLine(line))
    {
    vtkErrorMacro(<<"Premature EOF reading title! " << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    this->SetErrorCode( vtkErrorCode::PrematureEndOfFileError );
    return 0;
    }
  if (this->Header)
    {
    delete [] this->Header;
    }
  this->Header = new char[strlen(line) + 1];
  strcpy (this->Header, line);

  vtkDebugMacro(<< "Reading vtk file entitled: " << line);
  //
  // read type
  //
  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Premature EOF reading file type!" << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    this->SetErrorCode( vtkErrorCode::PrematureEndOfFileError );
    return 0;
    }

  if ( !strncmp(this->LowerCase(line), "ascii", 5) )
    {
    this->FileType = VTK_ASCII;
    }
  else if ( !strncmp(line, "binary", 6) )
    {
    this->FileType = VTK_BINARY;
    }
  else
    {
    vtkErrorMacro(<< "Unrecognized file type: "<< line << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    this->FileType = 0;
    this->SetErrorCode( vtkErrorCode::UnrecognizedFileTypeError );
    return 0;
    }

  // if this is a binary file we need to make sure that we opened it 
  // as a binary file.
  if (this->FileType == VTK_BINARY && this->ReadFromInputString == 0)
    {
    vtkDebugMacro(<< "Opening vtk file as binary");
    delete this->IS;
    this->IS = 0;
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
      this->SetErrorCode( vtkErrorCode::CannotOpenFileError );
      return 0;
      }
    // read up to the same point in the file
    this->ReadLine(line);
    this->ReadLine(line);
    this->ReadString(line);
    }

  float progress=this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));
  
  return 1;
}

int vtkDataReader::IsFileValid(const char *dstype)
{
  char line[1024];
  
  if (!dstype)
    {
    return 0;
    }
  
  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return 0;
    }

  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Data file ends prematurely!");
    this->CloseVTKFile ();
    this->SetErrorCode( vtkErrorCode::PrematureEndOfFileError );
    return 0;
    }

  if ( !strncmp(this->LowerCase(line),"dataset",(unsigned long)7) )
    {
    if (!this->ReadString(line))
      {
      vtkErrorMacro(<<"Data file ends prematurely!");
      this->CloseVTKFile ();
      this->SetErrorCode( vtkErrorCode::PrematureEndOfFileError );
      return 0;
      } 
    if (strncmp(this->LowerCase(line),dstype,strlen(dstype)))
      {
      this->CloseVTKFile ();
      return 0;
      }
    // everything looks good
    this->CloseVTKFile();
    return 1;
    }
  
  return 0;
}

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
      if ( ! this->ReadScalarData(a, numCells) )
        {
        return 0;
        }
      }
    //
    // read vector data
    //
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(a, numCells) )
        {
        return 0;
        }
      }
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(a, numCells) )
        {
        return 0;
        }
      }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
      {
      if ( ! this->ReadNormalData(a, numCells) )
        {
        return 0;
        }
      }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(a, numCells) )
        {
        return 0;
        }
      }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(a, numCells) )
        {
        return 0;
        }
      }
    //
    // read lookup table. Associate with scalar data.
    //
    else if ( ! strncmp(line, "lookup_table", 12) )
      {
      if ( ! this->ReadLutData(a) )
        {
        return 0;
        }
      }
    //
    // read field of data
    //
    else if ( ! strncmp(line, "field", 5) )
      {
      vtkFieldData *f;
      if ( ! (f=this->ReadFieldData()) )
        {
        return 0;
        }
      for(int i=0; i<f->GetNumberOfArrays(); i++)
        {
        a->AddArray(f->GetArray(i));
        }
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
                    << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
      return 0;
      }
    }

  return 1;
}


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
      if ( ! this->ReadScalarData(a, numPts) )
        {
        return 0;
        }
      }
    //
    // read vector data
    //
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(a, numPts) )
        {
        return 0;
        }
      }
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(a, numPts) )
        {
        return 0;
        }
      }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
      {

      if ( ! this->ReadNormalData(a, numPts) )
        {
        return 0;
        }
      }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(a, numPts) )
        {
        return 0;
        }
      }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(a, numPts) )
        {
        return 0;
        }
      }
    //
    // read lookup table. Associate with scalar data.
    //
    else if ( ! strncmp(line, "lookup_table", 12) )
      {
      if ( ! this->ReadLutData(a) )
        {
        return 0;
        }
      }
    //
    // read field of data
    //
    else if ( ! strncmp(line, "field", 5) )
      {
      vtkFieldData *f;
      if ( ! (f=this->ReadFieldData()) )
        {
        return 0;
        }
      for(int i=0; i<f->GetNumberOfArrays(); i++)
        {
        a->AddArray(f->GetArray(i));
        }
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
                    << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
      return 0;
      }
    }

  return 1;
}

// General templated function to read data of various types.
template <class T>
int vtkReadBinaryData(istream *IS, T *data, int numTuples, int numComp)
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
int vtkReadASCIIData(vtkDataReader *self, T *data, int numTuples, int numComp)
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
vtkDataArray *vtkDataReader::ReadArray(const char *dataType, int numTuples, int numComp)
{
  char *type=strdup(dataType);
  type=this->LowerCase(type);

  vtkDataArray *array;

  if ( ! strncmp(type, "bit", 3) )
    {
    array = vtkBitArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned char *ptr=((vtkBitArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      char line[256];
      this->IS->getline(line,256);
      this->IS->read((char *)ptr,sizeof(unsigned char)*(numTuples*numComp+7)/8);
      if (this->IS->eof())
        {
        vtkErrorMacro(<<"Error reading binary bit array!");
        free(type);
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
              vtkErrorMacro(<<"Error reading ascii bit array! tuple: " << i << ", component: " << j);
              free(type);
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
    array = vtkCharArray::New();
    array->SetNumberOfComponents(numComp);
    char *ptr = ((vtkCharArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "unsigned_char", 13) )
    {
    array = vtkUnsignedCharArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned char *ptr = ((vtkUnsignedCharArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "short", 5) )
    {
    array = vtkShortArray::New();
    array->SetNumberOfComponents(numComp);
    short *ptr = ((vtkShortArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap2BERange(ptr,numTuples*numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "unsigned_short", 14) )
    {
    array = vtkUnsignedShortArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned short *ptr = ((vtkUnsignedShortArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap2BERange((short *)ptr,numTuples*numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "int", 3) )
    {
    array = vtkIntArray::New();
    array->SetNumberOfComponents(numComp);
    int *ptr = ((vtkIntArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange(ptr,numTuples*numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "unsigned_int", 12) )
    {
    array = vtkUnsignedIntArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned int *ptr = ((vtkUnsignedIntArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange((int *)ptr,numTuples*numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "long", 4) )
    {
    array = vtkLongArray::New();
    array->SetNumberOfComponents(numComp);
    long *ptr = ((vtkLongArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange((int *)ptr,numTuples*numComp);
      }

    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "unsigned_long", 13) )
    {
    array = vtkUnsignedLongArray::New();
    array->SetNumberOfComponents(numComp);
    unsigned long *ptr = ((vtkUnsignedLongArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange((int *)ptr,numTuples*numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "float", 5) )
    {
    array = vtkFloatArray::New();
    array->SetNumberOfComponents(numComp);
    float *ptr = ((vtkFloatArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange(ptr,numTuples*numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else if ( ! strncmp(type, "double", 6) )
    {
    array = vtkDoubleArray::New();
    array->SetNumberOfComponents(numComp);
    double *ptr = ((vtkDoubleArray *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap8BERange(ptr,numTuples*numComp);
      }
    else 
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    }
  
  else 
    {
    vtkErrorMacro(<< "Unsupported data type: " << type);
    free(type);
    return NULL;
    }

  free(type);
  return array;
}

// Read point coordinates. Return 0 if error.
int vtkDataReader::ReadPoints(vtkPointSet *ps, int numPts)
{
  char line[256];
  vtkDataArray *data;

  if (!this->ReadString(line)) 
    {
    vtkErrorMacro(<<"Cannot read points type!" << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
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
  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

// Read the coordinates for a rectilinear grid. The axes parameter specifies
// which coordinate axes (0,1,2) is being read.
int vtkDataReader::ReadCoordinates(vtkRectilinearGrid *rg, int axes, 
                                   int numCoords)
{
  char line[256];
  vtkDataArray *data;

  if (!this->ReadString(line)) 
    {
    vtkErrorMacro(<<"Cannot read coordinates type!" << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }

  data = this->ReadArray(line, numCoords, 1);
  if ( !data  )
    {
    return 0;
    }

  if ( axes == 0 )
    {
    rg->SetXCoordinates(data);
    }
  else if ( axes == 1 )
    {
    rg->SetYCoordinates(data);
    }
  else
    {
    rg->SetZCoordinates(data);
    }

  vtkDebugMacro(<<"Read " << data->GetNumberOfTuples() << " coordinates");
  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  data->Delete();

  return 1;
}

// Read scalar point attributes. Return 0 if error.
int vtkDataReader::ReadScalarData(vtkDataSetAttributes *a, int numPts)
{
  char line[256], name[256], key[256], tableName[256];
  int skipScalar=0;
  vtkDataArray *data;
  int numComp = 1;
  char buffer[1024];
  
  if (!(this->ReadString(buffer) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read scalar header!" << " for file: " 
    << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }

  this->DecodeArrayName(name, buffer);

  if (!this->ReadString(key))
    {
    vtkErrorMacro(<<"Cannot read scalar header!" << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }

  // the next string could be an integer number of components or a lookup table
  if (strcmp(this->LowerCase(key), "lookup_table"))
    {
    numComp = atoi(key);
    if (numComp < 1 || !this->ReadString(key))
      {
      vtkErrorMacro(<<"Cannot read scalar header!" << " for file: " 
      << (this->FileName?this->FileName:"(Null FileName)"));
      return 0;
      }
    }
  
  if (strcmp(this->LowerCase(key), "lookup_table"))
    {
    vtkErrorMacro(<<"Lookup table must be specified with scalar.\n" <<
    "Use \"LOOKUP_TABLE default\" to use default table.");
    return 0;
    }

  if (!this->ReadString(tableName))
    {
    vtkErrorMacro(<<"Cannot read scalar header!" << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }

  // See whether scalar has been already read or scalar name (if specified) 
  // matches name in file. 
  //
  if ( a->GetScalars() != NULL || (this->ScalarsName && strcmp(name,this->ScalarsName)) )
    {
    skipScalar = 1;
    }
  else
    {
    this->SetScalarLut(tableName); //may be "default"
    }

  // Read the data
  data = this->ReadArray(line, numPts, numComp);
  if ( data != NULL )
    {
    data->SetName(name);
    if ( ! skipScalar )
      {
      a->SetScalars(data);
      }
    else if ( this->ReadAllScalars ) 
      {
      a->AddArray(data);
      }    data->Delete();
    }
  else
    {
    return 0;
    }
  
  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

// Read vector point attributes. Return 0 if error.
int vtkDataReader::ReadVectorData(vtkDataSetAttributes *a, int numPts)
{
  int skipVector=0;
  char line[256], name[256];
  vtkDataArray *data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read vector data!" << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }
  this->DecodeArrayName(name, buffer);

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
    data->SetName(name);
    if ( ! skipVector )
      {
      a->SetVectors(data);
      }
    else if ( this->ReadAllVectors )
      {
      a->AddArray(data);
      }
    data->Delete();
    }
  else
    {
    return 0;
    }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

// Read normal point attributes. Return 0 if error.
int vtkDataReader::ReadNormalData(vtkDataSetAttributes *a, int numPts)
{
  int skipNormal=0;
  char line[256], name[256];
  vtkDataArray *data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read normal data!" << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }
  this->DecodeArrayName(name, buffer);

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
    data->SetName(name);
    if ( ! skipNormal )
      {
      a->SetNormals(data);
      }
    else if ( this->ReadAllNormals )
      {
      a->AddArray(data);
      }
    data->Delete();
    }
  else
    {
    return 0;
    }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

// Read tensor point attributes. Return 0 if error.
int vtkDataReader::ReadTensorData(vtkDataSetAttributes *a, int numPts)
{
  int skipTensor=0;
  char line[256], name[256];
  vtkDataArray *data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read tensor data!" << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }
  this->DecodeArrayName(name, buffer);
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
    data->SetName(name);
    if ( ! skipTensor )
      {
      a->SetTensors(data);
      }
    else if ( this->ReadAllTensors )
      {
      a->AddArray(data);
      }
    data->Delete();
    }
  else
    {
    return 0;
    }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

// Read color scalar point attributes. Return 0 if error.
int vtkDataReader::ReadCoScalarData(vtkDataSetAttributes *a, int numPts)
{
  int i, j, idx, numComp, skipScalar=0;
  char name[256];
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->Read(&numComp)))
    {
    vtkErrorMacro(<<"Cannot read color scalar data!" << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }
  this->DecodeArrayName(name, buffer);
  //
  // See whether scalar has been already read or scalar name (if specified) 
  // matches name in file. 
  //
  if ( a->GetScalars() != NULL ||
  (this->ScalarsName && strcmp(name,this->ScalarsName)) )
    {
    skipScalar = 1;
    }

  // handle binary different from ASCII since they are stored
  // in a different format float versus uchar
  if ( this->FileType == VTK_BINARY)
    {
    vtkUnsignedCharArray *data;
    char type[14] = "unsigned_char";
    data = (vtkUnsignedCharArray *)this->ReadArray(type, numPts, numComp);

    if ( data != NULL )
      {
      data->SetName(name);
      if ( ! skipScalar ) 
        {
        a->SetScalars(data);
        }
      else if ( this->ReadAllColorScalars )
        {
        a->AddArray(data);
        }      data->Delete();
      }
    else
      {
        return 0;
      }
    }
  else
    {
    vtkFloatArray *data;
    char type[6] = "float";
    data = (vtkFloatArray *)this->ReadArray(type, numPts, numComp);
    
    if ( data != NULL )
      {
      if ( ! skipScalar || this->ReadAllColorScalars ) 
        {
        vtkUnsignedCharArray *scalars=vtkUnsignedCharArray::New();
        scalars->SetNumberOfComponents(numComp);
        scalars->SetNumberOfTuples(numPts);
        scalars->SetName(name);
        for (i=0; i<numPts; i++)
          {
          for (j=0; j<numComp; j++)
            {
            idx = i*numComp + j;
            scalars->SetValue(idx,(unsigned char)(255.0*data->GetValue(idx)+0.5));
            }
          }
        if ( ! skipScalar )
          {
          a->SetScalars(scalars);
          }
        else if ( this->ReadAllColorScalars )
          {
          a->AddArray(scalars);
          }        scalars->Delete();
        }
      data->Delete();
      }
    else
      {
        return 0;
      }
    }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

// Read texture coordinates point attributes. Return 0 if error.
int vtkDataReader::ReadTCoordsData(vtkDataSetAttributes *a, int numPts)
{
  int dim;
  int skipTCoord = 0;
  char line[256], name[256];
  vtkDataArray *data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->Read(&dim) && 
        this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read texture data!" << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }
  this->DecodeArrayName(name, buffer);

  if ( dim < 1 || dim > 3 )
    {
    vtkErrorMacro(<< "Unsupported texture coordinates dimension: " << dim 
                  << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
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
    data->SetName(name);
    if ( ! skipTCoord )
      {
      a->SetTCoords(data);
      }
    else if ( this->ReadAllTCoords )
      {
      a->AddArray(data);
      }
    data->Delete();
    }
  else
    {
    return 0;
    }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

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
                  << (this->FileName?this->FileName:"(Null FileName)"));
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
                    << (this->FileName?this->FileName:"(Null FileName)"));
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
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return 0;
        }
      lut->SetTableValue(i, rgba[0], rgba[1], rgba[2], rgba[3]);
      }
    }

  if ( ! skipTable )
    {
    a->GetScalars()->SetLookupTable(lut);
    }
  lut->Delete();

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}


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
                    << (this->FileName?this->FileName:"(Null FileName)"));
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
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return 0;
        }
      }
    }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

int vtkDataReader::ReadCells(int size, int *data, 
                             int skip1, int read2, int skip3)
{
  char line[256];
  int i, numCellPts, junk, *tmp, *pTmp;

  if ( this->FileType == VTK_BINARY)
    {
    // suck up newline
    this->IS->getline(line,256);
    // first read all the cells as one chunk (each cell has different length).
    if (skip1 == 0 && skip3 == 0)
      {
      tmp = data;
      }
    else
      {
      tmp = new int[size];
      }
    this->IS->read((char *)tmp,sizeof(int)*size);
    if (this->IS->eof())
      {
      vtkErrorMacro(<<"Error reading binary cell data!" << " for file: " 
                    << (this->FileName?this->FileName:"(Null FileName)"));
      return 0;
      }
    vtkByteSwap::Swap4BERange(tmp,size);
    if (tmp == data)
      {
      return 1;
      }
    // skip cells before the piece
    pTmp = tmp;
    while (skip1 > 0)
      {
      // the first value is the number of point ids
      // skip these plus one for the number itself.
      pTmp += *pTmp + 1;
      --skip1;
      }
    // copy the cells in the piece 
    // (ok, I am getting criptic with the loops and increments ...)
    while (read2 > 0)
      {
      // the first value is the number of point ids
      *data++ = i = *pTmp++;
      while (i-- > 0)
        {
        *data++ = *pTmp++;
        }
      --read2;
      }
    // delete the temporary array
    delete [] tmp;
    }
  else // ascii
    {
    // skip cells before the piece
    for (i=0; i<skip1; i++)
      {
      if (!this->Read(&numCellPts))
        {
        vtkErrorMacro(<<"Error reading ascii cell data!" << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return 0;
        }
      while (numCellPts-- > 0)
        {
        this->Read(&junk);
        }
      }
    // read the cells in the piece
    for (i=0; i<read2; i++)
      {
      if (!this->Read(data))
        {
        vtkErrorMacro(<<"Error reading ascii cell data!" << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return 0;
        }
      numCellPts = *data++;
      while (numCellPts-- > 0)
        {
        this->Read(data++);
        }
      }
    // skip cells after the piece
    for (i=0; i<skip3; i++)
      {
      if (!this->Read(&numCellPts))
        {
        vtkErrorMacro(<<"Error reading ascii cell data!" << " for file: " 
                      << (this->FileName?this->FileName:"(Null FileName)"));
        return 0;
        }
      while (numCellPts-- > 0)
        {
        this->Read(&junk);
        }
      }
    }

  float progress = this->GetProgress();
  this->UpdateProgress(progress + 0.5*(1.0 - progress));

  return 1;
}

vtkFieldData *vtkDataReader::ReadFieldData()
{
  int i, numArrays, skipField=0;
  vtkFieldData *f;
  char name[256], type[256];
  int numComp, numTuples;
  vtkDataArray *data;

  if ( !(this->ReadString(name) && this->Read(&numArrays)) )
    {
    vtkErrorMacro(<<"Cannot read field header!" << " for file: " 
                  << (this->FileName?this->FileName:"(Null FileName)"));
    return NULL;
    }
  
  // See whether field data name (if specified) 
  if ( (this->FieldDataName && strcmp(name,this->FieldDataName)) )
    {
    skipField = 1;
    }

  f = vtkFieldData::New();
  f->AllocateArrays(numArrays);
  
  // Read the number of arrays specified
  for (i=0; i<numArrays; i++)
    {
    char buffer[1024];
    this->ReadString(buffer);    
    this->DecodeArrayName(name, buffer);
    this->Read(&numComp);
    this->Read(&numTuples);
    this->ReadString(type);
    data = this->ReadArray(type, numTuples, numComp);
    if ( data != NULL )
      {
      data->SetName(name);
      if ( ! skipField  || this->ReadAllFields )
        {
        f->AddArray(data);
        }
      data->Delete();
      }
    else
      {
      f->Delete();
      return NULL;
      }
    }

  if ( skipField && ! this->ReadAllFields ) 
    {
    f->Delete();
    return NULL;
    }
  else 
    {
    return f;
    }
}


char *vtkDataReader::LowerCase(char *str, const size_t len)
{
  size_t i;
  char *s;

  for ( i=0, s=str; *s != '\0' && i<len; s++,i++) 
    {
    *s = tolower(*s);
    }
  return str;
}

// Close a vtk file.
void vtkDataReader::CloseVTKFile()
{
  vtkDebugMacro(<<"Closing vtk file");
  if ( this->IS != NULL )
    {
    delete this->IS;
    }
  this->IS = NULL;
}

void vtkDataReader::InitializeCharacteristics()
{
  int i;

  // Release any old stuff first
  if ( this->ScalarsNameInFile )
    {
    for (i=0; i<this->NumberOfScalarsInFile; i++)
      {
      delete [] this->ScalarsNameInFile[i];
      }
    this->NumberOfScalarsInFile = 0;
    delete [] this->ScalarsNameInFile;
    this->ScalarsNameInFile = NULL;
    }

  if ( this->VectorsNameInFile )
    {
    for (i=0; i<this->NumberOfVectorsInFile; i++)
      {
      delete [] this->VectorsNameInFile[i];
      }
    this->NumberOfVectorsInFile = 0;
    delete [] this->VectorsNameInFile;
    this->VectorsNameInFile = NULL;
    }

  if ( this->TensorsNameInFile )
    {
    for (i=0; i<this->NumberOfTensorsInFile; i++)
      {
      delete [] this->TensorsNameInFile[i];
      }
    this->NumberOfTensorsInFile = 0;
    delete [] this->TensorsNameInFile;
    this->TensorsNameInFile = NULL;
    }

  if ( this->NormalsNameInFile )
    {
    for (i=0; i<this->NumberOfNormalsInFile; i++)
      {
      delete [] this->NormalsNameInFile[i];
      }
    this->NumberOfNormalsInFile = 0;
    delete [] this->NormalsNameInFile;
    this->NormalsNameInFile = NULL;
    }

  if ( this->TCoordsNameInFile )
    {
    for (i=0; i<this->NumberOfTCoordsInFile; i++)
      {
      delete [] this->TCoordsNameInFile[i];
      }
    this->NumberOfTCoordsInFile = 0;
    delete [] this->TCoordsNameInFile;
    this->TCoordsNameInFile = NULL;
    }

  if ( this->FieldDataNameInFile )
    {
    for (i=0; i<this->NumberOfFieldDataInFile; i++)
      {
      delete [] this->FieldDataNameInFile[i];
      }
    this->NumberOfFieldDataInFile = 0;
    delete [] this->FieldDataNameInFile;
    this->FieldDataNameInFile = NULL;
    }

}

//read entire file, storing important characteristics
int vtkDataReader::CharacterizeFile()
{
  if ( this->CharacteristicsTime > this->MTime )
    {
    return 1;
    }

  this->InitializeCharacteristics();
  this->CharacteristicsTime.Modified();
  
  // Open the file
  if (!this->OpenVTKFile() || !this->ReadHeader())
    {
    return 0;
    }
  
  char line[256];
  while (this->ReadLine(line))
      {
    this->CheckFor("scalars", line, this->NumberOfScalarsInFile,
                   this->ScalarsNameInFile, this->ScalarsNameAllocSize);
    this->CheckFor("vectors", line, this->NumberOfVectorsInFile,
                   this->VectorsNameInFile, this->VectorsNameAllocSize);
    this->CheckFor("tensors", line, this->NumberOfTensorsInFile,
                   this->TensorsNameInFile, this->TensorsNameAllocSize);
    this->CheckFor("normals", line, this->NumberOfNormalsInFile,
                   this->NormalsNameInFile, this->NormalsNameAllocSize);
    this->CheckFor("tcoords", line, this->NumberOfTCoordsInFile,
                   this->TCoordsNameInFile, this->TCoordsNameAllocSize);
    this->CheckFor("field", line, this->NumberOfFieldDataInFile,
                   this->FieldDataNameInFile, this->FieldDataNameAllocSize);
    }

  this->CloseVTKFile ();
  return 1;
}

void vtkDataReader::CheckFor(const char* name, char *line, int &num, 
                             char** &array, int &allocSize)
{
  if ( !strncmp(this->LowerCase(line, strlen(name)), name, strlen(name)) )
    {
    int i;
    int newAllocSize;
    char **newArray;

    //update numbers
    num++;

    if ( !array )
      {
      allocSize = 25;
      array = new char* [allocSize];
      for (i=0; i<allocSize; i++)
        {
        array[i] = NULL;
        }
      }
    else if ( num >= allocSize )
      {
      newAllocSize = 2*num;
      newArray = new char* [newAllocSize];
      for (i=0; i<allocSize; i++)
        {
        newArray[i] = array[i];
        }
      for (i=allocSize; i<newAllocSize; i++)
        {
        newArray[i] = NULL;
        }
      allocSize = newAllocSize;
      delete [] array;
      array = newArray;
      }

    // enter the name
    char nameOfAttribute[256];
    sscanf(line, "%*s %s", nameOfAttribute);
    if ( nameOfAttribute )
      {
      array[num-1] = new char [strlen(nameOfAttribute)+1];
      strcpy(array[num-1],nameOfAttribute);
      }
    }//found one
}

const char *vtkDataReader::GetScalarsNameInFile(int i)
{
  this->CharacterizeFile();
  if ( !this->ScalarsNameInFile || 
       i < 0 || i >= this->NumberOfScalarsInFile )
    {
    return NULL;
    }
  else
    {
    return this->ScalarsNameInFile[i];
    }
}

const char *vtkDataReader::GetVectorsNameInFile(int i)
{
  this->CharacterizeFile();
  if ( !this->VectorsNameInFile || 
       i < 0 || i >= this->NumberOfVectorsInFile )
    {
    return NULL;
    }
  else
    {
    return this->VectorsNameInFile[i];
    }
}
const char *vtkDataReader::GetTensorsNameInFile(int i)
{
  this->CharacterizeFile();
  if ( !this->TensorsNameInFile || 
       i < 0 || i >= this->NumberOfTensorsInFile )
    {
    return NULL;
    }
  else
    {
    return this->TensorsNameInFile[i];
    }
}
const char *vtkDataReader::GetNormalsNameInFile(int i)
{
  this->CharacterizeFile();
  if ( !this->NormalsNameInFile || 
       i < 0 || i >= this->NumberOfNormalsInFile )
    {
    return NULL;
    }
  else
    {
    return this->NormalsNameInFile[i];
    }
}
const char *vtkDataReader::GetTCoordsNameInFile(int i)
{
  this->CharacterizeFile();
  if ( !this->TCoordsNameInFile || 
       i < 0 || i >= this->NumberOfTCoordsInFile )
    {
    return NULL;
    }
  else
    {
    return this->TCoordsNameInFile[i];
    }
}
const char *vtkDataReader::GetFieldDataNameInFile(int i)
{
  this->CharacterizeFile();
  if ( !this->FieldDataNameInFile || 
       i < 0 || i >= this->NumberOfFieldDataInFile )
    {
    return NULL;
    }
  else
    {
    return this->FieldDataNameInFile[i];
    }
}

int vtkDataReader::ProcessRequest(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

void vtkDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "File Name: " 
     << (this->FileName ? this->FileName : "(none)") << "\n";


  if ( this->FileType == VTK_BINARY )
    {
    os << indent << "File Type: BINARY\n";
    }
  else
    {
    os << indent << "File Type: ASCII\n";
    }

  if ( this->Header )
    {
    os << indent << "Header: " << this->Header << "\n";
    }
  else
    {
    os << indent << "Header: (None)\n";
    }

  os << indent << "ReadFromInputString: " << (this->ReadFromInputString ? "On\n" : "Off\n");
  if ( this->InputString )
    {
    os << indent << "Input String: " << this->InputString << "\n";
    }
  else
    {
    os << indent << "Input String: (None)\n";
    }

  if ( this->InputArray )
    {
    os << indent << "Input Array: "  << "\n";
    this->InputArray->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Input String: (None)\n";
    }

  os << indent << "Input String Length: " << this->InputStringLength << endl;

  if ( this->ScalarsName )
    {
    os << indent << "Scalars Name: " << this->ScalarsName << "\n";
    }
  else
    {
    os << indent << "Scalars Name: (None)\n";
    }
  os << indent << "ReadAllScalars: " 
     << (this->ReadAllScalars ? "On" : "Off") << "\n";

  if ( this->VectorsName )
    {
    os << indent << "Vectors Name: " << this->VectorsName << "\n";
    }
  else
    {
    os << indent << "Vectors Name: (None)\n";
    }
  os << indent << "ReadAllVectors: " 
     << (this->ReadAllVectors ? "On" : "Off") << "\n";

  if ( this->NormalsName )
    {
    os << indent << "Normals Name: " << this->NormalsName << "\n";
    }
  else
    {
    os << indent << "Normals Name: (None)\n";
    }
  os << indent << "ReadAllNormals: " 
     << (this->ReadAllNormals ? "On" : "Off") << "\n";

  if ( this->TensorsName )
    {
    os << indent << "Tensors Name: " << this->TensorsName << "\n";
    }
  else
    {
    os << indent << "Tensors Name: (None)\n";
    }
  os << indent << "ReadAllTensors: " 
     << (this->ReadAllTensors ? "On" : "Off") << "\n";

  if ( this->TCoordsName )
    {
    os << indent << "Texture Coords Name: " << this->TCoordsName << "\n";
    }
  else
    {
    os << indent << "Texture Coordinates Name: (None)\n";
    }
  os << indent << "ReadAllTCoords: " 
     << (this->ReadAllTCoords ? "On" : "Off") << "\n";

  if ( this->LookupTableName )
    {
    os << indent << "Lookup Table Name: " << this->LookupTableName << "\n";
    }
  else
    {
    os << indent << "Lookup Table Name: (None)\n";
    }
  os << indent << "ReadAllColorScalars: " 
     << (this->ReadAllColorScalars ? "On" : "Off") << "\n";

  if ( this->FieldDataName )
    {
    os << indent << "Field Data Name: " << this->FieldDataName << "\n";
    }
  else
    {
    os << indent << "Field Data Name: (None)\n";
    }
  os << indent << "ReadAllFields: " 
     << (this->ReadAllFields ? "On" : "Off") << "\n";
  
  os << indent << "InputStringLength: " << this->InputStringLength << endl;
}

int vtkDataReader::ReadDataSetData(vtkDataSet *vtkNotUsed(ds))
{
  return 0;
}


void vtkDataReader::DecodeArrayName(char *resname, const char* name)
{
  if ( !resname || !name )
    {
    return;
    }
  //strcpy(resname, name);
  ostrstream str;
  int cc = 0;
  unsigned int ch;
  int len = static_cast<int>(strlen(name));
  char buffer[10] = "0x";
  while(name[cc]) 
    {
    if ( name[cc] == '%' )
      {
      if ( cc < len - 3 )
        {
        buffer[2] = name[cc+1];
        buffer[3] = name[cc+2];
        buffer[4] = 0;
        sscanf(buffer, "%x", &ch);
        str << static_cast<char>(ch);
        cc+=2;
        }
      }
    else
      {
      str << name[cc];
      }
    cc ++;
    }
  str << ends;
  strcpy(resname, str.str());
  str.rdbuf()->freeze(0);
}
