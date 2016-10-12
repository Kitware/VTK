/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphHierarchicalBundle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkGraphHierarchicalBundle.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkEdgeListIterator.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTree.h"
#include "vtkVariantArray.h"

#include <map>

//---------------------------------------------------------------------------
template <typename T>
vtkVariant vtkGetValue(T* arr, vtkIdType index)
{
  return vtkVariant(arr[index]);
}

//---------------------------------------------------------------------------
static vtkVariant vtkGetVariantValue(vtkAbstractArray* arr, vtkIdType i)
{
  vtkVariant val;
  switch(arr->GetDataType())
  {
    vtkExtraExtendedTemplateMacro(val = vtkGetValue(
      static_cast<VTK_TT*>(arr->GetVoidPointer(0)), i));
  }
  return val;
}


vtkStandardNewMacro(vtkGraphHierarchicalBundle);

vtkGraphHierarchicalBundle::vtkGraphHierarchicalBundle()
{
  this->BundlingStrength = 0.8;
  this->SetNumberOfInputPorts(2);
  this->DirectMapping = false;
}

int vtkGraphHierarchicalBundle::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
    return 1;
  }
  return 0;
}

int vtkGraphHierarchicalBundle::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *graphInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *treeInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *graph = vtkGraph::SafeDownCast(
    graphInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree *tree = vtkTree::SafeDownCast(
    treeInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // If graph or tree is empty, we're done.
  if (graph->GetNumberOfVertices() == 0 ||
      tree->GetNumberOfVertices() == 0)
  {
    return 1;
  }

  // Create a map from graph indices to tree indices
  // If we are using DirectMapping this is trivial
  // we just create an identity map
  std::map<vtkIdType, vtkIdType> graphIndexToTreeIndex;
  if (this->DirectMapping)
  {
    if (graph->GetNumberOfVertices() > tree->GetNumberOfVertices())
    {
      vtkErrorMacro("Cannot have more graph vertices than tree vertices using direct mapping.");
      return 0;
    }
    // Create identity map.
    for (vtkIdType gv = 0; gv < graph->GetNumberOfVertices(); gv++)
    {
      graphIndexToTreeIndex[gv] = gv;
    }
  }

  // Okay if we do not have direct mapping then we need
  // to do some templated madness to go from an arbitrary
  // type to a nice vtkIdType to vtkIdType mapping
  if (!this->DirectMapping)
  {
    // Check for valid pedigree id arrays.
    vtkAbstractArray* graphIdArray =
      graph->GetVertexData()->GetPedigreeIds();
    if (!graphIdArray)
    {
      vtkErrorMacro("Graph pedigree id array not found.");
      return 0;
    }
    vtkAbstractArray* treeIdArray =
      tree->GetVertexData()->GetPedigreeIds();
    if (!treeIdArray)
    {
      vtkErrorMacro("Tree pedigree id array not found.");
      return 0;
    }

    std::map<vtkVariant,vtkIdType,vtkVariantLessThan> graphIdMap;

    // Create a map from graph id to graph index
    for (int i=0; i<graph->GetNumberOfVertices(); ++i)
    {
      graphIdMap[vtkGetVariantValue(graphIdArray,i)] = i;
    }

    // Now create the map from graph index to tree index
    for (int i=0; i<tree->GetNumberOfVertices(); ++i)
    {
      vtkVariant id = vtkGetVariantValue(treeIdArray,i);
      if (graphIdMap.count(id))
      {
        graphIndexToTreeIndex[graphIdMap[id]] = i;
      }
    }
  }

  // Make a point array holding the fraction of the distance
  // source to target.
  vtkPoints* newPoints = vtkPoints::New();
  newPoints->DeepCopy(tree->GetPoints());
  vtkFloatArray* fractionArray = vtkFloatArray::New();
  fractionArray->SetName("fraction");
  vtkIdType numVertices = tree->GetNumberOfVertices();
  for (vtkIdType i = 0; i < numVertices; i++)
  {
    fractionArray->InsertNextValue(0);
  }

  // Insert additional points for incoming vertices.
  for (vtkIdType i = 0; i < numVertices; i++)
  {
    double pt[3];
    newPoints->GetPoint(i, pt);
    newPoints->InsertNextPoint(pt);
    fractionArray->InsertNextValue(1);
  }

  // Prepare to copy cell data
  output->GetCellData()->CopyAllocate(graph->GetEdgeData());

  // Traverse graph edge list, adding polylines for each edge
  // using the tree hierarchy to "guide" the edges.
  vtkCellArray* newLines = vtkCellArray::New();
  vtkIdList* sourceList = vtkIdList::New();
  vtkIdList* targetList = vtkIdList::New();
  vtkSmartPointer<vtkEdgeListIterator> edges =
    vtkSmartPointer<vtkEdgeListIterator>::New();
  graph->GetEdges(edges);
  while (edges->HasNext())
  {
    vtkEdgeType e = edges->Next();
    unsigned int graphSourceIndex = e.Source;
    unsigned int graphTargetIndex = e.Target;

    // Do not render loops
    if (graphSourceIndex == graphTargetIndex)
    {
      continue;
    }

    vtkIdType source = 0;
    vtkIdType target = 0;
    if (graphIndexToTreeIndex.count(graphSourceIndex) > 0 &&
        graphIndexToTreeIndex.count(graphTargetIndex) > 0)
    {
      source = graphIndexToTreeIndex[graphSourceIndex];
      target = graphIndexToTreeIndex[graphTargetIndex];
    }
    else
    {
      // The endpoints of this edge are not found in the tree.
      continue;
    }

    // Find path from source to target
    sourceList->Reset();
    vtkIdType curSource = source;
    while (curSource != tree->GetRoot())
    {
      curSource = tree->GetParent(curSource);
      sourceList->InsertNextId(curSource);
    }
    targetList->Reset();
    vtkIdType curTarget = target;
    while (sourceList->IsId(curTarget) == -1 && curTarget != source)
    {
      curTarget = tree->GetParent(curTarget);
      targetList->InsertNextId(curTarget);
    }

    vtkIdType cellPoints;
    if (curTarget == source)
    {
      cellPoints = 2 + targetList->GetNumberOfIds();
    }
    else
    {
      cellPoints = 2 + sourceList->IsId(curTarget) + targetList->GetNumberOfIds();
    }

    // We may eliminate a common ancestor if:
    // 1. The source is not an ancestor of the target
    // 2. The target is not an ancestor of the source
    // 3. The number of points along the path is at least 4
    bool eliminateCommonAncestor = false;
    if (sourceList->IsId(target) == -1 && targetList->IsId(source) == -1 && cellPoints >= 4)
    {
      cellPoints--;
      eliminateCommonAncestor = true;
    }

    // Create the new cell
    vtkIdType cellId = newLines->InsertNextCell(cellPoints);
    output->GetCellData()->CopyData(graph->GetEdgeData(), e.Id, cellId);

    double cellPointsD = static_cast<double>(cellPoints);
    double sourcePt[3];
    newPoints->GetPoint(source, sourcePt);
    double targetPt[3];
    newPoints->GetPoint(target, targetPt);

    // Insert a point into the polyline for the source vertex.
    double pt[3];
    double interpPt[3];
    vtkIdType curPoint = 0;
    newLines->InsertCellPoint(source);
    curPoint++;

    // Insert points into the polyline going up the tree to
    // the common ancestor.
    for (vtkIdType s = 0; s < sourceList->IsId(curTarget); s++)
    {
      tree->GetPoint(sourceList->GetId(s), pt);
      for (int c = 0; c < 3; c++)
      {
        interpPt[c] = (1.0 - curPoint/cellPointsD)*sourcePt[c]
          + (curPoint/cellPointsD)*targetPt[c];
        interpPt[c] = (1.0 - this->BundlingStrength)*interpPt[c]
          + this->BundlingStrength*pt[c];
      }
      vtkIdType ptId = newPoints->InsertNextPoint(interpPt);
      newLines->InsertCellPoint(ptId);
      fractionArray->InsertNextValue(curPoint/cellPointsD);
      curPoint++;
    }

    // Insert points into the polyline going down the tree from
    // the common ancestor to the target vertex, possibly excluding
    // the common ancestor if it is a long path.
    vtkIdType maxTargetId = targetList->GetNumberOfIds() - 1;
    if (eliminateCommonAncestor)
    {
      maxTargetId = targetList->GetNumberOfIds() - 2;
    }
    for (vtkIdType t = maxTargetId; t >= 0; t--)
    {
      tree->GetPoint(targetList->GetId(t), pt);
      for (int c = 0; c < 3; c++)
      {
        interpPt[c] = (1.0 - curPoint/cellPointsD)*sourcePt[c]
          + (curPoint/cellPointsD)*targetPt[c];
        interpPt[c] = (1.0 - this->BundlingStrength)*interpPt[c]
          + this->BundlingStrength*pt[c];
      }
      vtkIdType ptId = newPoints->InsertNextPoint(interpPt);
      newLines->InsertCellPoint(ptId);
      fractionArray->InsertNextValue(curPoint/cellPointsD);
      curPoint++;
    }

    // The incoming vertex point is stored at vertex + numVertices
    newLines->InsertCellPoint(target + numVertices);
    curPoint++;
    if (curPoint != cellPoints)
    {
      vtkErrorMacro(<< "Number of points mismatch! Expected " << cellPoints << ", have " << curPoint);
    }
  }
  output->GetPointData()->AddArray(fractionArray);

  // Send the data to output.
  output->SetLines(newLines);
  output->SetPoints(newPoints);

  // Clean up.
  newLines->Delete();
  newPoints->Delete();
  sourceList->Delete();
  targetList->Delete();
  fractionArray->Delete();

  return 1;
}

void vtkGraphHierarchicalBundle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BundlingStrength: " << this->BundlingStrength << endl;
  os << indent << "DirectMapping: " << this->DirectMapping << endl;
}

