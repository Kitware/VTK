// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2000 - 2009, Lawrence Livermore National Security, LLC
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMREnzoReaderInternal
 *
 *
 * Consists of the low-level Enzo Reader used by the vtkAMREnzoReader.
 *
 * This file was adapted from the VisIt Enzo reader (avtEnzoFileFormat). For
 * details, see https://visit.llnl.gov/.
 *
 * @sa
 * vtkAMREnzoReader vtkAMREnzoParticlesReader
 */

#ifndef vtkAMREnzoReaderInternal_h
#define vtkAMREnzoReaderInternal_h

#include "vtkABINamespace.h"

#include "vtksys/SystemTools.hxx"

#include <cassert> // for assert()
#include <string>  // for STL string
#include <vector>  // for STL vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkDataSet;
static std::string GetEnzoDirectory(const char* path)
{
  return (vtksys::SystemTools::GetFilenamePath(std::string(path)));
}

// ----------------------------------------------------------------------------
//                       Class vtkEnzoReaderBlock (begin)
// ----------------------------------------------------------------------------

class vtkEnzoReaderBlock
{
public:
  vtkEnzoReaderBlock() { this->Init(); }
  ~vtkEnzoReaderBlock() { this->Init(); }
  vtkEnzoReaderBlock(const vtkEnzoReaderBlock& other) { this->DeepCopy(&other); }
  vtkEnzoReaderBlock& operator=(const vtkEnzoReaderBlock& other)
  {
    this->DeepCopy(&other);
    return *this;
  }

  int Index;
  int Level;
  int ParentId;
  std::vector<int> ChildrenIds;

  int MinParentWiseIds[3];
  int MaxParentWiseIds[3];
  int MinLevelBasedIds[3];
  int MaxLevelBasedIds[3];

  int NumberOfParticles;
  int NumberOfDimensions;
  int BlockCellDimensions[3];
  int BlockNodeDimensions[3];

  double MinBounds[3];
  double MaxBounds[3];
  double SubdivisionRatio[3];

  std::string BlockFileName;
  std::string ParticleFileName;

  void Init();
  void DeepCopy(const vtkEnzoReaderBlock* other);
  void GetParentWiseIds(std::vector<vtkEnzoReaderBlock>& blocks);
  void GetLevelBasedIds(std::vector<vtkEnzoReaderBlock>& blocks);
};

// ----------------------------------------------------------------------------
//                       Class vtkEnzoReaderBlock ( end )
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//                     Class  vtkEnzoReaderInternal (begin)
// ----------------------------------------------------------------------------

class vtkEnzoReaderInternal
{
public:
  vtkEnzoReaderInternal();
  ~vtkEnzoReaderInternal();

  // number of all vtkDataSet (vtkImageData / vtkRectilinearGrid / vtkPolyData)
  // objects that have been SUCCESSFULLY extracted and inserted to the output
  // vtkMultiBlockDataSet (including rectilinear blocks and particle sets)
  int NumberOfMultiBlocks;

  int NumberOfDimensions;
  int NumberOfLevels;
  int NumberOfBlocks;
  int ReferenceBlock;
  int CycleIndex;
  char* FileName;
  double DataTime;
  vtkDataArray* DataArray;
  //  vtkAMREnzoReader * TheReader;

  std::string DirectoryName;
  std::string MajorFileName;
  std::string BoundaryFileName;
  std::string HierarchyFileName;
  std::vector<std::string> BlockAttributeNames;
  std::vector<std::string> ParticleAttributeNames;
  std::vector<std::string> TracerParticleAttributeNames;
  std::vector<vtkEnzoReaderBlock> Blocks;

  void Init();
  void ReleaseDataArray();
  void SetFileName(char* fileName) { this->FileName = fileName; }
  void ReadMetaData();
  void GetAttributeNames();
  void CheckAttributeNames();
  void ReadBlockStructures();
  void ReadGeneralParameters();
  void DetermineRootBoundingBox();
  int LoadAttribute(const char* attribute, int blockIdx);
  int GetBlockAttribute(const char* attribute, int blockIdx, vtkDataSet* pDataSet);
  std::string GetBaseDirectory(const char* path) { return GetEnzoDirectory(path); }
};

// ----------------------------------------------------------------------------
//                     Class  vtkEnzoReaderInternal ( end )
// ----------------------------------------------------------------------------

VTK_ABI_NAMESPACE_END
#endif /* vtkAMREnzoReaderInternal_h */
// VTK-HeaderTest-Exclude: vtkAMREnzoReaderInternal.h
