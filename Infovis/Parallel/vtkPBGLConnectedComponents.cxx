/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLConnectedComponents.cxx

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
#include <boost/graph/use_mpi.hpp> // must precede PBGL headers

#include "vtkPBGLConnectedComponents.h"

#if !defined(VTK_LEGACY_REMOVE)

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDirectedGraph.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkType.h"
#include "vtkUndirectedGraph.h"
#include "vtkVertexListIterator.h"

#include <boost/graph/connected_components.hpp>
#include <boost/graph/distributed/connected_components.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/distributed/strong_components.hpp>

#include <utility> // for pair

using namespace boost;

vtkStandardNewMacro(vtkPBGLConnectedComponents);

// Constructor/Destructor
vtkPBGLConnectedComponents::vtkPBGLConnectedComponents()
{
  this->ComponentArrayName = 0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  VTK_LEGACY_BODY(vtkPBGLConnectedComponents::vtkPBGLConnectedComponents, "VTK 6.2");
}

vtkPBGLConnectedComponents::~vtkPBGLConnectedComponents()
{
  this->SetComponentArrayName(0);
}

int vtkPBGLConnectedComponents::RequestData(
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

  // Create the component array
  vtkIdTypeArray* componentArray = vtkIdTypeArray::New();
  if (this->ComponentArrayName)
  {
    componentArray->SetName(this->ComponentArrayName);
  }
  else
  {
    componentArray->SetName("Component");
  }
  componentArray->SetNumberOfTuples(output->GetNumberOfVertices());

  vtkDistributedGraphHelper *helper = output->GetDistributedGraphHelper();
  if (!helper)
  {
    vtkErrorMacro("Distributed vtkGraph is required.");
    return 1;
  }

  // We can only deal with Parallel BGL-distributed graphs.
  vtkPBGLDistributedGraphHelper *pbglHelper
    = vtkPBGLDistributedGraphHelper::SafeDownCast(helper);
  if (!pbglHelper)
  {
    vtkErrorMacro("Can only perform parallel shortest-paths on a Parallel BGL distributed graph");
    return 1;
  }

  int myRank = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  // Distributed component map
  typedef vtkDistributedVertexPropertyMapType<vtkIdTypeArray>::type
    DistributedComponentMap;
  DistributedComponentMap componentMap
    = MakeDistributedVertexPropertyMap(output, componentArray);

  // Execute the algorithm itself
  if (vtkUndirectedGraph::SafeDownCast(output))
  {
    // Distributed parent map. Used for scratch space in the algorithm
    // itself.
    vtkIdTypeArray* parentArray = vtkIdTypeArray::New();
    parentArray->SetNumberOfTuples(output->GetNumberOfVertices());
    typedef vtkDistributedVertexPropertyMapType<vtkIdTypeArray>::type
      DistributedParentMap;
    DistributedParentMap parentMap
      = MakeDistributedVertexPropertyMap(output, parentArray);

    vtkUndirectedGraph *g = vtkUndirectedGraph::SafeDownCast(output);
    boost::graph::distributed::cc_detail::parallel_connected_components
      (g, parentMap);
    boost::graph::distributed::number_components_from_parents(g, parentMap,
                                                              componentMap);

    // Remove the temporary parent array.
    parentArray->Delete();
  }
  else
  {
    vtkDirectedGraph *g = vtkDirectedGraph::SafeDownCast(output);
    boost::graph::distributed::fleischer_hendrickson_pinar_strong_components
      (g, componentMap, MakeDistributedVertexIndexMap(g));
  }

  // Add component array to the output
  output->GetVertexData()->AddArray(componentArray);
  componentArray->Delete();

  return 1;
}

void vtkPBGLConnectedComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ComponentArrayName: "
     << (this->ComponentArrayName ? this->ComponentArrayName : "(none)")
     << endl;
}

//----------------------------------------------------------------------------
int vtkPBGLConnectedComponents::FillInputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPBGLConnectedComponents::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
  }
  return 1;
}

#endif //VTK_LEGACY_REMOVE
