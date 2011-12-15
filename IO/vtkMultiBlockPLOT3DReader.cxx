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

struct vtkMultiBlockPLOT3DReaderInternals
{
  std::vector<vtkSmartPointer<vtkStructuredGrid> > Blocks;
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

  this->R = 1.0;
  this->Gamma = 1.4;
  this->Uvinf = 0.0;
  this->Vvinf = 0.0;
  this->Wvinf = 0.0;

  this->FunctionList = vtkIntArray::New();

  this->ScalarFunctionNumber = -1;
  this->SetScalarFunctionNumber(100);
  this->VectorFunctionNumber = -1;
  this->SetVectorFunctionNumber(202);

  this->PointCache = 0;
  this->IBlankCache = 0;

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
  if ( this->PointCache )
    {
    for ( int g=0; this->PointCache[g]; ++g )
      this->PointCache[g]->UnRegister( this );

    delete [] this->PointCache;
    this->PointCache = 0;
    }

  if ( this->IBlankCache )
    {
    for ( int i=0; this->IBlankCache[i]; ++i )
      this->IBlankCache[i]->UnRegister( this );

    delete [] this->IBlankCache;
    this->IBlankCache = 0;
    }
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
void vtkMultiBlockPLOT3DReader::SkipByteCount(FILE* fp)
{
  if (this->BinaryFile && this->HasByteCount)
    {
    int tmp;
    if (fread(&tmp, sizeof(int), 1, fp) != 1)
      {
      vtkErrorMacro ("MultiBlockPLOT3DReader error reading file: " << this->XYZFileName
                     << " Premature EOF while reading skipping byte count.");
      fclose (fp);
      return;
      }
    }
}

// Read a block of ints (ascii or binary) and return number read.
int vtkMultiBlockPLOT3DReader::ReadIntBlock(FILE* fp, int n, int* block)
{
  if (this->BinaryFile)
    {
    int retVal=static_cast<int>(fread(block, sizeof(int), n, fp));
    if (this->ByteOrder == FILE_LITTLE_ENDIAN)
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

int vtkMultiBlockPLOT3DReader::ReadFloatBlock(FILE* fp, int n, float* block)
{
  if (this->BinaryFile)
    {
    int retVal=static_cast<int>(fread(block, sizeof(float), n, fp));
    if (this->ByteOrder == FILE_LITTLE_ENDIAN)
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
      int num = fscanf(fp, "%f", &(block[i]));
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
  if (!this->TwoDimensionalGeometry)
    {
    size = 3*4;
    size += ni*nj*nk*3*4; // x, y, z
    }
  else
    {
    size = 2*4;
    size += ni*nj*nk*2*4; // x, y, z
    }
  if (this->HasByteCount)
    {
    size += 2*4; // the byte counts
    }
  if (this->IBlanking)
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

  int numBlocks = this->GetNumberOfBlocksInternal(xyzFp, 1);
  fclose(xyzFp);
  if (numBlocks != 0)
    {
    return 1;
    }
  return 0;
}

int vtkMultiBlockPLOT3DReader::GetNumberOfBlocks()
{
  FILE* xyzFp;

  if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
    {
    return 0;
    }
  this->CalculateFileSize(xyzFp);
  int numBlocks = this->GetNumberOfBlocksInternal(xyzFp, 1);
  fclose(xyzFp);
  if (numBlocks != 0)
    {
    return numBlocks;
    }
  return 1;
}

int vtkMultiBlockPLOT3DReader::GenerateDefaultConfiguration()
{
  FILE* xyzFp;
  
  if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
    {
    return 0;
    }
  char buf[1024];
  if (fread(buf, 1024, 1, xyzFp) != 1)
    {
    vtkErrorMacro ("MultiBlockPLOT3DReader error reading file: " << this->XYZFileName
                   << " Premature EOF while reading buffer.");
    fclose (xyzFp);
    return 0;
    }
  int retVal = this->VerifySettings(buf, 1024);
  fclose(xyzFp);
  return retVal;
}

void vtkMultiBlockPLOT3DReader::ReadIntBlockV(char** buf, int n, int* block)
{
  memcpy(block, *buf, sizeof(int)*n);

  if (this->ByteOrder == FILE_LITTLE_ENDIAN)
    {
    vtkByteSwap::Swap4LERange(block, n);
    }
  else
    {
    vtkByteSwap::Swap4BERange(block, n);
    }
  *buf += sizeof(int);
}

void vtkMultiBlockPLOT3DReader::SkipByteCountV(char** buf)
{
  if (this->HasByteCount)
    {
    *buf += sizeof(int);
    }
}

int vtkMultiBlockPLOT3DReader::VerifySettings(char* buf, int vtkNotUsed(bufSize))
{
  int numGrid=0;

  if ( this->MultiGrid )
    {
    this->SkipByteCountV(&buf);
    this->ReadIntBlockV(&buf, 1, &numGrid);
    this->SkipByteCountV(&buf);
    }
  else
    {
    numGrid=1;
    }

  int retVal=1;

  long fileSize = 0;
  // Size of number of grids information.
  if ( this->MultiGrid )
    {
    fileSize += 4; // numGrids
    if (this->HasByteCount)
      {
      fileSize += 4*4; // byte counts for the header
      }
    }

  // Add the size of each grid.
  this->SkipByteCountV(&buf);
  for(int i=0; i<numGrid; i++)
    {
    int ni, nj, nk;
    this->ReadIntBlockV(&buf, 1, &ni);
    this->ReadIntBlockV(&buf, 1, &nj);
    if (!this->TwoDimensionalGeometry)
      {
      this->ReadIntBlockV(&buf, 1, &nk);
      }
    else
      {
      nk = 1;
      }
    fileSize += this->EstimateSize(ni, nj, nk);
    // If this number is larger than the file size, there
    // is something wrong.
    if ( fileSize > this->FileSize )
      {
      retVal = 0;
      break;
      }
    }
  this->SkipByteCountV(&buf);
  // If this number is different than the file size, there
  // is something wrong.
  if ( fileSize != this->FileSize )
    {
    retVal = 0;
    }

  return retVal;
}

// Read the header and return the number of grids.
int vtkMultiBlockPLOT3DReader::GetNumberOfBlocksInternal(FILE* xyzFp, int verify)
{
  int numGrid=0;
  int numBlocks;

  if ( this->MultiGrid )
    {
    this->SkipByteCount(xyzFp);
    this->ReadIntBlock(xyzFp, 1, &numGrid);
    this->SkipByteCount(xyzFp);
    }
  else
    {
    numGrid=1;
    }

  if (!verify)
    {
    // We were told not the verify the number of grid. Just return it.
    numBlocks = numGrid;
    }
  else
    {
    // We were told to make sure that the file can really contain
    // the number of grid in the header (we can only check this
    // if file is binary)
    int error=0;
    if ( this->BinaryFile )
      {
      // Store the beginning of first grid.
      long pos = ftell(xyzFp);

      long fileSize = 0;
      // Size of number of grids information.
      if ( this->MultiGrid )
        {
        fileSize += 4; // numGrids
        if (this->HasByteCount)
          {
          fileSize += 4*4; // byte counts for the header
          }
        }
      // Add the size of each grid.
      this->SkipByteCount(xyzFp);
      for(int i=0; i<numGrid; i++)
        {
        int ni, nj, nk;
        this->ReadIntBlock(xyzFp, 1, &ni);
        this->ReadIntBlock(xyzFp, 1, &nj);
        if (!this->TwoDimensionalGeometry)
          {
          this->ReadIntBlock(xyzFp, 1, &nk);
          }
        else
          {
          nk = 1;
          }
        fileSize += this->EstimateSize(ni, nj, nk);
        // If this number is larger than the file size, there
        // is something wrong.
        if ( fileSize > this->FileSize )
          {
          error = 1;
          break;
          }
        }
      this->SkipByteCount(xyzFp);
      // If this number is different than the file size, there
      // is something wrong.
      if ( fileSize != this->FileSize && !this->ForceRead)
        {
        this->SetErrorCode(vtkErrorCode::FileFormatError);
        error = 1;
        }

      fseek(xyzFp, pos, SEEK_SET);
      }
    else
      {
      if (numGrid == 0)
        {
        this->SetErrorCode(vtkErrorCode::FileFormatError);
        }
      }
    
    // Now set the number of blocks.
    if (!error && numGrid != 0)
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
      numBlocks = numGrid;
      }
    else
      {
      numBlocks = 0;
      }
    }

  return numBlocks;
}

int vtkMultiBlockPLOT3DReader::ReadGeometryHeader(FILE* fp)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 1);
  int numBlocks = static_cast<int>(this->Internal->Blocks.size());
  int i;
  vtkDebugMacro("Geometry number of grids: " << numGrid);
  if ( numGrid == 0 )
    {
    // Bad file, set all extents to invalid.
    for (i=0; i<numBlocks; i++)
      {
      this->Internal->Blocks[i]->SetWholeExtent(0, -1, 0, -1, 0, -1);
      }
    return VTK_ERROR;
    }

  // Read and set extents of all blocks.
  this->SkipByteCount(fp);
  for(i=0; i<numGrid; i++)
    {
    int ni, nj, nk=1;
    this->ReadIntBlock(fp, 1, &ni);
    this->ReadIntBlock(fp, 1, &nj);
    if (!this->TwoDimensionalGeometry)
      {
      this->ReadIntBlock(fp, 1, &nk);
      }
    vtkDebugMacro("Geometry, block " << i << " dimensions: "
                  << ni << " " << nj << " " << nk);
    this->Internal->Blocks[i]->SetWholeExtent(
      0, ni-1, 0, nj-1, 0, nk-1);
    }
  this->SkipByteCount(fp);

  if ( !this->PointCache )
    {
    this->PointCache = new vtkFloatArray*[ numBlocks + 1 ];
    this->IBlankCache = new vtkUnsignedCharArray* [ numBlocks + 1 ];
    for ( int g=0; g < numBlocks+1; ++g )
      {
      this->PointCache[g] = 0;
      this->IBlankCache[g] = 0;
      }
    }
  return VTK_OK;
}

int vtkMultiBlockPLOT3DReader::ReadQHeader(FILE* fp)
{
  int numGrid = this->GetNumberOfBlocksInternal(fp, 0);
  vtkDebugMacro("Q number of grids: " << numGrid);
  if ( numGrid == 0 )
    {
    return VTK_ERROR;
    }

  // The number of grids read from q file does not match
  // internal structure, regenerate it.
  if (numGrid != static_cast<int>(this->Internal->Blocks.size()))
    {
    FILE* xyzFp;
    if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
      {
      return VTK_ERROR;
      }
    
    if ( this->ReadGeometryHeader(xyzFp) != VTK_OK )
      {
      vtkErrorMacro("Error reading geometry file.");
      fclose(xyzFp);
      return VTK_ERROR;
      }
    fclose(xyzFp);
    }

  // If the numbers of grids still do not match, the
  // q file is wrong
  if (numGrid != static_cast<int>(this->Internal->Blocks.size()))
    {
    vtkErrorMacro("The number of grids between the geometry "
                  "and the q file do not match.");
    return VTK_ERROR;
    }


  this->SkipByteCount(fp);
  for(int i=0; i<numGrid; i++)
    {
    int ni, nj, nk=1;
    this->ReadIntBlock(fp, 1, &ni);
    this->ReadIntBlock(fp, 1, &nj);
    if (!this->TwoDimensionalGeometry)
      {
      this->ReadIntBlock(fp, 1, &nk);
      }
    vtkDebugMacro("Q, block " << i << " dimensions: "
                  << ni << " " << nj << " " << nk);

    int extent[6];
    this->Internal->Blocks[i]->GetWholeExtent(extent);
    if ( extent[1] != ni-1 || extent[3] != nj-1 || extent[5] != nk-1)
      {
      this->SetErrorCode(vtkErrorCode::FileFormatError);
      vtkErrorMacro("Geometry and data dimensions do not match. "
                    "Data file may be corrupt.");
      return VTK_ERROR;
      }
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
  FILE* xyzFp;

  if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
    {
    return 0;
    }

  this->CalculateFileSize(xyzFp);
  this->ReadGeometryHeader(xyzFp);

  fclose(xyzFp);

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
  int ndim, nx, ny, nz;
  int numberOfDims;
  vtkIdType index;

  int numBlocks = static_cast<int>(this->Internal->Blocks.size());

  // Don't read the geometry if we already have it!
  if ( (!this->PointCache) || (!this->PointCache[0]) )
    {
    FILE* xyzFp;
    if ( this->CheckGeometryFile(xyzFp) != VTK_OK)
      {
      return 0;
      }

    if ( this->ReadGeometryHeader(xyzFp) != VTK_OK )
      {
      vtkErrorMacro("Error reading geometry file.");
      fclose(xyzFp);
      return 0;
      }

    if (!this->TwoDimensionalGeometry)
      {
      numberOfDims = 3;
      }
    else
      {
      numberOfDims = 2;
      }
  
    for(i=0; i<numBlocks; i++)
      {

      // Read the geometry of this grid.
      this->SkipByteCount(xyzFp);

      vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];
      int dims[6];
      nthOutput->GetWholeExtent(dims);
      nthOutput->SetExtent(dims);
      nthOutput->GetDimensions(dims);
      this->PointCache[i] = vtkFloatArray::New();
      this->PointCache[i]->SetNumberOfComponents(3);
      this->PointCache[i]->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );

      vtkPoints* points = vtkPoints::New();
      points->SetData(this->PointCache[i]);
      nthOutput->SetPoints(points);
      points->Delete();
      this->PointCache[i]->Register( this );
      this->PointCache[i]->Delete();

      float coord;
      for(ndim=0; ndim < numberOfDims; ndim++)
        {
        for(nz=0; nz < dims[2]; nz++)
          {
          for(ny=0; ny < dims[1]; ny++)
            {
            for(nx=0; nx < dims[0]; nx++)
              {
              if ( this->ReadFloatBlock(xyzFp, 1, &coord) == 0 )
                {
                vtkErrorMacro("Encountered premature end-of-file while reading "
                              "the geometry file (or the file is corrupt).");
                this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
                // We need to generate output (otherwise, this filter will
                // keep executing). So we produce all 0's
                double nullpt[3] = {0.0, 0.0, 0.0};
                vtkIdType ipts, npts=this->PointCache[i]->GetNumberOfTuples();
                for(ipts=0; ipts < npts; ipts++)
                  {
                  this->PointCache[i]->SetTuple(ipts, nullpt);
                  }
                fclose(xyzFp);
                return 0;
                }
              index = nz*dims[0]*dims[1]+ny*dims[0]+nx;
              this->PointCache[i]->SetComponent(index, ndim, coord);
              }
            }
          }
        }

      if (this->TwoDimensionalGeometry)
        {
        vtkIdType ipts, npts=this->PointCache[i]->GetNumberOfTuples();
        for(ipts=0; ipts < npts; ipts++)
          {
          this->PointCache[i]->SetComponent(ipts, 2, 0);
          }
        }

      if (this->IBlanking)
        {
        this->IBlankCache[i] = vtkUnsignedCharArray::New();
        this->IBlankCache[i]->SetNumberOfComponents(1);
        this->IBlankCache[i]->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
        this->IBlankCache[i]->SetName("Visibility");
        int* ib = new int[dims[0]*dims[1]*dims[2]];
        if ( this->ReadIntBlock(xyzFp, dims[0]*dims[1]*dims[2], ib) == 0)
          {
          vtkErrorMacro("Encountered premature end-of-file while reading "
                        "the q file (or the file is corrupt).");
          this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
          fclose(xyzFp);
          return 0;
          }
        vtkIdType ipts, npts=this->IBlankCache[i]->GetNumberOfTuples();
        unsigned char* ib2 = this->IBlankCache[i]->GetPointer(0);
        for (ipts=0; ipts<npts; ipts++)
          {
          ib2[ipts] = ib[ipts];
          }
        delete[] ib;
        nthOutput->SetPointVisibilityArray(this->IBlankCache[i]);
        this->IBlankCache[i]->Register( this );
        this->IBlankCache[i]->Delete();
        }
      this->SkipByteCount(xyzFp);
      }

    fclose(xyzFp);
    }
  else
    {
    numberOfDims = this->TwoDimensionalGeometry ? 2 : 3;

    for(i=0; i<numBlocks; i++)
      {
      vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];
      int dims[6];
      nthOutput->GetWholeExtent(dims);
      nthOutput->SetExtent(dims);

      vtkPoints* points = vtkPoints::New();
      points->SetData(this->PointCache[i]);
      nthOutput->SetPoints(points);
      points->Delete();

      if (this->IBlanking)
        {
        nthOutput->SetPointVisibilityArray(this->IBlankCache[i]);
        }
      }
    }

  // Now read the solution.
  if (this->QFileName && this->QFileName[0] != '\0')
    {
    FILE* qFp;
    if ( this->CheckSolutionFile(qFp) != VTK_OK)
      {
      return 0;
      }
    
    if ( this->ReadQHeader(qFp) != VTK_OK )
      {
      fclose(qFp);
      return 0;
      }

    for(i=0; i<numBlocks; i++)
      {
      vtkStructuredGrid* nthOutput = this->Internal->Blocks[i];

      float fsmach, alpha, re, time;

      this->SkipByteCount(qFp);
      this->ReadFloatBlock(qFp, 1, &fsmach);
      this->ReadFloatBlock(qFp, 1, &alpha);
      this->ReadFloatBlock(qFp, 1, &re);
      this->ReadFloatBlock(qFp, 1, &time);
      this->SkipByteCount(qFp);

      // Save the properties first
      vtkFloatArray* properties = vtkFloatArray::New();
      properties->SetName("Properties");
      properties->SetNumberOfTuples(4);
      properties->SetTuple1(0, fsmach); 
      properties->SetTuple1(1, alpha); 
      properties->SetTuple1(2, re); 
      properties->SetTuple1(3, time); 
      nthOutput->GetFieldData()->AddArray(properties);
      properties->Delete();
      
      int dims[6];
      nthOutput->GetWholeExtent(dims);
      nthOutput->SetExtent(dims);
      nthOutput->GetDimensions(dims);

      this->SkipByteCount(qFp);

      vtkFloatArray* density = vtkFloatArray::New();
      density->SetNumberOfComponents(1);
      density->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      density->SetName("Density");
      float* dens = density->GetPointer(0);
      if ( this->ReadFloatBlock(qFp, dims[0]*dims[1]*dims[2], dens) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        this->SetErrorCode(vtkErrorCode::PrematureEndOfFileError);
        fclose(qFp);
        return 0;
        }
      nthOutput->GetPointData()->AddArray(density);
      density->Delete();

      vtkFloatArray* momentum = vtkFloatArray::New();
      momentum->SetNumberOfComponents(3);
      momentum->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      momentum->SetName("Momentum");

      float comp;
      for(ndim=0; ndim < numberOfDims; ndim++)
        {
        for(nz=0; nz < dims[2]; nz++)
          {
          for(ny=0; ny < dims[1]; ny++)
            {
            for(nx=0; nx < dims[0]; nx++)
              {
              if ( this->ReadFloatBlock(qFp, 1, &comp) == 0 )
                {
                vtkErrorMacro("Encountered premature end-of-file while "
                              "reading the q file (or the file is corrupt).");
                fclose(qFp);
                return 0;
                }
              index = nz*dims[0]*dims[1]+ny*dims[0]+nx;
              momentum->SetComponent(index, ndim, comp);
              }
            }
          }
        }
      if (this->TwoDimensionalGeometry)
        {
        vtkIdType ipts, npts=momentum->GetNumberOfTuples();
        for(ipts=0; ipts < npts; ipts++)
          {
          momentum->SetComponent(ipts, 2, 0);
          }
        }

      nthOutput->GetPointData()->AddArray(momentum);
      momentum->Delete();

      vtkFloatArray* se = vtkFloatArray::New();
      se->SetNumberOfComponents(1);
      se->SetNumberOfTuples( dims[0]*dims[1]*dims[2] );
      se->SetName("StagnationEnergy");
      float* sen = se->GetPointer(0);
      if (this->ReadFloatBlock(qFp, dims[0]*dims[1]*dims[2], sen) == 0)
        {
        vtkErrorMacro("Encountered premature end-of-file while reading "
                      "the q file (or the file is corrupt).");
        fclose(qFp);
        return 0;
        }
      nthOutput->GetPointData()->AddArray(se);
      se->Delete();

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

  this->Internal->Blocks.clear();
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
  vtkFloatArray *temperature;

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
  temperature = vtkFloatArray::New();
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
    temperature->SetValue(i, p*rr*rrgas);
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
  vtkFloatArray *pressure;

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
  pressure = vtkFloatArray::New();
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
    pressure->SetValue(i, p);
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
  vtkFloatArray *enthalpy;

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
  enthalpy = vtkFloatArray::New();
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
    enthalpy->SetValue(i, this->Gamma*(e*rr - 0.5*v2));
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
  vtkFloatArray *kineticEnergy;

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
  kineticEnergy = vtkFloatArray::New();
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
    kineticEnergy->SetValue(i, 0.5*v2);
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
  vtkFloatArray *velocityMag;

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
  velocityMag = vtkFloatArray::New();
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
    velocityMag->SetValue(i, sqrt((double)v2));
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
  vtkFloatArray *entropy;

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
  entropy = vtkFloatArray::New();
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
    entropy->SetValue(i,s);
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
  vtkFloatArray *swirl;

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
  swirl = vtkFloatArray::New();
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

    swirl->SetValue(i,s);
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
  vtkFloatArray *velocity;

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
  velocity = vtkFloatArray::New();
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
  vtkFloatArray *vorticity;
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
  vorticity = vtkFloatArray::New();
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
  vtkFloatArray *gradient;
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
  gradient = vtkFloatArray::New();
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
  os << indent << "Uvinf: " << this->Uvinf << endl;
  os << indent << "Vvinf: " << this->Vvinf << endl;
  os << indent << "Wvinf: " << this->Wvinf << endl;
  os << indent << "ScalarFunctionNumber: " << this->ScalarFunctionNumber << endl;
  os << indent << "VectorFunctionNumber: " << this->VectorFunctionNumber << endl;
  os << indent << "MultiGrid: " << this->MultiGrid << endl;
  os << indent << "TwoDimensionalGeometry: " 
     << this->TwoDimensionalGeometry << endl;
  os << indent << "ForceRead: " << this->ForceRead << endl;
  os << indent << "IBlanking: " << this->IBlanking << endl;
  os << indent << "ByteOrder: " << this->ByteOrder << endl;
  os << indent << "TwoDimensionalGeometry: " << (this->TwoDimensionalGeometry?"on":"off") 
     << endl;
}

