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
#include "vtkBoostBreadthFirstSearch.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkOutEdgeIterator.h"
#include "vtkPRandomGraphSource.h"
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
  bool doBFS = true;

  if (argc > 2) 
    {
    wantVertices = boost::lexical_cast<vtkIdType>(argv[1]);
    wantEdges = boost::lexical_cast<vtkIdType>(argv[2]);
    }
                 
  for (int argIdx = 3; argIdx < argc; ++argIdx)
    {
      vtkstd::string arg = argv[argIdx];
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
    }

  vtkIdType totalNumberOfVertices;
  vtkIdType totalNumberOfEdges;
  vtkGraph* g;

  VTK_CREATE(vtkPRandomGraphSource, source);

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
                               vtkstd::plus<vtkIdType>());
    if (totalNumberOfVertices != wantVertices)
      {
      cerr << "ERROR: Wrong number of vertices (" 
           << totalNumberOfVertices << " != " << wantVertices << ")" << endl;
      errors++;
      }

    totalNumberOfEdges
      = boost::mpi::all_reduce(world, g->GetNumberOfEdges(),
                               vtkstd::plus<vtkIdType>());
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
                             vtkstd::plus<vtkIdType>());
  if (totalNumberOfVertices != wantVertices)
    {
    cerr << "ERROR: Wrong number of vertices (" 
         << totalNumberOfVertices << " != " << wantVertices << ")" << endl;
    errors++;
    }

  totalNumberOfEdges 
    = boost::mpi::all_reduce(world, g->GetNumberOfEdges(),
                             vtkstd::plus<vtkIdType>());
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
    vtkSmartPointer<vtkBoostBreadthFirstSearch> bfs
      = vtkSmartPointer<vtkBoostBreadthFirstSearch>::New();
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
  return errors;
}
