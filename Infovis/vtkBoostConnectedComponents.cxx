/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostConnectedComponents.cxx

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

#include "vtkBoostConnectedComponents.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkBoostGraphAdapter.h"
#include <boost/graph/strong_components.hpp>

using namespace boost;

vtkStandardNewMacro(vtkBoostConnectedComponents);

vtkBoostConnectedComponents::vtkBoostConnectedComponents()
{
}

vtkBoostConnectedComponents::~vtkBoostConnectedComponents()
{
}

int vtkBoostConnectedComponents::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  // Compute connected components.
  if (vtkDirectedGraph::SafeDownCast(input))
    {
    vtkDirectedGraph *g = vtkDirectedGraph::SafeDownCast(input);
    vtkIntArray* comps = vtkIntArray::New();
    comps->SetName("component");
    vector_property_map<default_color_type> color;
    vector_property_map<vtkIdType> root;
    vector_property_map<vtkIdType> discoverTime;
    strong_components(g, comps, color_map(color).root_map(root).discover_time_map(discoverTime));
    output->GetVertexData()->AddArray(comps);
    comps->Delete();
    }
  else
    {
    vtkUndirectedGraph *g = vtkUndirectedGraph::SafeDownCast(input);
    vtkIntArray* comps = vtkIntArray::New();
    comps->SetName("component");
    vector_property_map<default_color_type> color;
    connected_components(g, comps, color_map(color));
    output->GetVertexData()->AddArray(comps);
    comps->Delete();
    }

  return 1;
}

void vtkBoostConnectedComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

