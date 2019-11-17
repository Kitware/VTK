/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomHyperTreeGridSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRandomHyperTreeGridSource.h"

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
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkRandomHyperTreeGridSource);

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
vtkRandomHyperTreeGridSource::~vtkRandomHyperTreeGridSource() {}

//------------------------------------------------------------------------------
int vtkRandomHyperTreeGridSource::RequestInformation(
  vtkInformation* req, vtkInformationVector** inInfo, vtkInformationVector* outInfo)
{
  using SDDP = vtkStreamingDemandDrivenPipeline;

  if (!this->Superclass::RequestInformation(req, inInfo, outInfo))
  {
    return 0;
  }

  int wholeExtent[6] = {
    0,
    static_cast<int>(this->Dimensions[0] - 1),
    0,
    static_cast<int>(this->Dimensions[1] - 1),
    0,
    static_cast<int>(this->Dimensions[2] - 1),
  };

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

  int* updateExtent = outInfo->Get(SDDP::UPDATE_EXTENT());

  // Create dataset:
  auto fillArray = [](
                     vtkDoubleArray* array, vtkIdType numPoints, double minBound, double maxBound) {
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(numPoints);
    double step = (maxBound - minBound) / static_cast<double>(numPoints - 1);
    for (int i = 0; i < numPoints; ++i)
    {
      array->SetTypedComponent(i, 0, minBound + step * i);
    }
  };

  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::GetData(outInfo);
  htg->Initialize();
  htg->SetDimensions(this->Dimensions);
  htg->SetBranchFactor(2);

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
  htg->GetPointData()->AddArray(levels);
  this->Levels = levels;

  vtkIdType treeOffset = 0;
  for (int i = updateExtent[0]; i < updateExtent[1]; ++i)
  {
    for (int j = updateExtent[2]; j < updateExtent[3]; ++j)
    {
      for (int k = updateExtent[4]; k < updateExtent[5]; ++k)
      {
        vtkIdType treeId;
        htg->GetIndexFromLevelZeroCoordinates(treeId, static_cast<unsigned int>(i),
          static_cast<unsigned int>(j), static_cast<unsigned int>(k));

        // Initialize RNG per tree to make it easier to parallelize
        this->RNG->Initialize(this->Seed + treeId);

        // Build this tree:
        vtkHyperTreeGridNonOrientedCursor* cursor = htg->NewNonOrientedCursor(treeId, true);
        cursor->GetTree()->SetGlobalIndexStart(treeOffset);
        this->SubdivideLeaves(cursor, treeId);
        treeOffset += cursor->GetTree()->GetNumberOfVertices();
        cursor->Delete();
      }
    }
  }

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
  this->RNG->Next();
  return level < this->MaxDepth && this->RNG->GetValue() < this->SplitFraction;
}
