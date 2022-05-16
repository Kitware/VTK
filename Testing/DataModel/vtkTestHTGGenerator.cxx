/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestHTGGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyrgight notice for more information.

=========================================================================*/

#include "vtkTestHTGGenerator.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridNonOrientedCursor.h"

vtkStandardNewMacro(vtkTestHTGGenerator);

void vtkTestHTGGenerator::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->HTG)
  {
    os << "vtkTestHTGGenerator having generated the following HTG:\n";
    this->HTG->PrintSelf(os, indent);
  }
  else
  {
    os << "vtkTestHTGGenerator not having any internal HTG\n";
  }
}

template <int dim, int factor, int depth>
void vtkTestHTGGenerator::generateUnbalanced(
  const std::array<double, 2 * dim>& extent, const std::array<int, dim>& subdivisions)
{
  this->preprocess<dim, factor>(extent, subdivisions);

  vtkNew<vtkDoubleArray> levels;
  levels->SetName("Depth");
  this->HTG->GetCellData()->AddArray(levels);

  vtkHyperTreeGridNonOrientedCursor* cursor = this->HTG->NewNonOrientedCursor(0, true);
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
  cursor->Delete();
  unsigned int nTrees = 1;
  for (auto sub : subdivisions)
  {
    nTrees *= (sub - 1);
  }
  for (int iT = 1; iT < nTrees; iT++)
  {
    cursor = this->HTG->NewNonOrientedCursor(iT, true);
    cursor->GetTree()->SetGlobalIndexStart(treeOffset);
    vtkIdType vertexId = cursor->GetVertexId();
    vtkIdType globId = cursor->GetTree()->GetGlobalIndexFromLocal(vertexId);
    levels->InsertValue(globId, 0);
    treeOffset += cursor->GetTree()->GetNumberOfVertices();
    cursor->Delete();
  }
}

template <int dim, int factor, int depth>
void vtkTestHTGGenerator::generateBalanced(
  const std::array<double, 2 * dim>& extent, const std::array<int, dim>& subdivisions)
{
  this->preprocess<dim, factor>(extent, subdivisions);

  vtkNew<vtkDoubleArray> levels;
  levels->SetName("Depth");
  this->HTG->GetCellData()->AddArray(levels);

  vtkIdType treeOffset = 0;
  unsigned int nTrees = 1;
  for (auto sub : subdivisions)
  {
    nTrees *= sub;
  }
  for (int iT = 0; iT < nTrees; iT++)
  {
    vtkHyperTreeGridNonOrientedCursor* cursor = this->HTG->NewNonOrientedCursor(iT, true);
    cursor->GetTree()->SetGlobalIndexStart(treeOffset);
    this->recurseBalanced(cursor, levels, depth);
    treeOffset += cursor->GetTree()->GetNumberOfVertices();
    cursor->Delete();
  }
}

void vtkTestHTGGenerator::recurseBalanced(
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
      this->recurseBalanced(cursor, levels, maxDepth);
    }
  }
  else
  {
    int numChildren = cursor->GetNumberOfChildren();
    for (int iChild = 0; iChild < numChildren; iChild++)
    {
      cursor->ToChild(iChild);
      this->recurseBalanced(cursor, levels, maxDepth);
      cursor->ToParent();
    }
  }
}

template <int dim, int factor>
void vtkTestHTGGenerator::preprocess(
  const std::array<double, 2 * dim>& extent, const std::array<int, dim>& subdivisions)
{
  this->clear();
  this->HTG = vtkHyperTreeGrid::New();
  std::array<int, 3> subdivisions3d = { 1, 1, 1 };
  std::copy(subdivisions.begin(), subdivisions.end(), subdivisions3d.begin());
  this->HTG->SetDimensions(subdivisions3d.begin());
  this->HTG->SetBranchFactor(factor);
  for (int d = 0; d < dim; d++)
  {
    vtkNew<vtkDoubleArray> vtkCoords;
    double step = (extent[d * 2 + 1] - extent[d * 2]) / (subdivisions[d] - 1.0);
    for (int i = 0; i < subdivisions[d]; i++)
    {
      vtkCoords->InsertValue(i, extent[d * 2] + step * i);
    }
    switch (d)
    {
      case (0):
        this->HTG->SetXCoordinates(vtkCoords);
        break;
      case (1):
        this->HTG->SetYCoordinates(vtkCoords);
        break;
      case (2):
        this->HTG->SetZCoordinates(vtkCoords);
        break;
    }
  }
}

void vtkTestHTGGenerator::generateUnbalanced3DepthQuadTree2x3()
{
  std::array<double, 4> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::array<int, 2> subdivisions = { 2, 3 };
  generateUnbalanced<2, 2, 3>(extent, subdivisions);
}

void vtkTestHTGGenerator::generateBalanced3DepthQuadTree2x3()
{
  std::array<double, 4> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::array<int, 2> subdivisions = { 2, 3 };
  generateBalanced<2, 2, 3>(extent, subdivisions);
}

void vtkTestHTGGenerator::generateUnbalanced2Depth3BranchTree3x3()
{
  std::array<double, 4> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::array<int, 2> subdivisions = { 3, 3 };
  generateUnbalanced<2, 3, 2>(extent, subdivisions);
}

void vtkTestHTGGenerator::generateBalanced4Depth3BranchTree2x2()
{
  std::array<double, 4> extent = { -1.0, 1.0, -1.0, 1.0 };
  std::array<int, 2> subdivisions = { 2, 2 };
  generateBalanced<2, 3, 4>(extent, subdivisions);
}

void vtkTestHTGGenerator::generateUnbalanced3DepthOctTree3x2x3()
{
  std::array<double, 6> extent = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  std::array<int, 3> subdivisions = { 3, 2, 3 };
  generateUnbalanced<3, 2, 3>(extent, subdivisions);
}

void vtkTestHTGGenerator::generateBalanced2Depth3BranchTree3x3x2()
{
  std::array<double, 6> extent = { -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 };
  std::array<int, 3> subdivisions = { 3, 3, 2 };
  generateBalanced<3, 3, 2>(extent, subdivisions);
}
