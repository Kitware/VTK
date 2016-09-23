/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockPLOT3DReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockPLOT3DReader.h"

#include "vtkByteSwap.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkExtentTranslator.h"
#include "vtkFieldData.h"
#include "vtkFloatArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdList.h"
#include "vtkCellData.h"
#include "vtkMultiProcessController.h"
#include "vtkDummyController.h"
#include "vtkNew.h"

#include "vtkMultiBlockPLOT3DReaderInternals.h"

vtkObjectFactoryNewMacro(vtkMultiBlockPLOT3DReader);

vtkCxxSetObjectMacro(vtkMultiBlockPLOT3DReader,
                     Controller,
                     vtkMultiProcessController);

#define VTK_RHOINF 1.0
#define VTK_CINF 1.0
#define VTK_PINF ((VTK_RHOINF*VTK_CINF) * (VTK_RHOINF*VTK_CINF) / this->Gamma)
#define VTK_CV (this->R / (this->Gamma-1.0))

namespace
{
// helper class used to keep a FILE handle to close when the instance goes out
// of scope.
class vtkPlot3DCFile
{
  FILE* Handle;
  bool CloseOnDelete;
public:
  vtkPlot3DCFile(FILE* handle=NULL) : Handle(handle), CloseOnDelete(true) {}
  ~vtkPlot3DCFile() { if (this->Handle && this->CloseOnDelete) { fclose(this->Handle); } }
  operator FILE*&() { return this->Handle; }
  // This may be needed to tell vtkPlot3DCFile not to close on delete, instead
  // we're taking over the calling close on the file.
  void DisableClose() { this->CloseOnDelete = false; }
};

}

template <class DataType>
class vtkPLOT3DArrayReader
{
public:
  vtkPLOT3DArrayReader() : ByteOrder(
    vtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN)
  {
  }

  vtkIdType ReadScalar(FILE* fp,
                    vtkIdType preskip,
                    vtkIdType n,
                    vtkIdType postskip,
                    DataType* scalar,
                    const vtkMultiBlockPLOT3DReaderRecord& record = vtkMultiBlockPLOT3DReaderRecord())
  {
      vtkMultiBlockPLOT3DReaderRecord::SubRecordSeparators separators =
        record.GetSubRecordSeparators(vtk_ftell(fp), preskip);

      vtk_fseek(fp,
        preskip*sizeof(DataType)
        + separators.size() * vtkMultiBlockPLOT3DReaderRecord::SubRecordSeparatorWidth,
        SEEK_CUR);

      // Let's see if we encounter markers while reading the data from current
      // position.
      separators = record.GetSubRecordSeparators(vtk_ftell(fp), sizeof(DataType) * n);

      vtkIdType retVal;
      if (separators.size() == 0)
      {
        // no record separators will be encountered, yay! Just read the block.
        retVal = static_cast<vtkIdType>(fread(scalar, sizeof(DataType), n, fp));
      }
      else
      {
        // need to read in chunks to skip separators.
        vtkTypeUInt64 pos = vtk_ftell(fp);
        std::vector<std::pair<vtkTypeUInt64, vtkTypeUInt64> > chunks =
          record.GetChunksToRead(pos, sizeof(DataType) * n, separators);

        vtkTypeUInt64 bytesread = 0;
        for (size_t cc=0; cc < chunks.size(); ++cc)
        {
          vtk_fseek(fp, chunks[cc].first, SEEK_SET);
          bytesread += static_cast<vtkTypeUInt64>(
            fread(reinterpret_cast<char*>(scalar) + bytesread, 1, chunks[cc].second, fp));
        }
        retVal = static_cast<vtkIdType>(bytesread / sizeof(DataType));
      }

      // Let's count markers we encounter while postskipping. If any, we'll need
      // to step over them as well.
      separators = record.GetSubRecordSeparators(vtk_ftell(fp), sizeof(DataType)*postskip);
      vtk_fseek(fp,
        postskip*sizeof(DataType)
        + separators.size() * vtkMultiBlockPLOT3DReaderRecord::SubRecordSeparatorWidth,
        SEEK_CUR);
      if (this->ByteOrder == vtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
      {
        if (sizeof(DataType) == 4)
        {
          vtkByteSwap::Swap4LERange(scalar, n);
        }
        else
        {
          vtkByteSwap::Swap8LERange(scalar, n);
        }
      }
      else
      {
        if (sizeof(DataType) == 4)
        {
          vtkByteSwap::Swap4BERange(scalar, n);
        }
        else
        {
          vtkByteSwap::Swap8BERange(scalar, n);
        }
      }
      return retVal;
  }

  vtkIdType ReadVector(FILE* fp,
                    int extent[6], int wextent[6],
                    int numDims, DataType* vector,
                    const vtkMultiBlockPLOT3DReaderRecord &record)
  {
      vtkIdType n = vtkStructuredData::GetNumberOfPoints(extent);

      // Setting to 0 in case numDims == 0. We still need to
      // populate an array with 3 components but the code below
      // does not read the 3rd component (it doesn't exist
      // in the file)
      memset(vector, 0, n*3*sizeof(DataType));

      vtkIdType retVal = 0;
      DataType* buffer = new DataType[n];
      for (int component = 0; component < numDims; component++)
      {
        vtkIdType preskip, postskip;
        vtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
        retVal += this->ReadScalar(fp, preskip, n, postskip, buffer, record);
        for (vtkIdType i=0; i<n; i++)
        {
          vector[3*i+component] = buffer[i];
        }
      }
      delete[] buffer;
      return retVal;
  }

  int ByteOrder;
};

vtkMultiBlockPLOT3DReader::vtkMultiBlockPLOT3DReader()
{
  this->Internal = new vtkMultiBlockPLOT3DReaderInternals;

  this->XYZFileName = NULL;
  this->QFileName = NULL;
  this->FunctionFileName = NULL;
  this->BinaryFile = 1;
  this->HasByteCount = 0;
  this->FileSize = 0;
  this->MultiGrid = 0;
  this->ForceRead = 0;
  this->ByteOrder = FILE_BIG_ENDIAN;
  this->IBlanking = 0;
  this->TwoDimensionalGeometry = 0;
  this->DoublePrecision = 0;
  this->AutoDetectFormat = 0;

  this->R = 1.0;
  this->Gamma = 1.4;

  this->FunctionList = vtkIntArray::New();

  this->ScalarFunctionNumber = -1;
  this->SetScalarFunctionNumber(100);
  this->VectorFunctionNumber = -1;
  this->SetVectorFunctionNumber(202);

  this->SetNumberOfInputPorts(0);

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->ExecutedGhostLevels = 0;
}

vtkMultiBlockPLOT3DReader::~vtkMultiBlockPLOT3DReader()
{
  delete [] this->XYZFileName;
  delete [] this->QFileName;
  delete [] this->FunctionFileName;
  this->FunctionList->Delete();
  this->ClearGeometryCache();

  delete this->Internal;

  this->SetController(0);
}

void vtkMultiBlockPLOT3DReader::ClearGeometryCache()
{
  this->Internal->Blocks.clear();
}

int vtkMultiBlockPLOT3DReader::AutoDetectionCheck(FILE* fp)
{
  this->Internal->CheckBinaryFile(fp, this->FileSize);

  if (!this->Internal->Settings.BinaryFile)
  {
    vtkDebugMacro("Auto-detection only works with binary files.");
    if (this->BinaryFile)
    {
      vtkWarningMacro("This appears to be an ASCII file. Please make sure "
                      "that all settings are correct to read it correctly.");
    }
    this->Internal->Settings.ByteOrder = this->ByteOrder;
    this->Internal->Settings.HasByteCount = this->HasByteCount;
    this->Internal->Settings.MultiGrid = this->MultiGrid;
    this->Internal->Settings.NumberOfDimensions = this->TwoDimensionalGeometry ? 2 : 3;
    this->Internal->Settings.Precision = this->DoublePrecision ? 8 : 4;
    this->Internal->Settings.IBlanking = this->IBlanking;
    return 1;
  }

  if (!this->Internal->CheckByteOrder(fp))
  {
    vtkErrorMacro("Could not determine big/little endianness of file.")
    return 0;
  }
  if (!this->Internal->CheckByteCount(fp))
  {
    vtkErrorMacro("Could not determine if file has Fortran byte counts.")
    return 0;
  }

  if (!this->Internal->Settings.HasByteCount)
  {
    if (!this->Internal->CheckCFile(fp, this->FileSize))
    {
      vtkErrorMacro("Could not determine settings for file. Cannot read.")
      return 0;
    }
  }
  else
  {
    if (!this->Internal->CheckMultiGrid(fp))
    {
      vtkErrorMacro("Could not determine settings for file. Cannot read.")
      return 0;
    }
    if (!this->Internal->Check2DGeom(fp))
    {
      vtkErrorMacro("Could not determine settings for file. Cannot read.")
      return 0;
    }
    if (!this->Internal->CheckBlankingAndPrecision(fp))
    {
      vtkErrorMacro("Could not determine settings for file. Cannot read.")
      return 0;
    }
  }
  if (!this->AutoDetectFormat)
  {
    if ( !this->ForceRead && (
           this->Internal->Settings.BinaryFile != this->BinaryFile ||
           this->Internal->Settings.ByteOrder != this->ByteOrder ||
           this->Internal->Settings.HasByteCount != this->HasByteCount ||
           this->Internal->Settings.MultiGrid != this->MultiGrid ||
           this->Internal->Settings.NumberOfDimensions != (this->TwoDimensionalGeometry ? 2 : 3) ||
           this->Internal->Settings.Precision != (this->DoublePrecision ? 8 : 4) ||
           this->Internal->Settings.IBlanking != this->IBlanking ) )
    {
      vtkErrorMacro(<< "The settings that you provided do not match what was auto-detected "
              << "in the file. The detected settings are: " << "\n"
              << "BinaryFile: " << (this->Internal->Settings.BinaryFile ? 1 : 0) << "\n"
              << "ByteOrder: " << this->Internal->Settings.ByteOrder << "\n"
              << "HasByteCount: " << (this->Internal->Settings.HasByteCount ? 1 : 0) << "\n"
              << "MultiGrid: " << (this->Internal->Settings.MultiGrid ? 1 : 0) << "\n"
              << "NumberOfDimensions: " << this->Internal->Settings.NumberOfDimensions << "\n"
              << "DoublePrecision: " << (this->Internal->Settings.Precision == 4 ? 0 : 1) << "\n"
              << "IBlanking: " << (this->Internal->Settings.IBlanking ? 1 : 0) << endl);
      return 0;
    }
    this->Internal->Settings.BinaryFile = this->BinaryFile;
    this->Internal->Settings.ByteOrder = this->ByteOrder;
    this->Internal->Settings.HasByteCount = this->HasByteCount;
    this->Internal->Settings.MultiGrid = this->MultiGrid;
    this->Internal->Settings.NumberOfDimensions = this->TwoDimensionalGeometry ? 2 : 3;
    this->Internal->Settings.Precision = this->DoublePrecision ? 8 : 4;
    this->Internal->Settings.IBlanking = this->IBlanking;
    return 1;
  }
  return 1;
}

int vtkMultiBlockPLOT3DReader::OpenFileForDataRead(void*& fp, const char* fname)
{
  if (this->BinaryFile)
  {
    fp = fopen(fname, "rb");
  }
  else
  {
    fp = fopen(fname, "r");
  }
  if ( fp == NULL)
  {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    vtkErrorMacro(<< "File: " << fname << " not found.");
    return VTK_ERROR;
  }
  return VTK_OK;
}

void vtkMultiBlockPLOT3DReader::CloseFile(void* fp)
{
  fclose(reinterpret_cast<FILE*>(fp));
}

int vtkMultiBlockPLOT3DReader::CheckFile(FILE*& fp, const char* fname)
{
  if (this->BinaryFile)
  {
    fp = fopen(fname, "rb");
  }
  else
  {
    fp = fopen(fname, "r");
  }
  if ( fp == NULL)
  {
    this->SetErrorCode(vtkErrorCode::FileNotFoundError);
    vtkErrorMacro(<< "File: " << fname << " not found.");
    return VTK_ERROR;
  }
  return VTK_OK;
}

int vtkMultiBlockPLOT3DReader::CheckGeometryFile(FILE*& xyzFp)
{
  if ( this->XYZFileName == NULL || this->XYZFileName[0] == '\0'  )
  {
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    vtkErrorMacro(<< "Must specify geometry file");
    return VTK_ERROR;
  }
  return this->CheckFile(xyzFp, this->XYZFileName);
}

int vtkMultiBlockPLOT3DReader::CheckSolutionFile(FILE*& qFp)
{
  if ( this->QFileName == NULL || this->QFileName[0] == '\0' )
  {
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    vtkErrorMacro(<< "Must specify geometry file");
    return VTK_ERROR;
  }
  return this->CheckFile(qFp, this->QFileName);
}

int vtkMultiBlockPLOT3DReader::CheckFunctionFile(FILE*& fFp)
{
  if ( this->FunctionFileName == NULL || this->FunctionFileName[0] == '\0' )
  {
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    vtkErrorMacro(<< "Must specify geometry file");
    return VTK_ERROR;
  }
  return this->CheckFile(fFp, this->FunctionFileName);
}

int vtkMultiBlockPLOT3DReader::GetByteCountSize()
{
  if (this->Internal->Settings.BinaryFile && this->Internal->Settings.HasByteCount)
  {
    return sizeof(int);
  }
  return 0;
}

// Skip Fortran style byte count.
int vtkMultiBlockPLOT3DReader::SkipByteCount(FILE* fp)
{
  int byteCountSize = this->GetByteCountSize();
  if (byteCountSize > 0)
  {
    int tmp;
    if (fread(&tmp, byteCountSize, 1, fp) != 1)
    {
      vtkErrorMacro ("MultiBlockPLOT3DReader error reading file: " << this->XYZFileName
                     << " Premature EOF while reading skipping byte count.");
      fclose (fp);
      return 0;
    }
    if (this->Internal->Settings.ByteOrder == vtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
    {
      vtkByteSwap::Swap4LERange(&tmp, 1);
    }
    else
    {
      vtkByteSwap::Swap4BERange(&tmp, 1);
    }

    return tmp;
  }
  return 0;
}

// Read a block of ints (ascii or binary) and return number read.
int vtkMultiBlockPLOT3DReader::ReadIntBlock(FILE* fp, int n, int* block)
{
  if (this->Internal->Settings.BinaryFile)
  {
    vtkIdType retVal=static_cast<vtkIdType>(fread(block, sizeof(int), n, fp));
    if (this->Internal->Settings.ByteOrder == FILE_LITTLE_ENDIAN)
    {
      vtkByteSwap::Swap4LERange(block, n);
    }
    else
    {
      vtkByteSwap::Swap4BERange(block, n);
    }
    return retVal == n;
  }
  else
  {
    vtkIdType count = 0;
    for(int i=0; i<n; i++)
    {
      int num = fscanf(fp, "%d", &(block[i]));
      if ( num > 0 )
      {
        count++;
      }
      else
      {
        return 0;
      }
    }
    return count == n;
  }
}

vtkDataArray* vtkMultiBlockPLOT3DReader::NewFloatArray()
{
  if (this->Internal->Settings.Precision == 4)
  {
    return vtkFloatArray::New();
  }
  else
  {
    return vtkDoubleArray::New();
  }
}

vtkIdType vtkMultiBlockPLOT3DReader::ReadValues(
  FILE* fp, int n, vtkDataArray* scalar)
{
  if (this->Internal->Settings.BinaryFile)
  {
    if (this->Internal->Settings.Precision == 4)
    {
      vtkPLOT3DArrayReader<float> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(scalar);
      return arrayReader.ReadScalar(fp, 0, n, 0, floatArray->GetPointer(0));
    }
    else
    {
      vtkPLOT3DArrayReader<double> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(scalar);
      return arrayReader.ReadScalar(fp, 0, n, 0, doubleArray->GetPointer(0));
    }
  }
  else
  {
    if (this->Internal->Settings.Precision == 4)
    {
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(scalar);
      float* values = floatArray->GetPointer(0);

      int count = 0;
      for(int i=0; i<n; i++)
      {
        int num = fscanf(fp, "%f", &(values[i]));
        if ( num > 0 )
        {
          count++;
        }
        else
        {
          return 0;
        }
      }
      return count;
    }
    else
    {
      vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(scalar);
      double* values = doubleArray->GetPointer(0);

      int count = 0;
      for(int i=0; i<n; i++)
      {
        int num = fscanf(fp, "%lf", &(values[i]));
        if ( num > 0 )
        {
          count++;
        }
        else
        {
          return 0;
        }
      }
      return count;
    }
  }
}

int vtkMultiBlockPLOT3DReader::ReadIntScalar(
  void* vfp, int extent[6], int wextent[6],
  vtkDataArray* scalar, vtkTypeUInt64 offset,
  const vtkMultiBlockPLOT3DReaderRecord& record)
{
  FILE* fp = reinterpret_cast<FILE*>(vfp);
  vtkIdType n = vtkStructuredData::GetNumberOfPoints(extent);

  if (this->Internal->Settings.BinaryFile)
  {
    // precond: we assume the offset has been updated properly to step over
    // sub-record markers, if any.
    if (vtk_fseek(fp, offset, SEEK_SET) != 0)
    {
      return 0;
    }

    vtkPLOT3DArrayReader<int> arrayReader;
    arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
    vtkIdType preskip, postskip;
    vtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
    vtkIntArray* intArray = static_cast<vtkIntArray*>(scalar);
    return arrayReader.ReadScalar(
      fp, preskip, n, postskip, intArray->GetPointer(0), record) == n;
  }
  else
  {
    vtkIntArray* intArray = static_cast<vtkIntArray*>(scalar);
    return this->ReadIntBlock(fp, n, intArray->GetPointer(0));
  }
}

int vtkMultiBlockPLOT3DReader::ReadScalar(
  void* vfp, int extent[6], int wextent[6],
  vtkDataArray* scalar, vtkTypeUInt64 offset,
  const vtkMultiBlockPLOT3DReaderRecord& record)
{
  vtkIdType n = vtkStructuredData::GetNumberOfPoints(extent);

  FILE* fp = reinterpret_cast<FILE*>(vfp);

  if (this->Internal->Settings.BinaryFile)
  {
    // precond: we assume the offset has been updated properly to step over
    // sub-record markers, if any.
    if (vtk_fseek(fp, offset, SEEK_SET) != 0)
    {
      return 0;
    }

    if (this->Internal->Settings.Precision == 4)
    {
      vtkPLOT3DArrayReader<float> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      vtkIdType preskip, postskip;
      vtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(scalar);
      return arrayReader.ReadScalar(
        fp, preskip, n, postskip, floatArray->GetPointer(0),
        record) == n;
    }
    else
    {
      vtkPLOT3DArrayReader<double> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      vtkIdType preskip, postskip;
      vtkMultiBlockPLOT3DReaderInternals::CalculateSkips(extent, wextent, preskip, postskip);
      vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(scalar);
      return arrayReader.ReadScalar(
        fp, preskip, n, postskip, doubleArray->GetPointer(0),
        record) == n;
    }
  }
  else
  {
    if (this->Internal->Settings.Precision == 4)
    {
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(scalar);
      float* values = floatArray->GetPointer(0);

      int count = 0;
      for(int i=0; i<n; i++)
      {
        int num = fscanf(fp, "%f", &(values[i]));
        if ( num > 0 )
        {
          count++;
        }
        else
        {
          return 0;
        }
      }
      return count == n;
    }
    else
    {
      vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(scalar);
      double* values = doubleArray->GetPointer(0);

      int count = 0;
      for(int i=0; i<n; i++)
      {
        int num = fscanf(fp, "%lf", &(values[i]));
        if ( num > 0 )
        {
          count++;
        }
        else
        {
          return 0;
        }
      }
      return count == n;
    }
  }
}

int vtkMultiBlockPLOT3DReader::ReadVector(
  void* vfp, int extent[6], int wextent[6],
  int numDims, vtkDataArray* vector, vtkTypeUInt64 offset,
  const vtkMultiBlockPLOT3DReaderRecord& record)
{
  vtkIdType n = vtkStructuredData::GetNumberOfPoints(extent);
  vtkIdType nValues = n*numDims;

  FILE* fp = reinterpret_cast<FILE*>(vfp);

  if (this->Internal->Settings.BinaryFile)
  {
    // precond: we assume the offset has been updated properly to step over
    // sub-record markers, if any.
    if (vtk_fseek(fp, offset, SEEK_SET) != 0)
    {
      return 0;
    }
    if (this->Internal->Settings.Precision == 4)
    {
      vtkPLOT3DArrayReader<float> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(vector);
      return arrayReader.ReadVector(
        fp, extent, wextent, numDims, floatArray->GetPointer(0), record) == nValues;
    }
    else
    {
      vtkPLOT3DArrayReader<double> arrayReader;
      arrayReader.ByteOrder = this->Internal->Settings.ByteOrder;
      vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(vector);
      return arrayReader.ReadVector(
        fp, extent, wextent, numDims, doubleArray->GetPointer(0), record) == nValues;
    }
  }
  else
  {

    // Initialize the 3rd component to 0 in case the input file is
    // 2D
    vector->FillComponent(2, 0);

    vtkIdType count = 0;

    if (this->Internal->Settings.Precision == 4)
    {
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(vector);

      vtkFloatArray* tmpArray = vtkFloatArray::New();
      tmpArray->Allocate(n);
      for (int component = 0; component < numDims; component++)
      {
        count += this->ReadValues(fp, n, tmpArray);
        for (vtkIdType i=0; i<n; i++)
        {
          floatArray->SetValue(3*i+component, tmpArray->GetValue(i));
        }
      }
      tmpArray->Delete();
    }
    else
    {
      vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(vector);

      vtkDoubleArray* tmpArray = vtkDoubleArray::New();
      tmpArray->Allocate(n);
      for (int component = 0; component < numDims; component++)
      {
        count += this->ReadValues(fp, n, tmpArray);
        for (vtkIdType i=0; i<n; i++)
        {
          doubleArray->SetValue(3*i+component, tmpArray->GetValue(i));
        }
      }
      tmpArray->Delete();
    }

    return count == nValues;
  }
}

// Read a block of floats (ascii or binary) and return number read.
void vtkMultiBlockPLOT3DReader::CalculateFileSize(FILE* fp)
{
  vtk_off_t curPos = vtk_ftell(fp);
  vtk_fseek(fp, 0, SEEK_END);
  this->FileSize = static_cast<size_t>(vtk_ftell(fp));
  vtk_fseek(fp, curPos, SEEK_SET);
}

int vtkMultiBlockPLOT3DReader::CanReadBinaryFile(const char* fname)
{
  FILE* xyzFp;

  if (!fname || fname[0] == '\0')
  {
    return 0;
  }

  if ( this->CheckFile(xyzFp, fname) != VTK_OK)
  {
    return 0;
  }

  this->CalculateFileSize(xyzFp);

  if (!this->AutoDetectionCheck(xyzFp))
  {
    fclose(xyzFp);
    return 0;
  }
  rewind(xyzFp);

  int numBlocks = this->GetNumberOfBlocksInternal(xyzFp, 0);
  fclose(xyzFp);
  if (numBlocks != 0)
  {
    return 1;
  }
  return 0;
}

// Read the header and return the number of grids.
int vtkMultiBlockPLOT3DReader::GetNumberOfBlocksInternal(FILE* xyzFp, int vtkNotUsed(allocate))
{
  int numGrid = 0;

  if ( this->Internal->Settings.MultiGrid )
  {
    this->SkipByteCount(xyzFp);
    this->ReadIntBlock(xyzFp, 1, &numGrid);
    this->SkipByteCount(xyzFp);
  }
  else
  {
    numGrid=1;
  }


  if ( numGrid > (int)this->Internal->Dimensions.size() )
  {
    this->Internal->Dimensions.resize(numGrid);
  }

  return numGrid;
}

int vtkMultiBlockPLOT3DReader::ReadGeometryHeader(FILE* fp)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 1);
  int i;
  vtkDebugMacro("Geometry number of grids: " << numGrid);
  if ( numGrid == 0 )
  {
    return VTK_ERROR;
  }

  // Read and set extents of all blocks.
  this->SkipByteCount(fp);
  for(i=0; i<numGrid; i++)
  {
    int n[3];
    n[2] = 1;
    this->ReadIntBlock(fp, this->Internal->Settings.NumberOfDimensions, n);
    vtkDebugMacro("Geometry, block " << i << " dimensions: "
                  << n[0] << " " << n[1] << " " << n[2]);
    memcpy(this->Internal->Dimensions[i].Values, n, 3*sizeof(int));
  }
  this->SkipByteCount(fp);

  return VTK_OK;
}

int vtkMultiBlockPLOT3DReader::ReadQHeader(FILE* fp,
                                           bool checkGrid,
                                           int& nq,
                                           int& nqc,
                                           int& overflow)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 0);
  vtkDebugMacro("Q number of grids: " << numGrid);
  if ( numGrid == 0 )
  {
    return VTK_ERROR;
  }

  // If the numbers of grids still do not match, the
  // q file is wrong
  if (checkGrid &&
      numGrid != static_cast<int>(this->Internal->Blocks.size()))
  {
    vtkErrorMacro("The number of grids between the geometry "
                  "and the q file do not match.");
    return VTK_ERROR;
  }

  int bytes = this->SkipByteCount(fp);
  // If the header contains 2 additional ints, then we assume
  // that this is an Overflow file.
  if (bytes > 0 &&
      bytes == (numGrid*this->Internal->Settings.NumberOfDimensions+2)*4)
  {
    overflow = 1;
  }
  else
  {
    overflow = 0;
  }
  for(int i=0; i<numGrid; i++)
  {
    int n[3];
    n[2] = 1;
    this->ReadIntBlock(fp, this->Internal->Settings.NumberOfDimensions, n);
    vtkDebugMacro("Q, block " << i << " dimensions: "
                  << n[0] << " " << n[1] << " " << n[2]);

    if (checkGrid)
    {
      int* dims = this->Internal->Dimensions[i].Values;
      int extent[6] = {0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1};
      if ( extent[1] != n[0]-1 || extent[3] != n[1]-1 || extent[5] != n[2]-1)
      {
        this->SetErrorCode(vtkErrorCode::FileFormatError);
        vtkErrorMacro("Geometry and data dimensions do not match. "
                      "Data file may be corrupt.");
        this->Internal->Blocks[i]->Initialize();
        return VTK_ERROR;
      }
    }
  }
  if (overflow)
  {
    this->ReadIntBlock(fp, 1, &nq);
    this->ReadIntBlock(fp, 1, &nqc);
  }
  else
  {
    nq = 5;
    nqc = 0;
  }
  this->SkipByteCount(fp);
  return VTK_OK;
}

int vtkMultiBlockPLOT3DReader::ReadFunctionHeader(FILE* fp, int* nFunctions)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 0);
  vtkDebugMacro("Function number of grids: " << numGrid);
  if ( numGrid == 0 )
  {
    return VTK_ERROR;
  }

  // If the numbers of grids still do not match, the
  // function file is wrong
  if (numGrid != static_cast<int>(this->Internal->Blocks.size()))
  {
    vtkErrorMacro("The number of grids between the geometry "
                  "and the function file do not match.");
    return VTK_ERROR;
  }

  this->SkipByteCount(fp);
  for(int i=0; i<numGrid; i++)
  {
    int n[3];
    n[2] = 1;
    this->ReadIntBlock(fp, this->Internal->Settings.NumberOfDimensions, n);
    vtkDebugMacro("Function, block " << i << " dimensions: "
                  << n[0] << " " << n[1] << " " << n[2]);

    int* dims = this->Internal->Dimensions[i].Values;
    int extent[6] = {0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1};
    if ( extent[1] != n[0]-1 || extent[3] != n[1]-1 || extent[5] != n[2]-1)
    {
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      vtkErrorMacro("Geometry and data dimensions do not match. "
                    "Data file may be corrupt.");
      this->Internal->Blocks[i]->Initialize();
      return VTK_ERROR;
    }
    this->ReadIntBlock(fp, 1, nFunctions+i);
  }
  this->SkipByteCount(fp);
  return VTK_OK;
}

void vtkMultiBlockPLOT3DReader::SetXYZFileName( const char* name )
{
  if ( this->XYZFileName &&
       name &&
       ! strcmp( this->XYZFileName, name ) )
  {
    return;
  }

  delete [] this->XYZFileName;

  if ( name )
  {
    this->XYZFileName = new char [ strlen( name ) + 1 ];
    strcpy( this->XYZFileName, name );
  }
  else
  {
    this->XYZFileName = 0;
  }

  this->Internal->NeedToCheckXYZFile = true;
  this->ClearGeometryCache();
  this->Modified();
}

void vtkMultiBlockPLOT3DReader::SetScalarFunctionNumber(int num)
{
  if ( this->ScalarFunctionNumber == num)
  {
    return;
  }
  if (num >= 0)
  {
    // If this function is not in the list, add it.
    int found=0;
    for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++ )
    {
      if ( this->FunctionList->GetValue(i) == num )
      {
        found=1;
      }
    }
    if (!found)
    {
      this->AddFunction(num);
    }
  }
  this->ScalarFunctionNumber = num;
}

void vtkMultiBlockPLOT3DReader::SetVectorFunctionNumber(int num)
{
  if ( this->VectorFunctionNumber == num)
  {
    return;
  }
  if (num >= 0)
  {
    // If this function is not in the list, add it.
    int found=0;
    for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++ )
    {
      if ( this->FunctionList->GetValue(i) == num )
      {
        found=1;
      }
    }
    if (!found)
    {
      this->AddFunction(num);
    }
  }
  this->VectorFunctionNumber = num;
}

void vtkMultiBlockPLOT3DReader::RemoveFunction(int fnum)
{
  for (int i=0; i < this->FunctionList->GetNumberOfTuples(); i++ )
  {
    if ( this->FunctionList->GetValue(i) == fnum )
    {
      this->FunctionList->SetValue(i,-1);
      this->Modified();
    }
  }
}

int vtkMultiBlockPLOT3DReader::RequestInformation(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

  int rank = 0;
  // For now, only first rank does any reading.
  if (this->Controller)
  {
    rank = this->Controller->GetLocalProcessId();
  }

  double times[2];
  bool hasTime = false;
  int retval = 1;
  if (rank == 0)
  {
    try
    {
      if (this->XYZFileName &&
          this->XYZFileName[0] != '\0' &&
          (this->Internal->NeedToCheckXYZFile ||
           this->Internal->Blocks.size() == 0))
      {
        vtkPlot3DCFile xyzFp;
        if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
        {
          throw Plot3DException();
        }

        this->CalculateFileSize(xyzFp);

        if (!this->AutoDetectionCheck(xyzFp))
        {
          throw Plot3DException();
        }
        this->Internal->NeedToCheckXYZFile = false;
      }

      // We report time from the Q file for meta-type readers that
      // might support file series of Q files.
      if (this->QFileName && this->QFileName[0] != '\0')
      {
        vtkPlot3DCFile qFp;
        if ( this->CheckSolutionFile(qFp) != VTK_OK)
        {
          throw Plot3DException();
        }
        int nq, nqc, overflow;
        if (this->ReadQHeader(qFp, false, nq, nqc, overflow) != VTK_OK)
        {
          throw Plot3DException();
        }

        // I have seen Plot3D files with bogus time values so the only
        // type I have some confidence about having correct time values
        // is Overflow output.
        if (overflow)
        {
          vtkDataArray* properties = this->NewFloatArray();

          this->SkipByteCount(qFp);
          properties->SetNumberOfTuples(4);

          // Read fsmach, alpha, re, time;
          if (this->ReadValues(qFp, 4, properties) != 4)
          {
            vtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
            properties->Delete();
            throw Plot3DException();
          }
          double time = properties->GetTuple1(3);
          times[0] = times[1] = time;
          hasTime = true;
          properties->Delete();
        }
      }
    }
    catch (Plot3DException&)
    {
      retval = 0;
    }
  }

  if (this->Controller)
  {
    int vals[2] = {retval, (hasTime? 1 : 0)};
    this->Controller->Broadcast(vals, 2, 0);
    retval = vals[0];
    hasTime = (vals[1] == 1);
  }

  if (!retval)
  {
    return 0;
  }

  if (hasTime)
  {
    if (this->Controller)
    {
      this->Controller->Broadcast(times, 2, 0);
    }
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), times, 1);
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), times, 2);
  }

  return 1;
}

int vtkMultiBlockPLOT3DReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* info = outputVector->GetInformationObject(0);

  vtkDataObject* doOutput =
    info->Get(vtkDataObject::DATA_OBJECT());
  vtkMultiBlockDataSet* mb =
    vtkMultiBlockDataSet::SafeDownCast(doOutput);
  if (!mb)
  {
    this->ClearGeometryCache();
    return 0;
  }

  int updateNumPieces = info->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  int igl = info->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  if (updateNumPieces > 1)
  {
    if (igl == 0)
    {
      igl = 1;
    }
    mb->GetInformation()->Set(
      vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), igl);
  }

  if (igl > this->ExecutedGhostLevels)
  {
    this->ClearGeometryCache();
  }

  this->SetErrorCode(vtkErrorCode::NoError);

  // This may be wrong if geometry is not cached. It is
  // updated below.
  int numBlocks = static_cast<int>(this->Internal->Blocks.size());

  vtkSmartPointer<vtkMultiProcessController> mp;
  if (this->Controller)
  {
    mp = this->Controller;
  }
  else
  {
    mp.TakeReference(vtkDummyController::New());
  }

  int rank = mp->GetLocalProcessId();
  int size = mp->GetNumberOfProcesses();
  int realSize = size;

  int* settings = reinterpret_cast<int*>(&this->Internal->Settings);
  mp->Broadcast(settings, sizeof(
    vtkMultiBlockPLOT3DReaderInternals::InternalSettings) / sizeof(int), 0);

  // Special case where we are reading an ASCII or
  // 2D file in parallel. All the work is done by
  // rank 0 but we still need to communicate numBlocks
  // for other ranks to allocate output with the right
  // shape.
  if (!this->Internal->Settings.BinaryFile ||
      this->Internal->Settings.NumberOfDimensions == 2)
  {
    if (rank > 0)
    {
      this->Controller->Broadcast(&numBlocks, 1, 0);
      mb->SetNumberOfBlocks(numBlocks);
      this->ClearGeometryCache();
      return 1;
    }
    else
    {
      mp.TakeReference(vtkDummyController::New());
      rank = 0;
      size = 1;
    }
  }

  vtkNew<vtkExtentTranslator> et;
  et->SetPiece(rank);
  et->SetNumberOfPieces(size);
  et->SetSplitModeToZSlab();

  vtkPlot3DCFile xyzFp(0);

  // Don't read the geometry if we already have it!
  if ( numBlocks == 0 )
  {
    this->ExecutedGhostLevels = igl;

    vtkTypeUInt64 offset = 0;

    int error = 0;

    // Only the first rank does meta-data checking
    // using POSIX IO.
    if (rank == 0)
    {
      try
      {
        if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
        {
          throw Plot3DException();
        }

        if ( this->ReadGeometryHeader(xyzFp) != VTK_OK )
        {
          vtkErrorMacro("Error reading geometry file.");
          throw Plot3DException();
        }

        // Update from the value in the file.
        numBlocks = static_cast<int>(this->Internal->Dimensions.size());

        if (this->Internal->Settings.BinaryFile)
        {
          offset = vtk_ftell(xyzFp);
        }
      }
      catch (Plot3DException&)
      {
        error = 1;
      }
    }

    mp->Broadcast(&error, 1, 0);
    if (error)
    {
      vtkErrorMacro("Error reading file " << this->XYZFileName);
      this->ClearGeometryCache();
      return 0;
    }

    // All meta-data needs to be broadcasted.
    mp->Broadcast(&numBlocks, 1, 0);
    if (rank > 0)
    {
      this->Internal->Dimensions.resize(numBlocks);
    }

    int* rawdims = reinterpret_cast<int*>(&this->Internal->Dimensions[0]);
    mp->Broadcast(rawdims, 3*numBlocks, 0);

    mp->Broadcast(&offset, 1, 0);

    // Heavy reading is done collectively. POSIX in this
    // class but MPI-IO in subclass.
    void* xyzFp2;
    if (this->Internal->Settings.BinaryFile)
    {
      this->OpenFileForDataRead(xyzFp2, this->XYZFileName);
    }
    else
    {
      // For ASCII files, the first rank keeps reading without
      // worrying about offsets and such.
      xyzFp2 = xyzFp;
      xyzFp.DisableClose();
    }

    this->Internal->Blocks.resize(numBlocks);

    for(int i=0; i<numBlocks; i++)
    {
      //**************** RECORD START *********************************
      // precond: offset is at start of a record in the file.
      vtkMultiBlockPLOT3DReaderRecord record;
      if (!record.Initialize(xyzFp, offset, this->Internal->Settings, this->Controller))
      {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the geometry file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(xyzFp2);
        this->ClearGeometryCache();
        return 0;
      }

      // we now have determined how many (sub)records are part of this block.
      assert(record.AtStart(offset));

      offset += this->GetByteCountSize();

      // Read the geometry of this grid.

      int* dims = this->Internal->Dimensions[i].Values;
      int wextent[6] = {0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1};
      int extent[6];
      et->SetWholeExtent(wextent);
      et->SetGhostLevel(igl);
      et->PieceToExtent();
      et->GetExtent(extent);

      vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];
      if (!nthOutput)
      {
        nthOutput = vtkStructuredGrid::New();
        this->Internal->Blocks[i] = nthOutput;
        nthOutput->SetExtent(extent);
        nthOutput->Delete();
      }

      vtkDataArray* pointArray = this->NewFloatArray();
      pointArray->SetNumberOfComponents(3);
      vtkIdType npts = vtkStructuredData::GetNumberOfPoints(extent);
      vtkIdType nTotalPts = (vtkIdType)dims[0]*dims[1]*dims[2];
      pointArray->SetNumberOfTuples(npts);

      vtkPoints* points = vtkPoints::New();
      points->SetData(pointArray);
      pointArray->Delete();
      nthOutput->SetPoints(points);
      points->Delete();
      if ( this->ReadVector(xyzFp2,
                            extent, wextent,
                            this->Internal->Settings.NumberOfDimensions,
                            pointArray,
                            offset,
                            record) == 0)
      {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the geometry file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(xyzFp2);
        this->ClearGeometryCache();
        return 0;
      }
      // Increment the offset for next read. This points to the
      // beginning of next block.
      offset += record.GetLengthWithSeparators(offset,
        this->Internal->Settings.NumberOfDimensions* nTotalPts*this->Internal->Settings.Precision);

      if (this->Internal->Settings.IBlanking)
      {
        vtkIntArray* iblank = vtkIntArray::New();
        iblank->SetName("IBlank");
        iblank->SetNumberOfTuples(npts);
        if ( this->ReadIntScalar(xyzFp2, extent, wextent, iblank, offset, record) == 0)
        {
          vtkErrorMacro("Encountered premature end-of-file while reading "
                        "the xyz file (or the file is corrupt).");
          this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
          this->CloseFile(xyzFp2);
          this->ClearGeometryCache();
          return 0;
        }

        int* ib = iblank->GetPointer(0);
        nthOutput->GetPointData()->AddArray(iblank);
        iblank->Delete();
        offset += record.GetLengthWithSeparators(offset, nTotalPts*sizeof(int));

        vtkUnsignedCharArray *ghosts = vtkUnsignedCharArray::New();
        ghosts->SetNumberOfValues(nthOutput->GetNumberOfCells());
        ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
        vtkIdList* ids = vtkIdList::New();
        ids->SetNumberOfIds(8);
        vtkIdType numCells = nthOutput->GetNumberOfCells();
        for (vtkIdType cellId=0; cellId<numCells; cellId++)
        {
          nthOutput->GetCellPoints(cellId, ids);
          vtkIdType numIds = ids->GetNumberOfIds();
          unsigned char value = 0;
          for (vtkIdType ptIdx=0; ptIdx<numIds; ptIdx++)
          {
            if (ib[ids->GetId(ptIdx)] == 0)
            {
              value |= vtkDataSetAttributes::HIDDENCELL;
              break;
            }
          }
          ghosts->SetValue(cellId, value);
        }
        ids->Delete();
        nthOutput->GetCellData()->AddArray(ghosts);
        ghosts->Delete();
      }

      if (igl > 0)
      {
        et->SetGhostLevel(0);
        et->PieceToExtent();
        int zeroExtent[6];
        et->GetExtent(zeroExtent);
        nthOutput->GenerateGhostArray(zeroExtent, true);
      }

      offset += this->GetByteCountSize();
      assert(record.AtEnd(offset));
      //**************** RECORD END *********************************
    }

    this->CloseFile(xyzFp2);
  }

  // Special case where we are reading an ASCII or
  // 2D file in parallel. All the work is done by
  // rank 0 but we still need to communicate numBlocks
  // for other ranks to allocate output with the right
  // shape.
  if (!this->Internal->Settings.BinaryFile ||
      this->Internal->Settings.NumberOfDimensions == 2)
  {
    if (realSize > 1)
    {
      // This needs to broadcast with this->Controller
      // because mp is a dummy controller.
      this->Controller->Broadcast(&numBlocks, 1, 0);
    }
  }

  // Now read the solution.
  if (this->QFileName && this->QFileName[0] != '\0')
  {
    vtkPlot3DCFile qFp(NULL);
    int nq=0, nqc=0, isOverflow=0;

    int error = 0;
    if (rank == 0)
    {
      try
      {
        if ( this->CheckSolutionFile(qFp) != VTK_OK)
        {
          throw Plot3DException();
        }

        if ( this->ReadQHeader(qFp, true, nq, nqc, isOverflow) != VTK_OK )
        {
          throw Plot3DException();
        }
      }
      catch (Plot3DException&)
      {
        error = 1;
      }
    }

    mp->Broadcast(&error, 1, 0);
    if (error)
    {
      vtkErrorMacro("Error reading file " << this->XYZFileName);
      this->ClearGeometryCache();
      return 0;
    }

    int vals[3] = {nq, nqc, isOverflow};
    mp->Broadcast(vals, 3, 0);
    nq = vals[0];
    nqc = vals[1];
    isOverflow = vals[2];

    vtkTypeUInt64 offset = 0;

    void* qFp2;
    if (this->Internal->Settings.BinaryFile)
    {
      this->OpenFileForDataRead(qFp2, this->QFileName);
    }
    else
    {
      // We treat ASCII specially. We don't worry about
      // offsets and let the file move forward while reading
      // from the original file handle.
      qFp2 = qFp;
      qFp.DisableClose();
    }

    for(int i=0; i<numBlocks; i++)
    {
      vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];

      // Save the properties first
      vtkDataArray* properties = this->NewFloatArray();
      properties->SetName("Properties");

      int numProperties = 4;
      if (rank == 0)
      {
        int count = this->SkipByteCount(qFp);
        // We have a byte count to tell us how many Q values to
        // read. If this is more that 4, this is probably an Overflow
        // file.
        if (isOverflow)
        {
          // We take 4 bytes because there is an int there that
          // we will throw away
          numProperties = (count-4) / this->Internal->Settings.Precision + 1;
        }
      }
      mp->Broadcast(&numProperties, 1, 0);
      properties->SetNumberOfTuples(numProperties);

      error = 0;
      if (rank == 0)
      {
        try
        {
          // Read fsmach, alpha, re, time;
          if ( this->ReadValues(qFp, 4, properties) != 4)
          {
            vtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
            properties->Delete();
            throw Plot3DException();
          }

          if (isOverflow)
          {
            // We create a dummy array to use with ReadValues
            vtkDataArray* dummyArray = properties->NewInstance();
            dummyArray->SetVoidArray(properties->GetVoidPointer(4), 3, 1);

            // Read GAMINF, BETA, TINF
            if ( this->ReadValues(qFp, 3, dummyArray) != 3)
            {
              vtkErrorMacro("Encountered premature end-of-file while reading "
                            "the q file (or the file is corrupt).");
              this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
              properties->Delete();
              throw Plot3DException();
            }

            // igam is an int
            int igam;
            this->ReadIntBlock(qFp, 1, &igam);
            properties->SetTuple1(7, igam);

            dummyArray->SetVoidArray(properties->GetVoidPointer(8), 3, 1);
            // Read the rest of properties
            if ( this->ReadValues(qFp, numProperties - 8, dummyArray) !=
                 numProperties - 8)
            {
              vtkErrorMacro("Encountered premature end-of-file while reading "
                            "the q file (or the file is corrupt).");
              this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
              properties->Delete();
              throw Plot3DException();
            }
            dummyArray->Delete();
          }
          this->SkipByteCount(qFp);
        }
        catch (Plot3DException&)
        {
          error = 1;
        }
      }
      mp->Broadcast(&error, 1, 0);
      if (error)
      {
        vtkErrorMacro("Error reading file " << this->XYZFileName);
        this->ClearGeometryCache();
        return 0;
      }

      mp->Broadcast(properties, 0);

      nthOutput->GetFieldData()->AddArray(properties);
      properties->Delete();

      if (mp->GetLocalProcessId() == 0 &&
          this->Internal->Settings.BinaryFile)
      {
        offset = vtk_ftell(qFp);
      }
      mp->Broadcast(&offset, 1, 0);

      int* dims = this->Internal->Dimensions[i].Values;
      int wextent[6] = {0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1};
      int extent[6];
      et->SetWholeExtent(wextent);
      et->SetGhostLevel(igl);
      et->PieceToExtent();
      et->GetExtent(extent);

      int ldims[3];
      vtkStructuredData::GetDimensionsFromExtent(extent, ldims);

      vtkIdType npts = vtkStructuredData::GetNumberOfPoints(extent);
      vtkIdType nTotalPts = (vtkIdType)dims[0]*dims[1]*dims[2];

      //**************** RECORD START *********************************
      // precond: offset is at start of a record in the file.
      vtkMultiBlockPLOT3DReaderRecord record;
      if (!record.Initialize(qFp, offset, this->Internal->Settings, this->Controller))
      {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(qFp2);
        this->ClearGeometryCache();
        return 0;
      }

      // we now have determined how many (sub)records are part of this block.
      assert(record.AtStart(offset));

      offset += this->GetByteCountSize();

      vtkDataArray* density = this->NewFloatArray();
      density->SetNumberOfComponents(1);
      density->SetNumberOfTuples( npts );
      density->SetName("Density");
      if ( this->ReadScalar(qFp2, extent, wextent, density, offset, record) == 0)
      {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(qFp2);
        density->Delete();
        this->ClearGeometryCache();
        return 0;
      }
      nthOutput->GetPointData()->AddArray(density);
      density->Delete();
      offset += record.GetLengthWithSeparators(offset, nTotalPts*this->Internal->Settings.Precision);

      vtkDataArray* momentum = this->NewFloatArray();
      momentum->SetNumberOfComponents(3);
      momentum->SetNumberOfTuples( npts );
      momentum->SetName("Momentum");
      if ( this->ReadVector(qFp2,
                            extent, wextent,
                            this->Internal->Settings.NumberOfDimensions,
                            momentum,
                            offset,
                            record) == 0)
      {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        this->CloseFile(qFp2);
        momentum->Delete();
        this->ClearGeometryCache();
        return 0;
      }
      nthOutput->GetPointData()->AddArray(momentum);
      momentum->Delete();
      offset += record.GetLengthWithSeparators(offset,
        this->Internal->Settings.NumberOfDimensions* nTotalPts*this->Internal->Settings.Precision);

      vtkDataArray* se = this->NewFloatArray();
      se->SetNumberOfComponents(1);
      se->SetNumberOfTuples( npts );
      se->SetName("StagnationEnergy");
      if ( this->ReadScalar(qFp2, extent, wextent, se, offset, record) == 0)
      {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->CloseFile(qFp2);
        se->Delete();
        this->ClearGeometryCache();
        return 0;
      }
      nthOutput->GetPointData()->AddArray(se);
      se->Delete();
      offset += record.GetLengthWithSeparators(offset, nTotalPts*this->Internal->Settings.Precision);

      if (isOverflow)
      {
        if(nq >= 6) /// super new
        {
          vtkDataArray* gamma = this->NewFloatArray();
          gamma->SetNumberOfComponents(1);
          gamma->SetNumberOfTuples(npts);
          gamma->SetName("Gamma");
          if (this->ReadScalar(qFp2, extent, wextent, gamma, offset, record) == 0)
          {
            vtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->CloseFile(qFp2);
            gamma->Delete();
            this->ClearGeometryCache();
            return 0;
          }
          nthOutput->GetPointData()->AddArray(gamma);
          gamma->Delete();
          offset += record.GetLengthWithSeparators(offset, nTotalPts*this->Internal->Settings.Precision);
        } /// end of new

        char res[100];
        // Read species and turbulence variables for overflow q files
        for(int j=0; j<nqc; j++)
        {
          vtkDataArray* temp = this->NewFloatArray();
          temp->SetNumberOfComponents(1);
          temp->SetNumberOfTuples(npts);
          int k = j+1;
          sprintf(res, "Species Density #%d", k);
          temp->SetName(res);
          if (this->ReadScalar(qFp2, extent, wextent, temp, offset, record) == 0)
          {
            vtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->CloseFile(qFp2);
            temp->Delete();
            this->ClearGeometryCache();
            return 0;
          }
          nthOutput->GetPointData()->AddArray(temp);
          offset += record.GetLengthWithSeparators(offset, nTotalPts*this->Internal->Settings.Precision);
          temp->Delete();
        }
        float d, r;
        for(int v=0; v<nqc; v++)
        {
          vtkDataArray* rat = this->NewFloatArray();
          sprintf(res, "Species Density #%d", v+1);
          vtkPointData* outputPD = nthOutput->GetPointData();
          vtkDataArray* spec = outputPD->GetArray(res);
          vtkDataArray* dens = outputPD->GetArray("Density");
          rat->SetNumberOfComponents(1);
          rat->SetNumberOfTuples(ldims[0]*ldims[1]*ldims[2]);
          sprintf(res, "Spec Dens #%d / rho", v+1);
          rat->SetName(res);
          for(int w=0; w<npts; w++)
          {
            r = dens->GetComponent(w,0);
            r = (r != 0.0 ? r : 1.0);
            d = spec->GetComponent(w,0);
            rat->SetTuple1(w, d/r);
          }
          nthOutput->GetPointData()->AddArray(rat);
          rat->Delete();
        }
        for(int a=0; a<nq-6-nqc; a++)
        {
          vtkDataArray* temp = this->NewFloatArray();
          temp->SetNumberOfComponents(1);
          temp->SetNumberOfTuples(ldims[0]*ldims[1]*ldims[2]);
          int k = a+1;
          sprintf(res, "Turb Field Quant #%d", k);
          temp->SetName(res);
          if (this->ReadScalar(qFp2, extent, wextent, temp, offset, record) == 0)
          {
            vtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            this->CloseFile(qFp2);
            temp->Delete();
            this->ClearGeometryCache();
            return 0;
          }
          nthOutput->GetPointData()->AddArray(temp);
          offset += record.GetLengthWithSeparators(offset, nTotalPts*this->Internal->Settings.Precision);
          temp->Delete();
        }
      }

      offset += this->GetByteCountSize();
      assert(record.AtEnd(offset));
      //**************** RECORD END *********************************

      if (rank == 0 && this->Internal->Settings.BinaryFile)
      {
        vtk_fseek(qFp, offset, SEEK_SET);
      }

      if ( this->FunctionList->GetNumberOfTuples() > 0 )
      {
        int fnum;
        for (int tup=0; tup < this->FunctionList->GetNumberOfTuples(); tup++)
        {
          if ( (fnum=this->FunctionList->GetValue(tup)) >= 0 )
          {
            this->MapFunction(fnum, nthOutput);
          }
        }
      }
      this->AssignAttribute(this->ScalarFunctionNumber, nthOutput,
                            vtkDataSetAttributes::SCALARS);
      this->AssignAttribute(this->VectorFunctionNumber, nthOutput,
                            vtkDataSetAttributes::VECTORS);
    }
    this->CloseFile(qFp2);
  }

  // Now read the functions.
  if (this->FunctionFileName && this->FunctionFileName[0] != '\0')
  {
    vtkTypeUInt64 offset = 0;

    vtkPlot3DCFile fFp(NULL);

    std::vector<int> nFunctions(numBlocks);
    int error = 0;
    if (rank == 0)
    {
      try
      {
        if ( this->CheckFunctionFile(fFp) != VTK_OK)
        {
          throw Plot3DException();
        }

        if ( this->ReadFunctionHeader(fFp, &nFunctions[0]) != VTK_OK )
        {
          throw Plot3DException();
        }
        offset = vtk_ftell(fFp);
      }
      catch (Plot3DException&)
      {
        error = 1;
      }
    }
    mp->Broadcast(&error, 1, 0);
    if (error)
    {
      vtkErrorMacro("Error reading file " << this->XYZFileName);
      this->ClearGeometryCache();
      return 0;
    }

    mp->Broadcast(&nFunctions[0], numBlocks, 0);
    mp->Broadcast(&offset, 1, 0);


    void* fFp2;
    if (this->Internal->Settings.BinaryFile)
    {
      this->OpenFileForDataRead(fFp2, this->FunctionFileName);
    }
    else
    {
      fFp2 = fFp;
      fFp.DisableClose();
    }

    for(int i=0; i<numBlocks; i++)
    {
      vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];

      int* dims = this->Internal->Dimensions[i].Values;
      int wextent[6] = {0, dims[0]-1, 0, dims[1]-1, 0, dims[2]-1};
      int extent[6];
      et->SetWholeExtent(wextent);
      et->SetSplitModeToZSlab();
      et->PieceToExtent();
      et->GetExtent(extent);

      vtkIdType npts = vtkStructuredData::GetNumberOfPoints(extent);
      vtkIdType nTotalPts = (vtkIdType)dims[0]*dims[1]*dims[2];

      //**************** RECORD START *********************************
      // precond: offset is at start of a record in the file.
      vtkMultiBlockPLOT3DReaderRecord record;
      if (!record.Initialize(fFp, offset, this->Internal->Settings, this->Controller))
      {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the function file (or the file is corrupt).");
        this->CloseFile(fFp2);
        this->ClearGeometryCache();
        return 0;
      }

      // we now have determined how many (sub)records are part of this block.
      assert(record.AtStart(offset));

      offset += this->GetByteCountSize();

      for (int j=0; j<nFunctions[i]; j++)
      {
        vtkDataArray* functionArray = this->NewFloatArray();
        functionArray->SetNumberOfTuples(npts);
        char functionName[20];
        sprintf(functionName, "Function%d", j);
        functionArray->SetName(functionName);
        if (this->ReadScalar(fFp2, extent, wextent, functionArray, offset, record) == 0)
        {
          vtkErrorMacro("Encountered premature end-of-file while reading "
                        "the function file (or the file is corrupt).");
          this->CloseFile(fFp2);
          functionArray->Delete();
          this->ClearGeometryCache();
          return 0;
        }
        offset += record.GetLengthWithSeparators(offset, nTotalPts*this->Internal->Settings.Precision);
        nthOutput->GetPointData()->AddArray(functionArray);
        functionArray->Delete();
      }

      offset += this->GetByteCountSize();
      assert(record.AtEnd(offset));
      //**************** RECORD END *********************************
    }
    this->CloseFile(fFp2);
  }

  mb->SetNumberOfBlocks(numBlocks);
  for(int i=0; i<numBlocks; i++)
  {
    vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];
    mb->SetBlock(i, nthOutput);
  }

  return 1;
}

// Various PLOT3D functions.....................
void vtkMultiBlockPLOT3DReader::MapFunction(int fNumber, vtkStructuredGrid* output)
{
  switch (fNumber)
  {
    case 100: //Density
      break;

    case 110: //Pressure
      this->ComputePressure(output);
      break;

    case 111: // Pressure Coefficient
      this->ComputePressureCoefficient(output);
      break;

    case 112: // Mach Number
      this->ComputeMachNumber(output);
      break;

    case 113: // Sound Speed
      this->ComputeSoundSpeed(output);
      break;

    case 120: //Temperature
      this->ComputeTemperature(output);
      break;

    case 130: //Enthalpy
      this->ComputeEnthalpy(output);
      break;

    case 140: //Internal Energy
      break;

    case 144: //Kinetic Energy
      this->ComputeKineticEnergy(output);
      break;

    case 153: //Velocity Magnitude
      this->ComputeVelocityMagnitude(output);
      break;

    case 163: //Stagnation energy
      break;

    case 170: //Entropy
      this->ComputeEntropy(output);
      break;

    case 184: //Swirl
      this->ComputeSwirl(output);
      break;

    case 200: //Velocity
      this->ComputeVelocity(output);
      break;

    case 201: //Vorticity
      this->ComputeVorticity(output);
      break;

    case 202: //Momentum
      break;

    case 210: //PressureGradient
      this->ComputePressureGradient(output);
      break;

    case 211: // Vorticity Magnitude
      this->ComputeVorticityMagnitude(output);
      break;

    case 212: // Strain Rate
      this->ComputeStrainRate(output);
      break;

    default:
      vtkErrorMacro(<<"No function number " << fNumber);
  }
}

void vtkMultiBlockPLOT3DReader::AssignAttribute(int fNumber, vtkStructuredGrid* output,
                                  int attributeType)
{
  switch (fNumber)
  {
    case -1:  //empty mapping
      output->GetPointData()->SetActiveAttribute(0,
                                                 attributeType);
      break;

    case 100: //Density
      output->GetPointData()->SetActiveAttribute("Density",
                                                 attributeType);
      break;

    case 110: //Pressure
      output->GetPointData()->SetActiveAttribute("Pressure",
                                                 attributeType);
      break;

    case 120: //Temperature
      output->GetPointData()->SetActiveAttribute("Temperature",
                                                 attributeType);
      break;

    case 130: //Enthalpy
      output->GetPointData()->SetActiveAttribute("Enthalpy",
                                                 attributeType);
      break;

    case 140: //Internal Energy
      output->GetPointData()->SetActiveAttribute("StagnationEnergy",
                                                 attributeType);
      break;

    case 144: //Kinetic Energy
      output->GetPointData()->SetActiveAttribute("KineticEnergy",
                                                 attributeType);
      break;

    case 153: //Velocity Magnitude
      output->GetPointData()->SetActiveAttribute("VelocityMagnitude",
                                                 attributeType);
      break;

    case 163: //Stagnation energy
      output->GetPointData()->SetActiveAttribute("StagnationEnergy",
                                                 attributeType);
      break;

    case 170: //Entropy
      output->GetPointData()->SetActiveAttribute("Entropy",
                                                 attributeType);
      break;

    case 184: //Swirl
      output->GetPointData()->SetActiveAttribute("Swirl",
                                                 attributeType);
      break;

    case 200: //Velocity
      output->GetPointData()->SetActiveAttribute("Velocity",
                                                 attributeType);
      break;

    case 201: //Vorticity
      output->GetPointData()->SetActiveAttribute("Vorticity",
                                                 attributeType);
      break;

    case 202: //Momentum
      output->GetPointData()->SetActiveAttribute("Momentum",
                                                 attributeType);
      break;

    case 210: //PressureGradient
      output->GetPointData()->SetActiveAttribute("PressureGradient",
                                                 attributeType);
      break;

    default:
      vtkErrorMacro(<<"No function number " << fNumber);
  }
}

void vtkMultiBlockPLOT3DReader::ComputeTemperature(vtkStructuredGrid* output)
{
  double *m, e, rr, u, v, w, v2, p, d, rrgas;
  vtkIdType i;
  vtkDataArray *temperature;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");

  if ( density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute temperature");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  temperature = this->NewFloatArray();
  temperature->SetNumberOfTuples(numPts);

  //  Compute the temperature
  //
  rrgas = 1.0 / this->R;
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.) * (e - 0.5 * d * v2);
    temperature->SetTuple1(i, p*rr*rrgas);
  }

  temperature->SetName("Temperature");
  outputPD->AddArray(temperature);

  temperature->Delete();
  vtkDebugMacro(<<"Created temperature scalar");
}

void vtkMultiBlockPLOT3DReader::ComputePressure(vtkStructuredGrid* output)
{
  double *m, e, u, v, w, v2, p, d, rr;
  vtkIdType i;
  vtkDataArray *pressure;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute pressure");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  pressure = this->NewFloatArray();
  pressure->SetNumberOfTuples(numPts);

  //  Compute the pressure
  //
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.) * (e - 0.5 * d * v2);
    pressure->SetTuple1(i, p);
  }

  pressure->SetName("Pressure");
  outputPD->AddArray(pressure);
  pressure->Delete();
  vtkDebugMacro(<<"Created pressure scalar");
}

void vtkMultiBlockPLOT3DReader::ComputeEnthalpy(vtkStructuredGrid* output)
{
  double *m, e, u, v, w, v2, d, rr;
  vtkIdType i;
  vtkDataArray *enthalpy;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute enthalpy");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  enthalpy = this->NewFloatArray();
  enthalpy->SetNumberOfTuples(numPts);

  //  Compute the enthalpy
  //
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    enthalpy->SetTuple1(i, this->Gamma*(e*rr - 0.5*v2));
  }
  enthalpy->SetName("Enthalpy");
  outputPD->AddArray(enthalpy);
  enthalpy->Delete();
  vtkDebugMacro(<<"Created enthalpy scalar");
}

void vtkMultiBlockPLOT3DReader::ComputeKineticEnergy(vtkStructuredGrid* output)
{
  double *m, u, v, w, v2, d, rr;
  vtkIdType i;
  vtkDataArray *kineticEnergy;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  if ( density == NULL || momentum == NULL )
  {
    vtkErrorMacro(<<"Cannot compute kinetic energy");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  kineticEnergy = this->NewFloatArray();
  kineticEnergy->SetNumberOfTuples(numPts);

  //  Compute the kinetic energy
  //
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    kineticEnergy->SetTuple1(i, 0.5*v2);
  }
  kineticEnergy->SetName("KineticEnergy");
  outputPD->AddArray(kineticEnergy);
  kineticEnergy->Delete();
  vtkDebugMacro(<<"Created kinetic energy scalar");
}

void vtkMultiBlockPLOT3DReader::ComputeVelocityMagnitude(vtkStructuredGrid* output)
{
  double *m, u, v, w, v2, d, rr;
  vtkIdType i;
  vtkDataArray *velocityMag;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute velocity magnitude");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  velocityMag = this->NewFloatArray();
  velocityMag->SetNumberOfTuples(numPts);

  //  Compute the velocity magnitude
  //
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    velocityMag->SetTuple1(i, sqrt((double)v2));
  }
  velocityMag->SetName("VelocityMagnitude");
  outputPD->AddArray(velocityMag);
  velocityMag->Delete();
  vtkDebugMacro(<<"Created velocity magnitude scalar");
}

void vtkMultiBlockPLOT3DReader::ComputeEntropy(vtkStructuredGrid* output)
{
  double *m, u, v, w, v2, d, rr, s, p, e;
  vtkIdType i;
  vtkDataArray *entropy;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute entropy");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  entropy = this->NewFloatArray();
  entropy->SetNumberOfTuples(numPts);

  //  Compute the entropy
  //
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    p = (this->Gamma-1.)*(e - 0.5*d*v2);
    s = VTK_CV * log((p/VTK_PINF)/pow((double)d/VTK_RHOINF,(double)this->Gamma));
    entropy->SetTuple1(i,s);
  }
  entropy->SetName("Entropy");
  outputPD->AddArray(entropy);
  entropy->Delete();
  vtkDebugMacro(<<"Created entropy scalar");
}

void vtkMultiBlockPLOT3DReader::ComputeSwirl(vtkStructuredGrid* output)
{
  vtkDataArray *vorticity;
  double d, rr, *m, u, v, w, v2, *vort, s;
  vtkIdType i;
  vtkDataArray *swirl;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute swirl");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  swirl = this->NewFloatArray();
  swirl->SetNumberOfTuples(numPts);

  this->ComputeVorticity(output);
  vorticity = outputPD->GetArray("Vorticity");
//
//  Compute the swirl
//
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    vort = vorticity->GetTuple(i);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    if ( v2 != 0.0 )
    {
      s = (vort[0]*m[0] + vort[1]*m[1] + vort[2]*m[2]) / v2;
    }
    else
    {
      s = 0.0;
    }

    swirl->SetTuple1(i,s);
  }
  swirl->SetName("Swirl");
  outputPD->AddArray(swirl);
  swirl->Delete();
  vtkDebugMacro(<<"Created swirl scalar");

}

// Vector functions
void vtkMultiBlockPLOT3DReader::ComputeVelocity(vtkStructuredGrid* output)
{
  double *m, v[3], d, rr;
  vtkIdType i;
  vtkDataArray *velocity;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute velocity");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  velocity = this->NewFloatArray();
  velocity->SetNumberOfComponents(3);
  velocity->SetNumberOfTuples(numPts);

  //  Compute the velocity
  //
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    rr = 1.0 / d;
    v[0] = m[0] * rr;
    v[1] = m[1] * rr;
    v[2] = m[2] * rr;
    velocity->SetTuple(i, v);
  }
  velocity->SetName("Velocity");
  outputPD->AddArray(velocity);
  velocity->Delete();
  vtkDebugMacro(<<"Created velocity vector");
}

void vtkMultiBlockPLOT3DReader::ComputeVorticity(vtkStructuredGrid* output)
{
  vtkDataArray *velocity;
  vtkDataArray *vorticity;
  int dims[3], ijsize;
  vtkPoints *points;
  int i, j, k, idx, idx2, ii;
  double vort[3], xp[3], xm[3], vp[3], vm[3], factor;
  double xxi, yxi, zxi, uxi, vxi, wxi;
  double xeta, yeta, zeta, ueta, veta, weta;
  double xzeta, yzeta, zzeta, uzeta, vzeta, wzeta;
  double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( (points=output->GetPoints()) == NULL ||
       density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute vorticity");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  vorticity = this->NewFloatArray();
  vorticity->SetNumberOfComponents(3);
  vorticity->SetNumberOfTuples(numPts);

  this->ComputeVelocity(output);
  velocity = outputPD->GetArray("Velocity");

  output->GetDimensions(dims);
  ijsize = dims[0]*dims[1];

  for (k=0; k<dims[2]; k++)
  {
    for (j=0; j<dims[1]; j++)
    {
      for (i=0; i<dims[0]; i++)
      {
        //  Xi derivatives.
        if ( dims[0] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
          }
          xp[0] = 1.0;
        }
        else if ( i == 0 )
        {
          factor = 1.0;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else if ( i == (dims[0]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i-1 + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else
        {
          factor = 0.5;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = (i-1) + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }

        xxi = factor * (xp[0] - xm[0]);
        yxi = factor * (xp[1] - xm[1]);
        zxi = factor * (xp[2] - xm[2]);
        uxi = factor * (vp[0] - vm[0]);
        vxi = factor * (vp[1] - vm[1]);
        wxi = factor * (vp[2] - vm[2]);

        //  Eta derivatives.
        if ( dims[1] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
          }
          xp[1] = 1.0;
        }
        else if ( j == 0 )
        {
          factor = 1.0;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else if ( j == (dims[1]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else
        {
          factor = 0.5;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }


        xeta = factor * (xp[0] - xm[0]);
        yeta = factor * (xp[1] - xm[1]);
        zeta = factor * (xp[2] - xm[2]);
        ueta = factor * (vp[0] - vm[0]);
        veta = factor * (vp[1] - vm[1]);
        weta = factor * (vp[2] - vm[2]);

        //  Zeta derivatives.
        if ( dims[2] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
          }
          xp[2] = 1.0;
        }
        else if ( k == 0 )
        {
          factor = 1.0;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else if ( k == (dims[2]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else
        {
          factor = 0.5;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }

        xzeta = factor * (xp[0] - xm[0]);
        yzeta = factor * (xp[1] - xm[1]);
        zzeta = factor * (xp[2] - xm[2]);
        uzeta = factor * (vp[0] - vm[0]);
        vzeta = factor * (vp[1] - vm[1]);
        wzeta = factor * (vp[2] - vm[2]);

        // Now calculate the Jacobian.  Grids occasionally have
        // singularities, or points where the Jacobian is infinite (the
        // inverse is zero).  For these cases, we'll set the Jacobian to
        // zero, which will result in a zero vorticity.
        //
        aj =  xxi*yeta*zzeta+yxi*zeta*xzeta+zxi*xeta*yzeta
              -zxi*yeta*xzeta-yxi*xeta*zzeta-xxi*zeta*yzeta;
        if (aj != 0.0)
        {
          aj = 1. / aj;
        }

        //  Xi metrics.
        xix  =  aj*(yeta*zzeta-zeta*yzeta);
        xiy  = -aj*(xeta*zzeta-zeta*xzeta);
        xiz  =  aj*(xeta*yzeta-yeta*xzeta);

        //  Eta metrics.
        etax = -aj*(yxi*zzeta-zxi*yzeta);
        etay =  aj*(xxi*zzeta-zxi*xzeta);
        etaz = -aj*(xxi*yzeta-yxi*xzeta);

        //  Zeta metrics.
        zetax=  aj*(yxi*zeta-zxi*yeta);
        zetay= -aj*(xxi*zeta-zxi*xeta);
        zetaz=  aj*(xxi*yeta-yxi*xeta);

        //  Finally, the vorticity components.
        //
        vort[0]= xiy*wxi+etay*weta+zetay*wzeta - xiz*vxi-etaz*veta-zetaz*vzeta;
        vort[1]= xiz*uxi+etaz*ueta+zetaz*uzeta - xix*wxi-etax*weta-zetax*wzeta;
        vort[2]= xix*vxi+etax*veta+zetax*vzeta - xiy*uxi-etay*ueta-zetay*uzeta;
        idx = i + j*dims[0] + k*ijsize;
        vorticity->SetTuple(idx,vort);
      }
    }
  }
  vorticity->SetName("Vorticity");
  outputPD->AddArray(vorticity);
  vorticity->Delete();
  vtkDebugMacro(<<"Created vorticity vector");
}

void vtkMultiBlockPLOT3DReader::ComputePressureGradient(vtkStructuredGrid* output)
{
  vtkDataArray *pressure;
  vtkDataArray *gradient;
  int dims[3], ijsize;
  vtkPoints *points;
  int i, j, k, idx, idx2, ii;
  double g[3], xp[3], xm[3], pp, pm, factor;
  double xxi, yxi, zxi, pxi;
  double xeta, yeta, zeta, peta;
  double xzeta, yzeta, zzeta, pzeta;
  double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  if ( (points=output->GetPoints()) == NULL ||
       density == NULL || momentum == NULL ||
       energy == NULL )
  {
    vtkErrorMacro(<<"Cannot compute pressure gradient");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  gradient = this->NewFloatArray();
  gradient->SetNumberOfComponents(3);
  gradient->SetNumberOfTuples(numPts);

  this->ComputePressure(output);
  pressure = outputPD->GetArray("Pressure");

  output->GetDimensions(dims);
  ijsize = dims[0]*dims[1];

  for (k=0; k<dims[2]; k++)
  {
    for (j=0; j<dims[1]; j++)
    {
      for (i=0; i<dims[0]; i++)
      {
        //  Xi derivatives.
        if ( dims[0] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            xp[ii] = xm[ii] = 0.0;
          }
          xp[0] = 1.0; pp = pm = 0.0;
        }
        else if ( i == 0 )
        {
          factor = 1.0;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }
        else if ( i == (dims[0]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i-1 + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }
        else
        {
          factor = 0.5;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = (i-1) + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }

        xxi = factor * (xp[0] - xm[0]);
        yxi = factor * (xp[1] - xm[1]);
        zxi = factor * (xp[2] - xm[2]);
        pxi = factor * (pp - pm);

        //  Eta derivatives.
        if ( dims[1] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            xp[ii] = xm[ii] = 0.0;
          }
          xp[1] = 1.0; pp = pm = 0.0;
        }
        else if ( j == 0 )
        {
          factor = 1.0;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }
        else if ( j == (dims[1]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }
        else
        {
          factor = 0.5;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }

        xeta = factor * (xp[0] - xm[0]);
        yeta = factor * (xp[1] - xm[1]);
        zeta = factor * (xp[2] - xm[2]);
        peta = factor * (pp - pm);

        //  Zeta derivatives.
        if ( dims[2] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            xp[ii] = xm[ii] = 0.0;
          }
          xp[2] = 1.0; pp = pm = 0.0;
        }
        else if ( k == 0 )
        {
          factor = 1.0;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }
        else if ( k == (dims[2]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }
        else
        {
          factor = 0.5;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          points->GetPoint(idx,xp);
          points->GetPoint(idx2,xm);
          pp = pressure->GetComponent(idx,0);
          pm = pressure->GetComponent(idx2,0);
        }

        xzeta = factor * (xp[0] - xm[0]);
        yzeta = factor * (xp[1] - xm[1]);
        zzeta = factor * (xp[2] - xm[2]);
        pzeta = factor * (pp - pm);

        //  Now calculate the Jacobian.  Grids occasionally have
        //  singularities, or points where the Jacobian is infinite (the
        //  inverse is zero).  For these cases, we'll set the Jacobian to
        //  zero, which will result in a zero vorticity.
        //
        aj =  xxi*yeta*zzeta+yxi*zeta*xzeta+zxi*xeta*yzeta
              -zxi*yeta*xzeta-yxi*xeta*zzeta-xxi*zeta*yzeta;
        if (aj != 0.0)
        {
          aj = 1. / aj;
        }

        //  Xi metrics.
        xix  =  aj*(yeta*zzeta-zeta*yzeta);
        xiy  = -aj*(xeta*zzeta-zeta*xzeta);
        xiz  =  aj*(xeta*yzeta-yeta*xzeta);

        //  Eta metrics.
        etax = -aj*(yxi*zzeta-zxi*yzeta);
        etay =  aj*(xxi*zzeta-zxi*xzeta);
        etaz = -aj*(xxi*yzeta-yxi*xzeta);

        //  Zeta metrics.
        zetax=  aj*(yxi*zeta-zxi*yeta);
        zetay= -aj*(xxi*zeta-zxi*xeta);
        zetaz=  aj*(xxi*yeta-yxi*xeta);

        //  Finally, the vorticity components.
        g[0]= xix*pxi+etax*peta+zetax*pzeta;
        g[1]= xiy*pxi+etay*peta+zetay*pzeta;
        g[2]= xiz*pxi+etaz*peta+zetaz*pzeta;

        idx = i + j*dims[0] + k*ijsize;
        gradient->SetTuple(idx,g);
      }
    }
  }
  gradient->SetName("PressureGradient");
  outputPD->AddArray(gradient);
  gradient->Delete();
  vtkDebugMacro(<<"Created pressure gradient vector");
}

void vtkMultiBlockPLOT3DReader::ComputePressureCoefficient(vtkStructuredGrid* output)
{
  double *m, e, u, v, w, v2, p, d, g, rr, pc, gi, pi, fsm, den;
  vtkIdType i;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  vtkFieldData* outputFD = output->GetFieldData();
  // It's already computed
  if (outputPD->GetArray("PressureCoefficient"))
  {
    return;
  }
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  vtkDataArray* gamma = outputPD->GetArray("Gamma");
  vtkDataArray* props = outputFD->GetArray("Properties");
  if ( density == NULL || momentum == NULL ||
       energy == NULL  || gamma == NULL || props == NULL)
  {
    vtkErrorMacro(<<"Cannot compute pressure coefficient");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  vtkDataArray* pressure_coeff = this->NewFloatArray();
  pressure_coeff->SetNumberOfTuples(numPts);
  //  Compute the pressure coefficient
  //
  gi = props->GetComponent(0,4);
  fsm = props->GetComponent(0,0);
  den = .5*fsm*fsm;
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
    g = gamma->GetComponent(i,0);
    pi = 1.0 / gi;
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    p = (g-1.) * (e - 0.5 * d * v2);
    pc = (p - pi)/den;
    pressure_coeff->SetTuple1(i, pc);
  }

  pressure_coeff->SetName("PressureCoefficient");
  outputPD->AddArray(pressure_coeff);
  pressure_coeff->Delete();
  vtkDebugMacro(<<"Created pressure coefficient scalar");
}

void vtkMultiBlockPLOT3DReader::ComputeMachNumber(vtkStructuredGrid* output)
{
  double *m, e, u, v, w, v2, a2, d, g, rr;
  vtkIdType i;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  // It's already computed
  if (outputPD->GetArray("MachNumber"))
  {
    return;
  }
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  vtkDataArray* gamma = outputPD->GetArray("Gamma");
  if ( density == NULL || momentum == NULL ||
       energy == NULL  || gamma == NULL)
  {
    vtkErrorMacro(<<"Cannot compute mach number");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  vtkDataArray* machnumber = this->NewFloatArray();
  machnumber->SetNumberOfTuples(numPts);

  //  Compute the mach number
  //
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
    g = gamma->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    a2 = g * (g-1.) * (e * rr - .5*v2);
    machnumber->SetTuple1(i, sqrt(v2/a2));
  }

  machnumber->SetName("MachNumber");
  outputPD->AddArray(machnumber);
  machnumber->Delete();
  vtkDebugMacro(<<"Created mach number scalar");
}

void vtkMultiBlockPLOT3DReader::ComputeSoundSpeed(vtkStructuredGrid* output)
{
  double *m, e, u, v, w, v2, p, d, g, rr;
  vtkIdType i;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  // It's already computed
  if (outputPD->GetArray("SoundSpeed"))
  {
    return;
  }
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  vtkDataArray* energy = outputPD->GetArray("StagnationEnergy");
  vtkDataArray* gamma = outputPD->GetArray("Gamma");
  if ( density == NULL || momentum == NULL ||
       energy == NULL  || gamma == NULL)
  {
    vtkErrorMacro(<<"Cannot compute sound speed");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  vtkDataArray* soundspeed = this->NewFloatArray();
  soundspeed->SetNumberOfTuples(numPts);

  //  Compute sound speed
  //
  for (i=0; i < numPts; i++)
  {
    d = density->GetComponent(i,0);
    d = (d != 0.0 ? d : 1.0);
    m = momentum->GetTuple(i);
    e = energy->GetComponent(i,0);
    g = gamma->GetComponent(i,0);
    rr = 1.0 / d;
    u = m[0] * rr;
    v = m[1] * rr;
    w = m[2] * rr;
    v2 = u*u + v*v + w*w;
    p = (g-1.) * (e - 0.5 * d * v2);
    soundspeed->SetTuple1(i, sqrt(g*p*rr));
  }

  soundspeed->SetName("SoundSpeed");
  outputPD->AddArray(soundspeed);
  soundspeed->Delete();
  vtkDebugMacro(<<"Created sound speed scalar");
}

void vtkMultiBlockPLOT3DReader::ComputeVorticityMagnitude(vtkStructuredGrid* output)
{
  vtkPointData* outputPD = output->GetPointData();
  // It's already computed
  if (outputPD->GetArray("VorticityMagnitude"))
  {
    return;
  }
  this->ComputeVorticity(output);
  vtkDataArray* vorticity = outputPD->GetArray("Vorticity");
  vtkDataArray* vm = this->NewFloatArray();
  vtkIdType numPts = vorticity->GetNumberOfTuples();
  vm->SetNumberOfTuples(numPts);
  for (vtkIdType idx=0; idx<numPts; idx++)
  {
    double* vort = vorticity->GetTuple(idx);
    double magnitude = sqrt(vort[0]*vort[0]+
                            vort[1]*vort[1]+vort[2]*vort[2]);
    vm->SetTuple1(idx, magnitude);
  }
  vm->SetName("VorticityMagnitude");
  outputPD->AddArray(vm);
  vm->Delete();
}

void vtkMultiBlockPLOT3DReader::ComputeStrainRate(vtkStructuredGrid* output)
{
  vtkDataArray *velocity;
  int dims[3], ijsize;
  int i, j, k, idx, idx2, ii;
  double stRate[3], xp[3], xm[3], vp[3], vm[3], factor;
  double xxi, yxi, zxi, uxi, vxi, wxi;
  double xeta, yeta, zeta, ueta, veta, weta;
  double xzeta, yzeta, zzeta, uzeta, vzeta, wzeta;
  double aj, xix, xiy, xiz, etax, etay, etaz, zetax, zetay, zetaz;

  //  Check that the required data is available
  //
  vtkPointData* outputPD = output->GetPointData();
  if (outputPD->GetArray("StrainRate"))
  {
    return;
  }
  vtkDataArray* density = outputPD->GetArray("Density");
  vtkDataArray* momentum = outputPD->GetArray("Momentum");
  if ( density == NULL || momentum == NULL )
  {
    vtkErrorMacro("Cannot compute strain rate.");
    return;
  }

  vtkIdType numPts = density->GetNumberOfTuples();
  vtkDataArray* strainRate = this->NewFloatArray();
  strainRate->SetNumberOfComponents(3);
  strainRate->SetNumberOfTuples(numPts);
  strainRate->SetName("StrainRate");

  this->ComputeVelocity(output);
  velocity = outputPD->GetArray("Velocity");
  if(!velocity)
  {
    vtkErrorMacro("Could not compute strain rate.");
    return;
  }

  output->GetDimensions(dims);
  ijsize = dims[0]*dims[1];

  for (k=0; k<dims[2]; k++)
  {
    for (j=0; j<dims[1]; j++)
    {
      for (i=0; i<dims[0]; i++)
      {
        //  Xi derivatives.
        if ( dims[0] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
          }
          xp[0] = 1.0;
        }
        else if ( i == 0 )
        {
          factor = 1.0;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else if ( i == (dims[0]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i-1 + j*dims[0] + k*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else
        {
          factor = 0.5;
          idx = (i+1) + j*dims[0] + k*ijsize;
          idx2 = (i-1) + j*dims[0] + k*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }

        xxi = factor * (xp[0] - xm[0]);
        yxi = factor * (xp[1] - xm[1]);
        zxi = factor * (xp[2] - xm[2]);
        uxi = factor * (vp[0] - vm[0]);
        vxi = factor * (vp[1] - vm[1]);
        wxi = factor * (vp[2] - vm[2]);

        //  Eta derivatives.
        if ( dims[1] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
          }
          xp[1] = 1.0;
        }
        else if ( j == 0 )
        {
          factor = 1.0;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else if ( j == (dims[1]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else
        {
          factor = 0.5;
          idx = i + (j+1)*dims[0] + k*ijsize;
          idx2 = i + (j-1)*dims[0] + k*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }


        xeta = factor * (xp[0] - xm[0]);
        yeta = factor * (xp[1] - xm[1]);
        zeta = factor * (xp[2] - xm[2]);
        ueta = factor * (vp[0] - vm[0]);
        veta = factor * (vp[1] - vm[1]);
        weta = factor * (vp[2] - vm[2]);

        //  Zeta derivatives.
        if ( dims[2] == 1 ) // 2D in this direction
        {
          factor = 1.0;
          for (ii=0; ii<3; ii++)
          {
            vp[ii] = vm[ii] = xp[ii] = xm[ii] = 0.0;
          }
          xp[2] = 1.0;
        }
        else if ( k == 0 )
        {
          factor = 1.0;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + k*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else if ( k == (dims[2]-1) )
        {
          factor = 1.0;
          idx = i + j*dims[0] + k*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }
        else
        {
          factor = 0.5;
          idx = i + j*dims[0] + (k+1)*ijsize;
          idx2 = i + j*dims[0] + (k-1)*ijsize;
          output->GetPoint(idx,xp);
          output->GetPoint(idx2,xm);
          velocity->GetTuple(idx,vp);
          velocity->GetTuple(idx2,vm);
        }

        xzeta = factor * (xp[0] - xm[0]);
        yzeta = factor * (xp[1] - xm[1]);
        zzeta = factor * (xp[2] - xm[2]);
        uzeta = factor * (vp[0] - vm[0]);
        vzeta = factor * (vp[1] - vm[1]);
        wzeta = factor * (vp[2] - vm[2]);

        // Now calculate the Jacobian.  Grids occasionally have
        // singularities, or points where the Jacobian is infinite (the
        // inverse is zero).  For these cases, we'll set the Jacobian to
        // zero, which will result in a zero vorticity.
        //
        aj =  xxi*yeta*zzeta+yxi*zeta*xzeta+zxi*xeta*yzeta
              -zxi*yeta*xzeta-yxi*xeta*zzeta-xxi*zeta*yzeta;
        if (aj != 0.0)
        {
          aj = 1. / aj;
        }

        //  Xi metrics.
        xix  =  aj*(yeta*zzeta-zeta*yzeta);
        xiy  = -aj*(xeta*zzeta-zeta*xzeta);
        xiz  =  aj*(xeta*yzeta-yeta*xzeta);

        //  Eta metrics.
        etax = -aj*(yxi*zzeta-zxi*yzeta);
        etay =  aj*(xxi*zzeta-zxi*xzeta);
        etaz = -aj*(xxi*yzeta-yxi*xzeta);

        //  Zeta metrics.
        zetax=  aj*(yxi*zeta-zxi*yeta);
        zetay= -aj*(xxi*zeta-zxi*xeta);
        zetaz=  aj*(xxi*yeta-yxi*xeta);

        //  Finally, the strain rate components.
        //
        stRate[0] = xix*uxi+etax*ueta+zetax*uzeta;
        stRate[1] = xiy*vxi+etay*veta+zetay*vzeta;
        stRate[2] = xiz*wxi+etaz*weta+zetaz*wzeta;
        idx = i + j*dims[0] + k*ijsize;
        strainRate->SetTuple(idx,stRate);
      }
    }
  }
  outputPD->AddArray(strainRate);
  strainRate->Delete();
}

void vtkMultiBlockPLOT3DReader::SetByteOrderToBigEndian()
{
  this->ByteOrder = FILE_BIG_ENDIAN;
}

void vtkMultiBlockPLOT3DReader::SetByteOrderToLittleEndian()
{
  this->ByteOrder = FILE_LITTLE_ENDIAN;
}

const char *vtkMultiBlockPLOT3DReader::GetByteOrderAsString()
{
  if ( this->ByteOrder ==  FILE_LITTLE_ENDIAN)
  {
    return "LittleEndian";
  }
  else
  {
    return "BigEndian";
  }
}

void vtkMultiBlockPLOT3DReader::AddFunction(int functionNumber)
{
  this->FunctionList->InsertNextValue(functionNumber);
  this->Modified();
}

void vtkMultiBlockPLOT3DReader::RemoveAllFunctions()
{
  this->FunctionList->Reset();
  this->Modified();
}

int vtkMultiBlockPLOT3DReader::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
  return 1;
}

void vtkMultiBlockPLOT3DReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "XYZ File Name: " <<
    (this->XYZFileName ? this->XYZFileName : "(none)") << "\n";
  os << indent << "Q File Name: " <<
    (this->QFileName ? this->QFileName : "(none)") << "\n";
  os << indent << "Function File Name: " <<
    (this->FunctionFileName ? this->FunctionFileName : "(none)") << "\n";
  os << indent << "BinaryFile: " << this->BinaryFile << endl;
  os << indent << "HasByteCount: " << this->HasByteCount << endl;
  os << indent << "Gamma: " << this->Gamma << endl;
  os << indent << "R: " << this->R << endl;
  os << indent << "ScalarFunctionNumber: " << this->ScalarFunctionNumber << endl;
  os << indent << "VectorFunctionNumber: " << this->VectorFunctionNumber << endl;
  os << indent << "MultiGrid: " << this->MultiGrid << endl;
  os << indent << "ForceRead: " << this->ForceRead << endl;
  os << indent << "IBlanking: " << this->IBlanking << endl;
  os << indent << "ByteOrder: " << this->ByteOrder << endl;
  os << indent << "TwoDimensionalGeometry: " << (this->TwoDimensionalGeometry?"on":"off")
     << endl;
  os << indent << "Double Precision:" << this->DoublePrecision << endl;
  os << indent << "Auto Detect Format: " << this->AutoDetectFormat << endl;
}
