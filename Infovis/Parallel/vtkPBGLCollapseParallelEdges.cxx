/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLCollapseParallelEdges.cxx

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

#include "vtkPBGLCollapseParallelEdges.h"

#if !defined(VTK_LEGACY_REMOVE)

#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"
#include "vtkVertexListIterator.h"

#include <vtksys/stl/map>
#include <vtksys/stl/utility> // for pair

vtkStandardNewMacro(vtkPBGLCollapseParallelEdges);

vtkPBGLCollapseParallelEdges::vtkPBGLCollapseParallelEdges()
{
VTK_LEGACY_BODY(vtkPBGLCollapseParallelEdges::vtkPBGLCollapseParallelEdges, "VTK 6.2");
}

vtkPBGLCollapseParallelEdges::~vtkPBGLCollapseParallelEdges()
{
}

void vtkPBGLCollapseParallelEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

template<class MutableGraph>
int vtkPBGLCollapseParallelEdgesRequestData(
  vtkPBGLCollapseParallelEdges* self,
  vtkInformation*,
  vtkInformationVector** input_vec,
  vtkInformationVector* output_vec)
{
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();

  vtkGraph* input = vtkGraph::GetData(input_vec[0]);
  vtkGraph* output = vtkGraph::GetData(output_vec);

  vtkPBGLDistributedGraphHelper* input_helper =
    vtkPBGLDistributedGraphHelper::SafeDownCast(input->GetDistributedGraphHelper());

  // Create directed or undirected graph
  MutableGraph* builder = MutableGraph::New();

  // Setup the graph as a distributed graph
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> output_helper =
    vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  builder->SetDistributedGraphHelper(output_helper);

  // Distributed weight map
  vtkSmartPointer<vtkIntArray> weight_arr = vtkSmartPointer<vtkIntArray>::New();
  weight_arr->SetName("weight");
  typedef vtkDistributedEdgePropertyMapType<vtkIntArray>::type
    DistributedWeightMap;
  DistributedWeightMap distrib_weight_arr
    = MakeDistributedEdgePropertyMap(builder, weight_arr.GetPointer());

  // Prepare vertex data.
  vtkAbstractArray *input_pedigrees = input->GetVertexData()->GetPedigreeIds();
  vtkAbstractArray *pedigrees
    = vtkAbstractArray::CreateArray(input_pedigrees->GetDataType());
  pedigrees->SetName(input_pedigrees->GetName());
  builder->GetVertexData()->AddArray(pedigrees);
  builder->GetVertexData()->SetPedigreeIds(pedigrees);

  // Prepare edge data.
  builder->GetEdgeData()->AddArray(weight_arr);

  // Iterate through input graph, adding vertices.
  // This assumes the vertices will be distributed in the same way
  // as the input graph.
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  input->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType v = vertices->Next();
    vtkIdType ind = input->GetDistributedGraphHelper()->GetVertexIndex(v);
    builder->LazyAddVertex(input_pedigrees->GetVariantValue(ind));
    }
  output_helper->Synchronize();

  // Iterate through input edges, adding a new edge for every new (source,target) pair.
  vtksys_stl::map<vtksys_stl::pair<vtkIdType, vtkIdType>, int> edge_weights;
  vtkSmartPointer<vtkOutEdgeIterator> out_edges
    = vtkSmartPointer<vtkOutEdgeIterator>::New();
  input->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();
    input->GetOutEdges(u, out_edges);
    while (out_edges->HasNext())
      {
      vtkOutEdgeType e = out_edges->Next();
      //cerr << "processing edge " << u << " to " << e.Target << endl;
      vtkIdType a = u;
      vtkIdType b = e.Target;
      if (u > e.Target)
        {
        a = e.Target;
        b = u;
        }
      vtksys_stl::pair<vtkIdType,vtkIdType> p(a, b);
      if (edge_weights.count(p) > 0)
        {
        edge_weights[p]++;
        }
      else
        {
        edge_weights[p] = 1;
        }
      }
    }

  vtksys_stl::map<vtksys_stl::pair<vtkIdType, vtkIdType>, int>::iterator wi;
  for (wi = edge_weights.begin(); wi != edge_weights.end(); ++wi)
    {
    builder->LazyAddEdge(wi->first.first, wi->first.second);
    }
  output_helper->Synchronize();

  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  builder->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType e = edges->Next();
    vtksys_stl::pair<vtkIdType,vtkIdType> p(e.Source, e.Target);
    weight_arr->InsertNextValue(edge_weights[p]);
    //put(distrib_weight_arr, e, edge_weights[p]);
    }
  output_helper->Synchronize();

  int rank = process_id(output_helper->GetProcessGroup());

  // Copy into output graph
  if (!output->CheckedShallowCopy(builder))
    {
    vtkErrorWithObjectMacro(self, "Could not copy to output.");
    return 0;
    }

  timer->StopTimer();
  cerr << "vtkPBGLCollapseParallelEdges: " << timer->GetElapsedTime() << endl;

  return 1;
}

int vtkPBGLCollapseParallelEdges::RequestData(
  vtkInformation* info,
  vtkInformationVector** input_vec,
  vtkInformationVector* output_vec)
{
  vtkGraph* input = vtkGraph::GetData(input_vec[0]);
  if (vtkDirectedGraph::SafeDownCast(input))
    {
    return vtkPBGLCollapseParallelEdgesRequestData<vtkMutableDirectedGraph>(
      this, info, input_vec, output_vec);
    }
  return vtkPBGLCollapseParallelEdgesRequestData<vtkMutableUndirectedGraph>(
    this, info, input_vec, output_vec);
}

#endif //VTK_LEGACY_REMOVE
