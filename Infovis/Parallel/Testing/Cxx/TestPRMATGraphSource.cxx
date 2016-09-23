/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPRMATGraphSource.cxx

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
#include "vtkDoubleArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkOutEdgeIterator.h"
#include "vtkMath.h"
#include "vtkPBGLBreadthFirstSearch.h"
#include "vtkPBGLConnectedComponents.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkPBGLShortestPaths.h"
#include "vtkPBGLRMATGraphSource.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVertexListIterator.h"

#include <functional>
#include <string>

#include <boost/mpi/collectives.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/timer.hpp>
#include <boost/lexical_cast.hpp>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestPRMATGraphSource(int argc, char* argv[])
{
  boost::mpi::environment env(argc, argv);
  boost::mpi::communicator world;

  vtkIdType wantVertices = 128;
  vtkIdType wantEdges = 512;
  double A = 0.45;
  double B = 0.15;
  double C = 0.15;
  double D = 0.25;
  bool doPrint = false;
  bool doVerify = true;
  bool doBFS = true;
  bool doSSSP = true;
  bool doConnectedComponents = true;

  if (argc > 6)
  {
    wantVertices = boost::lexical_cast<vtkIdType>(argv[1]);
    wantEdges = boost::lexical_cast<vtkIdType>(argv[2]);
    A = boost::lexical_cast<double>(argv[3]);
    B = boost::lexical_cast<double>(argv[4]);
    C = boost::lexical_cast<double>(argv[5]);
    D = boost::lexical_cast<double>(argv[6]);
  }

  for (int argIdx = 7; argIdx < argc; ++argIdx)
  {
      std::string arg = argv[argIdx];
      if (arg == "--print")
      {
        doPrint = true;
      }
      else if (arg == "--no-bfs")
      {
        doBFS = false;
      }
      else if (arg == "--no-sssp")
      {
        doSSSP = false;
      }
      else if (arg == "--no-verify")
      {
        doVerify = false;
      }
      else if (arg == "--no-connected-components")
      {
        doConnectedComponents = false;
      }
  }

  vtkIdType totalNumberOfVertices;
  vtkIdType totalNumberOfEdges;
  vtkGraph* g;

  VTK_CREATE(vtkPBGLRMATGraphSource, source);

  int errors = 0;

  source->SetNumberOfVertices(wantVertices);
  if (source->GetNumberOfVertices() != wantVertices)
  {
    wantVertices = source->GetNumberOfVertices();
    if (world.rank() == 0)
    {
      cerr << "Note: number of vertices rounded to the nearest power of 2."
           << endl;
    }
  }

  source->SetNumberOfEdges(wantEdges);
  source->SetProbabilities(A, B, C, D);

  if (world.rank() == 0)
  {
    cerr << "Testing R-MAT generator (" << wantVertices << ", "
         << wantEdges << ", " << A << ", " << B << ", " << C << ", "
         << D << ")..." << endl;
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
        cerr << "  " << u << " -> " << e.Target << endl;
      }
    }
  }

  if (doBFS)
  {
    vtkSmartPointer<vtkPBGLBreadthFirstSearch> bfs
      = vtkSmartPointer<vtkPBGLBreadthFirstSearch>::New();
    bfs->SetInputData(g);
    bfs->SetOriginVertex(g->GetDistributedGraphHelper()->MakeDistributedId(0, 0));

    // Run the breadth-first search
    if (world.rank() == 0)
    {
      cerr << "Breadth-first search...";
    }
    boost::mpi::timer timer;
    bfs->Update(world.rank(), world.size(), 0);

    if (world.rank() == 0)
    {
      cerr << " done in " << timer.elapsed() << " seconds" << endl;
    }
  }

  if (doSSSP)
  {
    vtkSmartPointer<vtkPBGLShortestPaths> sssp
      = vtkSmartPointer<vtkPBGLShortestPaths>::New();
    sssp->SetInputData(g);
    sssp->SetOriginVertex(g->GetDistributedGraphHelper()->MakeDistributedId(0, 0));
    sssp->SetEdgeWeightArrayName("Weight");

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
      cerr << "Single-source shortest paths...";
    }
    boost::mpi::timer timer;
    sssp->Update(world.rank(), world.size(), 0);

    if (world.rank() == 0)
    {
      cerr << " done in " << timer.elapsed() << " seconds" << endl;
    }

    if (doVerify)
    {
      vtkGraph* output = vtkGraph::SafeDownCast(sssp->GetOutput());
      if (world.rank() == 0)
      {
        cerr << " Verifying shortest paths...";
      }

      // Create distributed property maps for path length and edge weight
      vtkDoubleArray* pathLengthArray
        = vtkArrayDownCast<vtkDoubleArray>
            (output->GetVertexData()->GetAbstractArray("PathLength"));
      vtkDistributedVertexPropertyMapType<vtkDoubleArray>::type pathLengthMap
        = MakeDistributedVertexPropertyMap(output, pathLengthArray);
      vtkDistributedEdgePropertyMapType<vtkDoubleArray>::type edgeWeightMap
        = MakeDistributedEdgePropertyMap(output, edgeWeightArray);

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
          vtkOutEdgeType eOut = outEdges->Next();
          vtkEdgeType e(u, eOut.Target, eOut.Id);
          if (get(pathLengthMap, u) + get(edgeWeightMap, e)
                < get(pathLengthMap, e.Target))
          {
            cerr << "ERROR: Found a shorter path from source to "
                 << e.Target << " through " << u << endl
                 << "  Recorded path length is "
                 << get(pathLengthMap, e.Target)
                 << ", but this path has length "
                 << get(pathLengthMap, u) + get(edgeWeightMap, e)
                 << "." << endl;
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
    cc->SetInputData(g);

    // Run the connected components algorithm
    if (world.rank() == 0)
    {
      cerr << "Connected components...";
    }
    boost::mpi::timer timer;
    cc->Update(world.rank(), world.size(), 0);

    if (world.rank() == 0)
    {
      cerr << " done in " << timer.elapsed() << " seconds" << endl;
    }
  }

  return errors;
}
