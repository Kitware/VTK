// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAMRDataSetCache.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTimerLog.h"
#include "vtkUniformGrid.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAMRDataSetCache);

vtkAMRDataSetCache::vtkAMRDataSetCache() = default;

//------------------------------------------------------------------------------
vtkAMRDataSetCache::~vtkAMRDataSetCache()
{
  AMRCacheType::iterator iter = this->Cache.begin();
  for (; iter != this->Cache.end(); ++iter)
  {
    if (iter->second != nullptr)
    {
      iter->second->Delete();
    }
  }
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlock(int compositeIdx, vtkUniformGrid* amrGrid)
{
  assert("pre: AMR block is nullptr" && (amrGrid != nullptr));

  vtkTimerLog::MarkStartEvent("AMRCache::InsertBlock");
  if (!this->HasAMRBlock(compositeIdx))
  {
    this->Cache[compositeIdx] = amrGrid;
  }
  vtkTimerLog::MarkEndEvent("AMRCache::InsertBlock");
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlockPointData(int compositeIdx, vtkDataArray* dataArray)
{
  assert("pre: AMR array is nullptr" && (dataArray != nullptr));
  assert("pre: AMR block is cached" && (this->HasAMRBlock(compositeIdx)));

  vtkTimerLog::MarkStartEvent("AMRCache::InsertAMRBlockPointData");

  vtkUniformGrid* amrBlock = this->GetAMRBlock(compositeIdx);
  assert("pre: AMR block should not be nullptr" && (amrBlock != nullptr));

  vtkPointData* PD = amrBlock->GetPointData();
  assert("pre: PointData should not be nullptr" && (PD != nullptr));

  if (!PD->HasArray(dataArray->GetName()))
  {
    PD->AddArray(dataArray);
  }

  vtkTimerLog::MarkEndEvent("AMRCache::InsertAMRBlockPointData");
}

//------------------------------------------------------------------------------
void vtkAMRDataSetCache::InsertAMRBlockCellData(int compositeIdx, vtkDataArray* dataArray)
{
  assert("pre: AMR array is nullptr" && (dataArray != nullptr));
  assert("pre: AMR block is cached" && (this->HasAMRBlock(compositeIdx)));

  vtkTimerLog::MarkStartEvent("AMRCache::InsertAMRBlockCellData");

  vtkUniformGrid* amrBlock = this->GetAMRBlock(compositeIdx);
  assert("pre: AMR block should not be nullptr" && (this->HasAMRBlock(compositeIdx)));

  vtkCellData* CD = amrBlock->GetCellData();
  assert("pre: CellData should not be nullptr" && (CD != nullptr));

  if (!CD->HasArray(dataArray->GetName()))
  {
    CD->AddArray(dataArray);
  }

  vtkTimerLog::MarkEndEvent("AMRCache::InsertAMRBlockCellData");
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAMRDataSetCache::GetAMRBlockCellData(int compositeIdx, const char* dataName)
{
  if (this->HasAMRBlockCellData(compositeIdx, dataName))
  {
    vtkUniformGrid* amrBlock = this->GetAMRBlock(compositeIdx);
    assert("pre: AMR block should not be nullptr" && (this->HasAMRBlock(compositeIdx)));

    vtkCellData* CD = amrBlock->GetCellData();
    assert("pre: CellData should not be nullptr" && (CD != nullptr));

    if (CD->HasArray(dataName))
    {
      return CD->GetArray(dataName);
    }
    else
    {
      return nullptr;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkAMRDataSetCache::GetAMRBlockPointData(int compositeIdx, const char* dataName)
{

  if (this->HasAMRBlockPointData(compositeIdx, dataName))
  {
    vtkUniformGrid* amrBlock = this->GetAMRBlock(compositeIdx);
    assert("pre: AMR block should not be nullptr" && (amrBlock != nullptr));

    vtkPointData* PD = amrBlock->GetPointData();
    assert("pre: PointData should not be nullptr" && (PD != nullptr));

    if (PD->HasArray(dataName))
    {
      return PD->GetArray(dataName);
    }
    else
    {
      return nullptr;
    }
  }
  return nullptr;
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRDataSetCache::GetAMRBlock(int compositeIdx)
{
  if (this->HasAMRBlock(compositeIdx))
  {
    return this->Cache[compositeIdx];
  }
  return nullptr;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlockCellData(int compositeIdx, const char* name)
{
  assert("pre: array name is nullptr" && (name != nullptr));

  if (this->HasAMRBlock(compositeIdx))
  {
    vtkUniformGrid* gridPtr = this->GetAMRBlock(compositeIdx);
    assert("pre: cachedk block is nullptr!" && (gridPtr != nullptr));

    vtkCellData* CD = gridPtr->GetCellData();
    assert("pre: cell data is nullptr" && (CD != nullptr));

    return CD->HasArray(name) != 0;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlockPointData(int compositeIdx, const char* name)
{
  assert("pre: array name is nullptr" && (name != nullptr));

  if (this->HasAMRBlock(compositeIdx))
  {
    vtkUniformGrid* gridPtr = this->GetAMRBlock(compositeIdx);
    assert("pre: cachedk block is nullptr!" && (gridPtr != nullptr));

    vtkPointData* PD = gridPtr->GetPointData();
    assert("pre: point data is nullptr" && (PD != nullptr));

    return PD->HasArray(name) != 0;
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRDataSetCache::HasAMRBlock(int compositeIdx)
{
  vtkTimerLog::MarkStartEvent("AMRCache::CheckIfBlockExists");

  if (this->Cache.empty())
  {
    vtkTimerLog::MarkEndEvent("AMRCache::CheckIfBlockExists");
    return false;
  }

  if (this->Cache.find(compositeIdx) != this->Cache.end())
  {
    vtkTimerLog::MarkEndEvent("AMRCache::CheckIfBlockExists");
    return true;
  }

  vtkTimerLog::MarkEndEvent("AMRCache::CheckIfBlockExists");
  return false;
}
VTK_ABI_NAMESPACE_END
