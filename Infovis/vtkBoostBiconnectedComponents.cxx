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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkBoostBiconnectedComponents.h"

#include "vtkCellData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPointData.h"
#include "vtkVertexListIterator.h"

#include "vtkGraph.h"
#ifdef VTK_USE_PARALLEL_BGL
// Work around header-ordering issues in Boost.Serialization
#  include <boost/parallel/mpi/bsp_process_group.hpp>
#endif
#include "vtkBoostGraphAdapter.h"
#include <boost/graph/biconnected_components.hpp>
#include <boost/vector_property_map.hpp>
#include <boost/version.hpp>
#include <vtksys/stl/vector>
#include <vtksys/stl/utility>

using namespace boost;
using vtksys_stl::vector;
using vtksys_stl::pair;

vtkCxxRevisionMacro(vtkBoostBiconnectedComponents, "1.9.4.1");
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

  // get the input and ouptut
  vtkUndirectedGraph *input = vtkUndirectedGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUndirectedGraph *output = vtkUndirectedGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  // TODO:
  // Biconnected components only works on undirected graphs.
  // Currently this will fail if given a directed graph as input.

  // Run the boost algorithm
  typedef vector_property_map<vtkIdType> PMap;
  PMap p;
  vtkGraphEdgePropertyMapHelper<PMap> helper(p);
  vector<vtkIdType> artPoints;
  pair<size_t, vtksys_stl::back_insert_iterator<vector<vtkIdType> > > 
    res(0, vtksys_stl::back_inserter(artPoints));
    
  // Initialize the helper pmap to all -1
  for (vtkIdType e = 0; e < output->GetNumberOfEdges(); e++)
    {
    helper.pmap[e] = -1;
    }  
    
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
  
  // Create the edge attribute array
  vtkIntArray* edgeComps = vtkIntArray::New();
  if (this->OutputArrayName)
    {
    edgeComps->SetName(this->OutputArrayName);
    }
  else
    {
    edgeComps->SetName("biconnected component");
    }
  edgeComps->Allocate(output->GetNumberOfEdges());
  for (vtkIdType e = 0; e < output->GetNumberOfEdges(); e++)
    {
    edgeComps->InsertNextValue(helper.pmap[e]);
    }
  output->GetEdgeData()->AddArray(edgeComps);
  edgeComps->Delete();

  // Assign component values to vertices based on the first edge
  // If isolated, assign a new value
  
  // Create the vertex attribute array
  vtkIntArray* vertComps = vtkIntArray::New();
  if (this->OutputArrayName)
    {
    vertComps->SetName(this->OutputArrayName);
    }
  else
    {
    vertComps->SetName("biconnected component");
    }
  vertComps->Allocate(output->GetNumberOfVertices());
  vtkVertexListIterator *vertIt = vtkVertexListIterator::New();
  vtkOutEdgeIterator *edgeIt = vtkOutEdgeIterator::New();
  output->GetVertices(vertIt);
  while (vertIt->HasNext())
    {
    vtkIdType u = vertIt->Next();
    output->GetOutEdges(u, edgeIt);
    int comp;
    if (edgeIt->HasNext())
      {
      vtkOutEdgeType e = edgeIt->Next();
      int value = edgeComps->GetValue(e.Id);
      while( (value == -1) && edgeIt->HasNext() )
        {
        e = edgeIt->Next();
        value = edgeComps->GetValue(e.Id);
        }
      comp = value;
      }
    else
      {
      comp = numComp;
      numComp++;
      }
    vertComps->InsertNextValue(comp);
    }
  vertIt->Delete();
  edgeIt->Delete();

  // Articulation points belong to multiple biconnected components.
  // Indicate these by assigning a component value of -1.
  // It belongs to whatever components its incident edges belong to.
  vector<vtkIdType>::size_type i;
  for (i = 0; i < artPoints.size(); i++)
    {
    vertComps->SetValue(artPoints[i], -1);
    }

  output->GetVertexData()->AddArray(vertComps);
  vertComps->Delete();

  return 1;
}

void vtkBoostBiconnectedComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
    
  os << indent << "OutputArrayName: " 
     << (this->OutputArrayName ? this->OutputArrayName : "(none)") << endl;
}

