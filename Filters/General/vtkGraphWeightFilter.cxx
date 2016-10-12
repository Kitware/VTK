/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphWeightFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphWeightFilter.h"

#include "vtkEdgeListIterator.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

bool vtkGraphWeightFilter::CheckRequirements(vtkGraph* const vtkNotUsed(graph)) const
{
  return true;
}

int vtkGraphWeightFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                         vtkInformationVector **inputVector,
                                         vtkInformationVector *outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the input and ouptut
  vtkGraph* input = vtkGraph::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkGraph* output = vtkGraph::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy the input graph to the output.
  // We want to keep the vertices and edges, just add a weight array.
  output->ShallowCopy(input);

  if(!this->CheckRequirements(input))
  {
    vtkErrorMacro(<< "Requirements are not met!");
    return 0;
  }

  // Create the edge weight array
  vtkSmartPointer<vtkFloatArray> weights =
    vtkSmartPointer<vtkFloatArray>::New();
  weights->SetNumberOfComponents(1);
  weights->SetNumberOfTuples(input->GetNumberOfEdges());
  weights->SetName("Weights");

  // Compute the Weight function (in a subclass) for every edge
  vtkSmartPointer<vtkEdgeListIterator> edgeListIterator =
    vtkSmartPointer<vtkEdgeListIterator>::New();
  input->GetEdges(edgeListIterator);

  while(edgeListIterator->HasNext())
  {
    vtkEdgeType edge = edgeListIterator->Next();

    float w = this->ComputeWeight(input, edge);

    weights->SetValue(edge.Id, w);
  }

  output->SetPoints(input->GetPoints());
  output->GetEdgeData()->AddArray(weights);

  return 1;
}

//----------------------------------------------------------------------------
void vtkGraphWeightFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGraphAlgorithm::PrintSelf(os,indent);
}
