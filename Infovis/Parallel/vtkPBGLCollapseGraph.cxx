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

#if !defined(VTK_LEGACY_REMOVE)

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
#include "vtkTable.h"
#include "vtkTimerLog.h"
#include "vtkVariantArray.h"
#include "vtkVertexListIterator.h"

#include <boost/graph/use_mpi.hpp>   // must precede all pbgl includes

vtkStandardNewMacro(vtkPBGLCollapseGraph);

vtkPBGLCollapseGraph::vtkPBGLCollapseGraph()
{
  VTK_LEGACY_BODY(vtkPBGLCollapseGraph::vtkPBGLCollapseGraph, "VTK 6.2");
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
  vtkSmartPointer<vtkTimerLog> timer = vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();

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

  // Distributed input array
  typedef vtkDistributedVertexPropertyMapType<vtkAbstractArray>::type
    DistributedLabelMap;
  DistributedLabelMap distrib_input_arr
    = MakeDistributedVertexPropertyMap(input, vtkArrayDownCast<vtkAbstractArray>(input_arr));

  // Create directed or undirected graph
  MutableGraph* builder = MutableGraph::New();

  // Setup the graph as a distributed graph
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> output_helper =
    vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();
  builder->SetDistributedGraphHelper(output_helper);
  int input_rank = process_id(input_helper->GetProcessGroup());
  int rank = process_id(output_helper->GetProcessGroup());

  // Prepare edge data.
  // FIXME (DPG): There's a problem with this approach to copying
  // properties, because the number of vertices in the resulting graph
  // may differ greatly from the number of vertices in the incoming
  // graph, and the distribution may also be completely different. So
  // we can't really safely allocate GetNumberOfComponents elements in
  // the arrays in the output graph, because a given processor may, in
  // some cases, end up with more edges than it started with.
#if 0
  vtkDataSetAttributes* in_edata = input->GetEdgeData();
  vtkDataSetAttributes* out_edata = builder->GetEdgeData();
  for (int a = 0; a < in_edata->GetNumberOfArrays(); ++a)
  {
    vtkAbstractArray* in_arr = in_edata->GetAbstractArray(a);
    vtkSmartPointer<vtkAbstractArray> arr;
    arr.TakeReference(vtkAbstractArray::CreateArray(in_arr->GetDataType()));
    arr->SetName(in_arr->GetName());
    arr->SetNumberOfComponents(in_arr->GetNumberOfComponents());
    out_edata->AddArray(arr);
  }
#endif

  // Prepare vertex data.
  vtkAbstractArray *pedigrees
    = vtkAbstractArray::CreateArray(input_arr->GetDataType());
  pedigrees->SetName(input_arr->GetName());
  builder->GetVertexData()->AddArray(pedigrees);
  builder->GetVertexData()->SetPedigreeIds(pedigrees);

  // Iterate through input graph, adding a vertex for every new value
  // TODO: Handle vertex properties?
  // For now, do not copy any vertex data since there seems to be a bug there
  vtkSmartPointer<vtkVertexListIterator> verts = vtkSmartPointer<vtkVertexListIterator>::New();
  input->GetVertices(verts);
  while (verts->HasNext())
  {
    vtkIdType v = verts->Next();
    vtkIdType ind = input_helper->GetVertexIndex(v);
    vtkVariant val = input_arr->GetVariantValue(ind);
    if (val.ToString().length() > 0)
    {
      builder->LazyAddVertex(input_arr->GetVariantValue(v));
      //builder->AddVertex(input_arr->GetVariantValue(v));
    }
  }
  output_helper->Synchronize();
  cerr << "input number of vertices " << input->GetNumberOfVertices() << endl;
  cerr << "builder number of vertices " << builder->GetNumberOfVertices() << endl;

  cerr << "adding edges" << endl;
  // Iterate through input edges, adding new edges
  vtkSmartPointer<vtkEdgeListIterator> edges
    = vtkSmartPointer<vtkEdgeListIterator>::New();
  //vtkSmartPointer<vtkVariantArray> edata
  //  = vtkSmartPointer<vtkVariantArray>::New();
  //vtkSmartPointer<vtkTable> in_etable
  //  = vtkSmartPointer<vtkTable>::New();
  //in_etable->SetRowData(in_edata);
  input->GetEdges(edges);
  while (edges->HasNext())
  {
    vtkEdgeType e = edges->Next();
    //vtkIdType eid = input_helper->GetEdgeIndex(e.Id);
    //in_etable->GetRow(eid, edata);
    vtkVariant source_val = get(distrib_input_arr, e.Source);
    vtkVariant target_val = get(distrib_input_arr, e.Target);
    if (source_val.ToString().length() > 0 && target_val.ToString().length() > 0)
    {
      builder->LazyAddEdge(source_val, target_val);
      //builder->LazyAddEdge(source_val, target_val, edata);
    }
  }
  output_helper->Synchronize();

  // Copy into output graph
  if (!output->CheckedShallowCopy(builder))
  {
    vtkErrorWithObjectMacro(self, "Could not copy to output.");
    return 0;
  }

  timer->StopTimer();
  cerr << "vtkPBGLCollapseGraph: " << timer->GetElapsedTime() << endl;

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

#endif //VTK_LEGACY_REMOVE
