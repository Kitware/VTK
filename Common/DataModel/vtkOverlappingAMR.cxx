// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOverlappingAMR.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMRMetaData.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMRDataIterator.h"
#include "vtkUnsignedCharArray.h"

#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOverlappingAMR);

vtkInformationKeyMacro(vtkOverlappingAMR, NUMBER_OF_BLANKED_POINTS, IdType);

//------------------------------------------------------------------------------
vtkOverlappingAMR::vtkOverlappingAMR() = default;

//------------------------------------------------------------------------------
vtkOverlappingAMR::~vtkOverlappingAMR() = default;

//------------------------------------------------------------------------------
vtkCompositeDataIterator* vtkOverlappingAMR::NewIterator()
{
  vtkUniformGridAMRDataIterator* iter = vtkUniformGridAMRDataIterator::New();
  iter->SetDataSet(this);
  return iter;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::InstantiateMetaData()
{
  this->SetAMRMetaData(vtkSmartPointer<vtkOverlappingAMRMetaData>::New());
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::SetRefinementRatio(unsigned int level, int ratio)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    oamrMetaData->SetRefinementRatio(level, ratio);
  }
}

//------------------------------------------------------------------------------
int vtkOverlappingAMR::GetRefinementRatio(unsigned int level)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    if (!oamrMetaData->HasRefinementRatio())
    {
      oamrMetaData->GenerateRefinementRatio();
    }
    return oamrMetaData->GetRefinementRatio(level);
  }
  else
  {
    return -1;
  }
}

//------------------------------------------------------------------------------
int vtkOverlappingAMR::GetRefinementRatio(vtkCompositeDataIterator* iter)
{
  vtkUniformGridAMRDataIterator* amrIter = vtkUniformGridAMRDataIterator::SafeDownCast(iter);

  unsigned int level = amrIter->GetCurrentLevel();
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  return oamrMetaData ? oamrMetaData->GetRefinementRatio(level) : -1;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::GenerateParentChildInformation()
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    oamrMetaData->GenerateParentChildInformation();
  }
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMR::HasChildrenInformation()
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  return oamrMetaData ? oamrMetaData->HasChildrenInformation() : false;
}

//------------------------------------------------------------------------------
unsigned int* vtkOverlappingAMR::GetParents(
  unsigned int level, unsigned int index, unsigned int& num)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  return oamrMetaData ? oamrMetaData->GetParents(level, index, num) : nullptr;
}

//------------------------------------------------------------------------------
unsigned int* vtkOverlappingAMR::GetChildren(
  unsigned int level, unsigned int index, unsigned int& num)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  return oamrMetaData ? oamrMetaData->GetChildren(level, index, num) : nullptr;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::PrintParentChildInfo(unsigned int level, unsigned int index)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    oamrMetaData->PrintParentChildInfo(level, index);
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::SetAMRBox(unsigned int level, unsigned int id, const vtkAMRBox& box)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    oamrMetaData->SetAMRBox(level, id, box);
  }
}

//------------------------------------------------------------------------------
const vtkAMRBox& vtkOverlappingAMR::GetAMRBox(unsigned int level, unsigned int id)
{
  // XXX: Ideally this method should not return a reference
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  assert(oamrMetaData);
  return oamrMetaData->GetAMRBox(level, id);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::SetSpacing(unsigned int level, const double spacing[3])
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    oamrMetaData->SetSpacing(level, spacing);
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::GetSpacing(unsigned int level, double spacing[3])
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    oamrMetaData->GetSpacing(level, spacing);
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::GetBounds(unsigned int level, unsigned int id, double bb[6])
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    oamrMetaData->GetBounds(level, id, bb);
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::GetOrigin(unsigned int level, unsigned int id, double origin[3])
{
  double bb[6];
  this->GetBounds(level, id, bb);
  origin[0] = bb[0];
  origin[1] = bb[2];
  origin[2] = bb[4];
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::SetOrigin(const double origin[3])
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    oamrMetaData->SetOrigin(origin);
  }
}

//------------------------------------------------------------------------------
double* vtkOverlappingAMR::GetOrigin()
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  return oamrMetaData ? oamrMetaData->GetOrigin() : nullptr;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::SetAMRBlockSourceIndex(unsigned int level, unsigned int id, int sourceId)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    unsigned int index = oamrMetaData->GetAbsoluteBlockIndex(level, id);
    oamrMetaData->SetAMRBlockSourceIndex(index, sourceId);
  }
}

//------------------------------------------------------------------------------
int vtkOverlappingAMR::GetAMRBlockSourceIndex(unsigned int level, unsigned int id)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (oamrMetaData)
  {
    unsigned int index = oamrMetaData->GetAbsoluteBlockIndex(level, id);
    return oamrMetaData->GetAMRBlockSourceIndex(index);
  }
  else
  {
    return -1;
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMR::Audit()
{
  std::ignore = this->CheckValidity();
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMR::CheckValidity()
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  if (!oamrMetaData)
  {
    return false;
  }

  bool ret = oamrMetaData->CheckValidity();

  int emptyDimension(-1);
  switch (this->GetGridDescription())
  {
    case vtkStructuredData::VTK_STRUCTURED_YZ_PLANE:
      emptyDimension = 0;
      break;
    case vtkStructuredData::VTK_STRUCTURED_XZ_PLANE:
      emptyDimension = 1;
      break;
    case vtkStructuredData::VTK_STRUCTURED_XY_PLANE:
      emptyDimension = 2;
      break;
  }

  vtkSmartPointer<vtkUniformGridAMRDataIterator> iter;
  iter.TakeReference(vtkUniformGridAMRDataIterator::SafeDownCast(this->NewIterator()));
  iter->SetSkipEmptyNodes(1);
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    vtkUniformGrid* grid = vtkUniformGrid::SafeDownCast(iter->GetCurrentDataObject());
    int hasGhost = grid->HasAnyGhostCells();

    unsigned int level = iter->GetCurrentLevel();
    unsigned int id = iter->GetCurrentIndex();
    const vtkAMRBox& box = oamrMetaData->GetAMRBox(level, id);
    int dims[3];
    box.GetNumberOfNodes(dims);

    double spacing[3];
    this->GetSpacing(level, spacing);

    double origin[3];
    this->GetOrigin(level, id, origin);

    for (int d = 0; d < 3; d++)
    {
      if (d == emptyDimension)
      {
        if (grid->GetSpacing()[d] != spacing[d])
        {
          vtkErrorMacro(
            "The grid spacing does not match AMRInfo at (" << level << ", " << id << ")");
          ret = false;
        }
        if (!hasGhost && grid->GetOrigin()[d] != origin[d])
        {
          vtkErrorMacro(
            "The grid origin does not match AMRInfo at (" << level << ", " << id << ")");
          ret = false;
        }
        if (!hasGhost && grid->GetDimensions()[d] != dims[d])
        {
          vtkErrorMacro(
            "The grid dimensions does not match AMRInfo at (" << level << ", " << id << ")");
          ret = false;
        }
      }
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMR::FindGrid(double q[3], unsigned int& level, unsigned int& gridId)
{
  vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
  return oamrMetaData ? oamrMetaData->FindGrid(q, level, gridId) : false;
}

//------------------------------------------------------------------------------
const double* vtkOverlappingAMR::GetBounds()
{
  const double* bounds = this->Superclass::GetBounds();
  if (vtkBoundingBox::IsValid(bounds))
  {
    return bounds;
  }
  else
  {
    vtkOverlappingAMRMetaData* oamrMetaData = this->GetOverlappingAMRMetaData();
    return oamrMetaData ? oamrMetaData->GetBounds() : bounds;
  }
}

//------------------------------------------------------------------------------
vtkOverlappingAMRMetaData* vtkOverlappingAMR::GetOverlappingAMRMetaData()
{
  return vtkOverlappingAMRMetaData::SafeDownCast(this->GetAMRMetaData());
}

// VTK_DEPRECATED_IN_9_6_0
//------------------------------------------------------------------------------
void vtkOverlappingAMR::SetAMRInfo(vtkOverlappingAMRMetaData* info)
{
  this->SetAMRMetaData(info);
}

VTK_ABI_NAMESPACE_END
