/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRVelodyneReaderInternal.hpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAMRVelodyneReaderInternal
 *
 *
 *  Consists of the low-level Velodyne Reader used by the vtkAMRVelodyneReader.
 *
 * @sa
 *  vtkAMRVelodyneReader
 */

#ifndef vtkAMRVelodyneReaderInternal_h
#define vtkAMRVelodyneReaderInternal_h

#include <cassert>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkObject.h"
#include "vtkSetGet.h"
#include "vtkUniformGrid.h"

#define H5_USE_16_API
#include "vtk_hdf5.h"
//#include "hdf5.h"

//================================================================================
//                          INTERNAL VELODYNE READER
//================================================================================

class vtkAMRVelodyneReaderInternal
{
public:
  typedef struct tagVelodyneBlock
  {
    int Index;
    int dSetLoc;
    int Level;
    double Origin[3];
    bool isFull;
    bool isLeaf;
  } Block;

  vtkAMRVelodyneReaderInternal();
  ~vtkAMRVelodyneReaderInternal();
  void SetFileName(VTK_FILEPATH VTK_FUTURE_CONST char* fileName);
  void ReadMetaData();
  void GetBlockAttribute(const char* attribute, int blockIdx, vtkUniformGrid* pDataSet);

  hid_t file_id;
  std::vector<Block> Blocks;
  std::vector<std::string> AttributeNames;
  std::vector<int> blockDims;

  std::vector<int> blocksPerLevel;
  std::vector<double> globalOrigin;
  std::vector<double> rootDX;
  double dataTime;
  int nBlocks;
  int nLevels;

private:
  void Init();
  void ReadBlocks();
  void AttachScalarToGrid(int, const char*, int, vtkUniformGrid*);
  void AttachVectorToGrid(int, const char*, int, vtkUniformGrid*);
  void AttachTensor6ToGrid(int, const char*, int, vtkUniformGrid*);
  void AttachTensorToGrid(int, const char*, int, vtkUniformGrid*);
  int ReadLevelsAndX0(hid_t grp_id, std::vector<int>& levels, std::vector<double>& X0);
  herr_t CloseFile(hid_t& fid);
  vtkDataArray* GetTypeAndArray(const int, hid_t&);

  std::string FileName;
  int nLeaves;
  int nFullLeaves;
  int nNodes;
  std::unordered_map<std::string, int> typeMap;
  std::unordered_map<std::string, int> arrayMap;
};
#endif
// VTK-HeaderTest-Exclude: vtkAMRVelodyneReaderInternal.h
