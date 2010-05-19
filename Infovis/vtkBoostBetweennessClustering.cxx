/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostGraphAdapter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBoostBetweennessClustering.h"

#include "vtkBoostGraphAdapter.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectedGraph.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkUndirectedGraph.h"

#include <boost/graph/bc_clustering.hpp>

vtkStandardNewMacro(vtkBoostBetweennessClustering);

//-----------------------------------------------------------------------------
vtkBoostBetweennessClustering::vtkBoostBetweennessClustering() :
  vtkGraphAlgorithm(),
  Threshold(0)
{
}

//-----------------------------------------------------------------------------
vtkBoostBetweennessClustering::~vtkBoostBetweennessClustering()
{
}

//-----------------------------------------------------------------------------
void vtkBoostBetweennessClustering::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkBoostBetweennessClustering::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if(!inInfo)
    {
    vtkErrorMacro("Failed to get input information.")
    return 1;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if(!outInfo)
    {
    vtkErrorMacro("Failed to get output information.")
    return 1;
    }

  // Get the input and output
  vtkGraph* input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if(!input)
    {
    vtkErrorMacro("Failed to get input graph.")
    return 1;
    }

  vtkGraph* output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if(!output)
    {
    vtkErrorMacro("Failed to get output graph.")
    return 1;
    }


  vtkFloatArray* edgeCM = vtkFloatArray::New();
  edgeCM->SetName("edge_centrality");
  boost::vtkGraphEdgePropertyMapHelper<vtkFloatArray*> helper(edgeCM);

  if(vtkDirectedGraph::SafeDownCast(input))
    {
    vtkMutableDirectedGraph* out  = vtkMutableDirectedGraph::New();
    // Send the data to output.
    out->DeepCopy(input);
    boost::betweenness_centrality_clustering(
      out, boost::bc_clustering_threshold<double>(
        this->Threshold, out, false), helper);
    out->GetEdgeData()->AddArray(edgeCM);
    output->ShallowCopy(out);
    out->Delete();
    }
  else
    {
    vtkMutableUndirectedGraph* out = vtkMutableUndirectedGraph::New();
    // Send the data to output.
    out->DeepCopy(input);
    boost::betweenness_centrality_clustering(
      out,
      boost::bc_clustering_threshold<double>(
        this->Threshold, out, false), helper);
    out->GetEdgeData()->AddArray(edgeCM);
    output->ShallowCopy(out);
    out->Delete();
    }

  edgeCM->Delete();
  return 1;
}
