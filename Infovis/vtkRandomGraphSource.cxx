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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkRandomGraphSource.h"
#include <vtkCellData.h>
#include <vtkFloatArray.h>
#include "vtkGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkInformation.h"
#include "vtkMath.h"

#include <vtksys/stl/set>
#include <vtksys/stl/algorithm>

vtkCxxRevisionMacro(vtkRandomGraphSource, "1.6");
vtkStandardNewMacro(vtkRandomGraphSource);

// ----------------------------------------------------------------------

vtkRandomGraphSource::vtkRandomGraphSource()
{
  this->NumberOfVertices = 10;
  this->NumberOfEdges = 10;
  this->Directed = 0;
  this->UseEdgeProbability = 0;
  this->IncludeEdgeWeights = false;
  this->AllowSelfLoops = false;
  this->EdgeProbability = 0.5;
  this->StartWithTree = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkRandomGraphSource::~vtkRandomGraphSource()
{
}

// ----------------------------------------------------------------------

void 
vtkRandomGraphSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfVertices: " << this->NumberOfVertices << endl;
  os << indent << "UseEdgeProbability: " << this->UseEdgeProbability << endl;
  os << indent << "NumberOfEdges: " << this->NumberOfEdges << endl;
  os << indent << "EdgeProbability: " << this->EdgeProbability << endl;
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "StartWithTree: " << this->StartWithTree << endl;
  os << indent << "IncludeEdgeWeights: " << this->IncludeEdgeWeights << endl;
  os << indent << "AllowSelfLoops: " << this->AllowSelfLoops << endl;
}

// ----------------------------------------------------------------------

int 
vtkRandomGraphSource::RequestData(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  // Generate the graph
  vtkGraph* output = vtkGraph::GetData(outputVector);
  output->SetNumberOfVertices(this->NumberOfVertices);
  output->SetDirected(this->Directed);

  if (this->StartWithTree)
    {
    for (vtkIdType i = 1; i < this->NumberOfVertices; i++)
      {
      // Pick a random vertex in [0, i-1].
      int j = static_cast<vtkIdType>(vtkMath::Random(0, i));
      output->AddEdge(j, i);
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
          output->AddEdge(i, j);
          }
        }
      }
    }
  else
    {
    // Don't duplicate edges.
    vtksys_stl::set< vtksys_stl::pair<vtkIdType, vtkIdType> > existingEdges;

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

        if (existingEdges.find(newEdge) == existingEdges.end())
          {
          vtkDebugMacro(<<"Adding edge " << s << " to " << t);
          output->AddEdge(s, t);
          existingEdges.insert(newEdge);
          newEdgeFound = true;
          }
        }
      }
    }

  if (this->IncludeEdgeWeights)
    {
    vtkFloatArray *weights = vtkFloatArray::New();
    weights->SetName("edge_weights");
    for (vtkIdType i = 0; i < output->GetNumberOfEdges(); ++i)
      {
      weights->InsertNextValue(vtkMath::Random());
      }
    output->GetEdgeData()->AddArray(weights);
    weights->Delete();
    }

  return 1;
}

