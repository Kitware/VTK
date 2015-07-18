/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiBlockPLOT3DReaderInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkMultiBlockPLOT3DReaderInternals_h
#define vtkMultiBlockPLOT3DReaderInternals_h

#include "vtkByteSwap.h"
#include "vtkMultiBlockPLOT3DReader.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"

#include <vector>

struct vtkMultiBlockPLOT3DReaderInternals
{
  struct Dims
  {

  Dims()
  {
    memset(this->Values, 0, 3*sizeof(int));
  }

  int Values[3];
  };

  std::vector<Dims> Dimensions;
  std::vector<vtkSmartPointer<vtkStructuredGrid> > Blocks;

  struct InternalSettings
  {
    int BinaryFile;
    int ByteOrder;
    int HasByteCount;
    int MultiGrid;
    int NumberOfDimensions;
    int Precision; // in bytes
    int IBlanking;
    InternalSettings() :
      BinaryFile(1),
      ByteOrder(vtkMultiBlockPLOT3DReader::FILE_BIG_ENDIAN),
      HasByteCount(1),
      MultiGrid(0),
      NumberOfDimensions(3),
      Precision(4),
      IBlanking(0)
      {
      }
  };

  InternalSettings Settings;
  bool NeedToCheckXYZFile;

  vtkMultiBlockPLOT3DReaderInternals() :
    NeedToCheckXYZFile(true)
    {
    }

  int ReadInts(FILE* fp, int n, int* val);
  void CheckBinaryFile(FILE *fp, size_t fileSize);
  int CheckByteOrder(FILE* fp);
  int CheckByteCount(FILE* fp);
  int CheckMultiGrid(FILE* fp);
  int Check2DGeom(FILE* fp);
  int CheckBlankingAndPrecision(FILE* fp);
  int CheckCFile(FILE* fp, size_t fileSize);
  size_t CalculateFileSize(int mgrid,
                         int precision, // in bytes
                         int blanking,
                         int ndims,
                         int hasByteCount,
                         int nGrids,
                         int* gridDims);
  size_t CalculateFileSizeForBlock(int precision, // in bytes
                                 int blanking,
                                 int ndims,
                                 int hasByteCount,
                                 int* gridDims);
};
#endif
// VTK-HeaderTest-Exclude: vtkMultiBlockPLOT3DReaderInternals.h
