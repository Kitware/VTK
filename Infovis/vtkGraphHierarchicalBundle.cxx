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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkGraphHierarchicalBundle.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkIdTypeArray.h"
#include "vtkAbstractGraph.h"
#include "vtkTree.h"
#include "vtkStdString.h"

#include <vtksys/stl/map>
using vtksys_stl::map;

vtkCxxRevisionMacro(vtkGraphHierarchicalBundle, "1.4");
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
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkAbstractGraph");
    return 1;
    }
  else if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
    return 1;
    }
  return 0;
}

template <typename T>
void mappingMadness(T *graphIds, T *treeIds, map<vtkIdType,vtkIdType> *idMap,
                    int numGraphVertices, int numTreeVertices)
{
  map<T,vtkIdType> graphIdMap;
  
  // Now create the two maps
  for (int i=0; i<numGraphVertices; ++i)
    {
    graphIdMap[graphIds[i]] = i;
    }
    
  // Now create the output map
  for (int i=0; i<numTreeVertices; ++i)
    {
    (*idMap)[graphIdMap[treeIds[i]]] = i;
    }
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

  // get the input and ouptut
  vtkAbstractGraph *graph = vtkAbstractGraph::SafeDownCast(
    graphInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTree *tree = vtkTree::SafeDownCast(
    treeInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
    

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
      graph->GetVertexData()->GetAbstractArray("PedigreeId");
    if (graphIdArray == NULL)
      {
      // Check for any id array.
      graphIdArray = graph->GetVertexData()->GetAbstractArray("id");
      if (graphIdArray == NULL)
        {
        vtkErrorMacro("Graph pedigree id array not found.");
        return 0;
        }
      }
    vtkAbstractArray* treeIdArray = 
      tree->GetVertexData()->GetAbstractArray("PedigreeId");
    if (treeIdArray == NULL)
      {
      // Check for any id array.
      treeIdArray = tree->GetVertexData()->GetAbstractArray("id");
      if (treeIdArray == NULL)
        {
        vtkErrorMacro("Tree pedigree id array not found.");
        return 0;
        }
      }
    if (graphIdArray->GetDataType() != treeIdArray->GetDataType())
      {
      vtkErrorMacro("Pedigree id types not not match.");
      return 0;
      }
      
    // Create void pointers that will be recast within
    // the template macro
    void *graphVoid = graphIdArray->GetVoidPointer(0);
    void *treeVoid = treeIdArray->GetVoidPointer(0);
    switch(graphIdArray->GetDataType())
      {
      vtkExtendedTemplateMacro(mappingMadness(static_cast<VTK_TT*>(graphVoid),
                                      static_cast<VTK_TT*>(treeVoid),
                                      &graphIndexToTreeIndex,
                                      graph->GetNumberOfVertices(),
                                      tree->GetNumberOfVertices()));
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
  for (vtkIdType i = 0; i < graph->GetNumberOfEdges(); i++)
    {
    unsigned int graphSourceIndex = graph->GetSourceVertex(i);
    unsigned int graphTargetIndex = graph->GetTargetVertex(i);

    // Do not render loops
    if (graphSourceIndex == graphTargetIndex)
      {
      continue;
      }

    vtkIdType source = 0;
    vtkIdType target = 0;
    if (graphSourceIndex < graphIndexToTreeIndex.size()&& 
        graphTargetIndex < graphIndexToTreeIndex.size())
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
    output->GetCellData()->CopyData(graph->GetEdgeData(), i, cellId);

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
      cerr << "Number of points mismatch! Expected " << cellPoints << ", have " << curPoint << endl;
      cerr << source << "," << target << endl;
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

