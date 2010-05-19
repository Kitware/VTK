/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphHierarchicalBundleEdges.cxx

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
#include "vtkGraphHierarchicalBundleEdges.h"

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
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTree.h"
#include "vtkVariantArray.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

vtkStandardNewMacro(vtkGraphHierarchicalBundleEdges);

vtkGraphHierarchicalBundleEdges::vtkGraphHierarchicalBundleEdges()
{
  this->BundlingStrength = 0.8;
  this->SetNumberOfInputPorts(2);
  this->DirectMapping = false;
}

int vtkGraphHierarchicalBundleEdges::FillInputPortInformation(int port, vtkInformation* info)
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

int vtkGraphHierarchicalBundleEdges::RequestData(
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
  vtkGraph *output = vtkGraph::SafeDownCast(
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
  map<vtkIdType, vtkIdType> graphIndexToTreeIndex;
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
    // Check for valid domain array, if any.
    vtkAbstractArray* graphDomainArray = 
      graph->GetVertexData()->GetAbstractArray("domain");

    vtkAbstractArray* treeIdArray = 
      tree->GetVertexData()->GetPedigreeIds();
    if (!treeIdArray)
      {
      vtkErrorMacro("Tree pedigree id array not found.");
      return 0;
      }
    // Check for valid domain array, if any.
    vtkAbstractArray* treeDomainArray = 
      tree->GetVertexData()->GetAbstractArray("domain");

    map<vtkVariant,vtkIdType,vtkVariantLessThan> graphIdMap;
    
    // Create a map from graph id to graph index
    for (int i=0; i<graph->GetNumberOfVertices(); ++i)
      {
      graphIdMap[graphIdArray->GetVariantValue(i)] = i;
      }
      
    // Now create the map from graph index to tree index
    for (int i=0; i<tree->GetNumberOfVertices(); ++i)
      {
      vtkVariant id = treeIdArray->GetVariantValue(i);
      if (graphIdMap.count(id))
        {
        // Make sure that the domain for this id in the graph matches
        // the one in the tree before adding to the map. This guards 
        // against drawing edges to group nodes in the tree.
        if(treeDomainArray)
          {
          vtkVariant treeDomain = treeDomainArray->GetVariantValue(i);
          vtkVariant graphDomain;
          if(graphDomainArray)
            {
            graphDomain = graphDomainArray->GetVariantValue(graphIdMap[id]);
            }
          else
            {
            graphDomain == graphIdArray->GetName();
            }
          if(graphDomain != treeDomain)
            {
            continue;
            }
          }

        graphIndexToTreeIndex[graphIdMap[id]] = i;
        }
      }
    }

  output->ShallowCopy(graph);
  output->DeepCopyEdgePoints(graph);
  vtkSmartPointer<vtkPoints> points =
    vtkSmartPointer<vtkPoints>::New();
  double pt[3];
  for (vtkIdType v = 0; v < graph->GetNumberOfVertices(); ++v)
    {
    vtkIdType treeVertex = 0;
    if (graphIndexToTreeIndex.count(v) > 0)
      {
      treeVertex = graphIndexToTreeIndex[v];
      tree->GetPoint(treeVertex, pt);
      }
    else
      {
      pt[0] = 0.0;
      pt[1] = 0.0;
      pt[2] = 0.0;
      }
    points->InsertNextPoint(pt);
    }
  output->SetPoints(points);

  vtkSmartPointer<vtkIdList> sourceList =
    vtkSmartPointer<vtkIdList>::New();
  vtkSmartPointer<vtkIdList> targetList =
    vtkSmartPointer<vtkIdList>::New();
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
      cellPoints = targetList->GetNumberOfIds();
      }
    else
      {
      cellPoints = sourceList->IsId(curTarget) + targetList->GetNumberOfIds();
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

    double cellPointsD = static_cast<double>(cellPoints);
    double sourcePt[3];
    tree->GetPoint(source, sourcePt);
    double targetPt[3];
    tree->GetPoint(target, targetPt);

    double interpPt[3];
    int curPoint = 1;

    // Insert points into the polyline going up the tree to
    // the common ancestor.
    for (vtkIdType s = 0; s < sourceList->IsId(curTarget); s++)
      {
      tree->GetPoint(sourceList->GetId(s), pt);
      for (int c = 0; c < 3; c++)
        {
        interpPt[c] = (1.0 - curPoint/(cellPointsD+1))*sourcePt[c] 
          + (curPoint/(cellPointsD+1))*targetPt[c];
        interpPt[c] = (1.0 - this->BundlingStrength)*interpPt[c] 
          + this->BundlingStrength*pt[c];
        }
      output->AddEdgePoint(e.Id, interpPt);
      ++curPoint;
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
        interpPt[c] = (1.0 - curPoint/(cellPointsD+1))*sourcePt[c] 
          + (curPoint/(cellPointsD+1))*targetPt[c];
        interpPt[c] = (1.0 - this->BundlingStrength)*interpPt[c] 
          + this->BundlingStrength*pt[c];
        }
      output->AddEdgePoint(e.Id, interpPt);
      ++curPoint;
      }
    }

  return 1;
}

void vtkGraphHierarchicalBundleEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BundlingStrength: " << this->BundlingStrength << endl;
  os << indent << "DirectMapping: " << this->DirectMapping << endl;
}

