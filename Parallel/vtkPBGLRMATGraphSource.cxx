/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLRMATGraphSource.cxx

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
/* 
 * Copyright (C) 2008 The Trustees of Indiana University.
 * Use, modification and distribution is subject to the Boost Software
 * License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)
 */
#include "vtkPBGLRMATGraphSource.h"

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

vtkStandardNewMacro(vtkPBGLRMATGraphSource);

// ----------------------------------------------------------------------
vtkPBGLRMATGraphSource::vtkPBGLRMATGraphSource()
{
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  this->NumberOfVertices = 128;
  this->NumberOfEdges = 512;
  this->A = 0.25;
  this->B = 0.25;
  this->C = 0.25;
  this->D = 0.25;
  this->IncludeEdgeWeights = false;
  this->AllowSelfLoops = false;
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

vtkPBGLRMATGraphSource::~vtkPBGLRMATGraphSource()
{
  this->SetVertexPedigreeIdArrayName(0);
  this->SetEdgePedigreeIdArrayName(0);
  this->SetEdgeWeightArrayName(0);
}

// ----------------------------------------------------------------------

void 
vtkPBGLRMATGraphSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfVertices: " << this->NumberOfVertices << endl;
  os << indent << "NumberOfEdges: " << this->NumberOfEdges << endl;
  os << indent << "Probabilities: " << this->A << ", " << this->B << ", " 
     << this->C << ", " << this->D << endl;
  os << indent << "IncludeEdgeWeights: " << this->IncludeEdgeWeights << endl;
  os << indent << "AllowSelfLoops: " << this->AllowSelfLoops << endl;
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
void vtkPBGLRMATGraphSource::SetNumberOfVertices(vtkIdType value)
{
  vtkIdType mask = (vtkIdType) 1 << ((sizeof(vtkIdType) * CHAR_BIT) - 2);
  while (mask != 0) 
    {
    if (value & mask)
      {
      // We've found the most significant 1-bit.
      if (value & (mask >> (vtkIdType) 1))
        {
        // Round up to the next power of two.
        mask = mask << (vtkIdType) 1;
        }
      break;
      }

    mask = mask >> (vtkIdType) 1;
    }

  this->NumberOfVertices = mask;
}

// ----------------------------------------------------------------------
void 
vtkPBGLRMATGraphSource::SetProbabilities(double A, double B, double C, double D)
{
  if (A + B + C + D != 1.0)
    {
    vtkErrorMacro("R-MAT probabilities do not add up to 1.0.");
    return;
    }

  this->A = A;
  this->B = B;
  this->C = C;
  this->D = D;
}

// ----------------------------------------------------------------------
void 
vtkPBGLRMATGraphSource::GetProbabilities(double *A, double *B, double *C, double *D)
{
  if (A) 
    {
    *A = this->A;
    }
  if (B) 
    {
    *B = this->B;
    }
  if (C) 
    {
    *C = this->C;
    }
  if (D) 
    {
    *D = this->D;
    }
}


// ----------------------------------------------------------------------

int 
vtkPBGLRMATGraphSource::RequestData(
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
  
  // Create a mutable, directed graph.
  vtkSmartPointer<vtkMutableDirectedGraph> dirBuilder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
    
  // Create a Parallel BGL distributed graph helper
  vtkSmartPointer<vtkPBGLDistributedGraphHelper> helper
    = vtkSmartPointer<vtkPBGLDistributedGraphHelper>::New();

  // Hook the distributed graph helper into the graph to make it a
  // distributed graph.
  dirBuilder->SetDistributedGraphHelper(helper);

  // Vertex distribution.
  vtkBlockDistribution distribution(this->NumberOfVertices, numProcs);

  // Add NumberOfVertices new vertices.
  vtkIdType myNumberOfVertices = distribution.GetBlockSize(myRank);
  vtkIdType myStartVertex = distribution.GetFirstGlobalIndexOnProcessor(myRank);
  vtkIdType myEndVertex = myStartVertex + myNumberOfVertices;
  for (vtkIdType i = 0; i < myNumberOfVertices; ++i)
    {
    dirBuilder->AddVertex();
    }

  // Make sure everyone has added their own local vertices.
  helper->Synchronize();

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

  vtkIdType numLevels = (vtkIdType) log2(this->NumberOfVertices) + 1;
  double AB = this->A + this->B;
  double CNorm = this->C / (this->C + this->D);
  double ANorm = this->A / (this->A + this->B);
  for (vtkIdType i = 0; i < myNumberOfEdges; i++)
    {
    bool newEdgeFound = false;
    while (!newEdgeFound)
      {
      vtkIdType s = 0;
      vtkIdType t = 0;

      for (vtkIdType level = 1; level < numLevels; ++level) 
        {
        bool sBit = vtkMath::Random() > (this->A + this->B);
        bool tBit 
          = (vtkMath::Random() > (CNorm * (sBit ? 1 : 0)
                                  + ANorm * (sBit ? 1 : 0))) ? 1 : 0;
        s |= ((vtkIdType) 1 << (level-1)) * (sBit ? 1 : 0); 
        t |= ((vtkIdType) 1 << (level-1)) * (tBit ? 1 : 0);
        }

      if (s == t && (!this->AllowSelfLoops))
        {
        continue;
        }

      // TODO: We should apply some permutation to the s and t vertex
      // numbers, so that we get some kind of randomized distribution
      // of the vertices. Otherwise, we'll have a severely imbalanced
      // distribution, since the high-degree vertices are likely to
      // all end up on the lower-numbered ranks. Note that this
      // doesn't change the block distribution (computed below); it
      // sits on top of the block distribution.

      vtkIdType sVertex 
        = helper->MakeDistributedId(distribution.GetProcessorOfElement(s),
                                    distribution.GetLocalIndexOfElement(s));
      vtkIdType tVertex 
        = helper->MakeDistributedId(distribution.GetProcessorOfElement(t),
                                    distribution.GetLocalIndexOfElement(t));

      vtkDebugMacro(<<"Adding edge " << s << " to " << t);
      dirBuilder->LazyAddEdge(sVertex, tVertex);
      newEdgeFound = true;
      }
    }

  // Make sure everybody has added their edges and back-edges.
  helper->Synchronize();

  // Copy the structure into the output.
  vtkGraph *output = vtkGraph::GetData(outputVector);
  if (!output->CheckedShallowCopy(dirBuilder))
    {
    vtkErrorMacro(<<"Invalid structure.");
    return 0;
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
    vtkIdType numEdge = output->GetNumberOfEdges();
    boost::mpi::communicator world;
    vtkIdType myStartEdge 
      = boost::mpi::scan(world, numEdge, vtkstd::plus<vtkIdType>()) - numEdge;

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
int vtkPBGLRMATGraphSource::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* )
{
  vtkDataObject *current = this->GetExecutive()->GetOutputData(0);
  if (!current || !vtkDirectedGraph::SafeDownCast(current))
    {
    vtkGraph *output = 0;
    output = vtkDirectedGraph::New();

    this->GetExecutive()->SetOutputData(0, output);
    output->Delete();
    }

  return 1;
}


