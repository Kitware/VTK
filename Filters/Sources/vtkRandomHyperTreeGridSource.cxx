// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRandomHyperTreeGridSource.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkExtentTranslator.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <numeric>
VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkRandomHyperTreeGridSource);

namespace
{
// The BranchingFactor could have its dedicated setter/getter
// and be a public member.
// But for now only 2 is supported as a value.
constexpr int BRANCHING_FACTOR = 2;

//------------------------------------------------------------------------------
/**
 * Wrapping around std::shuffle to use vtkMinimalStandardRandomSequence as a custom
 * generator.
 */
void ShuffleArray(std::vector<int>& array, vtkMinimalStandardRandomSequence* rng)
{
  for (size_t i = array.size() - 1; i > 0; i--)
  {
    double value = rng->GetValue() * static_cast<double>(array.size());
    int index = vtkMath::Floor(value);
    std::swap(array[i], array[static_cast<int>(index % array.size())]);
    rng->Next();
  }
}
}

//------------------------------------------------------------------------------
void vtkRandomHyperTreeGridSource::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkRandomHyperTreeGridSource::vtkRandomHyperTreeGridSource()
  : Seed(0)
  , MaxDepth(5)
  , SplitFraction(0.5)
  , Levels(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Dimensions[0] = 5 + 1;
  this->Dimensions[1] = 5 + 1;
  this->Dimensions[2] = 2 + 1;

  for (size_t i = 0; i < 3; ++i)
  {
    this->OutputBounds[2 * i] = -10.;
    this->OutputBounds[2 * i + 1] = 10.;
  }
}

//------------------------------------------------------------------------------
vtkRandomHyperTreeGridSource::~vtkRandomHyperTreeGridSource() = default;

//------------------------------------------------------------------------------
int vtkRandomHyperTreeGridSource::RequestInformation(
  vtkInformation* req, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  using SDDP = vtkStreamingDemandDrivenPipeline;

  if (!this->Superclass::RequestInformation(req, inInfo, outInfo))
  {
    return 0;
  }

  // this->Dimensions describes the dimension in terms of number of points
  // wholeExtent describes the dimension in terms of number of cells
  int wholeExtent[6] = {
    0,
    static_cast<int>(this->Dimensions[0] - 1),
    0,
    static_cast<int>(this->Dimensions[1] - 1),
    0,
    static_cast<int>(this->Dimensions[2] - 1),
  };
  // wholeExtent in context of HTG not be equal to 0
  for (unsigned int idim = 0; idim < 3; ++idim)
  {
    if (wholeExtent[2 * idim + 1] == 0)
    {
      wholeExtent[2 * idim + 1] = 1;
    }
  }
  // WARNING As it stands, the disadvantage of this logic is that it is not
  // possible to describe a 3D mesh with a thickness of a single cell.

  vtkInformation* info = outInfo->GetInformationObject(0);
  info->Set(SDDP::WHOLE_EXTENT(), wholeExtent, 6);
  info->Set(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);

  return 1;
}

//------------------------------------------------------------------------------
int vtkRandomHyperTreeGridSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outInfos)
{
  using SDDP = vtkStreamingDemandDrivenPipeline;

  vtkInformation* outInfo = outInfos->GetInformationObject(0);
  const int piece = outInfo->Get(SDDP::UPDATE_PIECE_NUMBER());
  int* updateExtent = outInfo->Get(SDDP::UPDATE_EXTENT());

  // Refresh masking cost per level if maxDepth did change
  if (this->MaxDepth + 1 != static_cast<vtkIdType>(this->MaskingCostPerLevel.size()))
  {
    this->InitializeMaskingNodeCostPerLevel();
  }

  // Create dataset:
  auto fillArray = [](
                     vtkDoubleArray* array, vtkIdType numPoints, double minBound, double maxBound) {
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(numPoints);
    // We differentiate the pathological case at one point from the other cases
    if (numPoints == 1)
    {
      array->SetTypedComponent(0, 0, 0.);
    }
    else
    {
      // Compute step for more than one point
      double step = (maxBound - minBound) / static_cast<double>(numPoints - 1);
      for (int i = 0; i < numPoints; ++i)
      {
        array->SetTypedComponent(i, 0, minBound + step * i);
      }
    }
  };

  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::GetData(outInfo);
  htg->Initialize();
  htg->SetDimensions(this->Dimensions);
  if (htg->GetDimension() == 0)
  {
    // No HyperTrees, we don't need to create anything
    return 1;
  }
  htg->SetBranchFactor(::BRANCHING_FACTOR);
  {
    vtkNew<vtkDoubleArray> coords;
    fillArray(coords, this->Dimensions[0], this->OutputBounds[0], this->OutputBounds[1]);
    htg->SetXCoordinates(coords);
  }

  {
    vtkNew<vtkDoubleArray> coords;
    fillArray(coords, this->Dimensions[1], this->OutputBounds[2], this->OutputBounds[3]);
    htg->SetYCoordinates(coords);
  }

  {
    vtkNew<vtkDoubleArray> coords;
    fillArray(coords, this->Dimensions[2], this->OutputBounds[4], this->OutputBounds[5]);
    htg->SetZCoordinates(coords);
  }

  vtkNew<vtkDoubleArray> levels;
  levels->SetName("Depth");
  htg->GetCellData()->AddArray(levels);
  this->Levels = levels;

  // Initialize Mask
  vtkNew<vtkBitArray> newMask;
  htg->SetMask(newMask);
  vtkIdType treeOffset = 0;
  int numberOfTrees = (updateExtent[1] - updateExtent[0]) * (updateExtent[3] - updateExtent[2]) *
    (updateExtent[5] - updateExtent[4]);
  if (numberOfTrees <= 0)
  {
    // Nothing to generate
    return 1;
  }

  // Gather all tree ids in a vector
  std::vector<int> hyperTrees;
  hyperTrees.reserve(numberOfTrees);
  for (int i = updateExtent[0]; i < updateExtent[1]; ++i)
  {
    for (int j = updateExtent[2]; j < updateExtent[3]; ++j)
    {
      for (int k = updateExtent[4]; k < updateExtent[5]; ++k)
      {
        vtkIdType treeId;
        htg->GetIndexFromLevelZeroCoordinates(treeId, static_cast<unsigned int>(i),
          static_cast<unsigned int>(j), static_cast<unsigned int>(k));
        hyperTrees.emplace_back(treeId);
      }
    }
  }

  /* Subdivision and masking are done in 2 separate loops, because
   * our algorithm tends to mask way more easily the first trees it encounters.
   * So we need to shuffle the order in which we process the trees to avoid having a highly
   * biaised masking. That's why we need to firstly generate the whole HTG before masking it.
   */

  // Subdivision
  for (int treeId : hyperTrees)
  {
    /* Initialize RNG per tree to make it easier to distribute,
     * also make the RNG piece dependent to avoid biais accross
     * distributed data.
     */
    this->NodeRNG->Initialize(this->Seed + treeId + piece);

    // Build this tree:
    auto cursor = vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::Take(
      htg->NewNonOrientedCursor(treeId, true));
    cursor->GetTree()->SetGlobalIndexStart(treeOffset);
    this->SubdivideLeaves(cursor, treeId);
    treeOffset += cursor->GetTree()->GetNumberOfVertices();
  }
  // Need to shuffle Trees to avoid bias
  vtkNew<vtkMinimalStandardRandomSequence> treesRNG;
  treesRNG->Initialize(this->Seed + piece);
  // Shuffle the tree ids order for masking
  ::ShuffleArray(hyperTrees, treesRNG);

  // We need to keep track of the fraction of trees masked at the root level,
  // since our algorithm masks a fraction of each level.
  double treeSiblingsFractionMasked = 0;
  double errorMargin = 1.0 / numberOfTrees;

  // Masking
  for (int treeId : hyperTrees)
  {
    /* Initialize RNG per tree to make it easier to distribute,
     * also make the RNG piece dependent to avoid biais accross
     * distributed data.
     */
    this->MaskRNG->Initialize(this->Seed + treeId + piece);

    auto cursor = vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::Take(
      htg->NewNonOrientedCursor(treeId, true));
    double unmaskedFraction = 1.0;
    if (this->MaskedFraction == 1.0)
    {
      cursor->SetMask(true);
      unmaskedFraction = 0.0;
    }
    else if (this->MaskedFraction > 0)
    {
      unmaskedFraction =
        this->GenerateMask(cursor, treeId, 1.0, false, treeSiblingsFractionMasked, errorMargin);
    }
    double maskedTreeFraction = 1.0 - unmaskedFraction;

    this->ActualMaskedCellFraction += maskedTreeFraction;
    // This accumulates floating point errors which cause the mask to not work
    // properly for masked fraction values very close to 1.
    treeSiblingsFractionMasked += (maskedTreeFraction / numberOfTrees);
  }

  // We could use treeSiblingsMasked but computing it at the end avoids
  // float error accumulation.
  this->ActualMaskedCellFraction /= numberOfTrees;
  // Cleanup
  this->Levels = nullptr;
  return 1;
}

//------------------------------------------------------------------------------
int vtkRandomHyperTreeGridSource::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
void vtkRandomHyperTreeGridSource::SubdivideLeaves(
  vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType treeId)
{
  vtkIdType vertexId = cursor->GetVertexId();
  vtkHyperTree* tree = cursor->GetTree();
  vtkIdType idx = tree->GetGlobalIndexFromLocal(vertexId);
  vtkIdType level = cursor->GetLevel();
  this->Levels->InsertValue(idx, level);
  cursor->SetMask(false);
  if (cursor->IsLeaf())
  {
    if (this->ShouldRefine(level))
    {
      cursor->SubdivideLeaf();
      this->SubdivideLeaves(cursor, treeId);
    }
  }
  else
  {
    int numChildren = cursor->GetNumberOfChildren();
    for (int childIdx = 0; childIdx < numChildren; ++childIdx)
    {
      cursor->ToChild(childIdx);
      this->SubdivideLeaves(cursor, treeId);
      cursor->ToParent();
    }
  }
}

//------------------------------------------------------------------------------
bool vtkRandomHyperTreeGridSource::ShouldRefine(vtkIdType level)
{
  this->NodeRNG->Next();
  return level < this->MaxDepth && this->NodeRNG->GetValue() < this->SplitFraction;
}

//------------------------------------------------------------------------------
double vtkRandomHyperTreeGridSource::GenerateMask(vtkHyperTreeGridNonOrientedCursor* cursor,
  vtkIdType treeId, double unmaskedFraction, bool isParentMasked, double siblingsFractionMasked,
  double errorMargin)
{
  int numChildren = cursor->GetNumberOfChildren();
  vtkIdType level = cursor->GetLevel();
  bool isMasked = false;
  double resultUnmaskedFraction = unmaskedFraction;

  // Initialize mask for new leaves
  cursor->SetMask(false);
  if (!isParentMasked)
  {
    double maskingCost = this->GetMaskingNodeCost(level);
    if (this->ShouldMask(siblingsFractionMasked, level, errorMargin))
    {
      /* Reduce the unmasked proportion only if we mask
       * the root of a subtree. Since its proportion
       * is equal to the sum of its children, we only need
       * the reduction for the root.
       * Also this line isn't thread safe
       */
      resultUnmaskedFraction -= maskingCost;
      isMasked = true;
    }
  }
  isMasked = isMasked || isParentMasked;
  if (!cursor->IsLeaf())
  {
    // Need to shuffle children to avoid bias and get interesting results
    int nbChildMasked = 0;
    double childUnmaskedFraction = unmaskedFraction;
    std::vector<int> children;
    children.resize(numChildren);
    std::iota(children.begin(), children.end(), 0);
    ::ShuffleArray(children, this->MaskRNG);

    for (int childIdx : children)
    {
      double previousUnmaskedFraction = childUnmaskedFraction;
      // Can't mask cursor before visiting child
      cursor->ToChild(childIdx);
      double maskedChildrenFraction =
        static_cast<double>(nbChildMasked) / static_cast<double>(numChildren);
      childUnmaskedFraction = this->GenerateMask(cursor, treeId, previousUnmaskedFraction, isMasked,
        maskedChildrenFraction, 1.0 / numChildren);
      if (childUnmaskedFraction < previousUnmaskedFraction)
      {
        nbChildMasked += 1;
      }
      cursor->ToParent();
    }
    if (!isMasked)
    {
      resultUnmaskedFraction = childUnmaskedFraction;
    }
    if (nbChildMasked == numChildren)
    {
      isMasked = true;
    }
  }
  cursor->SetMask(isMasked);
  return resultUnmaskedFraction;
}

//------------------------------------------------------------------------------
bool vtkRandomHyperTreeGridSource::ShouldMask(
  double siblingsFractionMasked, int level, double errorMargin)
{
  int levelWeight = std::max(1, static_cast<int>(level * this->MaxDepth));
  // We penalize the masking of high depth node since they are harder to attain.
  // This allows to have more deep nodes unmasked.
  return siblingsFractionMasked * levelWeight <= this->MaskedFraction - errorMargin;
}

//------------------------------------------------------------------------------
void vtkRandomHyperTreeGridSource::InitializeMaskingNodeCostPerLevel()
{
  this->MaskingCostPerLevel.resize(this->MaxDepth + 1);
  int numberOfDimensions = sizeof(this->Dimensions) / sizeof(unsigned int);
  int numberOfChildPerNode = std::pow(::BRANCHING_FACTOR, numberOfDimensions);
  for (vtkIdType i = 1; i <= this->MaxDepth; ++i)
  {
    double maskingCost = this->MaskingCostPerLevel.at(i - 1) / numberOfChildPerNode;
    this->MaskingCostPerLevel[i] = maskingCost;
  }
}

//------------------------------------------------------------------------------
double vtkRandomHyperTreeGridSource::GetMaskingNodeCost(int level)
{
  return this->MaskingCostPerLevel.at(level);
}

VTK_ABI_NAMESPACE_END
