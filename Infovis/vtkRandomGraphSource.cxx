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
#include "vtkGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkInformation.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkRandomGraphSource, "1.1");
vtkStandardNewMacro(vtkRandomGraphSource);


vtkRandomGraphSource::vtkRandomGraphSource()
{
  this->NumberOfNodes = 10;
  this->NumberOfArcs = 10;
  this->Directed = 0;
  this->UseArcProbability = 0;
  this->ArcProbability = 0.5;
  this->StartWithTree = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

vtkRandomGraphSource::~vtkRandomGraphSource()
{
}

void vtkRandomGraphSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfNodes: " << this->NumberOfNodes << endl;
  os << indent << "UseArcProbability: " << this->UseArcProbability << endl;
  os << indent << "NumberOfArcs: " << this->NumberOfArcs << endl;
  os << indent << "ArcProbability: " << this->ArcProbability << endl;
  os << indent << "Directed: " << this->Directed << endl;
}

int vtkRandomGraphSource::RequestData(
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
    for (vtkIdType i = 0; i < this->NumberOfArcs; i++)
      {
      vtkIdType s = static_cast<vtkIdType>(vtkMath::Random(0, this->NumberOfNodes));
      vtkIdType t = static_cast<vtkIdType>(vtkMath::Random(0, this->NumberOfNodes));
      output->AddArc(s, t);
      }
    }
  
  return 1;
}

