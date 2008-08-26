/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLCollapseGraph.cxx

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

#include "vtkPBGLCollapseGraph.h"

#include "vtkDataSetAttributes.h"
#include "vtkEdgeListIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVariantArray.h"

vtkCxxRevisionMacro(vtkPBGLCollapseGraph, "1.3");
vtkStandardNewMacro(vtkPBGLCollapseGraph);

vtkPBGLCollapseGraph::vtkPBGLCollapseGraph()
{
}

vtkPBGLCollapseGraph::~vtkPBGLCollapseGraph()
{
}

void vtkPBGLCollapseGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

template<class MutableGraph>
int vtkPBGLCollapseGraphRequestData(
  vtkPBGLCollapseGraph* self,
  vtkAbstractArray* input_arr,
  vtkInformation*,
  vtkInformationVector** input_vec,
  vtkInformationVector* output_vec)
{
  vtkGraph* input = vtkGraph::GetData(input_vec[0]);
  vtkGraph* output = vtkGraph::GetData(output_vec);

  vtkPBGLDistributedGraphHelper* input_helper =
    vtkPBGLDistributedGraphHelper::SafeDownCast(input->GetDistributedGraphHelper());

  if (!input_arr)
    {
    vtkErrorWithObjectMacro(self, "Invalid input array.");
    return 0;
    }
  if (input_arr->GetNumberOfComponents() != 1)
    {
    vtkErrorWithObjectMacro(self, "Input array must have a single component.");
    return 0;
    }

  // Distributed distance map
  typedef vtkDistributedVertexPropertyMapType<vtkAbstractArray>::type
    DistributedLabelMap;

  // Distributed distance recorder
  DistributedLabelMap distrib_input_arr
    = MakeDistributedVertexPropertyMap(input, input_arr);

  // Create directed or undirected graph
  MutableGraph* builder = MutableGraph::New();

  // Setup the graph as a distributed graph
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> output_helper =
    vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  builder->SetDistributedGraphHelper(output_helper);

  // Prepare vertex and edge data
#if 0
  // FIXME (DPG): There's a problem with this approach to copying
  // properties, because the number of vertices in the resulting graph
  // may differ greatly from the number of vertices in the incoming
  // graph, and the distribution may also be completely different. So
  // we can't really safely allocate GetNumberOfComponents elements in
  // the arrays in the output graph, because a given processor may, in
  // some cases, end up with more vertices than it started with. So,
  // see the #else block for a small hack that only deals with
  // pedigree IDs.
  vtkDataSetAttributes* in_data[2];
  in_data[0] = input->GetVertexData();
  in_data[1] = input->GetEdgeData();
  vtkDataSetAttributes* out_data[2];
  out_data[0] = builder->GetVertexData();
  out_data[1] = builder->GetEdgeData();
  for (int d = 0; d < 2; ++d)
    {
    for (int a = 0; a < in_data[d]->GetNumberOfArrays(); ++a)
      {
      vtkAbstractArray* in_arr = in_data[d]->GetAbstractArray(a);
      vtkSmartPointer<vtkAbstractArray> arr;
      arr.TakeReference(vtkAbstractArray::CreateArray(in_arr->GetDataType()));
      arr->SetName(in_arr->GetName());
      arr->SetNumberOfComponents(in_arr->GetNumberOfComponents());
      out_data[d]->AddArray(arr);
      // The output pedigree ids will be the field used to determine collapsed nodes
      if (in_arr == input_arr)
        {
        out_data[d]->SetPedigreeIds(arr);
        }
      }
    }
#else
  vtkAbstractArray *pedigrees 
    = vtkAbstractArray::CreateArray(input_arr->GetDataType());
  pedigrees->SetName(input_arr->GetName());
  builder->GetVertexData()->AddArray(pedigrees);
  builder->GetVertexData()->SetPedigreeIds(pedigrees);
#endif

  // Iterate through input graph, adding a vertex for every new value
  // TODO: Handle vertex properties?
  for (vtkIdType v = 0; v < input->GetNumberOfVertices(); ++v)
    {
    builder->LazyAddVertex(input_arr->GetVariantValue(v));
    }
  output_helper->Synchronize();

  // Iterate through input edges, adding new edges
  // For now, do not copy any vertex data since there seems to be a bug there
  vtkSmartPointer<vtkEdgeListIterator> edges 
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  input->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType e = edges->Next();
    vtkVariant source_val = get(distrib_input_arr, e.Source);
    vtkVariant target_val = get(distrib_input_arr, e.Target);
    builder->LazyAddEdge(source_val, target_val);
    }
  output_helper->Synchronize();

  // Copy into output graph
  if (!output->CheckedShallowCopy(builder))
    {
    vtkErrorWithObjectMacro(self, "Could not copy to output.");
    return 0;
    }

  return 1;
}

int vtkPBGLCollapseGraph::RequestData(
  vtkInformation* info,
  vtkInformationVector** input_vec,
  vtkInformationVector* output_vec)
{
  vtkGraph* input = vtkGraph::GetData(input_vec[0]);
  vtkAbstractArray* input_arr = this->GetInputAbstractArrayToProcess(0, input_vec);
  if (vtkDirectedGraph::SafeDownCast(input))
    {
    return vtkPBGLCollapseGraphRequestData<vtkMutableDirectedGraph>(
      this, input_arr, info, input_vec, output_vec);
    }
  return vtkPBGLCollapseGraphRequestData<vtkMutableUndirectedGraph>(
    this, input_arr, info, input_vec, output_vec);
}

