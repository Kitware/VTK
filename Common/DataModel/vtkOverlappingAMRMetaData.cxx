// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOverlappingAMRMetaData.h"

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkMathUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkStructuredData.h"

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
  size_t TotalNumBins;

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
    for (size_t i = 0; i < this->TotalNumBins; i++)
    {
      this->Bins[i].reserve(5);
    }
  }

  // Note that this does not check if the bin already
  // contains the blockId. This works fine for what this
  // class is used for.
  void AddToBin(unsigned int binIndex[3], int blockId)
  {
    size_t idx =
      binIndex[2] + binIndex[1] * this->NBins[2] + binIndex[0] * this->NBins[2] * this->NBins[1];
    std::vector<unsigned int>& bin = this->Bins[idx];
    bin.push_back(blockId);
  }

  const std::vector<unsigned int>& GetBin(unsigned int binIndex[3]) const
  {
    size_t idx =
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
vtkOverlappingAMRMetaData::vtkOverlappingAMRMetaData()
{
  this->Refinement = vtkSmartPointer<vtkIntArray>::New();
  this->SourceIndex = nullptr;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = VTK_DOUBLE_MAX;
  this->Spacing = nullptr;

  this->Bounds[0] = VTK_DOUBLE_MAX;
  this->Bounds[1] = VTK_DOUBLE_MIN;
  this->Bounds[2] = VTK_DOUBLE_MAX;
  this->Bounds[3] = VTK_DOUBLE_MIN;
  this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[5] = VTK_DOUBLE_MIN;
}

//------------------------------------------------------------------------------
vtkOverlappingAMRMetaData::~vtkOverlappingAMRMetaData() = default;

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Global origin: (" << this->GetOrigin()[0] << ", " << this->GetOrigin()[1] << ", "
     << this->GetOrigin()[2] << ")\n ";

  os << indent << "Refinemnt Ratio: ";
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

  // Check origin
  for (int d = 0; d < 3; d++)
  {
    if (d != emptyDimension)
    {
      if (this->Origin[d] != this->Bounds[2 * d])
      {
        vtkErrorMacro("Bound min does not match origin at dimension "
          << d << ": " << this->Origin[d] << " != " << this->Bounds[2 * d]);
      }
    }
  }

  // check refinement levels
  if (this->HasRefinementRatio() &&
    static_cast<unsigned int>(this->Refinement->GetNumberOfTuples()) != this->GetNumberOfLevels())
  {
    vtkErrorMacro("Refinement levels wrong " << this->Refinement->GetNumberOfTuples());
  }

  // check spacing
  for (unsigned int i = 0; i < this->GetNumberOfLevels(); i++)
  {
    double h[3];
    this->GetSpacing(i, h);
    for (int d = 0; d < 3; d++)
    {
      if (h[d] < 0)
      {
        vtkErrorMacro("Invalid spacing at level " << i << ": " << h[d] << endl);
      }
    }

    if (this->HasRefinementRatio())
    {
      double ratio = this->Refinement->GetTuple1(0);
      unsigned int nextLevel = i + 1;
      if (nextLevel < this->GetNumberOfLevels())
      {
        double nextSpacing[3];
        this->GetSpacing(nextLevel, nextSpacing);
        for (int axis = 0; axis < 3; axis++)
        {
          if (axis != emptyDimension &&
            !vtkMathUtilities::NearlyEqual(
              ratio, vtkMathUtilities::SafeDivision(h[axis], nextSpacing[axis]), 10e-6))
          {
            vtkErrorMacro("Spacing and refinement ratio are inconsistent for level " << i << endl);
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
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::Initialize(int numLevels, const int* blocksPerLevel)
{
  this->Superclass::Initialize(numLevels, blocksPerLevel);

  int numBlocks = this->GetNumberOfBlocks();
  this->AllocateBoxes(numBlocks);
  this->Spacing = vtkSmartPointer<vtkDoubleArray>::New();
  this->Spacing->SetNumberOfTuples(3 * numLevels);
  this->Spacing->SetNumberOfComponents(3);
  for (int i = 0; i < numLevels; i++)
  {
    double h[3] = { -1, -1, -1 };
    this->Spacing->SetTuple(i, h);
  }
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::AllocateBoxes(unsigned int n)
{
  this->Boxes.clear();
  for (unsigned int i = 0; i < n; i++)
  {
    vtkAMRBox box;
    this->Boxes.push_back(box);
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
  if (this->HasSpacing(level)) // has valid spacing
  {
    this->UpdateBounds(level, id);
  }
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
void vtkOverlappingAMRMetaData::GetOrigin(double o[3])
{
  for (int i = 0; i < 3; ++i)
  {
    o[i] = this->Origin[i];
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
  if (!HasRefinementRatio())
  {
    this->Refinement->SetNumberOfTuples(this->GetNumberOfLevels());
  }
  this->Refinement->SetValue(level, refRatio);
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasRefinementRatio()
{
  return this->Refinement &&
    static_cast<unsigned int>(this->Refinement->GetNumberOfTuples()) == this->GetNumberOfLevels();
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::GenerateRefinementRatio()
{
  this->Refinement->SetNumberOfTuples(this->GetNumberOfLevels());

  // sanity check
  int numLevels = this->GetNumberOfLevels();

  if (numLevels < 1)
  {
    // AMR is empty!
    return;
  }

  if (numLevels == 1)
  {
    // No refinement, data-set has only a single level.
    // The refinement ratio is set to 2 to satisfy the
    // vtkOverlappingAMR requirement.
    this->Refinement->SetValue(0, 2);
    return;
  }

  for (int level = 0; level < numLevels - 1; ++level)
  {
    int childLevel = level + 1;

    if (this->GetNumberOfBlocks(childLevel) < 1 || this->GetNumberOfBlocks(level) < 1)
    {
      continue;
    }

    unsigned int id = 0;
    for (; id < this->GetNumberOfBlocks(level); id++)
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
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasChildrenInformation()
{
  return !this->AllChildren.empty();
}

//------------------------------------------------------------------------------
unsigned int* vtkOverlappingAMRMetaData::GetParents(
  unsigned int level, unsigned int index, unsigned int& num)
{
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
bool vtkOverlappingAMRMetaData::HasValidOrigin()
{
  return this->Origin[0] != DBL_MAX && this->Origin[1] != DBL_MAX && this->Origin[2] != DBL_MAX;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasValidBounds()
{
  return this->Bounds[0] != DBL_MAX && this->Bounds[1] != DBL_MAX && this->Bounds[2] != DBL_MAX;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::SetSpacing(unsigned int level, const double* h)
{
  double* spacing = this->Spacing->GetTuple(level);
  for (unsigned int i = 0; i < 3; i++)
  {
    if (spacing[i] > 0 && spacing[i] != h[i])
    {
      vtkWarningMacro("Inconsistent spacing: " << spacing[i] << " != " << h[i]);
    }
  }
  this->Spacing->SetTuple(level, h);
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::GetBounds(unsigned int level, unsigned int id, double* bb)
{
  const vtkAMRBox& box = this->Boxes[this->GetAbsoluteBlockIndex(level, id)];
  vtkAMRBox::GetBounds(box, this->Origin, this->Spacing->GetTuple(level), bb);
}

//------------------------------------------------------------------------------
const vtkAMRBox& vtkOverlappingAMRMetaData::GetAMRBox(unsigned int level, unsigned int id) const
{
  return this->Boxes[this->GetAbsoluteBlockIndex(level, id)];
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
        if (loCorner[i] < extents[2 * i])
        {
          extents[2 * i] = loCorner[i];
        }
        if (hiCorner[i] > extents[2 * i + 1])
        {
          extents[2 * i + 1] = hiCorner[i];
        }
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
  children.resize(this->GetNumberOfBlocks(level - 1));
  parents.resize(this->GetNumberOfBlocks(level));

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

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::FindCell(
  double q[3], unsigned int level, unsigned int id, int& cellIdx)
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
  return false;
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::GetCoarsenedAMRBox(
  unsigned int level, unsigned int id, vtkAMRBox& box) const
{
  if (level == 0)
  {
    vtkErrorMacro("Cannot get AMR box at level 0.");
    return false;
  }

  box = this->GetAMRBox(level, id);
  if (box.IsInvalid())
  {
    vtkErrorMacro("Invalid AMR box.");
    return false;
  }

  int refinementRatio = this->GetRefinementRatio(level - 1);
  box.Coarsen(refinementRatio);
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

  for (size_t i = 0; i < this->Boxes.size(); i++)
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

  const vtkAMRBox& box = this->Boxes[this->GetAbsoluteBlockIndex(level, id)];
  if (box.IsInvalid())
  {
    vtkErrorMacro("Invalid AMR box.");
    return false;
  }

  vtkAMRBox::GetBoxOrigin(box, this->Origin, this->Spacing->GetTuple(level), origin);
  return true;
}

//------------------------------------------------------------------------------
void vtkOverlappingAMRMetaData::UpdateBounds(int level, int id)
{
  double bb[6];
  vtkAMRBox::GetBounds(
    this->GetAMRBox(level, id), this->Origin, this->Spacing->GetTuple(level), bb);
  for (int i = 0; i < 3; ++i)
  {
    if (bb[i * 2] < this->Bounds[i * 2])
    {
      this->Bounds[i * 2] = bb[i * 2];
    }
    if (bb[i * 2 + 1] > this->Bounds[i * 2 + 1])
    {
      this->Bounds[i * 2 + 1] = bb[i * 2 + 1];
    }
  } // END for each dimension
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
  if (otherMD->Spacing)
  {
    this->Spacing = vtkSmartPointer<vtkDoubleArray>::New();
    this->Spacing->DeepCopy(otherMD->Spacing);
  }
  memcpy(this->Bounds, otherMD->Bounds, sizeof(double) * 6);
}

//------------------------------------------------------------------------------
bool vtkOverlappingAMRMetaData::HasSpacing(unsigned int level)
{
  return this->Spacing->GetTuple(level)[0] >= 0 || this->Spacing->GetTuple(level)[1] >= 0 ||
    this->Spacing->GetTuple(level)[2] >= 0;
}

//------------------------------------------------------------------------------
const double* vtkOverlappingAMRMetaData::GetBounds()
{
  if (!HasValidBounds())
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
