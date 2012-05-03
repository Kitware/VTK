/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostBiconnectedComponents.cxx

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
#include "vtkBoostBiconnectedComponents.h"

#include "vtkCellData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkVertexListIterator.h"

#include "vtkGraph.h"
#include "vtkBoostGraphAdapter.h"
#include <boost/graph/biconnected_components.hpp>
#include <boost/version.hpp>
#include <vtksys/stl/vector>
#include <vtksys/stl/utility>

using namespace boost;
using vtksys_stl::vector;
using vtksys_stl::pair;

vtkStandardNewMacro(vtkBoostBiconnectedComponents);

vtkBoostBiconnectedComponents::vtkBoostBiconnectedComponents()
{
  this->OutputArrayName = 0;
}

vtkBoostBiconnectedComponents::~vtkBoostBiconnectedComponents()
{
  // release mem
  this->SetOutputArrayName(0);
}

int vtkBoostBiconnectedComponents::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkUndirectedGraph *input = vtkUndirectedGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUndirectedGraph *output = vtkUndirectedGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  // Create edge biconnected component array.
  // This will be populated directly by the boost algorithm.
  vtkSmartPointer<vtkIntArray> edgeCompArr = vtkSmartPointer<vtkIntArray>::New();
  edgeCompArr->SetNumberOfTuples(input->GetNumberOfEdges());
  for (vtkIdType i = 0; i < input->GetNumberOfEdges(); ++i)
    {
    edgeCompArr->SetValue(i, -1);
    }
  if (this->OutputArrayName)
    {
    edgeCompArr->SetName(this->OutputArrayName);
    }
  else
    {
    edgeCompArr->SetName("biconnected component");
    }
  vtkGraphEdgePropertyMapHelper<vtkIntArray*> helper(edgeCompArr.GetPointer());

  // Create vector of articulation points and set it up for insertion
  // by the algorithm.
  vector<vtkIdType> artPoints;
  pair<size_t, vtksys_stl::back_insert_iterator<vector<vtkIdType> > >
    res(0, vtksys_stl::back_inserter(artPoints));

  // Call BGL biconnected_components.
  // It appears that the signature for this
  // algorithm has changed in 1.32, 1.33, and 1.34 ;p
#if BOOST_VERSION < 103300      // Boost 1.32.x
  // TODO I have no idea what the 1.32 signature is suppose to be
  // res = biconnected_components(
  //  output, helper, vtksys_stl::back_inserter(artPoints), vtkGraphIndexMap());
#elif BOOST_VERSION < 103400    // Boost 1.33.x
  res = biconnected_components(
    output, helper, vtksys_stl::back_inserter(artPoints), vtkGraphIndexMap());
#else                           // Anything after Boost 1.34.x
  res = biconnected_components(
    output, helper, vtksys_stl::back_inserter(artPoints), vertex_index_map(vtkGraphIndexMap()));
#endif

  size_t numComp = res.first;

  // Assign component values to vertices based on the first edge.
  // If isolated, assign a new value.
  vtkSmartPointer<vtkIntArray> vertCompArr = vtkSmartPointer<vtkIntArray>::New();
  if (this->OutputArrayName)
    {
    vertCompArr->SetName(this->OutputArrayName);
    }
  else
    {
    vertCompArr->SetName("biconnected component");
    }
  vertCompArr->SetNumberOfTuples(output->GetNumberOfVertices());
  vtkSmartPointer<vtkVertexListIterator> vertIt = vtkSmartPointer<vtkVertexListIterator>::New();
  vtkSmartPointer<vtkOutEdgeIterator> edgeIt = vtkSmartPointer<vtkOutEdgeIterator>::New();
  output->GetVertices(vertIt);
  while (vertIt->HasNext())
    {
    vtkIdType u = vertIt->Next();
    output->GetOutEdges(u, edgeIt);
    int comp = -1;
    while (edgeIt->HasNext() && comp == -1)
      {
      vtkOutEdgeType e = edgeIt->Next();
      int value = edgeCompArr->GetValue(e.Id);
      comp = value;
      }
    if (comp == -1)
      {
      comp = static_cast<int>(numComp);
      numComp++;
      }
    vertCompArr->SetValue(u, comp);
    }

  // Articulation points belong to multiple biconnected components.
  // Indicate these by assigning a component value of -1.
  // It belongs to whatever components its incident edges belong to.
  vector<vtkIdType>::size_type i;
  for (i = 0; i < artPoints.size(); i++)
    {
    vertCompArr->SetValue(artPoints[i], -1);
    }

  // Add edge and vertex component arrays to the output
  output->GetEdgeData()->AddArray(edgeCompArr);
  output->GetVertexData()->AddArray(vertCompArr);

  return 1;
}

void vtkBoostBiconnectedComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputArrayName: "
     << (this->OutputArrayName ? this->OutputArrayName : "(none)") << endl;
}

