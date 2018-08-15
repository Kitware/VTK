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
#include "vtkHyperTreeCursor.h"
#include "vtkHyperTreeGrid.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkRandomHyperTreeGridSource)

//------------------------------------------------------------------------------
void vtkRandomHyperTreeGridSource::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkRandomHyperTreeGridSource::vtkRandomHyperTreeGridSource()
  : Seed(0)
  , MaxDepth(5)
  , SplitFraction(0.5)
  , HTG(nullptr)
  , Levels(nullptr)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->GridSize[0] = 5;
  this->GridSize[1] = 5;
  this->GridSize[2] = 2;

  for (size_t i = 0; i < 3; ++i)
  {
    this->OutputBounds[2 * i] = -10.;
    this->OutputBounds[2 * i + 1] = 10.;
  }
}

//------------------------------------------------------------------------------
vtkRandomHyperTreeGridSource::~vtkRandomHyperTreeGridSource()
{
}

//------------------------------------------------------------------------------
int vtkRandomHyperTreeGridSource::RequestInformation(
    vtkInformation *req, vtkInformationVector **inInfo,
    vtkInformationVector *outInfo)
{
  using SDDP = vtkStreamingDemandDrivenPipeline;

  if (!this->Superclass::RequestInformation(req, inInfo, outInfo))
  {
    return 0;
  }

  int wholeExtent[6] = {
    0, static_cast<int>(this->GridSize[0]),
    0, static_cast<int>(this->GridSize[1]),
    0, static_cast<int>(this->GridSize[2]),
  };

  vtkInformation *info = outInfo->GetInformationObject(0);
  info->Set(SDDP::WHOLE_EXTENT(), wholeExtent, 6);
  info->Set(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 1);

  return 1;
}

//------------------------------------------------------------------------------
int vtkRandomHyperTreeGridSource::RequestData(vtkInformation *,
                                              vtkInformationVector **,
                                              vtkInformationVector *outInfos)
{
  using SDDP = vtkStreamingDemandDrivenPipeline;

  vtkInformation *outInfo = outInfos->GetInformationObject(0);

  int *updateExtent = outInfo->Get(SDDP::UPDATE_EXTENT());

  // Create dataset:
  auto fillArray = [](vtkDoubleArray *array, vtkIdType numPoints,
                      double minBound, double maxBound)
  {
    array->SetNumberOfComponents(1);
    array->SetNumberOfTuples(numPoints);
    double step = (maxBound - minBound) / static_cast<double>(numPoints);
    for (int i = 0; i < numPoints; ++i)
    {
      array->SetTypedComponent(i, 0, minBound + step * i);
    }
  };

  vtkHyperTreeGrid *htg = vtkHyperTreeGrid::GetData(outInfo);
  htg->Initialize();
  htg->SetGridSize(this->GridSize);
  htg->SetDimension(3);
  htg->SetBranchFactor(2);
  this->HTG = htg;

  {
    vtkNew<vtkDoubleArray> coords;
    fillArray(coords, this->GridSize[0] + 1,
              this->OutputBounds[0], this->OutputBounds[1]);
    htg->SetXCoordinates(coords);
  }

  {
    vtkNew<vtkDoubleArray> coords;
    fillArray(coords, this->GridSize[1] + 1,
              this->OutputBounds[2], this->OutputBounds[3]);
    htg->SetYCoordinates(coords);
  }

  {
    vtkNew<vtkDoubleArray> coords;
    fillArray(coords, this->GridSize[2] + 1,
              this->OutputBounds[4], this->OutputBounds[5]);
    htg->SetZCoordinates(coords);
  }

  vtkNew<vtkDoubleArray> levels;
  levels->SetName("level");
  htg->GetPointData()->AddArray(levels);
  this->Levels = levels;

  vtkNew<vtkIdTypeArray> mask;
  mask->SetName("MaterialMaskIndex");

  vtkIdType treeOffset = 0;
  for (int i = updateExtent[0]; i < updateExtent[1]; ++i)
  {
    for (int j = updateExtent[2]; j < updateExtent[3]; ++j)
    {
      for (int k = updateExtent[4]; k < updateExtent[5]; ++k)
      {
        vtkIdType treeId;
        htg->GetIndexFromLevelZeroCoordinates(treeId,
                                              static_cast<unsigned int>(i),
                                              static_cast<unsigned int>(j),
                                              static_cast<unsigned int>(k));

        mask->InsertNextValue(treeId);

        // Initialize RNG per tree to make it easier to parallelize
        this->RNG->Initialize(this->Seed + treeId);

        // Build this tree:
        vtkHyperTreeCursor *cursor = htg->NewCursor(treeId, true);
        cursor->ToRoot();
        cursor->GetTree()->SetGlobalIndexStart(treeOffset);
        this->SubdivideLeaves(cursor, treeId);
        treeOffset += cursor->GetTree()->GetNumberOfVertices();
        cursor->Delete();
      }
    }
  }

  htg->SetMaterialMaskIndex(mask);

  // Cleanup
  this->HTG = nullptr;
  this->Levels = nullptr;

  return 1;
}

//------------------------------------------------------------------------------
int vtkRandomHyperTreeGridSource::FillOutputPortInformation(
    int, vtkInformation *info)
{
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
    return 1;
}

//------------------------------------------------------------------------------
void vtkRandomHyperTreeGridSource::SubdivideLeaves(vtkHyperTreeCursor *cursor,
                                                   vtkIdType treeId)
{
  vtkIdType vertexId = cursor->GetVertexId();
  vtkHyperTree *tree = cursor->GetTree();
  vtkIdType idx = tree->GetGlobalIndexFromLocal(vertexId);
  vtkIdType level = cursor->GetLevel();

  this->Levels->InsertValue(idx, level);

  if (cursor->IsLeaf())
  {
    if (this->ShouldRefine(level))
    {
      this->HTG->SubdivideLeaf(cursor, treeId);
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
