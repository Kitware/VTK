/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockPLOT3DReaderInternals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMultiBlockPLOT3DReaderInternals.h"

int vtkMultiBlockPLOT3DReaderInternals::ReadInts(FILE* fp, int n, int* val)
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

void vtkMultiBlockPLOT3DReaderInternals::CheckBinaryFile(FILE *fp)
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

int vtkMultiBlockPLOT3DReaderInternals::CheckByteOrder(FILE* fp)
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

int vtkMultiBlockPLOT3DReaderInternals::CheckByteCount(FILE* fp)
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

int vtkMultiBlockPLOT3DReaderInternals::CheckMultiGrid(FILE* fp)
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

int vtkMultiBlockPLOT3DReaderInternals::Check2DGeom(FILE* fp)
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
    if(recMarkBeg == static_cast<int>(nMax*sizeof(int) + 2*sizeof(int)))
      {
      ndims = 3;
      }
    else
      {
      if(recMarkBeg == static_cast<int>(nMax*sizeof(int)))
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

int vtkMultiBlockPLOT3DReaderInternals::CheckBlankingAndPrecision(FILE* fp)
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
int vtkMultiBlockPLOT3DReaderInternals::CheckCFile(FILE* fp, long fileSize)
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

long vtkMultiBlockPLOT3DReaderInternals::CalculateFileSize(int mgrid,
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

long vtkMultiBlockPLOT3DReaderInternals::CalculateFileSizeForBlock(int precision, // in bytes
                                                                   int blanking,
                                                                   int ndims,
                                                                   int hasByteCount,
                                                                   int* gridDims)
{
  long size = 0;
  // x, y, (z)
  long npts = 1;
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
