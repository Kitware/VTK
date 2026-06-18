// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAMReXGridReader.h"

#include "vtkAMRBox.h"
#include "vtkAMRDataSetCache.h"
#include "vtkAMReXGridReaderInternal.h"
#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtksys/SystemTools.hxx"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAMReXGridReader);
//------------------------------------------------------------------------------
vtkAMReXGridReader::vtkAMReXGridReader()
{
  this->IsReady = false;
  this->Internal = new vtkAMReXGridReaderInternal;
  this->Initialize();
}

//------------------------------------------------------------------------------
vtkAMReXGridReader::~vtkAMReXGridReader()
{
  delete this->Internal;
  this->Internal = nullptr;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
void vtkAMReXGridReader::SetFileName(const char* arg)
{
  // both nullptr just return
  if (this->FileName == nullptr && arg == nullptr)
  {
    return;
  }

  // both set to the same value, just return
  if (this->FileName && arg && strcmp(this->FileName, arg) == 0)
  {
    return;
  }

  delete[] this->FileName;
  this->FileName = vtksys::SystemTools::DuplicateString(arg);
  this->Internal->SetFileName(this->FileName);
  this->LoadedMetaData = false;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkAMReXGridReader::ReadMetaData()
{
  this->Internal->ReadMetaData();
}

//------------------------------------------------------------------------------
int vtkAMReXGridReader::FillMetaData()
{
  this->ReadMetaData();
  if (!this->Internal->headersAreRead)
  {
    // Error: failed to read header files
    return (-1);
  }

  this->SetUpDataArraySelections();

  int dimension = this->GetDimension();
  int numberOfLevels = this->GetNumberOfLevels() + 1;
  std::vector<unsigned int> numberOfBlocks(numberOfLevels);
  for (int i = 0; i < numberOfLevels; ++i)
  {
    numberOfBlocks[i] = this->Internal->Header->levelSize[i];
  }
  this->Metadata->Initialize(numberOfBlocks);
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
    this->Metadata->SetGridDescription(vtkStructuredData::VTK_STRUCTURED_XYZ_GRID);
  if (dimension == 2)
    this->Metadata->SetGridDescription(vtkStructuredData::VTK_STRUCTURED_XY_PLANE);
  int boxLo;
  int boxHi;
  long globalID = 0;
  // For nodal main fab, AMReX writes the high index already bumped by 1 so
  // that (hi - lo + 1) is the number of nodes (= vtkUniformGrid point count).
  // For cell-centered main fab, (hi - lo + 1) is the number of cells, so the
  // point count is one larger.
  const bool nodalMainFab = (this->Internal->Header->mainFabTopology == 0);
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
        blockOrigin[k] = origin[k] + boxLo * spacing[k];
        blockDimension[k] = nodalMainFab ? ((boxHi - boxLo) + 1) : (((boxHi - boxLo) + 1) + 1);
      }
      if (dimension == 3)
      {
        vtkAMRBox block(
          blockOrigin, blockDimension, spacing, origin, vtkStructuredData::VTK_STRUCTURED_XYZ_GRID);
        this->Metadata->SetAMRBox(i, j, block);
      }
      if (dimension == 2)
      {
        vtkAMRBox block(
          blockOrigin, blockDimension, spacing, origin, vtkStructuredData::VTK_STRUCTURED_XY_PLANE);
        this->Metadata->SetAMRBox(i, j, block);
      }
      this->Metadata->SetAMRBlockSourceIndex(i, j, globalID++);
    }
  }

  // Add time information.
  auto info = this->Metadata->GetInformation();
  info->Set(vtkDataObject::DATA_TIME_STEP(), this->Internal->Header->time);
  return (1);
  // TODO: Need to handle Ghost Cells - Patrick O'Leary
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMReXGridReader::GetAMRGrid(int blockIdx)
{
  if (!this->Internal->headersAreRead)
  {
    // Error: failed to read header files
    return nullptr;
  }

  int dimension = this->GetDimension();
  int level = this->GetBlockLevel(blockIdx);
  int blockID = this->GetLevelBlockID(blockIdx);

  int ng = this->Internal->LevelHeader[level]->levelNumberOfGhostCells;

  // The vtkUniformGrid always has 3 dimensions
  double spacing[3] = { 0.0, 0.0, 0.0 };
  for (int i = 0; i < dimension; ++i)
  {
    spacing[i] = this->Internal->Header->cellSize[level][i];
  }
  if (dimension == 2)
    spacing[2] = spacing[1]; // Add spacing for the 3rd dimension

  vtkAMRBox block = this->Metadata->GetAMRBox(level, blockID);
  int boxLo[3];
  int boxHi[3];
  block.GetDimensions(boxLo, boxHi);

  vtkUniformGrid* uniformGrid = vtkUniformGrid::New();
  uniformGrid->Initialize();

  if (ng > 0)
  {
    // Grid includes ghost cells. Set origin at ghost region corner,
    // extent so that index 0 = AMR box lo corner. Ghost cells on
    // the low side get negative indices.
    double ghostOrigin[3] = { 0.0, 0.0, 0.0 };
    const double* amrOrigin = this->Metadata->GetOrigin();
    int validCells[3] = { 1, 1, 1 };
    int extent[6] = { 0, 1, 0, 1, 0, 1 };
    for (int i = 0; i < dimension; ++i)
    {
      ghostOrigin[i] = amrOrigin[i] + (boxLo[i] - ng) * spacing[i];
      validCells[i] = boxHi[i] - boxLo[i] + 1;
      extent[2 * i] = -ng;
      extent[2 * i + 1] = validCells[i] + ng;
    }
    if (dimension == 2)
    {
      ghostOrigin[2] = 0.0;
    }
    uniformGrid->SetOrigin(ghostOrigin);
    uniformGrid->SetSpacing(spacing);
    uniformGrid->SetExtent(extent);

    // Create vtkGhostType array marking ghost cells
    int cellDims[3] = { 1, 1, 1 };
    vtkIdType totalCells = 1;
    for (int i = 0; i < 3; ++i)
    {
      cellDims[i] = extent[2 * i + 1] - extent[2 * i];
      totalCells *= cellDims[i];
    }
    vtkNew<vtkUnsignedCharArray> ghosts;
    ghosts->SetName(vtkDataSetAttributes::GhostArrayName());
    ghosts->SetNumberOfTuples(totalCells);
    for (vtkIdType idx = 0; idx < totalCells; ++idx)
    {
      // Convert flat index to ijk
      int ci = idx % cellDims[0];
      int cj = (idx / cellDims[0]) % cellDims[1];
      int ck = idx / (cellDims[0] * cellDims[1]);
      // Check if this cell is in the ghost region
      bool isGhost = false;
      int ijk[3] = { ci, cj, ck };
      for (int d = 0; d < dimension; ++d)
      {
        int extCoord = ijk[d] + extent[2 * d]; // convert to extent coords
        if (extCoord < 0 || extCoord >= validCells[d])
        {
          isGhost = true;
          break;
        }
      }
      ghosts->SetValue(idx, isGhost ? vtkDataSetAttributes::DUPLICATECELL : 0);
    }
    uniformGrid->GetCellData()->AddArray(ghosts);
  }
  else
  {
    // No ghost cells - original behavior
    int dimensions[3] = { 1, 1, 1 };
    for (int i = 0; i < dimension; ++i)
    {
      dimensions[i] = (boxHi[i] - boxLo[i] + 1) + 1;
    }
    double origin[3] = { 0.0, 0.0, 0.0 };
    vtkAMRBox::GetBoxOrigin(block, this->Metadata->GetOrigin(), spacing, origin);
    uniformGrid->SetOrigin(origin);
    uniformGrid->SetSpacing(spacing);
    uniformGrid->SetDimensions(dimensions);
  }
  return (uniformGrid);
}

//------------------------------------------------------------------------------
int vtkAMReXGridReader::GetDimension()
{
  return this->Internal->headersAreRead ? this->Internal->Header->dim : -1;
}

//------------------------------------------------------------------------------
int vtkAMReXGridReader::GetNumberOfLevels()
{
  return this->Internal->headersAreRead ? this->Internal->Header->finestLevel : -1;
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
int vtkAMReXGridReader::GetBlockLevel(int blockIdx)
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

//------------------------------------------------------------------------------
int vtkAMReXGridReader::GetLevelBlockID(int blockIdx)
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

//------------------------------------------------------------------------------
void vtkAMReXGridReader::GetAMRGridData(int blockIdx, vtkUniformGrid* block, const char* field)
{
  if (!this->Internal->headersAreRead || field == nullptr)
  {
    return;
  }
  // Dispatch by where the variable lives. Main-fab variables are read through
  // GetBlockAttribute (which honors mainFabTopology to attach as cell or point
  // data); extra-multifab variables are read through GetExtraMultiFabBlockAttribute.
  const auto& mainVars = this->Internal->Header->parsedVariableNames;
  if (mainVars.find(field) != mainVars.end())
  {
    this->Internal->GetBlockAttribute(field, blockIdx, block);
    return;
  }
  if (this->Internal->extraMultiFabHeadersAreRead)
  {
    this->Internal->GetExtraMultiFabBlockAttribute(field, blockIdx, block);
  }
}

//------------------------------------------------------------------------------
void vtkAMReXGridReader::GetAMRGridPointData(
  const int blockIdx, vtkUniformGrid* block, const char* field)
{
  if (!this->Internal->headersAreRead || field == nullptr)
  {
    return;
  }
  // Variables defined on the main multifab live in parsedVariableNames; when
  // the main fab is nodal they were registered as point arrays and must be
  // read through GetBlockAttribute. Anything else is an extra multifab.
  const auto& mainVars = this->Internal->Header->parsedVariableNames;
  if (mainVars.find(field) != mainVars.end())
  {
    if (this->Internal->Header->mainFabTopology == 0)
    {
      this->Internal->GetBlockAttribute(field, blockIdx, block);
    }
    return;
  }
  if (this->Internal->extraMultiFabHeadersAreRead)
  {
    this->Internal->GetExtraMultiFabBlockAttribute(field, blockIdx, block);
  }
}
//------------------------------------------------------------------------------
void vtkAMReXGridReader::SetUpDataArraySelections()
{
  if (!this->Internal->headersAreRead)
  {
    return;
  }
  const int mainTopology = this->Internal->Header->mainFabTopology;
  if (mainTopology == -1)
  {
    vtkWarningMacro("Main multifab has unsupported topology (face/edge centered); "
                    "variables will not be exposed.");
  }
  for (const auto& variable : this->Internal->Header->parsedVariableNames)
  {
    // all arrays are added as disabled.
    if (mainTopology == 0)
    {
      this->PointDataArraySelection->AddArray(variable.first.c_str(), false);
    }
    else if (mainTopology == 3)
    {
      this->CellDataArraySelection->AddArray(variable.first.c_str(), false);
    }
  }

  // add extra multifab variables
  for (size_t fab = 0; fab < this->Internal->Header->extraMultiFabVariables.size(); ++fab)
  {
    const int fabTopology = this->Internal->Header->extraMultiFabVarTopology[fab];
    for (const auto& variable : this->Internal->Header->extraMultiFabParsedVarNames)
    {
      if (fabTopology == 3)
      {
        this->CellDataArraySelection->AddArray(variable.first.c_str(), false);
      }
      if (fabTopology == 0)
      {
        this->PointDataArraySelection->AddArray(variable.first.c_str(), false);
      }
    }
  }
}
VTK_ABI_NAMESPACE_END
