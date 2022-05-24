/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridPreConfiguredSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyrgight notice for more information.

=========================================================================*/

#include "vtkHyperTreeGridPreConfiguredSource.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"

#include <array>
#include <vector>

vtkStandardNewMacro(vtkHyperTreeGridPreConfiguredSource);

vtkHyperTreeGridPreConfiguredSource::vtkHyperTreeGridPreConfiguredSource()
  : HTGMode(UNBALANCED_3DEPTH_2BRANCH_2X3)
  , CustomArchitecture(UNBALANCED)
  , CustomDim(2)
  , CustomFactor(2)
  , CustomDepth(2)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->AppropriateOutput = true;

  this->CustomExtent[0] = this->CustomExtent[2] = this->CustomExtent[4] = 0.0;
  this->CustomExtent[1] = this->CustomExtent[3] = this->CustomExtent[5] = 1.0;

  this->CustomSubdivisions[0] = this->CustomSubdivisions[1] = this->CustomSubdivisions[2] = 2;
}

int vtkHyperTreeGridPreConfiguredSource::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

int vtkHyperTreeGridPreConfiguredSource::RequestInformation(
  vtkInformation* req, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestInformation(req, inputVector, outputVector))
  {
    return 0;
  }
  int wholeExtent[6] = { 0, 1, 0, 1, 0, 1 };
  unsigned int dimension = 2;
  unsigned int depth = 3;
  switch (this->HTGMode)
  {
    case UNBALANCED_3DEPTH_2BRANCH_2X3:
      wholeExtent[3] = 2;
      break;
    case BALANCED_3DEPTH_2BRANCH_2X3:
      wholeExtent[3] = 2;
      break;
    case UNBALANCED_2DEPTH_3BRANCH_3X3:
      wholeExtent[1] = wholeExtent[3] = 2;
      depth = 2;
      break;
    case BALANCED_4DEPTH_3BRANCH_2X2:
      depth = 4;
      break;
    case UNBALANCED_3DEPTH_2BRANCH_3X2X3:
      wholeExtent[1] = wholeExtent[5] = 2;
      dimension = 3;
      break;
    case BALANCED_2DEPTH_3BRANCH_3X3X2:
      wholeExtent[1] = wholeExtent[3] = 2;
      dimension = 3;
      depth = 2;
      break;
    case CUSTOM:
      wholeExtent[1] = this->CustomSubdivisions[0] - 1;
      wholeExtent[3] = this->CustomSubdivisions[1] - 1;
      wholeExtent[5] = this->CustomSubdivisions[2] - 1;
      dimension = this->CustomDim;
      depth = this->CustomDepth;
      break;
    default:
      vtkErrorMacro("No suitable HTG mode found.");
      return 0;
  }

  vtkInformation* info = outputVector->GetInformationObject(0);
  info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);
  info->Set(vtkHyperTreeGrid::LEVELS(), depth);
  info->Set(vtkHyperTreeGrid::DIMENSION(), dimension);
  info->Set(vtkAlgorithm::CAN_PRODUCE_SUB_EXTENT(), 0);
  return 1;
}

int vtkHyperTreeGridPreConfiguredSource::RequestData(vtkInformation* vtkNotUsed(req),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
  {
    vtkErrorMacro("Output information not found");
    return 0;
  }
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::GetData(outInfo);
  if (!htg)
  {
    vtkErrorMacro("Could not get HyperTreeGrid output");
    return 0;
  }

  return this->ProcessTrees(nullptr, htg);
}

int vtkHyperTreeGridPreConfiguredSource::ProcessTrees(
  vtkHyperTreeGrid* vtkNotUsed(inputObj), vtkDataObject* htgObj)
{
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htgObj);
  if (!htg)
  {
    vtkErrorMacro("Could not cast vtkDataObject to vtkHyperTreeGrid");
    return 0;
  }
  switch (this->HTGMode)
  {
    case UNBALANCED_3DEPTH_2BRANCH_2X3:
      this->GenerateUnbalanced3DepthQuadTree2x3(htg);
      break;
    case BALANCED_3DEPTH_2BRANCH_2X3:
      this->GenerateBalanced3DepthQuadTree2x3(htg);
      break;
    case UNBALANCED_2DEPTH_3BRANCH_3X3:
      this->GenerateUnbalanced2Depth3BranchTree3x3(htg);
      break;
    case BALANCED_4DEPTH_3BRANCH_2X2:
      this->GenerateBalanced4Depth3BranchTree2x2(htg);
      break;
    case UNBALANCED_3DEPTH_2BRANCH_3X2X3:
      this->GenerateUnbalanced3DepthOctTree3x2x3(htg);
      break;
    case BALANCED_2DEPTH_3BRANCH_3X3X2:
      this->GenerateBalanced2Depth3BranchTree3x3x2(htg);
      break;
    case CUSTOM:
      if (!this->GenerateCustom(htg))
      {
        vtkErrorMacro("Could not generate custom HyperTreeGrid");
        return 0;
      }
      break;
    default:
      vtkErrorMacro("Unsupported HTG mode");
      return 0;
  }
  return 1;
}

void vtkHyperTreeGridPreConfiguredSource::GenerateUnbalanced(vtkHyperTreeGrid* htg,
  unsigned int dim, unsigned int factor, unsigned int depth, const std::vector<double>& extent,
  const std::vector<unsigned int>& subdivisions)
{
  this->Preprocess(htg, dim, factor, extent, subdivisions);

  vtkNew<vtkDoubleArray> levels;
  levels->SetName("Depth");
  levels->SetNumberOfComponents(1);
  levels->SetNumberOfTuples(0);
  htg->GetCellData()->AddArray(levels);

  auto cursor = vtk::TakeSmartPointer(htg->NewNonOrientedCursor(0, true));
  cursor->GetTree()->SetGlobalIndexStart(0);
  levels->InsertValue(0, 0);
  for (vtkIdType l = 0; l < depth; l++)
  {
    cursor->SubdivideLeaf();
    int numChildren = cursor->GetNumberOfChildren();
    for (int iChild = 0; iChild < numChildren; iChild++)
    {
      cursor->ToChild(iChild);
      vtkIdType vertexId = cursor->GetVertexId();
      vtkIdType globId = cursor->GetTree()->GetGlobalIndexFromLocal(vertexId);
      levels->InsertValue(globId, l + 1);
      cursor->ToParent();
    }
    cursor->ToChild(0);
  }
  vtkIdType treeOffset = cursor->GetTree()->GetNumberOfVertices();

  vtkIdType nTrees = htg->GetMaxNumberOfTrees();
  for (int iT = 1; iT < nTrees; iT++)
  {
    cursor = vtk::TakeSmartPointer(htg->NewNonOrientedCursor(iT, true));
    vtkHyperTree* tree = cursor->GetTree();
    tree->SetGlobalIndexStart(treeOffset);
    vtkIdType globId = tree->GetGlobalIndexFromLocal(0);
    levels->InsertValue(globId, 0);
    treeOffset += tree->GetNumberOfVertices();
  }
}

void vtkHyperTreeGridPreConfiguredSource::GenerateBalanced(vtkHyperTreeGrid* htg, unsigned int dim,
  unsigned int factor, unsigned int depth, const std::vector<double>& extent,
  const std::vector<unsigned int>& subdivisions)
{
  this->Preprocess(htg, dim, factor, extent, subdivisions);

  vtkNew<vtkDoubleArray> levels;
  levels->SetName("Depth");
  levels->SetNumberOfComponents(1);
  levels->SetNumberOfTuples(0);
  htg->GetCellData()->AddArray(levels);

  vtkIdType treeOffset = 0;
  vtkIdType nTrees = htg->GetMaxNumberOfTrees();
  for (int iT = 0; iT < nTrees; iT++)
  {
    auto cursor = vtk::TakeSmartPointer(htg->NewNonOrientedCursor(iT, true));
    cursor->GetTree()->SetGlobalIndexStart(treeOffset);
    this->RecurseBalanced(cursor, levels, depth);
    treeOffset += cursor->GetTree()->GetNumberOfVertices();
  }
}

void vtkHyperTreeGridPreConfiguredSource::RecurseBalanced(
  vtkHyperTreeGridNonOrientedCursor* cursor, vtkDoubleArray* levels, const int maxDepth)
{
  vtkIdType vertexId = cursor->GetVertexId();
  vtkHyperTree* tree = cursor->GetTree();
  vtkIdType globId = tree->GetGlobalIndexFromLocal(vertexId);
  vtkIdType thisLevel = cursor->GetLevel();

  levels->InsertValue(globId, thisLevel);

  if (cursor->IsLeaf())
  {
    if (thisLevel < maxDepth)
    {
      cursor->SubdivideLeaf();
      this->RecurseBalanced(cursor, levels, maxDepth);
    }
  }
  else
  {
    int numChildren = cursor->GetNumberOfChildren();
    for (int iChild = 0; iChild < numChildren; iChild++)
    {
      cursor->ToChild(iChild);
      this->RecurseBalanced(cursor, levels, maxDepth);
      cursor->ToParent();
    }
  }
}

void vtkHyperTreeGridPreConfiguredSource::Preprocess(vtkHyperTreeGrid* htg, unsigned int dim,
  unsigned int factor, const std::vector<double>& extent,
  const std::vector<unsigned int>& subdivisions)
{
  if (extent.size() < static_cast<unsigned int>(2 * dim))
  {
    vtkErrorMacro("Supplied extent is not long enough");
    return;
  }
  if (subdivisions.size() < static_cast<unsigned int>(dim))
  {
    vtkErrorMacro("Supplied subdivisions is not long enough");
    return;
  }
  htg->Initialize();
  std::array<unsigned int, 3> subdivisions3d = { 1, 1, 1 };
  std::copy(subdivisions.begin(), subdivisions.end(), subdivisions3d.begin());
  htg->SetDimensions(subdivisions3d.data());
  htg->SetBranchFactor(factor);

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

  {
    vtkNew<vtkDoubleArray> vtkCoords;
    fillArray(vtkCoords, subdivisions[0], extent[0], extent[1]);
    htg->SetXCoordinates(vtkCoords);
  }

  if (dim > 1)
  {
    vtkNew<vtkDoubleArray> vtkCoords;
    fillArray(vtkCoords, subdivisions[1], extent[2], extent[3]);
    htg->SetYCoordinates(vtkCoords);
  }

  if (dim > 2)
  {
    vtkNew<vtkDoubleArray> vtkCoords;
    fillArray(vtkCoords, subdivisions[2], extent[4], extent[5]);
    htg->SetZCoordinates(vtkCoords);
  }
}

void vtkHyperTreeGridPreConfiguredSource::GenerateUnbalanced3DepthQuadTree2x3(vtkHyperTreeGrid* htg)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::vector<unsigned int> subdivisions = { 2, 3 };
  this->GenerateUnbalanced(htg, 2, 2, 3, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateBalanced3DepthQuadTree2x3(vtkHyperTreeGrid* htg)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::vector<unsigned int> subdivisions = { 2, 3 };
  this->GenerateBalanced(htg, 2, 2, 3, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateUnbalanced2Depth3BranchTree3x3(
  vtkHyperTreeGrid* htg)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::vector<unsigned int> subdivisions = { 3, 3 };
  this->GenerateUnbalanced(htg, 2, 3, 2, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateBalanced4Depth3BranchTree2x2(
  vtkHyperTreeGrid* htg)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::vector<unsigned int> subdivisions = { 2, 2 };
  this->GenerateBalanced(htg, 2, 3, 4, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateUnbalanced3DepthOctTree3x2x3(
  vtkHyperTreeGrid* htg)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  std::vector<unsigned int> subdivisions = { 3, 2, 3 };
  this->GenerateUnbalanced(htg, 3, 2, 3, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateBalanced2Depth3BranchTree3x3x2(
  vtkHyperTreeGrid* htg)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  std::vector<unsigned int> subdivisions = { 3, 3, 2 };
  this->GenerateBalanced(htg, 3, 3, 2, extent, subdivisions);
}

int vtkHyperTreeGridPreConfiguredSource::GenerateCustom(vtkHyperTreeGrid* htg)
{
  switch (this->CustomArchitecture)
  {
    case BALANCED:
    {
      this->GenerateBalanced(htg, this->CustomDim, this->CustomFactor, this->CustomDepth,
        std::vector<double>(this->CustomExtent, this->CustomExtent + this->CustomDim * 2),
        std::vector<unsigned int>(
          this->CustomSubdivisions, this->CustomSubdivisions + this->CustomDim));
      break;
    }
    case UNBALANCED:
    {
      this->GenerateUnbalanced(htg, this->CustomDim, this->CustomFactor, this->CustomDepth,
        std::vector<double>(this->CustomExtent, this->CustomExtent + this->CustomDim * 2),
        std::vector<unsigned int>(
          this->CustomSubdivisions, this->CustomSubdivisions + this->CustomDim));
      break;
    }
    default:
      vtkErrorMacro("Using an HTGArchitecture not yet supported by the source");
      return 0;
  }
  return 1;
}
