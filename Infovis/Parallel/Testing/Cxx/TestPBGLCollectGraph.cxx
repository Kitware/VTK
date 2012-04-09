/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPBGLCollectGraph.cxx

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
#include "vtkDataSetAttributes.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkDirectedGraph.h"
#include "vtkDoubleArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkOutEdgeIterator.h"
#include "vtkMath.h"
#include "vtkPBGLCollectGraph.h"
#include "vtkPBGLRMATGraphSource.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVertexListIterator.h"

#include <vtksys/stl/functional>
#include <vtksys/stl/string>

#include <boost/mpi/collectives.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/timer.hpp>
#include <boost/lexical_cast.hpp>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

bool HasPrintableAttributes(vtkDataSetAttributes *attrs)
{
  int numArrays = attrs->GetNumberOfArrays();
  for (int arrayIndex = 0; arrayIndex < numArrays; ++arrayIndex)
    {
    vtkAbstractArray *array = attrs->GetAbstractArray(arrayIndex);
    const char* name = attrs->GetArrayName(arrayIndex);
    if (name && *name)
      {
      return true;
      }
    }
  return false;
}

void PrintAttributes(vtkDataSetAttributes *attrs, vtkIdType index)
{
  bool printedAttr = false;
  int numArrays = attrs->GetNumberOfArrays();
  for (int arrayIndex = 0; arrayIndex < numArrays; ++arrayIndex)
    {
    vtkAbstractArray *array = attrs->GetAbstractArray(arrayIndex);
    const char *name = attrs->GetArrayName(arrayIndex);
    if (name && *name)
      {
      if (!printedAttr)
        {
        cerr << " [";
        printedAttr = true;
        }
      else
        {
        cerr << ", ";
        }

      cerr << name << " = \"" << array->GetVariantValue(index).ToString()
           << "\"";
      }
    }

  if (printedAttr)
    {
    cerr << "]";
    }
}

void PrintGraph(vtkGraph *graph)
{
  bool isDirected = vtkDirectedGraph::SafeDownCast(graph) != 0;
  if (isDirected)
    {
    cerr << "digraph G {" << endl;
    }
  else
    {
    cerr << "graph G {" << endl;
    }

  vtkDataSetAttributes *vertexData = graph->GetVertexData();
  vtkDataSetAttributes *edgeData = graph->GetEdgeData();
  bool hasVertexAttributes = HasPrintableAttributes(vertexData);
  bool hasEdgeAttributes = HasPrintableAttributes(edgeData);
  vtkSmartPointer<vtkVertexListIterator> vertices
    = vtkSmartPointer<vtkVertexListIterator>::New();
  graph->GetVertices(vertices);
  while (vertices->HasNext())
    {
    vtkIdType u = vertices->Next();

    if (hasVertexAttributes)
      {
      // Print vertex attributes
      cerr << "  " << u;
      PrintAttributes(vertexData, u);
      cerr << ";" << endl;
      }

    vtkSmartPointer<vtkOutEdgeIterator> outEdges
      = vtkSmartPointer<vtkOutEdgeIterator>::New();
      
    graph->GetOutEdges(u, outEdges);
    while (outEdges->HasNext()) 
      {
      // Print edge
      vtkOutEdgeType e = outEdges->Next();
      if (isDirected)
        {
        cerr << "  " << u << " -> " << e.Target;
        }
      else
        {
        cerr << "  " << u << " -- " << e.Target;
        }

      // Print edge attributes
      if (hasEdgeAttributes)
        {
        // Print edge attributes
        PrintAttributes(edgeData, e.Id);
        }
      cerr << ";" << endl;
      }
    }

  cerr << "}" << endl;
}

int main(int argc, char* argv[])
{           
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;
                            
  vtkIdType wantVertices = 128;
  vtkIdType wantEdges = 512;
  double A = 0.45;
  double B = 0.15;
  double C = 0.15;
  double D = 0.25;

  int errors = 0;

  if (argc > 6) 
    {
    wantVertices = boost::lexical_cast<vtkIdType>(argv[1]);
    wantEdges = boost::lexical_cast<vtkIdType>(argv[2]);
    A = boost::lexical_cast<double>(argv[3]);
    B = boost::lexical_cast<double>(argv[4]);
    C = boost::lexical_cast<double>(argv[5]);
    D = boost::lexical_cast<double>(argv[6]);
    }
                 
  // Set up a random graph source  
  VTK_CREATE(vtkPBGLRMATGraphSource, source);
  source->SetNumberOfVertices(wantVertices);
  wantVertices = source->GetNumberOfVertices();
  source->SetNumberOfEdges(wantEdges);
  source->SetProbabilities(A, B, C, D);

  // Set up the graph collector
  VTK_CREATE(vtkPBGLCollectGraph, collect);
  collect->SetInputConnection(0, source->GetOutputPort());

  // Build a distributed graph and collect the results.
  collect->Update();

  vtkGraph* output = vtkGraph::SafeDownCast(collect->GetOutput()); 
  if (world.rank() == 0)
    {
    if (output->GetNumberOfVertices() != source->GetNumberOfVertices())
      {
      cerr << "Output graph has the wrong number of vertices." << endl;
      ++errors;
      }

    if (output->GetNumberOfEdges() != source->GetNumberOfEdges())
      {
      cerr << "Output graph has the wrong number of edges." << endl;
      ++errors;
      }

    PrintGraph(output);
    }

  return 0;
}
