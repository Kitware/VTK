// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOverlappingAMRMetaData.h"

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredData.h"

#include <iostream>
#include <set>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOverlappingAMRMetaData);

//------------------------------------------------------------------------------
namespace
{
inline bool Inside(double q[3], double gbounds[6])
{
  return gbounds[0] <= q[0] && q[0] <= gbounds[1] && gbounds[2] <= q[1] && q[1] <= gbounds[3] &&
    gbounds[4] <= q[2] && q[2] <= gbounds[5];
}

//------------------------------------------------------------------------------
// Utility class used to store bin properties
// and contents
class DataSetBinner
{
  std::vector<std::vector<unsigned int>> Bins;
  unsigned int NBins[3];
  unsigned int LoCorner[3];
  // Binsize in "extent coordinates"
  unsigned int BinSize[3];
  std::size_t TotalNumBins;

public:
  // Create a set of bins given:
  // - number of bins in x, y, z
  // - lower extent of the binned space
  // - the size of bins in "extent coordinates"
  DataSetBinner(unsigned int nbins[3], unsigned int locorner[3], unsigned int binsize[3])
  {
    memcpy(this->NBins, nbins, 3 * sizeof(unsigned int));
    memcpy(this->LoCorner, locorner, 3 * sizeof(unsigned int));
    memcpy(this->BinSize, binsize, 3 * sizeof(unsigned int));
    this->TotalNumBins = nbins[0] * nbins[1] * nbins[2];
    this->Bins.resize(this->TotalNumBins);
    for (std::size_t i = 0; i < this->TotalNumBins; i++)
    {
      this->Bins[i].reserve(5);
    }
  }

  // Note that this does not check if the bin already
  // contains the blockId. This works fine for what this
  // class is used for.
  void AddToBin(unsigned int binIndex[3], int blockId)
  {
    std::size_t idx =
      binIndex[2] + binIndex[1] * this->NBins[2] + binIndex[0] * this->NBins[2] * this->NBins[1];
    std::vector<unsigned int>& bin = this->Bins[idx];
    bin.push_back(blockId);
  }

  const std::vector<unsigned int>& GetBin(unsigned int binIndex[3]) const
  {
    std::size_t idx =
      binIndex[2] + binIndex[1] * this->NBins[2] + binIndex[0] * this->NBins[2] * this->NBins[1];
    return this->Bins[idx];
  }

  // Given an input AMR box, return all boxes in the bins that intersect it
  void GetBoxesInIntersectingBins(const vtkAMRBox& box, std::set<unsigned int>& boxes)
  {
    boxes.clear();

    unsigned int minbin[3];
    unsigned int maxbin[3];

    const int* loCorner = box.GetLoCorner();
    int hiCorner[3];
    box.GetValidHiCorner(hiCorner);

    for (int j = 0; j < 3; j++)
    {
      minbin[j] = (loCorner[j] - this->LoCorner[j]) / this->BinSize[j];
      maxbin[j] = (hiCorner[j] - this->LoCorner[j]) / this->BinSize[j];
    }

    unsigned int idx[3];
    for (idx[0] = minbin[0]; idx[0] <= maxbin[0]; idx[0]++)
      for (idx[1] = minbin[1]; idx[1] <= maxbin[1]; idx[1]++)
        for (idx[2] = minbin[2]; idx[2] <= maxbin[2]; idx[2]++)
        {
          const std::vector<unsigned int>& bin = this->GetBin(idx);
          std::vector<unsigned int>::const_iterator iter;
          for (iter = bin.begin(); iter != bin.end(); ++iter)
          {
            boxes.insert(*iter);
          }
        }
  }
};
}

//------------------------------------------------------------------------------
vtkOverlappingAMRMetaData::vtkOverlappingAMRMetaData() = default;

//------------------------------------------------------------------------------
vtkOverlappingAMRMetaData::~vtkOverlappingAMRMetaData() = default;

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Global origin: (" << this->GetOrigin()[0] << ", " << this->GetOrigin()[1] << ", "
     << this->GetOrigin()[2] << ")\n ";

  os << indent << "Spacing: \n";
  for (unsigned int i = 0; i < this->GetNumberOfLevels(); i++)
  {
    if (this->HasSpacing(i))
    {
      double spacing[3];
      this->GetSpacing(i, spacing);
      os << indent << "level " << i << ": " << spacing[0] << " " << spacing[1] << " " << spacing[2]
         << "\n";
    }
    else
    {
      os << indent << "level " << i << ": empty\n";
    }
  }

  os << indent << "Refinement Ratio: ";
  if (this->HasRefinementRatio())
  {
    for (unsigned int i = 0; i < this->GetNumberOfLevels(); i++)
    {
      os << this->GetRefinementRatio(i) << " ";
    }
    os << "\n";
  }
  else
  {
    os << "None\n";
  }

  os << indent << "Block bounds: \n";
  if (this->HasBlockBounds())
  {
    for (unsigned int i = 0; i < static_cast<unsigned int>(this->BlockBounds.size()); i++)
    {
      if (this->BlockBounds[i].IsValid())
      {
        const double* minPoint = this->BlockBounds[i].GetMinPoint();
        const double* maxPoint = this->BlockBounds[i].GetMinPoint();
        os << indent << "index " << i << ": " << minPoint[0] << " " << minPoint[1] << " "
           << minPoint[2] << " " << maxPoint[0] << " " << maxPoint[1] << " " << maxPoint[2] << "\n";
      }
      else
      {
        os << indent << "index " << i << ": invalid\n";
      }
    }
  }
  else
  {
    os << indent << "None\n";
  }

  for (unsigned int levelIdx = 0; levelIdx < this->GetNumberOfLevels(); levelIdx++)
  {
    unsigned int numBlocks = this->GetNumberOfBlocks(levelIdx);
    os << indent << "level " << levelIdx << "-------------------------" << endl;
    for (unsigned int dataIdx = 0; dataIdx < numBlocks; ++dataIdx)
    {
      const vtkAMRBox& box = this->GetAMRBox(levelIdx, dataIdx);
      os << indent;
      os << "[" << box.GetLoCorner()[0] << ", " << box.GetHiCorner()[0] << "]"
         << "[" << box.GetLoCorner()[1] << ", " << box.GetHiCorner()[1] << "]"
         << "[" << box.GetLoCorner()[2] << ", " << box.GetHiCorner()[2] << "]" << endl;
    }
  }
  if (this->HasChildrenInformation())
  {
    os << indent << "Parent Child information: \n";
    for (unsigned int levelIdx = 0; levelIdx < this->GetNumberOfLevels(); levelIdx++)
    {
      unsigned int numBlocks = this->GetNumberOfBlocks(levelIdx);
      for (unsigned int dataIdx = 0; dataIdx < numBlocks; ++dataIdx)
      {
        this->PrintParentChildInfo(levelIdx, dataIdx);
      }
    }
  }
  os << "\n";
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::CheckValidity()
{
  int emptyDimension = -1;
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

  // Check origin
  for (int dim = 0; dim < 3; dim++)
  {
    if (dim != emptyDimension)
    {
      if (this->Origin[dim] != this->Bounds[2 * dim])
      {
        vtkErrorMacro("Bound min does not match origin at dimension "
          << dim << ": " << this->Origin[dim] << " != " << this->Bounds[2 * dim]);
        return false;
      }
    }
  }

  // check refinement levels
  if (this->HasRefinementRatio() &&
    static_cast<unsigned int>(this->Refinement->GetNumberOfTuples()) != this->GetNumberOfLevels())
  {
    vtkErrorMacro("Refinement levels wrong " << this->Refinement->GetNumberOfTuples());
    return false;
  }

  for (unsigned int level = 0; level < this->GetNumberOfLevels(); level++)
  {
    // check spacing

    double spacing[3];
    if (this->HasSpacing())
    {
      this->GetSpacing(level, spacing);
      for (int dim = 0; dim < 3; dim++)
      {
        if (spacing[dim] < 0)
        {
          vtkErrorMacro("Invalid spacing at level " << level << ": " << spacing[dim] << endl);
          return false;
        }
      }
    }

    // check refinement ratio
    if (this->HasRefinementRatio())
    {
      double ratio = this->Refinement->GetTuple1(0);
      unsigned int nextLevel = level + 1;
      if (nextLevel < this->GetNumberOfLevels())
      {
        double nextSpacing[3];
        this->GetSpacing(nextLevel, nextSpacing);
        for (int axis = 0; axis < 3; axis++)
        {
          if (axis != emptyDimension &&
            !vtkMathUtilities::NearlyEqual(
              ratio, vtkMathUtilities::SafeDivision(spacing[axis], nextSpacing[axis]), 10e-6))
          {
            vtkErrorMacro(
              "Spacing and refinement ratio are inconsistent for level " << level << endl);
            return false;
          }
        }
      }
    }
  }

  // check amr boxes
  for (unsigned int i = 0; i < this->Boxes.size(); i++)
  {
    const vtkAMRBox& box = this->Boxes[i];
    if (box.IsInvalid())
    {
      vtkErrorMacro("Invalid AMR Box");
      return false;
    }
    bool valid(true);
    switch (this->GetGridDescription())
    {
      case vtkStructuredData::VTK_STRUCTURED_YZ_PLANE:
        valid = box.EmptyDimension(0);
        break;
      case vtkStructuredData::VTK_STRUCTURED_XZ_PLANE:
        valid = box.EmptyDimension(1);
        break;
      case vtkStructuredData::VTK_STRUCTURED_XY_PLANE:
        valid = box.EmptyDimension(2);
        break;
    }
    if (!valid)
    {
      vtkErrorMacro("Invalid AMRBox. Wrong dimension");
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::Initialize(const std::vector<unsigned int>& blocksPerLevel)
{
  this->Superclass::Initialize(blocksPerLevel);

  int numBlocks = this->GetNumberOfBlocks();
  this->AllocateBoxes(numBlocks);
  this->Spacing->SetNumberOfTuples(3 * blocksPerLevel.size());
  this->Spacing->SetNumberOfComponents(3);
  for (std::size_t i = 0; i < blocksPerLevel.size(); i++)
  {
    double spacing[3] = { -1, -1, -1 };
    this->Spacing->SetTuple(i, spacing);
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::AllocateBlockBounds(unsigned int n)
{
  this->BlockBounds.clear();
  for (unsigned int i = 0; i < n; i++)
  {
    this->BlockBounds.emplace_back();
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::AllocateBoxes(unsigned int n)
{
  this->Boxes.clear();
  for (unsigned int i = 0; i < n; i++)
  {
    vtkAMRBox box;
    this->Boxes.emplace_back(box);
  }

  for (unsigned int i = 0; i < n; i++)
  {
    this->Boxes[i].Invalidate();
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::SetAMRBox(unsigned int level, unsigned int id, const vtkAMRBox& box)
{
  unsigned int index = this->GetAbsoluteBlockIndex(level, id);
  this->Boxes[index] = box;
  this->UpdateBounds(level, id);
}

//------------------------------------------------------------------------------
int vtkOverlappingAMRMetaData::GetAMRBlockSourceIndex(int index)
{
  return this->SourceIndex->GetValue(index);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::SetAMRBlockSourceIndex(int index, int sourceId)
{
  if (!this->SourceIndex)
  {
    this->SourceIndex = vtkSmartPointer<vtkIntArray>::New();
    this->SourceIndex->SetNumberOfValues(this->GetNumberOfBlocks());
  }
  if (index >= this->SourceIndex->GetNumberOfTuples())
  {
    vtkErrorMacro("Invalid index");
    return;
  }
  this->SourceIndex->SetValue(index, sourceId);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::GetOrigin(double origin[3])
{
  for (int i = 0; i < 3; ++i)
  {
    origin[i] = this->Origin[i];
  }
}

//------------------------------------------------------------------------------
double* vtkOverlappingAMRMetaData::GetOrigin()
{
  if (!this->HasValidOrigin())
  {
    vtkErrorMacro("Invalid Origin");
  }
  return this->Origin;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::SetOrigin(const double* origin)
{
  for (int d = 0; d < 3; d++)
  {
    this->Origin[d] = origin[d];
  }
}

//------------------------------------------------------------------------------
int vtkOverlappingAMRMetaData::GetRefinementRatio(unsigned int level) const
{
  return this->Refinement->GetValue(level);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::SetRefinementRatio(unsigned int level, int refRatio)
{
  if (!this->HasRefinementRatio())
  {
    this->Refinement->SetNumberOfTuples(this->GetNumberOfLevels());
  }
  this->Refinement->SetValue(level, refRatio);
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasRefinementRatio() const
{
  return this->Refinement &&
    static_cast<unsigned int>(this->Refinement->GetNumberOfTuples()) == this->GetNumberOfLevels();
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::GenerateRefinementRatio()
{
  if (!this->HasSpacing())
  {
    return false;
  }

  this->Refinement->SetNumberOfTuples(this->GetNumberOfLevels());

  // sanity check
  int numLevels = this->GetNumberOfLevels();

  if (numLevels < 1)
  {
    // AMR is empty!
    return true;
  }

  if (numLevels == 1)
  {
    // No refinement, data-set has only a single level.
    // The refinement ratio is set to 2 to satisfy the
    // vtkOverlappingAMR requirement.
    this->Refinement->SetValue(0, 2);
    return true;
  }

  for (int level = 0; level < numLevels - 1; ++level)
  {
    int childLevel = level + 1;

    if (this->GetNumberOfBlocks(childLevel) < 1 || this->GetNumberOfBlocks(level) < 1)
    {
      continue;
    }

    for (unsigned int id = 0; id < this->GetNumberOfBlocks(level); id++)
    {
      if (!this->GetAMRBox(level, id).IsInvalid())
      {
        break;
      }
    }

    double childSpacing[3];
    this->GetSpacing(childLevel, childSpacing);

    double currentSpacing[3];
    this->GetSpacing(level, currentSpacing);

    // Note current implementation assumes uniform spacing. The
    // refinement ratio is the same in each dimension i,j,k.
    int nonEmptyDimension = 0;
    switch (this->GetGridDescription())
    {
      case vtkStructuredData::VTK_STRUCTURED_XY_PLANE:
        nonEmptyDimension = 0;
        break;
      case vtkStructuredData::VTK_STRUCTURED_YZ_PLANE:
        nonEmptyDimension = 1;
        break;
      case vtkStructuredData::VTK_STRUCTURED_XZ_PLANE:
        nonEmptyDimension = 2;
        break;
    }

    int ratio = static_cast<int>(
      std::round(currentSpacing[nonEmptyDimension] / childSpacing[nonEmptyDimension]));

    // Set the ratio at the last level, i.e., level numLevels-1, to be the
    // same as the ratio at the previous level,since the highest level
    // doesn't really have a refinement ratio.
    if (level == numLevels - 2)
    {
      this->Refinement->SetValue(level + 1, ratio);
    }
    this->Refinement->SetValue(level, ratio);
  } // END for all hi-res levels

  return true;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasChildrenInformation() const
{
  return !this->AllChildren.empty();
}

//------------------------------------------------------------------------------
unsigned int* vtkOverlappingAMRMetaData::GetParents(
  unsigned int level, unsigned int index, unsigned int& num)
{
  if (!this->HasChildrenInformation())
  {
    this->GenerateParentChildInformation();
  }

  if (level >= this->AllParents.size() || index >= this->AllParents[level].size() ||
    this->AllParents[level][index].empty())
  {
    num = 0;
    return nullptr;
  }

  num = static_cast<unsigned int>(this->AllParents[level][index].size());

  return this->AllParents[level][index].data();
}

//------------------------------------------------------------------------------
unsigned int* vtkOverlappingAMRMetaData::GetChildren(
  unsigned int level, unsigned int index, unsigned int& size)
{
  if (!this->HasChildrenInformation())
  {
    this->GenerateParentChildInformation();
  }

  if (level >= this->AllChildren.size() || index >= this->AllChildren[level].size() ||
    this->AllChildren[level][index].empty())
  {
    size = 0;
    return nullptr;
  }

  size = static_cast<unsigned int>(this->AllChildren[level][index].size());

  return this->AllChildren[level][index].data();
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::PrintParentChildInfo(unsigned int level, unsigned int index)
{
  if (!this->HasChildrenInformation())
  {
    this->GenerateParentChildInformation();
  }

  unsigned int *ptr, i, numParents;
  std::cerr << "Parent Child Info for block " << index << " of Level: " << level << endl;
  ptr = this->GetParents(level, index, numParents);
  std::cerr << "  Parents: ";
  for (i = 0; i < numParents; i++)
  {
    std::cerr << ptr[i] << " ";
  }
  std::cerr << endl;
  std::cerr << "  Children: ";
  unsigned int numChildren;
  ptr = this->GetChildren(level, index, numChildren);
  for (i = 0; i < numChildren; i++)
  {
    std::cerr << ptr[i] << " ";
  }
  std::cerr << endl;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::GenerateParentChildInformation()
{
  if (!this->HasRefinementRatio())
  {
    // RefinementRatio takes priority over block bounds has it is faster
    // once generated
    this->GenerateRefinementRatio();
  }
  this->AllChildren.resize(this->GetNumberOfLevels());
  this->AllParents.resize(this->GetNumberOfLevels());

  unsigned int numLevels = this->GetNumberOfLevels();
  for (unsigned int i = 1; i < numLevels; i++)
  {
    this->CalculateParentChildRelationShip(i, this->AllChildren[i - 1], this->AllParents[i]);
  }
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasValidOrigin() const
{
  return this->Origin[0] != VTK_DOUBLE_MAX && this->Origin[1] != VTK_DOUBLE_MAX &&
    this->Origin[2] != VTK_DOUBLE_MAX;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasValidBounds() const
{
  return this->Bounds[0] != VTK_DOUBLE_MAX && this->Bounds[1] != VTK_DOUBLE_MIN &&
    this->Bounds[2] != VTK_DOUBLE_MAX && this->Bounds[3] != VTK_DOUBLE_MIN &&
    this->Bounds[4] != VTK_DOUBLE_MAX && this->Bounds[5] != VTK_DOUBLE_MIN;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::SetSpacing(unsigned int level, const double* userSpacing)
{
  double* spacing = this->Spacing->GetTuple(level);
  for (unsigned int i = 0; i < 3; i++)
  {
    if (spacing[i] > 0 && spacing[i] != userSpacing[i])
    {
      vtkWarningMacro("Inconsistent spacing: " << spacing[i] << " != " << userSpacing[i]);
    }
  }
  this->Spacing->SetTuple(level, userSpacing);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::SetBounds(unsigned int level, unsigned int id, double* bb)
{
  if (this->BlockBounds.empty())
  {
    this->AllocateBlockBounds(this->GetNumberOfBlocks());
  }

  unsigned int index = this->GetAbsoluteBlockIndex(level, id);
  this->BlockBounds[index] = vtkBoundingBox(bb);
  this->UpdateBounds(level, id);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::GetBounds(unsigned int level, unsigned int id, double* bb)
{
  unsigned int index = this->GetAbsoluteBlockIndex(level, id);
  if (this->HasBlockBounds(index))
  {
    const vtkBoundingBox& bounds = this->BlockBounds[index];
    if (bounds.IsValid())
    {
      bounds.GetBounds(bb);
    }
    else
    {
      vtkErrorMacro("Could not GetBounds, please SetBounds for all blocks");
    }
  }
  else if (this->HasSpacing(level))
  {
    const vtkAMRBox& box = this->Boxes[index];
    vtkAMRBox::GetBounds(box, this->Origin, this->Spacing->GetTuple(level), bb);
  }
  else
  {
    vtkErrorMacro("Could not GetBounds, please set Spacing or BlockBounds");
  }
}

//------------------------------------------------------------------------------
const vtkAMRBox& vtkOverlappingAMRMetaData::GetAMRBox(unsigned int level, unsigned int id) const
{
  return this->Boxes[this->GetAbsoluteBlockIndex(level, id)];
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::GetAMRBox(unsigned int level, unsigned int id, vtkAMRBox& box) const
{
  int idx = this->GetAbsoluteBlockIndex(level, id);
  if (idx < 0)
  {
    return false;
  }
  box = this->Boxes[idx];
  return true;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::GetSpacing(unsigned int level, double spacing[3])
{
  this->Spacing->GetTuple(level, spacing);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::CalculateParentChildRelationShip(unsigned int level,
  std::vector<std::vector<unsigned int>>& children, std::vector<std::vector<unsigned int>>& parents)
{
  if (level == 0 || level > this->GetNumberOfLevels())
  {
    return;
  }

  children.resize(this->GetNumberOfBlocks(level - 1));
  parents.resize(this->GetNumberOfBlocks(level));

  if (this->HasRefinementRatio())
  {
    // RefinementRatio takes priority over block bounds has it is faster

    // 1. Find the bounds of all boxes at level n-1
    // 2. Find the average block size
    int extents[6] = { VTK_INT_MAX, -VTK_INT_MAX, VTK_INT_MAX, -VTK_INT_MAX, VTK_INT_MAX,
      -VTK_INT_MAX };
    float totalsize[3] = { 0, 0, 0 };
    unsigned int numParentBlocks = this->GetNumberOfBlocks(level - 1);
    int refinementRatio = this->GetRefinementRatio(level - 1);
    for (unsigned int id = 0; id < numParentBlocks; id++)
    {
      vtkAMRBox box = this->GetAMRBox(level - 1, id);
      if (!box.IsInvalid())
      {
        box.Refine(refinementRatio);
        const int* loCorner = box.GetLoCorner();
        int hiCorner[3];
        box.GetValidHiCorner(hiCorner);
        for (int i = 0; i < 3; i++)
        {
          extents[2 * i] = std::min(loCorner[i], extents[2 * i]);
          extents[2 * i + 1] = std::max(hiCorner[i], extents[2 * i + 1]);
          totalsize[i] += (hiCorner[i] - loCorner[i] + 1);
        }
      }
    }

    // Calculate number of bins and binsize. Note that bins
    // are cell aligned and we use AMRBox indices to represent
    // them
    unsigned int nbins[3];
    unsigned int binsize[3];
    for (int i = 0; i < 3; i++)
    {
      binsize[i] = static_cast<int>(std::round(totalsize[i] / numParentBlocks));
      nbins[i] = (extents[2 * i + 1] - extents[2 * i]) / binsize[i] + 1;
    }

    double origin[3];
    double spacing[3];

    this->GetOrigin(origin);
    this->GetSpacing(0, spacing);
    for (unsigned int i = 0; i < level; i++)
    {
      for (int j = 0; j < 3; j++)
        spacing[j] /= this->GetRefinementRatio(i);
    }

    unsigned int loExtent[3];
    loExtent[0] = extents[0];
    loExtent[1] = extents[2];
    loExtent[2] = extents[4];
    DataSetBinner binner(nbins, loExtent, binsize);

    // Bin the blocks
    for (unsigned int i = 0; i < numParentBlocks; i++)
    {
      vtkAMRBox box = this->GetAMRBox(level - 1, i);
      if (!box.IsInvalid())
      {
        unsigned int minbin[3];
        unsigned int maxbin[3];

        box.Refine(refinementRatio);

        const int* loCorner = box.GetLoCorner();
        int hiCorner[3];
        box.GetValidHiCorner(hiCorner);

        for (int j = 0; j < 3; j++)
        {
          minbin[j] = (loCorner[j] - extents[2 * j]) / binsize[j];
          maxbin[j] = (hiCorner[j] - extents[2 * j]) / binsize[j];
        }

        unsigned int idx[3];
        for (idx[0] = minbin[0]; idx[0] <= maxbin[0]; idx[0]++)
        {
          for (idx[1] = minbin[1]; idx[1] <= maxbin[1]; idx[1]++)
          {
            for (idx[2] = minbin[2]; idx[2] <= maxbin[2]; idx[2]++)
            {
              binner.AddToBin(idx, i);
            }
          }
        }
      }
    }

    // Write bins for debugging
    // WriteBins(origin, spacing, extents, binsize, nbins, binner);

    // Actually find parent-children relationship
    // between blocks in level and level-1
    unsigned int numBlocks = this->GetNumberOfBlocks(level);
    for (unsigned int i = 0; i < numBlocks; i++)
    {
      const vtkAMRBox& box = this->GetAMRBox(level, i);
      if (!box.IsInvalid())
      {
        std::set<unsigned int> boxes;
        binner.GetBoxesInIntersectingBins(box, boxes);
        std::set<unsigned int>::iterator iter;
        for (iter = boxes.begin(); iter != boxes.end(); ++iter)
        {
          vtkAMRBox potentialParent = this->GetAMRBox(level - 1, *iter);
          if (!potentialParent.IsInvalid())
          {
            potentialParent.Refine(refinementRatio);
            if (box.DoesIntersect(potentialParent))
            {
              children[*iter].push_back(i);
              parents[i].push_back(*iter);
            }
          }
        }
      }
    }
  }
  else
  {
    // Check each block bounds at this level
    // against each block bounds at parent level
    // to find parents and children
    // if a block bounds doesn't exist, just skip it.
    unsigned int numBlocks = this->GetNumberOfBlocks(level);
    unsigned int numBlocksParents = this->GetNumberOfBlocks(level - 1);
    for (unsigned int i = 0; i < numBlocks; i++)
    {
      unsigned int index = this->GetAbsoluteBlockIndex(level, i);
      if (this->HasBlockBounds(index))
      {
        const vtkBoundingBox& childrenBox = this->BlockBounds[index];
        for (unsigned int j = 0; j < numBlocksParents; j++)
        {
          index = this->GetAbsoluteBlockIndex(level - 1, j);
          if (this->HasBlockBounds(index))
          {
            const vtkBoundingBox& parentBox = this->BlockBounds[index];
            if (parentBox.Contains(childrenBox))
            {
              children[j].emplace_back(i);
              parents[i].emplace_back(j);
            }
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::FindCell(
  double q[3], unsigned int level, unsigned int id, int& cellIdx)
{
  if (this->HasSpacing(level))
  {
    double h[3];
    this->GetSpacing(level, h);

    const vtkAMRBox& box = this->GetAMRBox(level, id);
    double gbounds[6];
    this->GetBounds(level, id, gbounds);
    if ((q[0] < gbounds[0]) || (q[0] > gbounds[1]) || (q[1] < gbounds[2]) || (q[1] > gbounds[3]) ||
      (q[2] < gbounds[4]) || (q[2] > gbounds[5]))
    {
      return false;
    }
    int ijk[3];
    double pcoords[3];
    int status = vtkAMRBox::ComputeStructuredCoordinates(box, this->Origin, h, q, ijk, pcoords);
    if (status == 1)
    {
      int dims[3];
      box.GetNumberOfNodes(dims);
      cellIdx = vtkStructuredData::ComputeCellId(dims, ijk);
      return true;
    }
  }
  else
  {
    // XXX: This cannot be implemented with block bounds but instead we should add
    // vtkOverlappingAMR::FindCell and use the cartesian grid there when spacing is not available.
    vtkErrorMacro("Cannot FindCell, please use SetSpacing");
    return false;
  }

  return false;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::GetCoarsenedAMRBox(
  unsigned int level, unsigned int id, vtkAMRBox& box)
{
  if (level == 0)
  {
    vtkErrorMacro("Cannot get AMR box at level 0.");
    return false;
  }

  if (this->HasRefinementRatio())
  {
    box = this->GetAMRBox(level, id);
    if (box.IsInvalid())
    {
      vtkErrorMacro("Invalid AMR box.");
      return false;
    }

    int refinementRatio = this->GetRefinementRatio(level - 1);
    box.Coarsen(refinementRatio);
  }
  else
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::operator==(const vtkOverlappingAMRMetaData& other) const
{
  // Compare with superclass
  if (!this->Superclass::operator==(other))
  {
    return false;
  }

  for (int i = 0; i < 3; i++)
  {
    if (this->Origin[i] != other.Origin[i])
    {
      return false;
    }
  }

  for (std::size_t i = 0; i < this->Boxes.size(); i++)
  {
    if (this->Boxes[i] != other.Boxes[i])
    {
      return false;
    }
  }

  if (this->SourceIndex && other.SourceIndex)
  {
    for (vtkIdType i = 0; i < this->SourceIndex->GetNumberOfTuples(); i++)
    {
      if (this->SourceIndex->GetValue(i) != other.SourceIndex->GetValue(i))
      {
        return false;
      }
    }
  }

  if (this->Spacing->GetNumberOfTuples() != other.Spacing->GetNumberOfTuples())
  {
    return false;
  }
  for (vtkIdType i = 0; i < this->Spacing->GetNumberOfTuples(); i++)
  {
    if (this->Spacing->GetValue(i) != other.Spacing->GetValue(i))
    {
      return false;
    }
  }

  if (this->BlockBounds.size() != other.BlockBounds.size())
  {
    return false;
  }
  for (size_t i = 0; i < this->BlockBounds.size(); i++)
  {
    for (unsigned int j = 0; j < 6; j++)
    {
      // Bounds computation can have numerical imprecision
      if (!vtkMathUtilities::NearlyEqual(
            this->BlockBounds[i].GetBound(j), other.BlockBounds[i].GetBound(j), 1e-5))
      {
        return false;
      }
    }
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::GetOrigin(unsigned int level, unsigned int id, double* origin)
{
  if (level == 0)
  {
    vtkErrorMacro("Cannot get AMR box at level 0.");
    return false;
  }

  unsigned int index = this->GetAbsoluteBlockIndex(level, id);

  if (this->HasSpacing(level))
  {
    const vtkAMRBox& box = this->Boxes[this->GetAbsoluteBlockIndex(level, id)];
    if (box.IsInvalid())
    {
      vtkErrorMacro("Invalid AMR box.");
      return false;
    }

    vtkAMRBox::GetBoxOrigin(box, this->Origin, this->Spacing->GetTuple(level), origin);
  }
  else if (this->HasBlockBounds(index))
  {
    this->BlockBounds[index].GetMinPoint(origin);
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::UpdateBounds(int level, int id)
{
  double bb[6];

  unsigned int index = this->GetAbsoluteBlockIndex(level, id);
  bool update = false;
  if (this->HasBlockBounds(index))
  {
    const vtkBoundingBox& blockBounds = this->BlockBounds[index];
    blockBounds.GetBounds(bb);
    update = true;
  }
  else if (this->HasSpacing(level))
  {
    vtkAMRBox::GetBounds(
      this->GetAMRBox(level, id), this->Origin, this->Spacing->GetTuple(level), bb);
    update = true;
  }

  if (update)
  {
    for (int i = 0; i < 3; ++i)
    {
      this->Bounds[i * 2] = std::min(bb[i * 2], this->Bounds[i * 2]);
      this->Bounds[i * 2 + 1] = std::max(bb[i * 2 + 1], this->Bounds[i * 2 + 1]);
    } // END for each dimension
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::DeepCopy(vtkAMRMetaData* other)
{
  vtkOverlappingAMRMetaData* otherMD = vtkOverlappingAMRMetaData::SafeDownCast(other);
  if (!otherMD)
  {
    vtkErrorMacro("Cannot deep copy different types");
    return;
  }

  this->Superclass::DeepCopy(other);

  memcpy(this->Origin, otherMD->Origin, sizeof(double) * 3);
  this->Boxes = otherMD->Boxes;
  if (otherMD->SourceIndex)
  {
    this->SourceIndex = vtkSmartPointer<vtkIntArray>::New();
    this->SourceIndex->DeepCopy(otherMD->SourceIndex);
  }
  this->Spacing->DeepCopy(otherMD->Spacing);
  memcpy(this->Bounds, otherMD->Bounds, sizeof(double) * 6);

  this->BlockBounds = otherMD->BlockBounds;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasBlockBounds() const
{
  bool hasBlockBounds =
    !this->BlockBounds.empty() && this->BlockBounds.size() == this->GetNumberOfBlocks();
  for (unsigned int i = 0; i < static_cast<unsigned int>(this->BlockBounds.size()); i++)
  {
    if (!this->BlockBounds[i].IsValid())
    {
      hasBlockBounds = false;
    }
  }
  return hasBlockBounds;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasBlockBounds(unsigned int index) const
{
  return !this->BlockBounds.empty() && this->BlockBounds[index].IsValid();
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasSpacing() const
{
  bool hasSpacing = true;
  for (unsigned int i = 0; i < this->GetNumberOfLevels(); i++)
  {
    if (!this->HasSpacing(i))
    {
      hasSpacing = false;
    }
  }
  return hasSpacing;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasSpacing(unsigned int level) const
{
  return this->Spacing->GetTuple(level)[0] >= 0 || this->Spacing->GetTuple(level)[1] >= 0 ||
    this->Spacing->GetTuple(level)[2] >= 0;
}

//------------------------------------------------------------------------------
const double* vtkOverlappingAMRMetaData::GetBounds()
{
  if (!this->HasValidBounds())
  {
    for (unsigned int i = 0; i < this->GetNumberOfLevels(); i++)
    {
      for (unsigned int j = 0; j < this->GetNumberOfBlocks(i); j++)
      {
        this->UpdateBounds(i, j);
      }
    }
  }
  return this->Bounds;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::FindGrid(double q[3], unsigned int& level, unsigned int& gridId)
{
  if (!this->HasChildrenInformation())
  {
    this->GenerateParentChildInformation();
  }

  if (!this->FindGrid(q, 0, gridId))
  {
    return false;
  }

  unsigned int maxLevels = this->GetNumberOfLevels();
  for (level = 0; level < maxLevels; level++)
  {
    unsigned int n;
    unsigned int* children = this->GetChildren(level, gridId, n);
    if (children == nullptr)
    {
      break;
    }
    unsigned int i;
    for (i = 0; i < n; i++)
    {
      double bb[6];
      this->GetBounds(level + 1, children[i], bb);
      if (::Inside(q, bb))
      {
        gridId = children[i];
        break;
      }
    }
    if (i >= n)
    {
      break;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::FindGrid(double q[3], int level, unsigned int& gridId)
{
  for (unsigned int i = 0; i < this->GetNumberOfBlocks(level); i++)
  {
    double gbounds[6];
    this->GetBounds(level, i, gbounds);
    bool inside = ::Inside(q, gbounds);
    if (inside)
    {
      gridId = i;
      return true;
    }
  }
  return false;
}
VTK_ABI_NAMESPACE_END
