/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMReXGridReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMReXGridReader.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellArray.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include "vtkAMRBox.h"
#include "vtkAMRDataSetCache.h"
#include "vtkAMReXGridReaderInternal.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkOverlappingAMR.h"
#include "vtkUniformGrid.h"

#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>

using vtksystools = vtksys::SystemTools;

vtkStandardNewMacro(vtkAMReXGridReader);
//----------------------------------------------------------------------------
vtkAMReXGridReader::vtkAMReXGridReader()
{
  this->IsReady = false;
  this->Internal = new vtkAMReXGridReaderInternal;
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkAMReXGridReader::~vtkAMReXGridReader()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//----------------------------------------------------------------------------
void vtkAMReXGridReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << endl;
  }
  else
  {
    os << indent << "FileName: (none)" << endl;
  }

  if (this->Internal->Header)
  {
    os << indent << "Header: " << endl;
    this->Internal->Header->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Header: (none)" << endl;
  }

  os << indent << "LevelHeader(s): " << (this->GetNumberOfLevels() >= 0 ? "" : "(none)") << endl;
  for (int cc = 0; cc < GetNumberOfLevels() + 1; ++cc)
  {
    this->Internal->LevelHeader[cc]->PrintSelfLevelHeader(os, indent.GetNextIndent());
  }
}

//-----------------------------------------------------------------------------
void vtkAMReXGridReader::SetFileName(const char* fileName)
{
  if (fileName && strcmp(fileName, "") &&
    ((this->FileName == nullptr) || strcmp(fileName, this->FileName)))
  {
    if (this->FileName)
    {
      delete[] this->FileName;
      this->FileName = nullptr;
      this->Internal->SetFileName(nullptr);
    }

    this->FileName = new char[strlen(fileName) + 1];
    strcpy(this->FileName, fileName);
    this->FileName[strlen(fileName)] = '\0';
    this->Internal->SetFileName(this->FileName);

    this->LoadedMetaData = false;
  }

  this->Modified();
}

void vtkAMReXGridReader::ReadMetaData()
{
  this->Internal->ReadMetaData();
}

//-----------------------------------------------------------------------------
int vtkAMReXGridReader::FillMetaData()
{
  this->ReadMetaData();
  if (!this->Internal->headersAreRead)
  {
    // Error: failed to read header files
    return (-1);
  }

  this->SetUpDataArraySelections();
  this->InitializeArraySelections();

  int dimension = this->GetDimension();
  int numberOfLevels = this->GetNumberOfLevels() + 1;
  std::vector<int> numberOfBlocks(numberOfLevels);
  for (int i = 0; i < numberOfLevels; ++i)
  {
    numberOfBlocks[i] = this->Internal->Header->levelSize[i];
  }
  this->Metadata->Initialize(numberOfLevels, &numberOfBlocks[0]);
  // The AMRBox always has 3 dimensions
  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 0.0, 0.0, 0.0 };
  double blockOrigin[3] = { 0.0, 0.0, 0.0 };
  int blockDimension[3] = { 1, 1, 1 };
  for (int i = 0; i < dimension; ++i)
  {
    origin[i] = this->Internal->Header->problemDomainLoEnd[i];
  }
  this->Metadata->SetOrigin(origin);
  if (dimension == 3)
    this->Metadata->SetGridDescription(VTK_XYZ_GRID);
  if (dimension == 2)
    this->Metadata->SetGridDescription(VTK_XY_PLANE);
  int boxLo;
  int boxHi;
  long globalID = 0;
  for (int i = 0; i < numberOfLevels; ++i)
  {
    for (int cc = 0; cc < dimension; ++cc)
    {
      spacing[cc] = this->Internal->Header->cellSize[i][cc];
    }
    if (dimension == 2)
      spacing[2] = spacing[1]; // Add spacing for the 3rd dimension
    this->Metadata->SetSpacing(i, spacing);
    if (i == numberOfLevels - 1)
    {
      this->Metadata->SetRefinementRatio(i, 1);
    }
    else
    {
      this->Metadata->SetRefinementRatio(i, this->Internal->Header->refinementRatio[i]);
    }
    for (int j = 0; j < this->Internal->LevelHeader[i]->levelBoxArraySize; ++j)
    {
      for (int k = 0; k < dimension; ++k)
      {
        boxLo = this->Internal->LevelHeader[i]->levelBoxArrays[j][0][k];
        boxHi = this->Internal->LevelHeader[i]->levelBoxArrays[j][1][k];
        blockOrigin[k] = boxLo * spacing[k];
        blockDimension[k] =
          ((boxHi - boxLo) + 1) + 1; // block dimension - '(hi - lo + 1)' is the number of cells '+
                                     // 1' is the number of points
      }
      if (dimension == 3)
      {
        vtkAMRBox block(blockOrigin, blockDimension, spacing, origin, VTK_XYZ_GRID);
        this->Metadata->SetAMRBox(i, j, block);
      }
      if (dimension == 2)
      {
        vtkAMRBox block(blockOrigin, blockDimension, spacing, origin, VTK_XY_PLANE);
        this->Metadata->SetAMRBox(i, j, block);
      }
      this->Metadata->SetAMRBlockSourceIndex(i, j, globalID++);
    }
  }
  return (1);
  // TODO: Need to handle Ghost Cells - Patrick O'Leary
}

//-----------------------------------------------------------------------------
vtkUniformGrid* vtkAMReXGridReader::GetAMRGrid(const int blockIdx)
{
  if (!this->Internal->headersAreRead)
  {
    // Error: failed to read header files
    return nullptr;
  }

  int dimension = this->GetDimension();
  int level = this->GetBlockLevel(blockIdx);
  int blockID = this->GetLevelBlockID(blockIdx);

  // TODO: Need to handle Ghost Cells - Patrick O'Leary
  // int ghostCells = this->Internal->LevelHeader[level]->levelNumberOfGhostCells;

  // The vtkUniformGrid always has 3 dimensions
  double spacing[3] = { 0.0, 0.0, 0.0 };
  double origin[3] = { 0.0, 0.0, 0.0 };
  for (int i = 0; i < dimension; ++i)
  {
    spacing[i] = this->Internal->Header->cellSize[level][i];
  }
  if (dimension == 2)
    spacing[2] = spacing[1]; // Add spacing for the 3rd dimension
  for (int k = 0; k < dimension; ++k)
  {
    origin[k] = this->Internal->LevelHeader[level]->levelBoxArrays[blockID][0][k] * spacing[k];
  }
  vtkAMRBox block = this->Metadata->GetAMRBox(level, blockID);
  int boxLo[3];
  int boxHi[3];
  block.GetDimensions(boxLo, boxHi);
  int dimensions[3] = { 1, 1, 1 };
  for (int i = 0; i < dimension; ++i)
  {
    dimensions[i] = ((boxHi[i] - boxLo[i]) + 1) +
      1; // block dimension - '(hi - lo + 1)' is the number of cells '+ 1' is the number of points
  }
  vtkUniformGrid* uniformGrid = vtkUniformGrid::New();
  uniformGrid->Initialize();
  uniformGrid->SetOrigin(origin);
  uniformGrid->SetSpacing(spacing);
  uniformGrid->SetDimensions(dimensions);
  return (uniformGrid);
  // TODO: Need to handle Ghost Cells - Patrick O'Leary
}

//-----------------------------------------------------------------------------
int vtkAMReXGridReader::GetDimension()
{
  return this->Internal->headersAreRead ? this->Internal->Header->dim : -1;
}

//-----------------------------------------------------------------------------
int vtkAMReXGridReader::GetNumberOfLevels()
{
  return this->Internal->headersAreRead ? this->Internal->Header->finestLevel : -1;
}

//-----------------------------------------------------------------------------
int vtkAMReXGridReader::GetNumberOfBlocks()
{
  if (!this->Internal->headersAreRead)
  {
    return (-1);
  }

  int numberOfLevels = this->GetNumberOfLevels() + 1;
  int numberOfBlocks = 0;
  for (int i = 0; i < numberOfLevels; ++i)
  {
    numberOfBlocks += this->Internal->Header->levelSize[i];
  }
  return (numberOfBlocks);
}

//-----------------------------------------------------------------------------
int vtkAMReXGridReader::GetBlockLevel(const int blockIdx)
{
  if (!this->Internal->headersAreRead)
  {
    return (-1);
  }

  int numberOfLevels = this->GetNumberOfLevels() + 1;
  int levelBlocksLo = 0;
  int levelBlocksHi = 0;
  for (int cc = 0; cc < numberOfLevels; ++cc)
  {
    levelBlocksHi += this->Internal->LevelHeader[cc]->levelBoxArraySize;
    if (blockIdx >= levelBlocksLo && blockIdx < levelBlocksHi)
    {
      return (cc);
    }
    levelBlocksLo = levelBlocksHi;
  }
  return (-1);
}

//-----------------------------------------------------------------------------
int vtkAMReXGridReader::GetLevelBlockID(const int blockIdx)
{
  if (!this->Internal->headersAreRead)
  {
    return (-1);
  }

  int numberOfLevels = this->GetNumberOfLevels() + 1;
  int levelBlocksLo = 0;
  int levelBlocksHi = 0;
  for (int cc = 0; cc < numberOfLevels; ++cc)
  {
    levelBlocksHi += this->Internal->LevelHeader[cc]->levelBoxArraySize;
    if (blockIdx >= levelBlocksLo && blockIdx < levelBlocksHi)
    {
      return blockIdx - levelBlocksLo;
    }
    levelBlocksLo = levelBlocksHi;
  }
  return (-1);
}

//-----------------------------------------------------------------------------
void vtkAMReXGridReader::GetAMRGridData(
  const int blockIdx, vtkUniformGrid* block, const char* field)
{
  if (!this->Internal->headersAreRead)
  {
    return;
  }
  this->Internal->GetBlockAttribute(field, blockIdx, block);
}

//-----------------------------------------------------------------------------
void vtkAMReXGridReader::SetUpDataArraySelections()
{
  if (!this->Internal->headersAreRead)
  {
    return;
  }
  int numberOfVariables = this->Internal->Header->variableNamesSize;
  for (int i = 0; i < numberOfVariables; ++i)
  {
    this->CellDataArraySelection->AddArray(this->Internal->Header->variableNames[i].c_str());
  }
}
