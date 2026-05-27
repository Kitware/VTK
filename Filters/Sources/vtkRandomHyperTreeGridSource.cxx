// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkRandomHyperTreeGridSource.h"

#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkObjectFactory.h"
#include "vtkSetGet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkType.h"

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkRandomHyperTreeGridSource);

namespace
{
// The BranchingFactor could have its dedicated setter/getter
// and be a public member.
// But for now only 2 is supported as a value.
constexpr int BRANCHING_FACTOR = 2;
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
  const int* updateExtent = outInfo->Get(SDDP::UPDATE_EXTENT());

  // Create dataset:
  auto fillArray = [](vtkDoubleArray* array, vtkIdType numPoints, double minBound, double maxBound)
  {
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

  vtkIdType treeOffset = 0;
  int numberOfTrees = (updateExtent[1] - updateExtent[0]) * (updateExtent[3] - updateExtent[2]) *
    (updateExtent[5] - updateExtent[4]);
  if (numberOfTrees <= 0)
  {
    // Nothing to generate
    return 1;
  }

  // Gather all tree ids in a vector
  std::vector<vtkIdType> hyperTrees;
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

  // Estimate the number of cells using the geometric series summation formula
  // for numberOfTrees * (branch_factor^dimension * SplitFraction)^k, k in
  vtkNew<vtkDoubleArray> levels;
  levels->SetName("Depth");
  vtkIdType expFactor = std::pow(::BRANCHING_FACTOR, htg->GetDimension()) * this->SplitFraction;
  vtkIdType estNumTuples =
    numberOfTrees * (std::pow(expFactor, this->MaxDepth + 1) - 1) / (expFactor - 1);
  levels->ReserveTuples(estNumTuples);

  htg->GetCellData()->AddArray(levels);
  vtkNew<vtkBitArray> newMask;
  htg->SetMask(newMask);

  // Subdivision & Masking
  for (int treeId : hyperTrees)
  {
    this->NodeRNG->Initialize(this->Seed + treeId);
    this->MaskRNG->Initialize(this->Seed + treeId + 1); // Use a different seed for masking

    // Build this tree:
    auto cursor = vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor>::Take(
      htg->NewNonOrientedCursor(treeId, true));
    cursor->GetTree()->SetGlobalIndexStart(treeOffset);
    this->SubdivideLeaves(cursor, treeId, levels);
    treeOffset += cursor->GetTree()->GetNumberOfVertices();
  }

  vtkDebugMacro(<< "Estimated tuple size " << estNumTuples << " ; Real "
                << levels->GetNumberOfTuples());

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
  vtkHyperTreeGridNonOrientedCursor* cursor, vtkIdType treeId, vtkDoubleArray* depthArray)
{
  vtkIdType vertexId = cursor->GetVertexId();
  vtkIdType idx = cursor->GetTree()->GetGlobalIndexFromLocal(vertexId);
  vtkIdType level = cursor->GetLevel();
  depthArray->InsertValue(idx, level);
  cursor->SetMask(false);

  if (this->ShouldRefine() && level < this->MaxDepth)
  {
    cursor->SubdivideLeaf();
    int numChildren = cursor->GetNumberOfChildren();
    for (int childIdx = 0; childIdx < numChildren; ++childIdx)
    {
      cursor->ToChild(childIdx);
      this->SubdivideLeaves(cursor, treeId, depthArray);
      cursor->ToParent();
    }
  }
  else if (this->ShouldMask())
  {
    // Only mask leaf cells
    cursor->SetMask(true);
  }
}

//------------------------------------------------------------------------------
bool vtkRandomHyperTreeGridSource::ShouldRefine()
{
  this->NodeRNG->Next();
  return this->NodeRNG->GetValue() < this->SplitFraction;
}

//------------------------------------------------------------------------------
bool vtkRandomHyperTreeGridSource::ShouldMask()
{
  this->MaskRNG->Next();
  return this->MaskRNG->GetValue() < this->MaskedFraction;
}

VTK_ABI_NAMESPACE_END
