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

#include "vtkSmartPointer.h"
#include <vector>

vtkStandardNewMacro(vtkMultiBlockPLOT3DReader);

#define VTK_RHOINF 1.0
#define VTK_CINF 1.0
#define VTK_PINF ((VTK_RHOINF*VTK_CINF) * (VTK_RHOINF*VTK_CINF) / this->Gamma)
#define VTK_CV (this->R / (this->Gamma-1.0))

template <class DataType>
class vtkPLOT3DArrayReader
{
public:
  vtkPLOT3DArrayReader() : ByteOrder(
    vtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN)
    {
    }

  int ReadScalar(FILE* fp, int n, DataType* scalar)
    {
      int retVal = static_cast<int>(fread(scalar, sizeof(DataType), n, fp));
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

  int ReadVector(FILE* fp, int n, int numDims, DataType* vector)
    {
      // Setting to 0 in case numDims == 0. We still need to
      // populate an array with 3 components but the code below
      // does not read the 3rd component (it doesn't exist
      // in the file)
      memset(vector, 0, n*3*sizeof(DataType));

      int retVal = 0;
      DataType* buffer = new DataType[n];
      for (int component = 0; component < numDims; component++)
        {
        retVal += this->ReadScalar(fp, n, buffer);
        for (int i=0; i<n; i++)
          {
          vector[3*i+component] = buffer[i];
          }
        }
      delete[] buffer;

      return retVal;
    }

  int ByteOrder;
};

struct vtkMultiBlockPLOT3DReaderInternals
{
  std::vector<vtkSmartPointer<vtkStructuredGrid> > Blocks;
  int BinaryFile;
  int ByteOrder;
  int HasByteCount;
  int MultiGrid;
  int NumberOfDimensions;
  int Precision; // in bytes
  int IBlanking;

  vtkMultiBlockPLOT3DReaderInternals() :
    BinaryFile(1),
    ByteOrder(vtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN),
    HasByteCount(1),
    MultiGrid(0),
    NumberOfDimensions(3),
    Precision(4),
    IBlanking(0)
    {
    }

  int ReadInts(FILE* fp, int n, int* val)
    {
    int retVal = static_cast<int>(fread(val, sizeof(int), n, fp));
    if (this->ByteOrder == vtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::Swap4LERange(val, n);
      }
    else
      {
      vtkByteSwap::Swap4BERange(val, n);
      }
    return retVal;
    }

  void CheckBinaryFile(FILE *fp)
    {
      int j, k, l;
      rewind(fp);
      // Try fscanf to read 3 integers. If it fails,
      // must be a binary file.
      if(0 == fscanf(fp, "%d %d %d\n", &j, &k, &l))
        {
        this->BinaryFile = 1;
        }
      else
        {
        this->BinaryFile = 0;
        }
    }
  int CheckByteOrder(FILE* fp)
    {
      rewind(fp);
      int i;
      if (fread(&i, sizeof(int), 1, fp) < 1)
        {
        return 0;
        }
      char* cpy = (char*)&i;

      // If binary, we can assume that the first value is going to be either a
      // count (Fortran) or a number of block or a dimension. We assume that
      // this number will be smaller than 2^24. Then we check the first byte.
      // If it is 0 and the last byte is not 0, it is likely that this is big
      // endian.
      if(cpy[0] == 0 && cpy[3] != 0)
        {
        this->ByteOrder = vtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN;
        }
      else
        {
        this->ByteOrder = vtkMultiBlockPLOT3DReader::FILE_LITTLE_ENDIAN;
        }
      return 1;
    }
  int CheckByteCount(FILE* fp)
    {
      rewind(fp);
      // Read the first integer, then skip by that many bytes, then
      // read the value again. If the two match, it is likely that
      // the file has byte counts.
      int count;
      if (!this->ReadInts(fp, 1, &count))
        {
        return 0;
        }
      if (fseek(fp, count, SEEK_CUR) != 0)
        {
        return 0;
        }
      int count2;
      if (!this->ReadInts(fp, 1, &count2))
        {
        return 0;
        }
      if (count == count2)
        {
        this->HasByteCount = 1;
        }
      else
        {
        this->HasByteCount = 0;
        }
      return 1;
    }
  int CheckMultiGrid(FILE* fp)
    {
      if (this->HasByteCount)
        {
        rewind(fp);
        // We read the byte count, if it is 4 (1 int),
        // then this is multi-grid because the first
        // value is the number of grids rather than
        // an array of 2 or 3 values.
        int recMarkBeg;
        if (!this->ReadInts(fp, 1, &recMarkBeg))
          {
          return 0;
          }
        if(recMarkBeg == sizeof(int))
          {
          this->MultiGrid = 1;
          }
        else
          {
          this->MultiGrid = 0;
          }
        return 1;
        }
      return 0;
    }
  int Check2DGeom(FILE* fp)
    {
      if (this->HasByteCount)
        {
        rewind(fp);
        int recMarkBeg, recMarkEnd;
        int numGrids = 1;
        if(this->MultiGrid)
          {
          if (!this->ReadInts(fp, 1, &recMarkBeg) ||
              !this->ReadInts(fp, 1, &numGrids) ||
              !this->ReadInts(fp, 1, &recMarkEnd))
            {
            return 0;
            }
          }
        if (!this->ReadInts(fp, 1, &recMarkBeg))
          {
          return 0;
          }
        int nMax = 3*numGrids;
        int ndims;
        if(recMarkBeg == nMax*sizeof(int) + 2*sizeof(int))
          {
          ndims = 3;
          }
        else
          {
          if(recMarkBeg == nMax*sizeof(int))
            {
            ndims = 3;
            }
          else
            {
            ndims = 2;
            }
          }
        this->NumberOfDimensions = ndims;
        return 1;
        }
      return 0;
    }
  int CheckBlankingAndPrecision(FILE* fp)
    {
      int recMarkBeg, recMarkEnd, numGrids = 1, nMax, totPts;
      int* jmax;

      rewind(fp);
      if(this->MultiGrid)
        {
        if (!this->ReadInts(fp, 1, &recMarkBeg) ||
            !this->ReadInts(fp, 1, &numGrids) ||
            !this->ReadInts(fp, 1, &recMarkEnd))
          {
          return 0;
          }
        }
      if (!this->ReadInts(fp, 1, &recMarkBeg))
        {
        return 0;
        }
      nMax = this->NumberOfDimensions * numGrids;
      jmax = new int[numGrids*3]; // allocate memory for jmax
      if (!this->ReadInts(fp, nMax, jmax) ||
          !this->ReadInts(fp, 1, &recMarkEnd))
        {
        delete[] jmax;
        return 0;
        }
      totPts = 1;
      for (int i=0; i<this->NumberOfDimensions; i++)
        {
        totPts *= jmax[i];
        }
      this->ReadInts(fp, 1, &recMarkBeg);
      // single precision, with iblanking
      if(recMarkBeg == totPts*(this->NumberOfDimensions*4 + 4))
        {
        this->Precision = 4;
        this->IBlanking = 1;
        delete[] jmax;
        return 1;
        }
      // double precision, with iblanking
      else if(recMarkBeg == totPts*(this->NumberOfDimensions*8 + 4))
        {
        this->Precision = 8;
        this->IBlanking = 1;
        delete[] jmax;
        return 1;
        }
      // single precision, no iblanking
      else if(recMarkBeg == totPts*this->NumberOfDimensions*4)
        {
        this->Precision = 4;
        this->IBlanking = 0;
        delete[] jmax;
        return 1;
        }
      // double precision, no iblanking
      else if(recMarkBeg == totPts*this->NumberOfDimensions*8)
        {
        this->Precision = 8;
        this->IBlanking = 0;
        delete[] jmax;
        return 1;
        }
      return 0;
    }

  // Unfortunately, a Plot3D file written in C is trickier
  // to check becaues it has no byte count markers. We need
  // to do a bit more brute force checks based on reading
  // data and estimating file size.
  int CheckCFile(FILE* fp, long fileSize)
    {
      int precisions[2] = {4, 8};
      int blankings[2] = {0, 1};
      int dimensions[2] = {2, 3};

      rewind(fp);
      int gridDims[3];
      if (this->ReadInts(fp, 3, gridDims) != 3)
        {
        return 0;
        }

      // Single grid
      for (int i=0; i<2; i++)
        {
        int precision = precisions[i];
        for (int j=0; j<2; j++)
          {
          int blanking = blankings[j];
          for (int k=0; k<2; k++)
            {
            int dimension = dimensions[k];

            if (fileSize ==
                this->CalculateFileSize(false,
                                        precision,
                                        blanking,
                                        dimension,
                                        false, // always
                                        1,
                                        gridDims))
              {
              this->MultiGrid = 0;
              this->Precision = precision;
              this->IBlanking = blanking;
              this->NumberOfDimensions = dimension;
              return 1;
              }
            }
          }
        }

      // Multi grid
      int nGrids;
      rewind(fp);
      if (!this->ReadInts(fp, 1, &nGrids))
        {
        return 0;
        }
      int* gridDims2 = new int[3*nGrids];
      if (this->ReadInts(fp, nGrids*3, gridDims2) != nGrids*3)
        {
        delete[] gridDims2;
        return 0;
        }

      for (int i=0; i<2; i++)
        {
        int precision = precisions[i];
        for (int j=0; j<2; j++)
          {
          int blanking = blankings[j];
          for (int k=0; k<2; k++)
            {
            int dimension = dimensions[k];

            if (fileSize ==
                this->CalculateFileSize(true,
                                        precision,
                                        blanking,
                                        dimension,
                                        false, // always
                                        nGrids,
                                        gridDims2))
              {
              this->MultiGrid = 1;
              this->Precision = precision;
              this->IBlanking = blanking;
              this->NumberOfDimensions = dimension;
              delete[] gridDims2;
              return 1;
              }
            }
          }
        }
      delete[] gridDims2;
      return 0;
    }

  long CalculateFileSize(int mgrid,
                         int precision, // in bytes
                         int blanking,
                         int ndims,
                         int hasByteCount,
                         int nGrids,
                         int* gridDims)
    {
      long size = 0; // the header portion, 3 ints

      // N grids
      if (mgrid)
        {
        size += 4; // int for nblocks
        if (hasByteCount)
          {
          size += 2*4; // byte counts for nblocks
          }
        }
      // Header
      size += nGrids*ndims*4;

      if (hasByteCount)
        {
        size += 2*4; // byte counts for grid dims
        }
      for (int i=0; i<nGrids; i++)
        {
        size += this->CalculateFileSizeForBlock(
          precision, blanking, ndims, hasByteCount, gridDims + ndims*i);
        }
      return size;
    }

  long CalculateFileSizeForBlock(int precision, // in bytes
                                 int blanking,
                                 int ndims,
                                 int hasByteCount,
                                 int* gridDims)
    {
      long size = 0;
      // x, y, (z)
      int npts = 1;
      for (int i=0; i<ndims; i++)
        {
        npts *= gridDims[i];
        }
      size += npts*ndims*precision;

      if (blanking)
        {
        size += npts*4;
        }

      if (hasByteCount)
        {
        size += 2*4;
        }
      return size;
    }
};

vtkMultiBlockPLOT3DReader::vtkMultiBlockPLOT3DReader()
{
  this->XYZFileName = NULL;
  this->QFileName = NULL;
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

  this->Internal = new vtkMultiBlockPLOT3DReaderInternals;
}

vtkMultiBlockPLOT3DReader::~vtkMultiBlockPLOT3DReader()
{
  delete [] this->XYZFileName;
  delete [] this->QFileName;
  this->FunctionList->Delete();
  this->ClearGeometryCache();

  delete this->Internal;
}

void vtkMultiBlockPLOT3DReader::ClearGeometryCache()
{
  this->Internal->Blocks.clear();
}

int vtkMultiBlockPLOT3DReader::AutoDetectionCheck(FILE* fp)
{
  this->Internal->CheckBinaryFile(fp);

  if (!this->Internal->BinaryFile)
    {
    vtkDebugMacro("Auto-detection only works with binary files.");
    if (this->BinaryFile)
      {
      vtkWarningMacro("This appears to be an ASCII file. Please make sure "
                      "that all settings are correct to read it correctly.");
      }
    this->Internal->ByteOrder = this->ByteOrder;
    this->Internal->HasByteCount = this->HasByteCount;
    this->Internal->MultiGrid = this->MultiGrid;
    this->Internal->NumberOfDimensions = this->TwoDimensionalGeometry ? 2 : 3;
    this->Internal->Precision = this->DoublePrecision ? 8 : 4;
    this->Internal->IBlanking = this->IBlanking;
    return 1;
    }

  if (!this->Internal->CheckByteOrder(fp))
    {
    return 0;
    }
  if (!this->Internal->CheckByteCount(fp))
    {
    return 0;
    }

  if (!this->Internal->HasByteCount)
    {
    if (!this->Internal->CheckCFile(fp, this->FileSize))
      {
      return 0;
      }
    }
  else
    {
    if (!this->Internal->CheckMultiGrid(fp))
      {
      return 0;
      }
    if (!this->Internal->Check2DGeom(fp))
      {
      return 0;
      }
    if (!this->Internal->CheckBlankingAndPrecision(fp))
      {
      return 0;
      }
    }
  if (!this->AutoDetectFormat)
    {
    if ( !this->ForceRead && (
           this->Internal->BinaryFile != this->BinaryFile ||
           this->Internal->ByteOrder != this->ByteOrder ||
           this->Internal->HasByteCount != this->HasByteCount ||
           this->Internal->MultiGrid != this->MultiGrid ||
           this->Internal->NumberOfDimensions != (this->TwoDimensionalGeometry ? 2 : 3) ||
           this->Internal->Precision != (this->DoublePrecision ? 8 : 4) ||
           this->Internal->IBlanking != this->IBlanking ) )
      {
      vtkErrorMacro(<< "The settings that you provided do not match what was auto-detected "
                    << "in the file. The detected settings are: " << "\n"
                    << "BinaryFile: " << (this->Internal->BinaryFile ? 1 : 0) << "\n"
                    << "ByteOrder: " << this->Internal->ByteOrder << "\n"
                    << "HasByteCount: " << (this->Internal->HasByteCount ? 1 : 0) << "\n"
                    << "MultiGrid: " << (this->Internal->MultiGrid ? 1 : 0) << "\n"
                    << "NumberOfDimensions: " << this->Internal->NumberOfDimensions << "\n"
                    << "DoublePrecision: " << (this->Internal->Precision == 4 ? 0 : 1) << "\n"
                    << "IBlanking: " << (this->Internal->IBlanking ? 1 : 0) << endl);
      return 0;
      }
    this->Internal->BinaryFile = this->BinaryFile;
    this->Internal->ByteOrder = this->ByteOrder;
    this->Internal->HasByteCount = this->HasByteCount;
    this->Internal->MultiGrid = this->MultiGrid;
    this->Internal->NumberOfDimensions = this->TwoDimensionalGeometry ? 2 : 3;
    this->Internal->Precision = this->DoublePrecision ? 8 : 4;
    this->Internal->IBlanking = this->IBlanking;
    return 1;
    }
  return 1;
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

// Skip Fortran style byte count.
int vtkMultiBlockPLOT3DReader::SkipByteCount(FILE* fp)
{
  if (this->Internal->BinaryFile && this->Internal->HasByteCount)
    {
    int tmp;
    if (fread(&tmp, sizeof(int), 1, fp) != 1)
      {
      vtkErrorMacro ("MultiBlockPLOT3DReader error reading file: " << this->XYZFileName
                     << " Premature EOF while reading skipping byte count.");
      fclose (fp);
      return 0;
      }
    return tmp;
    }
  return 0;
}

// Read a block of ints (ascii or binary) and return number read.
int vtkMultiBlockPLOT3DReader::ReadIntBlock(FILE* fp, int n, int* block)
{
  if (this->Internal->BinaryFile)
    {
    int retVal=static_cast<int>(fread(block, sizeof(int), n, fp));
    if (this->Internal->ByteOrder == FILE_LITTLE_ENDIAN)
      {
      vtkByteSwap::Swap4LERange(block, n);
      }
    else
      {
      vtkByteSwap::Swap4BERange(block, n);
      }
    return retVal;
    }
  else
    {
    int count = 0;
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
    return count;
    }
}

vtkDataArray* vtkMultiBlockPLOT3DReader::NewFloatArray()
{
  if (this->Internal->Precision == 4)
    {
    return vtkFloatArray::New();
    }
  else
    {
    return vtkDoubleArray::New();
    }
}

int vtkMultiBlockPLOT3DReader::ReadScalar(FILE* fp, int n, vtkDataArray* scalar)
{
  if (this->Internal->BinaryFile)
    {
    if (this->Internal->Precision == 4)
      {
      vtkPLOT3DArrayReader<float> arrayReader;
      arrayReader.ByteOrder = this->Internal->ByteOrder;
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(scalar);
      return arrayReader.ReadScalar(fp, n, floatArray->GetPointer(0));
      }
    else
      {
      vtkPLOT3DArrayReader<double> arrayReader;
      arrayReader.ByteOrder = this->Internal->ByteOrder;
      vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(scalar);
      return arrayReader.ReadScalar(fp, n, doubleArray->GetPointer(0));
      }
    }
  else
    {
    if (this->Internal->Precision == 4)
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

int vtkMultiBlockPLOT3DReader::ReadVector(FILE* fp, int n, int numDims, vtkDataArray* vector)
{
  if (this->Internal->BinaryFile)
    {
    if (this->Internal->Precision == 4)
      {
      vtkPLOT3DArrayReader<float> arrayReader;
      arrayReader.ByteOrder = this->Internal->ByteOrder;
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(vector);
      return arrayReader.ReadVector(fp, n, numDims, floatArray->GetPointer(0));
      }
    else
      {
      vtkPLOT3DArrayReader<double> arrayReader;
      arrayReader.ByteOrder = this->Internal->ByteOrder;
      vtkDoubleArray* doubleArray = static_cast<vtkDoubleArray*>(vector);
      return arrayReader.ReadVector(fp, n, numDims, doubleArray->GetPointer(0));
      }
    }
  else
    {
    // Initialize the 3rd component to 0 in case the input file is
    // 2D
    vector->FillComponent(2, 0);

    int count = 0;

    if (this->Internal->Precision == 4)
      {
      vtkFloatArray* floatArray = static_cast<vtkFloatArray*>(vector);

      vtkFloatArray* tmpArray = vtkFloatArray::New();
      tmpArray->Allocate(n);
      for (int component = 0; component < numDims; component++)
        {
        count += this->ReadScalar(fp, n, tmpArray);
        for (int i=0; i<n; i++)
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
        count += this->ReadScalar(fp, n, tmpArray);
        for (int i=0; i<n; i++)
          {
          doubleArray->SetValue(3*i+component, tmpArray->GetValue(i));
          }
        }
      tmpArray->Delete();
      }

    return count;
    }
}

// Read a block of floats (ascii or binary) and return number read.
void vtkMultiBlockPLOT3DReader::CalculateFileSize(FILE* fp)
{
  long curPos = ftell(fp);
  fseek(fp, 0, SEEK_END);
  this->FileSize = ftell(fp);
  fseek(fp, curPos, SEEK_SET);
}


// Estimate the size of a grid (binary file only)
long vtkMultiBlockPLOT3DReader::EstimateSize(int ni, int nj, int nk)
{
  long size; // the header portion, 3 ints
  size = this->Internal->NumberOfDimensions*4;
  // x, y, z
  size += ni*nj*nk*this->Internal->NumberOfDimensions*this->Internal->Precision;

  if (this->Internal->HasByteCount)
    {
    size += 2*4; // the byte counts
    }
  if (this->Internal->IBlanking)
    {
    size += ni*nj*nk*4;
    }

  return size;
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
int vtkMultiBlockPLOT3DReader::GetNumberOfBlocksInternal(FILE* xyzFp, int allocate)
{
  int numGrid = 0;

  if ( this->Internal->MultiGrid )
    {
    this->SkipByteCount(xyzFp);
    this->ReadIntBlock(xyzFp, 1, &numGrid);
    this->SkipByteCount(xyzFp);
    }
  else
    {
    numGrid=1;
    }


  if (allocate)
    {
    // Now set the number of blocks.
    if (numGrid != 0)
      {
      if ( numGrid > (int)this->Internal->Blocks.size() )
        {
        this->Internal->Blocks.resize(numGrid);
        }
      for (int i=0; i<numGrid; i++)
        {
        if (!this->Internal->Blocks[i])
          {
          vtkStructuredGrid* sg = vtkStructuredGrid::New();
          this->Internal->Blocks[i] = sg;
          sg->Delete();
          }
        }
      }
    }

  return numGrid;
}

int vtkMultiBlockPLOT3DReader::ReadGeometryHeader(FILE* fp)
{
  if (!this->AutoDetectionCheck(fp))
    {
    return VTK_ERROR;
    }
  rewind(fp);

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
    this->ReadIntBlock(fp, this->Internal->NumberOfDimensions, n);
    vtkDebugMacro("Geometry, block " << i << " dimensions: "
                  << n[0] << " " << n[1] << " " << n[2]);
    this->Internal->Blocks[i]->SetExtent(0, n[0]-1, 0, n[1]-1, 0, n[2]-1);
    }
  this->SkipByteCount(fp);

  return VTK_OK;
}

int vtkMultiBlockPLOT3DReader::ReadQHeader(FILE* fp, int& nq, int& nqc, int& overflow)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 0);
  vtkDebugMacro("Q number of grids: " << numGrid);
  if ( numGrid == 0 )
    {
    return VTK_ERROR;
    }

  // If the numbers of grids still do not match, the
  // q file is wrong
  if (numGrid != static_cast<int>(this->Internal->Blocks.size()))
    {
    vtkErrorMacro("The number of grids between the geometry "
                  "and the q file do not match.");
    return VTK_ERROR;
    }

  int bytes = this->SkipByteCount(fp);
  // If the header contains 2 additional ints, then we assume
  // that this is an Overflow file.
  if (bytes > 0 &&
      bytes == (numGrid*this->Internal->NumberOfDimensions+2)*4)
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
    this->ReadIntBlock(fp, this->Internal->NumberOfDimensions, n);
    vtkDebugMacro("Q, block " << i << " dimensions: "
                  << n[0] << " " << n[1] << " " << n[2]);

    int extent[6];
    this->Internal->Blocks[i]->GetExtent(extent);
    if ( extent[1] != n[0]-1 || extent[3] != n[1]-1 || extent[5] != n[2]-1)
      {
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      vtkErrorMacro("Geometry and data dimensions do not match. "
                    "Data file may be corrupt.");
      this->Internal->Blocks[i]->Initialize();
      return VTK_ERROR;
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

void vtkMultiBlockPLOT3DReader::SetXYZFileName( const char* name )
{
  if ( this->XYZFileName && ! strcmp( this->XYZFileName, name ) )
    {
    return;
    }

  if ( this->XYZFileName )
    {
    delete [] this->XYZFileName;
    }

  if ( name )
    {
    this->XYZFileName = new char [ strlen( name ) + 1 ];
    strcpy( this->XYZFileName, name );
    }
  else
    {
    this->XYZFileName = 0;
    }

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
  info->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);
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
    return 0;
    }

  this->SetErrorCode(vtkErrorCode::NoError);

  int i;

  // This may be wrong if geometry is not cached. It is
  // update below.
  int numBlocks = static_cast<int>(this->Internal->Blocks.size());

  // Don't read the geometry if we already have it!
  if ( numBlocks == 0 )
    {
    FILE* xyzFp;
    if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
      {
      return 0;
      }

    this->CalculateFileSize(xyzFp);

    if ( this->ReadGeometryHeader(xyzFp) != VTK_OK )
      {
      vtkErrorMacro("Error reading geometry file.");
      fclose(xyzFp);
      return 0;
      }

    // Update from the value in the file.
    numBlocks = static_cast<int>(this->Internal->Blocks.size());

    for(i=0; i<numBlocks; i++)
      {

      // Read the geometry of this grid.
      this->SkipByteCount(xyzFp);

      vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];
      int dims[3];
      nthOutput->GetDimensions(dims);
      vtkDataArray* pointArray = this->NewFloatArray();
      pointArray->SetNumberOfComponents(3);
      pointArray->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );

      vtkPoints* points = vtkPoints::New();
      points->SetData(pointArray);
      pointArray->Delete();
      nthOutput->SetPoints(points);
      points->Delete();
      if ( this->ReadVector(xyzFp,
                            dims[0]*dims[1]*dims[2],
                            this->Internal->NumberOfDimensions,
                            pointArray) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the geometry file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        fclose(xyzFp);
        return 0;
        }

      if (this->Internal->IBlanking)
        {
        vtkUnsignedCharArray* blank = vtkUnsignedCharArray::New();
        blank->SetNumberOfComponents(1);
        blank->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
        blank->SetName("Visibility");
        int* ib = new int[dims[0]*dims[1]*dims[2]];
        if ( this->ReadIntBlock(xyzFp, dims[0]*dims[1]*dims[2], ib) == 0)
          {
          vtkErrorMacro("Encountered premature end-of-file while reading "
                        "the q file (or the file is corrupt).");
          this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
          fclose(xyzFp);
          return 0;
          }
        vtkIdType ipts, npts=blank->GetNumberOfTuples();
        unsigned char* ib2 = blank->GetPointer(0);
        for (ipts=0; ipts<npts; ipts++)
          {
          ib2[ipts] = ib[ipts];
          }
        delete[] ib;
        nthOutput->SetPointVisibilityArray(blank);
        }
      this->SkipByteCount(xyzFp);
      }

    fclose(xyzFp);
    }

  // Now read the solution.
  if (this->QFileName && this->QFileName[0] != '\0')
    {
    FILE* qFp;
    if ( this->CheckSolutionFile(qFp) != VTK_OK)
      {
      return 0;
      }

    int nq, nqc, isOverflow;
    if ( this->ReadQHeader(qFp, nq, nqc, isOverflow) != VTK_OK )
      {
      fclose(qFp);
      return 0;
      }

    for(i=0; i<numBlocks; i++)
      {
      vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];


      // Save the properties first
      vtkDataArray* properties = this->NewFloatArray();
      properties->SetName("Properties");

      int numProperties = 4;
      int count = this->SkipByteCount(qFp);
      // We have a byte count to tell us how many Q values to
      // read. If this is more that 4, this is probably an Overflow
      // file.
      if (isOverflow)
        {
        // We take 4 bytes because there is an int there that
        // we will throw away
        numProperties = (count-4) / this->Internal->Precision + 1;
        }
      properties->SetNumberOfTuples(numProperties);

      // Read fsmach, alpha, re, time;
      if ( this->ReadScalar(qFp, 4, properties) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        fclose(qFp);
        properties->Delete();
        return 0;
        }

      if (isOverflow)
        {
        // We create a dummy array to use with ReadScalar
        vtkDataArray* dummyArray = properties->NewInstance();
        dummyArray->SetVoidArray(properties->GetVoidPointer(4), 3, 1);

        // Read GAMINF, BETA, TINF
        if ( this->ReadScalar(qFp, 3, dummyArray) == 0)
          {
          vtkErrorMacro("Encountered premature end-of-file while reading "
                        "the q file (or the file is corrupt).");
          this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
          fclose(qFp);
          properties->Delete();
          return 0;
          }

        // igam is an int
        int igam;
        this->ReadIntBlock(qFp, 1, &igam);
        properties->SetTuple1(7, igam);

        dummyArray->SetVoidArray(properties->GetVoidPointer(8), 3, 1);
        // Read the rest of properties
        if ( this->ReadScalar(qFp, numProperties - 8, dummyArray) == 0)
          {
          vtkErrorMacro("Encountered premature end-of-file while reading "
                        "the q file (or the file is corrupt).");
          this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
          fclose(qFp);
          properties->Delete();
          return 0;
          }
        dummyArray->Delete();
        }

      nthOutput->GetFieldData()->AddArray(properties);
      properties->Delete();
      this->SkipByteCount(qFp);

      int dims[3];
      nthOutput->GetDimensions(dims);

      this->SkipByteCount(qFp);

      vtkDataArray* density = this->NewFloatArray();
      density->SetNumberOfComponents(1);
      density->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      density->SetName("Density");
      if ( this->ReadScalar(qFp, dims[0]*dims[1]*dims[2], density) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        fclose(qFp);
        density->Delete();
        return 0;
        }
      nthOutput->GetPointData()->AddArray(density);
      density->Delete();

      vtkDataArray* momentum = this->NewFloatArray();
      momentum->SetNumberOfComponents(3);
      momentum->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      momentum->SetName("Momentum");
      if ( this->ReadVector(qFp,
                            dims[0]*dims[1]*dims[2],
                            this->Internal->NumberOfDimensions,
                            momentum) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        fclose(qFp);
        momentum->Delete();
        return 0;
        }
      nthOutput->GetPointData()->AddArray(momentum);
      momentum->Delete();

      vtkDataArray* se = this->NewFloatArray();
      se->SetNumberOfComponents(1);
      se->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      se->SetName("StagnationEnergy");
      if (this->ReadScalar(qFp, dims[0]*dims[1]*dims[2], se) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        fclose(qFp);
        se->Delete();
        return 0;
        }
      nthOutput->GetPointData()->AddArray(se);
      se->Delete();

      if (isOverflow)
        {
        if(nq >= 6) /// super new
          {
          vtkDataArray* gamma = this->NewFloatArray();
          gamma->SetNumberOfComponents(1);
          gamma->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
          gamma->SetName("Gamma");
          if (this->ReadScalar(qFp, dims[0]*dims[1]*dims[2], gamma) == 0)
            {
            vtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            fclose(qFp);
            gamma->Delete();
            return 0;
            }
          nthOutput->GetPointData()->AddArray(gamma);
          gamma->Delete();
        } /// end of new

        char res[100];
        // Read species and turbulence variables for overflow q files
        for(int j=0; j<nqc; j++)
          {
          vtkDataArray* temp = this->NewFloatArray();
          temp->SetNumberOfComponents(1);
          temp->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
          int k = j+1;
          sprintf(res, "Species Density #%d", k);
          temp->SetName(res);
          if (this->ReadScalar(qFp, dims[0]*dims[1]*dims[2], temp) == 0)
            {
            vtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            fclose(qFp);
            temp->Delete();
            return 0;
            }
          nthOutput->GetPointData()->AddArray(temp);
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
          rat->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
          sprintf(res, "Spec Dens #%d / rho", v+1);
          rat->SetName(res);
          for(int w=0; w<dims[0]*dims[1]*dims[2]; w++)
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
          temp->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
          int k = a+1;
          sprintf(res, "Turb Field Quant #%d", k);
          temp->SetName(res);
          if (this->ReadScalar(qFp, dims[0]*dims[1]*dims[2], temp) == 0)
            {
            vtkErrorMacro("Encountered premature end-of-file while reading "
                          "the q file (or the file is corrupt).");
            fclose(qFp);
            temp->Delete();
            return 0;
            }
          nthOutput->GetPointData()->AddArray(temp);
          temp->Delete();
          }
        }

      this->SkipByteCount(qFp);

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
    fclose(qFp);
    }

  mb->SetNumberOfBlocks(numBlocks);
  for(i=0; i<numBlocks; i++)
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

