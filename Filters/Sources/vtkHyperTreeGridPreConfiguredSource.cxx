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

vtkStandardNewMacro(vtkHyperTreeGridPreConfiguredSource);

vtkHyperTreeGridPreConfiguredSource::vtkHyperTreeGridPreConfiguredSource()
  : CustomDim(2)
  , CustomFactor(2)
  , CustomDepth(2)
  , CustomSubdivisions(2, 2)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->AppropriateOutput = true;

  this->CustomExtent.resize(4);
  this->CustomExtent[0] = this->CustomExtent[2] = 0.0;
  this->CustomExtent[1] = this->CustomExtent[3] = 1.0;
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
  int dimension = 2;
  int depth = 3;
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
      std::copy(this->CustomExtent.begin(), this->CustomExtent.end(), wholeExtent);
      dimension = this->CustomDim;
      depth = this->CustomDepth;
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
  vtkHyperTreeGrid* HTG = vtkHyperTreeGrid::GetData(outInfo);
  if (!HTG)
  {
    vtkErrorMacro("Could not get HyperTreeGrid output");
    return 0;
  }

  return this->ProcessTrees(nullptr, HTG);
}

int vtkHyperTreeGridPreConfiguredSource::ProcessTrees(
  vtkHyperTreeGrid* vtkNotUsed(inputObj), vtkDataObject* HTGObj)
{
  vtkHyperTreeGrid* HTG = vtkHyperTreeGrid::SafeDownCast(HTGObj);
  switch (this->HTGMode)
  {
    case UNBALANCED_3DEPTH_2BRANCH_2X3:
      this->GenerateUnbalanced3DepthQuadTree2x3(HTG);
      break;
    case BALANCED_3DEPTH_2BRANCH_2X3:
      this->GenerateBalanced3DepthQuadTree2x3(HTG);
      break;
    case UNBALANCED_2DEPTH_3BRANCH_3X3:
      this->GenerateUnbalanced2Depth3BranchTree3x3(HTG);
      break;
    case BALANCED_4DEPTH_3BRANCH_2X2:
      this->GenerateBalanced4Depth3BranchTree2x2(HTG);
      break;
    case UNBALANCED_3DEPTH_2BRANCH_3X2X3:
      this->GenerateUnbalanced3DepthOctTree3x2x3(HTG);
      break;
    case BALANCED_2DEPTH_3BRANCH_3X3X2:
      this->GenerateBalanced2Depth3BranchTree3x3x2(HTG);
      break;
    case CUSTOM:
      if (!this->GenerateCustom(HTG))
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

void vtkHyperTreeGridPreConfiguredSource::GenerateUnbalanced(vtkHyperTreeGrid* HTG, int dim,
  int factor, int depth, const std::vector<double>& extent, const std::vector<int>& subdivisions)
{
  this->Preprocess(HTG, dim, factor, extent, subdivisions);

  vtkNew<vtkDoubleArray> levels;
  levels->SetName("Depth");
  levels->SetNumberOfComponents(1);
  levels->SetNumberOfTuples(0);
  HTG->GetCellData()->AddArray(levels);

  auto cursor = vtk::TakeSmartPointer(HTG->NewNonOrientedCursor(0, true));
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

  int nTrees = 1;
  for (auto sub : subdivisions)
  {
    nTrees *= (sub - 1);
  }
  for (int iT = 1; iT < nTrees; iT++)
  {
    cursor = vtk::TakeSmartPointer(HTG->NewNonOrientedCursor(iT, true));
    vtkHyperTree* tree = cursor->GetTree();
    tree->SetGlobalIndexStart(treeOffset);
    vtkIdType globId = tree->GetGlobalIndexFromLocal(0);
    levels->InsertValue(globId, 0);
    treeOffset += tree->GetNumberOfVertices();
  }
}

void vtkHyperTreeGridPreConfiguredSource::GenerateBalanced(vtkHyperTreeGrid* HTG, int dim,
  int factor, int depth, const std::vector<double>& extent, const std::vector<int>& subdivisions)
{
  this->Preprocess(HTG, dim, factor, extent, subdivisions);

  vtkNew<vtkDoubleArray> levels;
  levels->SetName("Depth");
  levels->SetNumberOfComponents(1);
  levels->SetNumberOfTuples(0);
  HTG->GetCellData()->AddArray(levels);

  vtkIdType treeOffset = 0;
  int nTrees = 1;
  for (auto sub : subdivisions)
  {
    nTrees *= (sub - 1);
  }
  for (int iT = 0; iT < nTrees; iT++)
  {
    auto cursor = vtk::TakeSmartPointer(HTG->NewNonOrientedCursor(iT, true));
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

void vtkHyperTreeGridPreConfiguredSource::Preprocess(vtkHyperTreeGrid* HTG, int dim, int factor,
  const std::vector<double>& extent, const std::vector<int>& subdivisions)
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
  HTG->Initialize();
  std::array<int, 3> subdivisions3d = { 1, 1, 1 };
  std::copy(subdivisions.begin(), subdivisions.end(), subdivisions3d.begin());
  HTG->SetDimensions(subdivisions3d.data());
  HTG->SetBranchFactor(factor);
  for (int d = 0; d < dim; d++)
  {
    vtkNew<vtkDoubleArray> vtkCoords;
    vtkCoords->SetNumberOfComponents(1);
    vtkCoords->SetNumberOfTuples(subdivisions[d]);
    double step = (extent[d * 2 + 1] - extent[d * 2]) / (subdivisions[d] - 1.0);
    for (int i = 0; i < subdivisions[d]; i++)
    {
      vtkCoords->InsertValue(i, extent[d * 2] + step * i);
    }
    switch (d)
    {
      case (0):
        HTG->SetXCoordinates(vtkCoords);
        break;
      case (1):
        HTG->SetYCoordinates(vtkCoords);
        break;
      case (2):
        HTG->SetZCoordinates(vtkCoords);
        break;
    }
  }
}

void vtkHyperTreeGridPreConfiguredSource::GenerateUnbalanced3DepthQuadTree2x3(vtkHyperTreeGrid* HTG)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::vector<int> subdivisions = { 2, 3 };
  this->GenerateUnbalanced(HTG, 2, 2, 3, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateBalanced3DepthQuadTree2x3(vtkHyperTreeGrid* HTG)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::vector<int> subdivisions = { 2, 3 };
  this->GenerateBalanced(HTG, 2, 2, 3, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateUnbalanced2Depth3BranchTree3x3(
  vtkHyperTreeGrid* HTG)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::vector<int> subdivisions = { 3, 3 };
  this->GenerateUnbalanced(HTG, 2, 3, 2, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateBalanced4Depth3BranchTree2x2(
  vtkHyperTreeGrid* HTG)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::vector<int> subdivisions = { 2, 2 };
  this->GenerateBalanced(HTG, 2, 3, 4, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateUnbalanced3DepthOctTree3x2x3(
  vtkHyperTreeGrid* HTG)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  std::vector<int> subdivisions = { 3, 2, 3 };
  this->GenerateUnbalanced(HTG, 3, 2, 3, extent, subdivisions);
}

void vtkHyperTreeGridPreConfiguredSource::GenerateBalanced2Depth3BranchTree3x3x2(
  vtkHyperTreeGrid* HTG)
{
  std::vector<double> extent = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  std::vector<int> subdivisions = { 3, 3, 2 };
  this->GenerateBalanced(HTG, 3, 3, 2, extent, subdivisions);
}

int vtkHyperTreeGridPreConfiguredSource::GenerateCustom(vtkHyperTreeGrid* HTG)
{
  if (this->CustomExtent.size() < static_cast<unsigned int>(this->CustomDim * 2))
  {
    vtkErrorMacro("Custom extent array is not long enough");
    return 0;
  }

  if (this->CustomSubdivisions.size() < static_cast<unsigned int>(this->CustomDim))
  {
    vtkErrorMacro("Custom subdivisions array is not long enough");
    return 0;
  }

  switch (this->CustomArchitecture)
  {
    case BALANCED:
    {
      this->GenerateBalanced(HTG, this->CustomDim, this->CustomFactor, this->CustomDepth,
        this->CustomExtent, this->CustomSubdivisions);
      break;
    }
    case UNBALANCED:
    {
      this->GenerateUnbalanced(HTG, this->CustomDim, this->CustomFactor, this->CustomDepth,
        this->CustomExtent, this->CustomSubdivisions);
      break;
    }
    default:
      return 0;
  }
  return 1;
}

void vtkHyperTreeGridPreConfiguredSource::SetCustomExtent(int extentSize, double* extent)
{
  vtkDebugMacro(<< "setting CustomExtent dynamically");
  this->CustomExtent.resize(extentSize);
  std::copy(extent, extent + extentSize, this->CustomExtent.begin());
  this->Modified();
}

void vtkHyperTreeGridPreConfiguredSource::SetCustomSubdivisions(int subSize, int* subdivisions)
{
  vtkDebugMacro(<< "setting CustomSubdivisons dynamically");
  this->CustomSubdivisions.resize(subSize);
  std::copy(subdivisions, subdivisions + subSize, this->CustomSubdivisions.begin());
  this->Modified();
}
