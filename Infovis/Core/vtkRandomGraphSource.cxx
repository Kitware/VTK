/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomGraphSource.cxx

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
#include "vtkRandomGraphSource.h"

#include "vtkCellData.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"

#include <vtksys/stl/set>
#include <vtksys/stl/algorithm>

vtkStandardNewMacro(vtkRandomGraphSource);

// ----------------------------------------------------------------------

vtkRandomGraphSource::vtkRandomGraphSource()
{
  this->NumberOfVertices = 10;
  this->NumberOfEdges = 10;
  this->EdgeProbability = 0.5;
  this->IncludeEdgeWeights = false;
  this->Directed = 0;
  this->UseEdgeProbability = 0;
  this->StartWithTree = 0;
  this->AllowSelfLoops = false;
  this->AllowParallelEdges = false;
  this->GeneratePedigreeIds = true;
  this->VertexPedigreeIdArrayName = 0;
  this->SetVertexPedigreeIdArrayName("vertex id");
  this->EdgePedigreeIdArrayName = 0;
  this->SetEdgePedigreeIdArrayName("edge id");
  this->EdgeWeightArrayName = 0;
  this->SetEdgeWeightArrayName("edge weight");
  this->Seed = 1177;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkRandomGraphSource::~vtkRandomGraphSource()
{
  this->SetVertexPedigreeIdArrayName(0);
  this->SetEdgePedigreeIdArrayName(0);
  this->SetEdgeWeightArrayName(0);
}

// ----------------------------------------------------------------------

void
vtkRandomGraphSource::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "AllowParallelEdges: " << this->AllowParallelEdges << endl;
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
vtkRandomGraphSource::RequestData(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector *outputVector)
{
  // Seed the random number generator so we can produce repeatable results
  vtkMath::RandomSeed(this->Seed);

  // Create a mutable graph of the appropriate type.
  vtkSmartPointer<vtkMutableDirectedGraph> dirBuilder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkSmartPointer<vtkMutableUndirectedGraph> undirBuilder =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  for (vtkIdType i = 0; i < this->NumberOfVertices; ++i)
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

  if (this->StartWithTree)
    {
    for (vtkIdType i = 1; i < this->NumberOfVertices; i++)
      {
      // Pick a random vertex in [0, i-1].
      int j = static_cast<vtkIdType>(vtkMath::Random(0, i));
      if (this->Directed)
        {
        dirBuilder->AddEdge(j, i);
        }
      else
        {
        undirBuilder->AddEdge(j, i);
        }
      }
    }

  if (this->UseEdgeProbability)
    {
    for (vtkIdType i = 0; i < this->NumberOfVertices; i++)
      {
      vtkIdType begin = this->Directed ? 0 : i + 1;
      for (vtkIdType j = begin; j < this->NumberOfVertices; j++)
        {
        double r = vtkMath::Random();
        if (r < this->EdgeProbability)
          {
          if (this->Directed)
            {
            dirBuilder->AddEdge(i, j);
            }
          else
            {
            undirBuilder->AddEdge(i, j);
            }
          }
        }
      }
    }
  else
    {
    // Don't duplicate edges.
    vtksys_stl::set< vtksys_stl::pair<vtkIdType, vtkIdType> > existingEdges;

    vtkIdType MaxEdges;
    if (this->AllowParallelEdges)
      {
      MaxEdges = this->NumberOfEdges;
      }
    else if (this->AllowSelfLoops)
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

    for (vtkIdType i = 0; i < this->NumberOfEdges; i++)
      {
      bool newEdgeFound = false;
      while (!newEdgeFound)
        {
        vtkIdType s = static_cast<vtkIdType>(vtkMath::Random(0, this->NumberOfVertices));
        vtkIdType t = static_cast<vtkIdType>(vtkMath::Random(0, this->NumberOfVertices));
        if (s == t && (!this->AllowSelfLoops))
          {
          continue;
          }

        if (!this->Directed)
          {
          vtkIdType tmp;
          if (s > t)
            {
            tmp = t;
            t = s;
            s = tmp;
            }
          }

        vtksys_stl::pair<vtkIdType, vtkIdType> newEdge(s, t);

        if (this->AllowParallelEdges
          || existingEdges.find(newEdge) == existingEdges.end())
          {
          vtkDebugMacro(<<"Adding edge " << s << " to " << t);
          if (this->Directed)
            {
            dirBuilder->AddEdge(s, t);
            }
          else
            {
            undirBuilder->AddEdge(s, t);
            }
          existingEdges.insert(newEdge);
          newEdgeFound = true;
          }
        }
      }
    }

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
      vertIds->SetValue(i, i);
      }
    output->GetVertexData()->SetPedigreeIds(vertIds);

    vtkIdType numEdge = output->GetNumberOfEdges();
    vtkSmartPointer<vtkIdTypeArray> edgeIds =
      vtkSmartPointer<vtkIdTypeArray>::New();
    edgeIds->SetName(this->EdgePedigreeIdArrayName);
    edgeIds->SetNumberOfTuples(numEdge);
    for (vtkIdType i = 0; i < numEdge; ++i)
      {
      edgeIds->SetValue(i, i);
      }
    output->GetEdgeData()->SetPedigreeIds(edgeIds);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkRandomGraphSource::RequestDataObject(
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


