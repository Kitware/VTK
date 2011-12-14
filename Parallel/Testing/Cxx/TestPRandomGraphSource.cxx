/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPRandomGraphSource.cxx

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
#include "vtkAdjacentVertexIterator.h"
#include "vtkBitArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPBGLBreadthFirstSearch.h"
#include "vtkPBGLConnectedComponents.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkPBGLMinimumSpanningTree.h"
#include "vtkPBGLVertexColoring.h"
#include "vtkPBGLRandomGraphSource.h"
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

int main(int argc, char* argv[])
{           
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;
                            
  vtkIdType wantVertices = 100;
  vtkIdType wantEdges = 200;

  bool doPrint = false;
  bool onlyConnected = false;
  bool doVerify = true;
  bool doBFS = true;
  bool doColoring = true;
  bool doConnectedComponents = true;
  bool doMST = true;

  if (argc > 2) 
    {
    wantVertices = boost::lexical_cast<vtkIdType>(argv[1]);
    wantEdges = boost::lexical_cast<vtkIdType>(argv[2]);
    }

  // The Dehne-Gotz minimum spanning tree algorithm actually allocates
  // O(|V|) memory, where |V| is the size of the full distributed
  // graph. For this reason, we won't (by default) run the minimum
  // spanning tree algorithm with > 1 million vertices.
  if (wantVertices > 1000000)
    {
    doMST = false;
    }

  for (int argIdx = 3; argIdx < argc; ++argIdx)
    {
      std::string arg = argv[argIdx];
      if (arg == "--print")
        {
        doPrint = true;
        }
      else if (arg == "--only-connected")
        {
        onlyConnected = true;
        }
      else if (arg == "--no-bfs")
        {
        doBFS = false;
        }
      else if (arg == "--no-coloring")
        {
        doColoring = false;
        }
      else if (arg == "--no-connected-components")
        {
        doConnectedComponents = false;
        }
      else if (arg == "--no-minumum-spanning-tree")
        {
        doMST = false;
        }
      else if (arg == "--minumum-spanning-tree")
        {
        doMST = true;
        }
    }

  vtkIdType totalNumberOfVertices;
  vtkIdType totalNumberOfEdges;
  vtkGraph* g;

  VTK_CREATE(vtkPBGLRandomGraphSource, source);

  int errors = 0;

  source->SetNumberOfVertices(wantVertices);
  source->SetNumberOfEdges(wantEdges);
  
  if (!onlyConnected)
    {
    if (world.rank() == 0)
      {
      cerr << "Testing simple random generator (" << wantVertices << ", "
           << wantEdges << ")..." << endl;
      }
    source->Update();
    g = source->GetOutput();

    totalNumberOfVertices
      = boost::mpi::all_reduce(world, g->GetNumberOfVertices(),
                               std::plus<vtkIdType>());
    if (totalNumberOfVertices != wantVertices)
      {
      cerr << "ERROR: Wrong number of vertices (" 
           << totalNumberOfVertices << " != " << wantVertices << ")" << endl;
      errors++;
      }

    totalNumberOfEdges
      = boost::mpi::all_reduce(world, g->GetNumberOfEdges(),
                               std::plus<vtkIdType>());
    if (totalNumberOfEdges != wantEdges)
      {
      cerr << "ERROR: Wrong number of edges ("
           << totalNumberOfEdges << " != " << wantEdges << ")" << endl;
      errors++;
      }
    if (world.rank() == 0)
      {
      cerr << "...done." << endl;
      }
    }

  if (world.rank() == 0)
    {
    cerr << "Testing simple tree+random generator (" << wantVertices << ", "
         << wantEdges << ")..." << endl;
    }
  source->SetStartWithTree(true);
  source->Update();
  g = source->GetOutput();

  totalNumberOfVertices
    = boost::mpi::all_reduce(world, g->GetNumberOfVertices(),
                             std::plus<vtkIdType>());
  if (totalNumberOfVertices != wantVertices)
    {
    cerr << "ERROR: Wrong number of vertices (" 
         << totalNumberOfVertices << " != " << wantVertices << ")" << endl;
    errors++;
    }

  totalNumberOfEdges 
    = boost::mpi::all_reduce(world, g->GetNumberOfEdges(),
                             std::plus<vtkIdType>());
  if (totalNumberOfEdges != wantEdges + wantVertices - 1)
    {
    cerr << "ERROR: Wrong number of edges ("
         << totalNumberOfEdges << " != " << (wantEdges + wantVertices - 1) 
         << ")" << endl;
    errors++;
    }

  if (world.rank() == 0)
    {
    cerr << "...done." << endl;
    }

  if (doPrint)
    {
    vtkSmartPointer<vtkVertexListIterator> vertices
      = vtkSmartPointer<vtkVertexListIterator>::New();
    g->GetVertices(vertices);
    while (vertices->HasNext())
      {
      vtkIdType u = vertices->Next();
      
      vtkSmartPointer<vtkOutEdgeIterator> outEdges
        = vtkSmartPointer<vtkOutEdgeIterator>::New();
        
      g->GetOutEdges(u, outEdges);
      while (outEdges->HasNext()) 
        {
        vtkOutEdgeType e = outEdges->Next();
        cerr << "  " << u << " -- " << e.Target << endl;
        }
      }
    }

  if (doBFS)
    {
    vtkSmartPointer<vtkPBGLBreadthFirstSearch> bfs
      = vtkSmartPointer<vtkPBGLBreadthFirstSearch>::New();
    bfs->SetInput(g);
    bfs->SetOriginVertex(g->GetDistributedGraphHelper()->MakeDistributedId(0, 0));

    // Run the breadth-first search
    if (world.rank() == 0)
      {
      cerr << "Breadth-first search...";
      }
    boost::mpi::timer timer;
    bfs->UpdateInformation();
    vtkStreamingDemandDrivenPipeline* exec =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(bfs->GetExecutive());
    exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), world.size());
    exec->SetUpdatePiece(exec->GetOutputInformation(0), world.rank());
    bfs->Update();

    // Verify the results of the breadth-first search
    if (world.rank() == 0)
      {
      cerr << " done in " << timer.elapsed() << " seconds" << endl;
      }
    }

  if (doColoring)
    {
    vtkSmartPointer<vtkPBGLVertexColoring> coloring
      = vtkSmartPointer<vtkPBGLVertexColoring>::New();
    coloring->SetInput(g);

    // Run the vertex-coloring
    if (world.rank() == 0)
      {
      cerr << "Vertex coloring...";
      }
    boost::mpi::timer timer;
    coloring->UpdateInformation();
    vtkStreamingDemandDrivenPipeline* exec =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(coloring->GetExecutive());
    exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), world.size());
    exec->SetUpdatePiece(exec->GetOutputInformation(0), world.rank());
    coloring->Update();

    if (world.rank() == 0)
      {
      cerr << " done in " << timer.elapsed() << " seconds" << endl;
      }

    if (doVerify)
      {
      vtkGraph* output = vtkGraph::SafeDownCast(coloring->GetOutput());
      if (world.rank() == 0)
        {
        cerr << " Verifying vertex coloring...";
        }

      // Turn the color array into a property map
      vtkIdTypeArray* colorArray
        = vtkIdTypeArray::SafeDownCast
            (output->GetVertexData()->GetAbstractArray("Color"));
      vtkDistributedVertexPropertyMapType<vtkIdTypeArray>::type colorMap 
        = MakeDistributedVertexPropertyMap(output, colorArray);

      // Restart the timer
      timer.restart();

      vtkSmartPointer<vtkVertexListIterator> vertices
        = vtkSmartPointer<vtkVertexListIterator>::New();
      output->GetVertices(vertices);
      while (vertices->HasNext())
        {
        vtkIdType u = vertices->Next();
        vtkSmartPointer<vtkOutEdgeIterator> outEdges
          = vtkSmartPointer<vtkOutEdgeIterator>::New();
        
        output->GetOutEdges(u, outEdges);
        while (outEdges->HasNext()) 
          {
          vtkOutEdgeType e = outEdges->Next();
          if (get(colorMap, u) == get(colorMap, e.Target))
            {
            cerr << "ERROR: Found adjacent vertices " << u << " and " 
                 << e.Target << " with the same color value (" 
                 << get(colorMap, u) << ")" << endl;
            ++errors;
            }
          }
        }
      
      output->GetDistributedGraphHelper()->Synchronize();

      if (world.rank() == 0)
        {
        cerr << " done in " << timer.elapsed() << " seconds" << endl;
        }
      }
    }

  if (doConnectedComponents)
    {
    vtkSmartPointer<vtkPBGLConnectedComponents> cc
      = vtkSmartPointer<vtkPBGLConnectedComponents>::New();
    cc->SetInput(g);

    // Run the connected components algorithm
    if (world.rank() == 0)
      {
      cerr << "Connected components...";
      }
    boost::mpi::timer timer;
    cc->UpdateInformation();
    vtkStreamingDemandDrivenPipeline* exec =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(cc->GetExecutive());
    exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), world.size());
    exec->SetUpdatePiece(exec->GetOutputInformation(0), world.rank());
    cc->Update();

    if (world.rank() == 0)
      {
      cerr << " done in " << timer.elapsed() << " seconds" << endl;
      }

    if (doVerify)
      {
      vtkGraph* output = vtkGraph::SafeDownCast(cc->GetOutput());
      if (world.rank() == 0)
        {
        cerr << " Verifying connected components...";
        }

      // Turn the component array into a property map
      vtkIdTypeArray* componentArray
        = vtkIdTypeArray::SafeDownCast
            (output->GetVertexData()->GetAbstractArray("Component"));
      vtkDistributedVertexPropertyMapType<vtkIdTypeArray>::type componentMap 
        = MakeDistributedVertexPropertyMap(output, componentArray);

      // Restart the timer
      timer.restart();

      vtkSmartPointer<vtkVertexListIterator> vertices
        = vtkSmartPointer<vtkVertexListIterator>::New();
      output->GetVertices(vertices);
      while (vertices->HasNext())
        {
        vtkIdType u = vertices->Next();
        vtkSmartPointer<vtkOutEdgeIterator> outEdges
          = vtkSmartPointer<vtkOutEdgeIterator>::New();
        
        output->GetOutEdges(u, outEdges);
        while (outEdges->HasNext()) 
          {
          vtkOutEdgeType e = outEdges->Next();
          if (get(componentMap, u) != get(componentMap, e.Target))
            {
            cerr << "ERROR: Found adjacent vertices " << u << " and " 
                 << e.Target << " with different component values (" 
                 << get(componentMap, u) << " and " 
                 << get(componentMap, e.Target) << ")" << endl;
            ++errors;
            }
          }
        }
      
      output->GetDistributedGraphHelper()->Synchronize();

      if (world.rank() == 0)
        {
        cerr << " done in " << timer.elapsed() << " seconds" << endl;
        }
      }
    }

  if (doMST)
    {
    vtkSmartPointer<vtkPBGLMinimumSpanningTree> mst
      = vtkSmartPointer<vtkPBGLMinimumSpanningTree>::New();
    mst->SetInput(g);
    mst->SetEdgeWeightArrayName("Weight");

    // Create an edge-weight array with edge weights in [0, 1).
    vtkDoubleArray *edgeWeightArray = vtkDoubleArray::New();
    edgeWeightArray->SetName("Weight");
    g->GetEdgeData()->AddArray(edgeWeightArray);
    edgeWeightArray->SetNumberOfTuples(g->GetNumberOfEdges());
    vtkMath::RandomSeed(1177 + 17 * world.rank());
    for (vtkIdType i = 0; i < g->GetNumberOfEdges(); ++i)
      {
      edgeWeightArray->SetTuple1(i, vtkMath::Random());
      }

    // Run the shortest paths algorithm.
    if (world.rank() == 0)
      {
      cerr << "Minimum spanning tree...";
      }
    boost::mpi::timer timer;
    mst->UpdateInformation();
    vtkStreamingDemandDrivenPipeline* exec =
      vtkStreamingDemandDrivenPipeline::SafeDownCast(mst->GetExecutive());
    exec->SetUpdateNumberOfPieces(exec->GetOutputInformation(0), world.size());
    exec->SetUpdatePiece(exec->GetOutputInformation(0), world.rank());
    mst->Update();

    if (world.rank() == 0)
      {
      cerr << " done in " << timer.elapsed() << " seconds" << endl;
      }
    }

  return errors;
}
