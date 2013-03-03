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
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
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
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnicodeStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkVariantArray.h"
#include <vtksys/ios/sstream>

// We only have vtkTypeUInt64Array if we have long long
// or we have __int64 with conversion to double.
#if defined(VTK_TYPE_USE_LONG_LONG) || (defined(VTK_TYPE_USE___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE))
#include "vtkTypeUInt64Array.h"
#endif

#include <ctype.h>
#include <sys/stat.h>

// I need a safe way to read a line of arbitrary length.  It exists on
// some platforms but not others so I'm afraid I have to write it
// myself.
// This function is also defined in Infovis/vtkDelimitedTextReader.cxx,
// so it would be nice to put this in a common file.
static int my_getline(istream& stream, vtkStdString &output, char delim='\n');

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
  int len = 0;
  if (in != NULL)
    {
    len = static_cast<int>(strlen(in));
    }
  this->SetInputString(in, len);
}

void vtkDataReader::SetBinaryInputString(const char *in, int len)
{
  this->SetInputString(in, len);
}

void vtkDataReader::SetInputString(const char *in, int len)
{
  if (this->Debug)
    {
    vtkDebugMacro(<< "SetInputString len: " << len
      << " in: " << (in ? in : "(null)"));
    }

  if (this->InputString && in && strncmp(in, this->InputString, len) == 0)
    {
    return;
    }

  if (this->InputString)
    {
    delete [] this->InputString;
    }

  if (in && len>0)
    {
    // Add a NULL terminator so that GetInputString
    // callers (from wrapped languages) get a valid
    // C string in *ALL* cases...
    //
    this->InputString = new char[len+1];
    memcpy(this->InputString,in,len);
    this->InputString[len] = 0;
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

#if defined(VTK_TYPE_USE___INT64)
int vtkDataReader::Read(__int64 *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(unsigned __int64 *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}
#endif

#if defined(VTK_TYPE_USE_LONG_LONG)
int vtkDataReader::Read(long long *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}

int vtkDataReader::Read(unsigned long long *result)
{
  *this->IS >> *result;
  if (this->IS->fail())
    {
    return 0;
    }
  return 1;
}
#endif

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
  if ( this->IS != NULL )
    {
    this->CloseVTKFile ();
    }
  if (this->ReadFromInputString)
    {
    if (this->InputArray)
      {
      vtkDebugMacro(<< "Reading from InputArray");
      std::string str(this->InputArray->GetPointer(0),
        static_cast<size_t>( this->InputArray->GetNumberOfTuples()  *
                             this->InputArray->GetNumberOfComponents()) );
      this->IS = new vtksys_ios::istringstream(str);
      return 1;
      }
    else if (this->InputString)
      {
      vtkDebugMacro(<< "Reading from InputString");
      std::string str(this->InputString, this->InputStringLength);
      this->IS = new vtksys_ios::istringstream(str);
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
    this->CloseVTKFile ();
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

  this->CloseVTKFile ();
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
    // read the global id data
    //
    else if ( ! strncmp(line, "global_ids", 10) )
      {
      if ( ! this->ReadGlobalIds(a, numCells) )
        {
        return 0;
        }
      }
    //
    // read the pedigree id data
    //
    else if ( ! strncmp(line, "pedigree_ids", 10) )
      {
      if ( ! this->ReadPedigreeIds(a, numCells) )
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
        a->AddArray(f->GetAbstractArray(i));
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
    // read the global id data
    //
    else if ( ! strncmp(line, "global_ids", 10) )
      {
      if ( ! this->ReadGlobalIds(a, numPts) )
        {
        return 0;
        }
      }
    //
    // read the pedigree id data
    //
    else if ( ! strncmp(line, "pedigree_ids", 10) )
      {
      if ( ! this->ReadPedigreeIds(a, numPts) )
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
        a->AddArray(f->GetAbstractArray(i));
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


// Read the vertex data of a vtk data file. The number of vertices (from the
// graph) must match the number of vertices defined in vertex attributes (unless
// no geometry was defined).
int vtkDataReader::ReadVertexData(vtkGraph *g, int numVertices)
{
  char line[256];
  vtkDataSetAttributes *a=g->GetVertexData();

  vtkDebugMacro(<< "Reading vtk vertex data");

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
      if ( ! this->ReadScalarData(a, numVertices) )
        {
        return 0;
        }
      }
    //
    // read vector data
    //
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(a, numVertices) )
        {
        return 0;
        }
      }
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(a, numVertices) )
        {
        return 0;
        }
      }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
      {
      if ( ! this->ReadNormalData(a, numVertices) )
        {
        return 0;
        }
      }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(a, numVertices) )
        {
        return 0;
        }
      }
    //
    // read the global id data
    //
    else if ( ! strncmp(line, "global_ids", 10) )
      {
      if ( ! this->ReadGlobalIds(a, numVertices) )
        {
        return 0;
        }
      }
    //
    // read the pedigree id data
    //
    else if ( ! strncmp(line, "pedigree_ids", 10) )
      {
      if ( ! this->ReadPedigreeIds(a, numVertices) )
        {
        return 0;
        }
      }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(a, numVertices) )
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
        a->AddArray(f->GetAbstractArray(i));
        }
      f->Delete();
      }
    //
    // maybe bumped into edge data
    //
    else if ( ! strncmp(line, "edge_data", 10) )
      {
      int npts;
      if (!this->Read(&npts))
        {
        vtkErrorMacro(<<"Cannot read point data!");
        return 0;
        }

      this->ReadEdgeData(g, npts);
      }

    else
      {
      vtkErrorMacro(<< "Unsupported vertex attribute type: " << line
                    << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
      return 0;
      }
    }

  return 1;
}

// Read the edge data of a vtk data file. The number of edges (from the
// graph) must match the number of edges defined in edge attributes (unless
// no geometry was defined).
int vtkDataReader::ReadEdgeData(vtkGraph *g, int numEdges)
{
  char line[256];
  vtkDataSetAttributes *a=g->GetEdgeData();

  vtkDebugMacro(<< "Reading vtk edge data");

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
      if ( ! this->ReadScalarData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read vector data
    //
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
      {
      if ( ! this->ReadNormalData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read the global id data
    //
    else if ( ! strncmp(line, "global_ids", 10) )
      {
      if ( ! this->ReadGlobalIds(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read the pedigree id data
    //
    else if ( ! strncmp(line, "pedigree_ids", 10) )
      {
      if ( ! this->ReadPedigreeIds(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(a, numEdges) )
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
        a->AddArray(f->GetAbstractArray(i));
        }
      f->Delete();
      }
    //
    // maybe bumped into vertex data
    //
    else if ( ! strncmp(line, "vertex_data", 10) )
      {
      int npts;
      if (!this->Read(&npts))
        {
        vtkErrorMacro(<<"Cannot read vertex data!");
        return 0;
        }

      this->ReadVertexData(g, npts);
      }

    else
      {
      vtkErrorMacro(<< "Unsupported vertex attribute type: " << line
                    << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
      return 0;
      }
    }

  return 1;
}

// Read the row data of a vtk data file.
int vtkDataReader::ReadRowData(vtkTable *t, int numEdges)
{
  char line[256];
  vtkDataSetAttributes *a=t->GetRowData();

  vtkDebugMacro(<< "Reading vtk row data");

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
      if ( ! this->ReadScalarData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read vector data
    //
    else if ( ! strncmp(line, "vectors", 7) )
      {
      if ( ! this->ReadVectorData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read 3x3 tensor data
    //
    else if ( ! strncmp(line, "tensors", 7) )
      {
      if ( ! this->ReadTensorData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read normals data
    //
    else if ( ! strncmp(line, "normals", 7) )
      {
      if ( ! this->ReadNormalData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read texture coordinates data
    //
    else if ( ! strncmp(line, "texture_coordinates", 19) )
      {
      if ( ! this->ReadTCoordsData(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read the global id data
    //
    else if ( ! strncmp(line, "global_ids", 10) )
      {
      if ( ! this->ReadGlobalIds(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read the pedigree id data
    //
    else if ( ! strncmp(line, "pedigree_ids", 10) )
      {
      if ( ! this->ReadPedigreeIds(a, numEdges) )
        {
        return 0;
        }
      }
    //
    // read color scalars data
    //
    else if ( ! strncmp(line, "color_scalars", 13) )
      {
      if ( ! this->ReadCoScalarData(a, numEdges) )
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
        a->AddArray(f->GetAbstractArray(i));
        }
      f->Delete();
      }
    else
      {
      vtkErrorMacro(<< "Unsupported row attribute type: " << line
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
  if (numTuples==0 || numComp==0)
    {
    // nothing to read here.
    return 1;
    }
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
        vtkGenericWarningMacro(<<"Error reading ascii data. Possible mismatch of "
          "datasize with declaration.");
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
vtkAbstractArray *vtkDataReader::ReadArray(const char *dataType, int numTuples, int numComp)
{
  char *type=strdup(dataType);
  type=this->LowerCase(type);

  vtkAbstractArray *array;
  if ( ! strncmp(type, "bit", 3) )
    {
    array = vtkBitArray::New();
    array->SetNumberOfComponents(numComp);
    if (numTuples !=0 && numComp !=0)
      {
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
          array->Delete();
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
              vtkErrorMacro("Error reading ascii bit array! tuple: " << i << ", component: " << j);
              free(type);
              array->Delete();
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

  else if ( ! strncmp(type, "vtkidtype", 9) )
    {
    // currently writing vtkIdType as int.
    array = vtkIdTypeArray::New();
    array->SetNumberOfComponents(numComp);
    int *ptr = new int [numTuples*numComp];
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap4BERange(ptr,numTuples*numComp);
      }
    else
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
    vtkIdType *ptr2 = ((vtkIdTypeArray *)array)->WritePointer(
      0,numTuples*numComp);
    for(vtkIdType idx=0; idx<numTuples*numComp; idx++)
      {
      ptr2[idx] = ptr[idx];
      }
    delete[] ptr;
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

  else if ( ! strncmp(type, "vtktypeint64", 12) )
    {
    array = vtkTypeInt64Array::New();
    array->SetNumberOfComponents(numComp);
    vtkTypeInt64 *ptr = ((vtkTypeInt64Array *)array)->WritePointer(0,numTuples*numComp);
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

  else if ( ! strncmp(type, "vtktypeuint64", 13) )
    {
// We only have vtkTypeUInt64Array if we have long long
// or we have __int64 with conversion to double.
#if defined(VTK_TYPE_USE_LONG_LONG) || (defined(VTK_TYPE_USE___INT64) && defined(VTK_TYPE_CONVERT_UI64_TO_DOUBLE))
    array = vtkTypeUInt64Array::New();
    array->SetNumberOfComponents(numComp);
    vtkTypeUInt64 *ptr = ((vtkTypeUInt64Array *)array)->WritePointer(0,numTuples*numComp);
    if ( this->FileType == VTK_BINARY )
      {
      vtkReadBinaryData(this->IS, ptr, numTuples, numComp);
      vtkByteSwap::Swap8BERange(ptr,numTuples*numComp);
      }

    else
      {
      vtkReadASCIIData(this, ptr, numTuples, numComp);
      }
#else
    vtkErrorMacro("This version of VTK cannot read unsigned 64-bit integers.");
    free(type);
    return NULL;
#endif
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

  else if ( ! strncmp(type, "string", 6) )
    {
    array = vtkStringArray::New();
    array->SetNumberOfComponents(numComp);

    if ( this->FileType == VTK_BINARY )
      {
      // read in newline
      char line[256];
      IS->getline(line,256);

      for (int i=0; i<numTuples; i++)
        {
        for (int j=0; j<numComp; j++)
          {
          vtkTypeUInt8 firstByte;
          vtkTypeUInt8 headerType;
          vtkStdString::size_type stringLength;
          firstByte = IS->peek();
          headerType = firstByte >> 6;
          if (headerType == 3)
            {
            vtkTypeUInt8 length = IS->get();
            length <<= 2;
            length >>= 2;
            stringLength = length;
            }
          else if (headerType == 2)
            {
            vtkTypeUInt16 length;
            IS->read(reinterpret_cast<char*>(&length), 2);
            vtkByteSwap::Swap2BE(&length);
            length <<= 2;
            length >>= 2;
            stringLength = length;
            }
          else if (headerType == 1)
            {
            vtkTypeUInt32 length;
            IS->read(reinterpret_cast<char*>(&length), 4);
            vtkByteSwap::Swap4BE(&length);
            length <<= 2;
            length >>= 2;
            stringLength = length;
            }
          else
            {
            vtkTypeUInt64 length;
            IS->read(reinterpret_cast<char*>(&length), 8);
            vtkByteSwap::Swap4BE(&length);
            stringLength = length;
            }
          char* str = new char[stringLength];
          IS->read(str, stringLength);
          vtkStdString s(str, stringLength);
          ((vtkStringArray*)array)->InsertNextValue(s);
          delete [] str;
          }
        }
      }
    else
      {
      // read in newline
      vtkStdString s;
      my_getline(*(this->IS), s);

      for (int i=0; i<numTuples; i++)
        {
        for (int j=0; j<numComp; j++)
          {
          my_getline(*(this->IS), s);
          int length = static_cast<int>(s.length());
          char* decoded = new char[length + 1];
          int decodedLength = this->DecodeString(decoded, s.c_str());
          vtkStdString decodedStr(decoded, decodedLength);
          ((vtkStringArray*)array)->InsertNextValue(decodedStr);
          delete[] decoded;
          }
        }
      }
    }
  else if ( ! strncmp(type, "utf8_string", 11) )
    {
    array = vtkUnicodeStringArray::New();
    array->SetNumberOfComponents(numComp);

    if ( this->FileType == VTK_BINARY )
      {
      // read in newline
      char line[256];
      IS->getline(line,256);

      for (int i=0; i<numTuples; i++)
        {
        for (int j=0; j<numComp; j++)
          {
          vtkTypeUInt8 firstByte;
          vtkTypeUInt8 headerType;
          vtkStdString::size_type stringLength;
          firstByte = IS->peek();
          headerType = firstByte >> 6;
          if (headerType == 3)
            {
            vtkTypeUInt8 length = IS->get();
            length <<= 2;
            length >>= 2;
            stringLength = length;
            }
          else if (headerType == 2)
            {
            vtkTypeUInt16 length;
            IS->read(reinterpret_cast<char*>(&length), 2);
            vtkByteSwap::Swap2BE(&length);
            length <<= 2;
            length >>= 2;
            stringLength = length;
            }
          else if (headerType == 1)
            {
            vtkTypeUInt32 length;
            IS->read(reinterpret_cast<char*>(&length), 4);
            vtkByteSwap::Swap4BE(&length);
            length <<= 2;
            length >>= 2;
            stringLength = length;
            }
          else
            {
            vtkTypeUInt64 length;
            IS->read(reinterpret_cast<char*>(&length), 8);
            vtkByteSwap::Swap4BE(&length);
            stringLength = length;
            }
          char* str = new char[stringLength];
          IS->read(str, stringLength);
          vtkUnicodeString s = vtkUnicodeString::from_utf8(str, str + stringLength);
          ((vtkUnicodeStringArray*)array)->InsertNextValue(s);
          delete [] str;
          }
        }
      }
    else
      {
      // read in newline
      vtkStdString s;
      my_getline(*(this->IS), s);

      for (int i=0; i<numTuples; i++)
        {
        for (int j=0; j<numComp; j++)
          {
          my_getline(*(this->IS), s);
          int length = static_cast<int>(s.length());
          char* decoded = new char[length + 1];
          int decodedLength = this->DecodeString(decoded, s.c_str());
          vtkUnicodeString decodedStr = vtkUnicodeString::from_utf8(decoded, decoded + decodedLength);
          ((vtkUnicodeStringArray*)array)->InsertNextValue(decodedStr);
          delete[] decoded;
          }
        }
      }
    }
  else if ( ! strncmp(type, "variant", 7) )
    {
    array = vtkVariantArray::New();
    array->SetNumberOfComponents(numComp);
    for (int i=0; i<numTuples; i++)
      {
      for (int j=0; j<numComp; j++)
        {
        int t;
        vtkStdString str;
        *(this->IS) >> t >> str;
        char* decoded = new char[str.length() + 1];
        int decodedLength = this->DecodeString(decoded, str.c_str());
        vtkStdString decodedStr(decoded, decodedLength);
        delete[] decoded;
        vtkVariant sv(decodedStr);
        vtkVariant v;
        switch (t)
          {
          case VTK_CHAR:
            v = sv.ToChar();
            break;
          case VTK_SIGNED_CHAR:
            v = sv.ToSignedChar();
            break;
          case VTK_UNSIGNED_CHAR:
            v = sv.ToUnsignedChar();
            break;
          case VTK_SHORT:
            v = sv.ToShort();
            break;
          case VTK_UNSIGNED_SHORT:
            v = sv.ToUnsignedShort();
            break;
          case VTK_INT:
            v = sv.ToInt();
            break;
          case VTK_UNSIGNED_INT:
            v = sv.ToUnsignedInt();
            break;
          case VTK_LONG:
            v = sv.ToLong();
            break;
          case VTK_UNSIGNED_LONG:
            v = sv.ToUnsignedLong();
            break;
          case VTK_FLOAT:
            v = sv.ToFloat();
            break;
          case VTK_DOUBLE:
            v = sv.ToDouble();
            break;
#ifdef VTK_TYPE_USE_LONG_LONG
          case VTK_LONG_LONG:
            v = sv.ToLongLong();
            break;
          case VTK_UNSIGNED_LONG_LONG:
            v = sv.ToUnsignedLongLong();
            break;
#endif
#ifdef VTK_TYPE_USE___INT64
          case VTK___INT64:
            v = sv.To__Int64();
            break;
          case VTK_UNSIGNED___INT64:
            v = sv.ToUnsigned__Int64();
            break;
#endif
          case VTK_STRING:
            v = sv.ToString();
            break;
          default:
            vtkErrorMacro("Unknown variant type " << t);
          }
        ((vtkVariantArray*)array)->InsertNextValue(v);
        }
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

  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numPts, 3));
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

// Read point coordinates. Return 0 if error.
int vtkDataReader::ReadPoints(vtkGraph *g, int numPts)
{
  char line[256];
  vtkDataArray *data;

  if (!this->ReadString(line))
    {
    vtkErrorMacro(<<"Cannot read points type!" << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }

  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numPts, 3));
  if ( data != NULL )
    {
    vtkPoints *points=vtkPoints::New();
    points->SetData(data);
    data->Delete();
    g->SetPoints(points);
    points->Delete();
    }
  else
    {
    return 0;
    }

  vtkDebugMacro(<<"Read " << g->GetNumberOfVertices() << " points");
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

  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numCoords, 1));
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

  this->DecodeString(name, buffer);

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
  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numPts, numComp));
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
  this->DecodeString(name, buffer);

  //
  // See whether vector has been already read or vector name (if specified)
  // matches name in file.
  //
  if ( a->GetVectors() != NULL || (this->VectorsName && strcmp(name,this->VectorsName)) )
    {
    skipVector = 1;
    }

  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numPts, 3));
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
  this->DecodeString(name, buffer);

  //
  // See whether normal has been already read or normal name (if specified)
  // matches name in file.
  //
  if ( a->GetNormals() != NULL || (this->NormalsName && strcmp(name,this->NormalsName)) )
    {
    skipNormal = 1;
    }

  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numPts, 3));
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
  this->DecodeString(name, buffer);
  //
  // See whether tensor has been already read or tensor name (if specified)
  // matches name in file.
  //
  if ( a->GetTensors() != NULL || (this->TensorsName && strcmp(name,this->TensorsName)) )
    {
    skipTensor = 1;
    }

  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numPts, 9));
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
  int i, j, idx, numComp=0, skipScalar=0;
  char name[256];
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->Read(&numComp)))
    {
    vtkErrorMacro(<<"Cannot read color scalar data!" << " for file: "
                  << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }
  this->DecodeString(name, buffer);
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
  int dim = 0;
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
  this->DecodeString(name, buffer);

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

  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numPts, dim));
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

// Read texture coordinates point attributes. Return 0 if error.
int vtkDataReader::ReadGlobalIds(vtkDataSetAttributes *a, int numPts)
{
  int skipGlobalIds = 0;
  char line[256], name[256];
  vtkDataArray *data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read global id data" << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }
  this->DecodeString(name, buffer);

  //
  // See whether global ids have been already read
  //
  if ( a->GetGlobalIds() != NULL )
    {
    skipGlobalIds = 1;
    }

  data = vtkDataArray::SafeDownCast(
    this->ReadArray(line, numPts, 1));
  if ( data != NULL )
    {
    data->SetName(name);
    if ( ! skipGlobalIds )
      {
      a->SetGlobalIds(data);
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

// Read pedigree ids. Return 0 if error.
int vtkDataReader::ReadPedigreeIds(vtkDataSetAttributes *a, int numPts)
{
  int skipPedigreeIds = 0;
  char line[256], name[256];
  vtkAbstractArray *data;
  char buffer[1024];

  if (!(this->ReadString(buffer) && this->ReadString(line)))
    {
    vtkErrorMacro(<<"Cannot read global id data" << " for file: " << (this->FileName?this->FileName:"(Null FileName)"));
    return 0;
    }
  this->DecodeString(name, buffer);

  //
  // See whether pedigree ids have been already read
  //
  if ( a->GetPedigreeIds() != NULL )
    {
    skipPedigreeIds = 1;
    }

  data = this->ReadArray(line, numPts, 1);
  if ( data != NULL )
    {
    data->SetName(name);
    if ( ! skipPedigreeIds )
      {
      a->SetPedigreeIds(data);
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
  int size=0, skipTable=0;
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
  int i, numArrays=0, skipField=0;
  vtkFieldData *f;
  char name[256], type[256];
  int numComp, numTuples;
  vtkAbstractArray *data;

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
    if ( strcmp(buffer, "NULL_ARRAY") == 0 )
      {
      continue;
      }
    this->DecodeString(name, buffer);
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
    this->CloseVTKFile ();
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
    if ( *nameOfAttribute )
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

int vtkDataReader::DecodeString(char *resname, const char* name)
{
  if ( !resname || !name )
    {
    return 0;
    }
  vtksys_ios::ostringstream str;
  size_t cc = 0;
  unsigned int ch;
  size_t len = strlen(name);
  size_t reslen = 0;
  char buffer[10] = "0x";
  while(name[cc])
    {
    if ( name[cc] == '%' )
      {
      if ( cc <= (len - 3) )
        {
        buffer[2] = name[cc+1];
        buffer[3] = name[cc+2];
        buffer[4] = 0;
        sscanf(buffer, "%x", &ch);
        str << static_cast<char>(ch);
        cc+=2;
        reslen++;
        }
      }
    else
      {
      str << name[cc];
      reslen++;
      }
    cc ++;
    }
  strncpy(resname, str.str().c_str(), reslen+1);
  resname[reslen] = 0;
  return static_cast<int>(reslen);
}

static int
my_getline(istream& in, vtkStdString &out, char delimiter)
{
  out = vtkStdString();
  unsigned int numCharactersRead = 0;
  int nextValue = 0;

  while ((nextValue = in.get()) != EOF &&
         numCharactersRead < out.max_size())
    {
    ++numCharactersRead;

    char downcast = static_cast<char>(nextValue);
    if (downcast != delimiter)
      {
      out += downcast;
      }
    else
      {
      return numCharactersRead;
      }
    }

  return numCharactersRead;
}

//----------------------------------------------------------------------------
void vtkDataReader::SetScalarLut(const char* sl)
{
  if (!this->ScalarLut  && !sl)
    {
    return;
    }
  if (this->ScalarLut && sl && (strcmp(this->ScalarLut,sl)) == 0)
    {
    return;
    }
  if (this->ScalarLut)
    {
    delete[] this->ScalarLut;
    this->ScalarLut = 0;
    }
  if (sl)
    {
    size_t n = strlen(sl) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = sl;
    this->ScalarLut = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
}
