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

vtkCxxRevisionMacro(vtkRandomGraphSource, "1.4");
vtkStandardNewMacro(vtkRandomGraphSource);

// ----------------------------------------------------------------------

vtkRandomGraphSource::vtkRandomGraphSource()
{
  this->NumberOfNodes = 10;
  this->NumberOfArcs = 10;
  this->Directed = 0;
  this->UseArcProbability = 0;
  this->IncludeArcWeights = false;
  this->AllowSelfLoops = false;
  this->ArcProbability = 0.5;
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
  os << indent << "NumberOfNodes: " << this->NumberOfNodes << endl;
  os << indent << "UseArcProbability: " << this->UseArcProbability << endl;
  os << indent << "NumberOfArcs: " << this->NumberOfArcs << endl;
  os << indent << "ArcProbability: " << this->ArcProbability << endl;
  os << indent << "Directed: " << this->Directed << endl;
  os << indent << "StartWithTree: " << this->StartWithTree << endl;
  os << indent << "IncludeArcWeights: " << this->IncludeArcWeights << endl;
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
  output->SetNumberOfNodes(this->NumberOfNodes);
  output->SetDirected(this->Directed);

  if (this->StartWithTree)
    {
    for (vtkIdType i = 1; i < this->NumberOfNodes; i++)
      {
      // Pick a random node in [0, i-1].
      int j = static_cast<vtkIdType>(vtkMath::Random(0, i));
      output->AddArc(j, i);
      }
    }

  if (this->UseArcProbability)
    {
    for (vtkIdType i = 0; i < this->NumberOfNodes; i++)
      {
      vtkIdType begin = this->Directed ? 0 : i + 1;
      for (vtkIdType j = begin; j < this->NumberOfNodes; j++)
        {
        double r = vtkMath::Random();
        if (r < this->ArcProbability)
          {
          output->AddArc(i, j);
          }
        }
      }
    }
  else
    {
    // Don't duplicate edges.
    vtksys_stl::set< vtksys_stl::pair<vtkIdType, vtkIdType> > existingArcs;

    vtkIdType MaxEdges;
    if (this->AllowSelfLoops)
      {
      MaxEdges = this->NumberOfNodes * this->NumberOfNodes;
      }
    else
      {
      MaxEdges = (this->NumberOfNodes * (this->NumberOfNodes-1)) / 2;
      }
    
    if (this->NumberOfArcs > MaxEdges)
      {
      this->NumberOfArcs = MaxEdges;
      }

    for (vtkIdType i = 0; i < this->NumberOfArcs; i++)
      {
      bool newArcFound = false;
      while (!newArcFound)
        {
        vtkIdType s = static_cast<vtkIdType>(vtkMath::Random(0, this->NumberOfNodes));
        vtkIdType t = static_cast<vtkIdType>(vtkMath::Random(0, this->NumberOfNodes));
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

        vtksys_stl::pair<vtkIdType, vtkIdType> newArc(s, t);

        if (existingArcs.find(newArc) == existingArcs.end())
          {
          vtkDebugMacro(<<"Adding arc " << s << " to " << t);
          output->AddArc(s, t);
          existingArcs.insert(newArc);
          newArcFound = true;
          }
        }
      }
    }

  if (this->IncludeArcWeights)
    {
    vtkFloatArray *weights = vtkFloatArray::New();
    weights->SetName("arc_weights");
    for (vtkIdType i = 0; i < output->GetNumberOfArcs(); ++i)
      {
      weights->InsertNextValue(vtkMath::Random());
      }
    output->GetArcData()->AddArray(weights);
    weights->Delete();
    }

  return 1;
}

