/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLRandomGraphSource.cxx

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
#include "vtkPBGLRandomGraphSource.h"

#include "vtkBlockDistribution.h"
#include "vtkCellData.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMPI.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include <vtksys/stl/set>
#include <vtksys/stl/algorithm>
#include <vtksys/stl/functional>

#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives/scan.hpp>

vtkStandardNewMacro(vtkPBGLRandomGraphSource);

// ----------------------------------------------------------------------
vtkPBGLRandomGraphSource::vtkPBGLRandomGraphSource()
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  this->NumberOfVertices = 10;
  this->NumberOfEdges = 10;
  this->EdgeProbability = 0.5;
  this->IncludeEdgeWeights = false;
  this->Directed = 0;
  this->UseEdgeProbability = 0;
  this->StartWithTree = 0;
  this->AllowSelfLoops = false;
  this->AllowBalancedEdgeDistribution = true;
  this->GeneratePedigreeIds = true;
  this->VertexPedigreeIdArrayName = 0;
  this->SetVertexPedigreeIdArrayName("vertex id");
  this->EdgePedigreeIdArrayName = 0;
  this->SetEdgePedigreeIdArrayName("edge id");
  this->EdgeWeightArrayName = 0;
  this->SetEdgeWeightArrayName("edge weight");
  this->Seed = 1177 + 17 * rank;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkPBGLRandomGraphSource::~vtkPBGLRandomGraphSource()
{
  this->SetVertexPedigreeIdArrayName(0);
  this->SetEdgePedigreeIdArrayName(0);
  this->SetEdgeWeightArrayName(0);
}

// ----------------------------------------------------------------------

void 
vtkPBGLRandomGraphSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfVertices: " << this->NumberOfVertices << endl;
  os << indent << "NumberOfEdges: " << this->NumberOfEdges << endl;
  os << indent << "EdgeProbability: " << this->EdgeProbability << endl;
  os << indent << "IncludeEdgeWeights: " << this->IncludeEdgeWeights << endl;
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "UseEdgeProbability: " << this->UseEdgeProbability << endl;
  os << indent << "StartWithTree: " << this->StartWithTree << endl;
  os << indent << "AllowSelfLoops: " << this->AllowSelfLoops << endl;
  os << indent << "AllowBalancedEdgeDistribution: " << this->AllowBalancedEdgeDistribution << endl;
  os << indent << "GeneratePedigreeIds: " << this->GeneratePedigreeIds << endl;
  os << indent << "VertexPedigreeIdArrayName: "
    << (this->VertexPedigreeIdArrayName ? this->VertexPedigreeIdArrayName : "(null)") << endl;
  os << indent << "EdgePedigreeIdArrayName: "
    << (this->EdgePedigreeIdArrayName ? this->EdgePedigreeIdArrayName : "(null)") << endl;
  os << indent << "EdgeWeightArrayName: "
    << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(null)") << endl;
  os << indent << "Seed: " << this->Seed << endl;
}

// ----------------------------------------------------------------------

int 
vtkPBGLRandomGraphSource::RequestData(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector *outputVector)
{
  int myRank;
  int numProcs;
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

  // Seed the random number generator so we can produce repeatable results
  vtkMath::RandomSeed(this->Seed);
  
  // Create a mutable graph of the appropriate type.
  vtkSmartPointer<vtkMutableDirectedGraph> dirBuilder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkSmartPointer<vtkMutableUndirectedGraph> undirBuilder =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();
    
  // Create a Parallel BGL distributed graph helper
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper
    = vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();

  // Hook the distributed graph helper into the graph to make it a
  // distributed graph.
  if (this->Directed)
    {
    dirBuilder->SetDistributedGraphHelper(helper);
    }
  else
    {
    undirBuilder->SetDistributedGraphHelper(helper);
    }

  // A simple block distribution of vertices.
  vtkBlockDistribution distribution(this->NumberOfVertices, numProcs);

  // Add NumberOfVertices new vertices.
  vtkIdType myNumberOfVertices = distribution.GetBlockSize(myRank);
  vtkIdType myStartVertex = distribution.GetFirstGlobalIndexOnProcessor(myRank);
  vtkIdType myEndVertex = myStartVertex + myNumberOfVertices;
  for (vtkIdType i = 0; i < myNumberOfVertices; ++i)
    {
    if (this->Directed)
      {
      dirBuilder->AddVertex();
      }
    else
      {
      undirBuilder->AddVertex();
      }
    }

  // Make sure everyone has added their own local vertices.
  helper->Synchronize();

  if (this->StartWithTree)
    {
    vtkIdType myStart = myStartVertex;
    if (myStart == 0)
      {
      myStart = 1;
      }

    for (vtkIdType i = myStart; i < myEndVertex; i++)
      {
      // Pick a random vertex in [0, i-1].
      int j = static_cast<vtkIdType>(vtkMath::Random(0, i));

      vtkIdType iVertex 
        = helper->MakeDistributedId(distribution.GetProcessorOfElement(i),
                                    distribution.GetLocalIndexOfElement(i));
      vtkIdType jVertex 
        = helper->MakeDistributedId(distribution.GetProcessorOfElement(j),
                                    distribution.GetLocalIndexOfElement(j));

      if (this->Directed)
        {
        dirBuilder->LazyAddEdge(jVertex, iVertex); 
        }
      else
        {
        undirBuilder->LazyAddEdge(jVertex, iVertex);
        }
      }

    // Make sure everyone has added the edges in the random tree.
    helper->Synchronize();
    }

  if (this->UseEdgeProbability)
    {
    for (vtkIdType i = myStartVertex; i < myEndVertex; i++)
      {
      vtkIdType iVertex 
        = helper->MakeDistributedId(distribution.GetProcessorOfElement(i),
                                    distribution.GetLocalIndexOfElement(i));
      vtkIdType begin = this->Directed ? 0 : i + 1;
      for (vtkIdType j = begin; j < this->NumberOfVertices; j++)
        {
        vtkIdType jVertex 
          = helper->MakeDistributedId(distribution.GetProcessorOfElement(j),
                                      distribution.GetLocalIndexOfElement(j));
        double r = vtkMath::Random();
        if (r < this->EdgeProbability)
          {
          if (this->Directed)
            {
            dirBuilder->LazyAddEdge(iVertex, jVertex);
            }
          else
            {
            undirBuilder->LazyAddEdge(iVertex, jVertex);
            }
          }
        }
      }
    }
  else
    {
    vtkIdType MaxEdges;
    if (this->AllowSelfLoops)
      {
      MaxEdges = this->NumberOfVertices * this->NumberOfVertices;
      }
    else
      {
      MaxEdges = (this->NumberOfVertices * (this->NumberOfVertices-1)) / 2;
      }
    
    if (this->NumberOfEdges > MaxEdges)
      {
      this->NumberOfEdges = MaxEdges;
      }

    vtkIdType avgNumberOfEdges = this->NumberOfEdges / numProcs;
    vtkIdType myNumberOfEdges = avgNumberOfEdges;
    if (myRank < this->NumberOfEdges % numProcs)
      {
      ++myNumberOfEdges;
      }

    for (vtkIdType i = 0; i < myNumberOfEdges; i++)
      {
      bool newEdgeFound = false;
      while (!newEdgeFound)
        {
        vtkIdType s;
        vtkIdType t;

        if (this->AllowBalancedEdgeDistribution)
          {
          s = static_cast<vtkIdType>(vtkMath::Random(myStartVertex, 
                                                     myEndVertex));
          }
        else
          {
          s = static_cast<vtkIdType>(vtkMath::Random(0, 
                                                     this->NumberOfVertices));
          }
        t = static_cast<vtkIdType>(vtkMath::Random(0, this->NumberOfVertices));
        if (s == t && (!this->AllowSelfLoops))
          {
          continue;
          }

      vtkIdType sVertex 
        = helper->MakeDistributedId(distribution.GetProcessorOfElement(s),
                                    distribution.GetLocalIndexOfElement(s));
      vtkIdType tVertex 
        = helper->MakeDistributedId(distribution.GetProcessorOfElement(t),
                                    distribution.GetLocalIndexOfElement(t));

        vtkDebugMacro(<<"Adding edge " << s << " to " << t);
        if (this->Directed)
          {
          dirBuilder->LazyAddEdge(sVertex, tVertex);
          }
        else
          {
          undirBuilder->LazyAddEdge(sVertex, tVertex);
          }
        newEdgeFound = true;
        }
      }
    }

  // Make sure everybody has added their edges and back-edges.
  helper->Synchronize();

  // Copy the structure into the output.
  vtkGraph *output = vtkGraph::GetData(outputVector);
  if (this->Directed)
    {
    if (!output->CheckedShallowCopy(dirBuilder))
      {
      vtkErrorMacro(<<"Invalid structure.");
      return 0;
      }
    }
  else
    {
    if (!output->CheckedShallowCopy(undirBuilder))
      {
      vtkErrorMacro(<<"Invalid structure.");
      return 0;
      }
    }

  if (this->IncludeEdgeWeights)
    {
    if (!this->EdgeWeightArrayName)
      {
      vtkErrorMacro("When generating edge weights, "
        << "edge weights array name must be defined.");
      return 0;
      }
    vtkFloatArray *weights = vtkFloatArray::New();
    weights->SetName(this->EdgeWeightArrayName);
    for (vtkIdType i = 0; i < output->GetNumberOfEdges(); ++i)
      {
      weights->InsertNextValue(vtkMath::Random());
      }
    output->GetEdgeData()->AddArray(weights);
    weights->Delete();
    }

  if (this->GeneratePedigreeIds)
    {
    if (!this->VertexPedigreeIdArrayName || !this->EdgePedigreeIdArrayName)
      {
      vtkErrorMacro("When generating pedigree ids, "
        << "vertex and edge pedigree id array names must be defined.");
      return 0;
      }
    vtkIdType numVert = output->GetNumberOfVertices();
    vtkSmartPointer<vtkIdTypeArray> vertIds =
      vtkSmartPointer<vtkIdTypeArray>::New();
    vertIds->SetName(this->VertexPedigreeIdArrayName);
    vertIds->SetNumberOfTuples(numVert);
    for (vtkIdType i = 0; i < numVert; ++i)
      {
      vertIds->SetValue(i, myStartVertex + i);
      }
    output->GetVertexData()->SetPedigreeIds(vertIds);

    // Attach a distribution function to the helper that maps these
    // global vertex numbers back to .  TODO: Actually do it :)

    // Figure out how many edges come before us in the graph.
    boost::mpi::communicator world;
    vtkIdType myStartEdge 
      = boost::mpi::scan(world, output->GetNumberOfEdges(), 
                         vtkstd::plus<vtkIdType>());

    vtkIdType numEdge = output->GetNumberOfEdges();
    vtkSmartPointer<vtkIdTypeArray> edgeIds =
      vtkSmartPointer<vtkIdTypeArray>::New();
    edgeIds->SetName(this->EdgePedigreeIdArrayName);
    edgeIds->SetNumberOfTuples(numEdge);
    for (vtkIdType i = 0; i < numEdge; ++i)
      {
      edgeIds->SetValue(i, myStartEdge + i);
      }
    output->GetEdgeData()->SetPedigreeIds(edgeIds);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPBGLRandomGraphSource::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* )
{
  vtkDataObject *current = this->GetExecutive()->GetOutputData(0);
  if (!current 
    || (this->Directed && !vtkDirectedGraph::SafeDownCast(current))
    || (!this->Directed && vtkDirectedGraph::SafeDownCast(current)))
    {
    vtkGraph *output = 0;
    if (this->Directed)
      {
      output = vtkDirectedGraph::New();
      }
    else
      {
      output = vtkUndirectedGraph::New();
      }

    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    }

  return 1;
}


