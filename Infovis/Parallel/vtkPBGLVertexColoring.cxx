/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLVertexColoring.cxx

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
#include "vtkPBGLVertexColoring.h"

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
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkPointData.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkType.h"
#include "vtkUndirectedGraph.h"
#include "vtkVertexListIterator.h"

#include <boost/graph/use_mpi.hpp>

#include <boost/graph/distributed/boman_et_al_graph_coloring.hpp>
#include <boost/graph/parallel/algorithm.hpp>
#include <boost/property_map/property_map.hpp>

#include <vtksys/stl/utility> // for pair

using namespace boost;

vtkStandardNewMacro(vtkPBGLVertexColoring);

// Constructor/Destructor
vtkPBGLVertexColoring::vtkPBGLVertexColoring()
{
  // Default values for the origin vertex
  this->BlockSize = 10000;
  this->ColorArrayName = 0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

vtkPBGLVertexColoring::~vtkPBGLVertexColoring()
{
  this->SetColorArrayName(0);
}

int vtkPBGLVertexColoring::RequestData(
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

  // Create the color array
  vtkIdTypeArray* colorArray = vtkIdTypeArray::New();
  if (this->ColorArrayName)
    {
    colorArray->SetName(this->ColorArrayName);
    }
  else
    {
    colorArray->SetName("Color");
    }
  colorArray->SetNumberOfTuples(output->GetNumberOfVertices());

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

  // Distributed color map
  typedef vtkDistributedVertexPropertyMapType<vtkIdTypeArray>::type
    DistributedColorMap;
  DistributedColorMap colorMap
    = MakeDistributedVertexPropertyMap(output, colorArray);

  // Execute the algorithm itself
  if (vtkUndirectedGraph::SafeDownCast(output))
    {
    vtkUndirectedGraph *g = vtkUndirectedGraph::SafeDownCast(output);
    boost::graph::distributed::boman_et_al_graph_coloring(g, colorMap,
                                                          this->BlockSize);
    }
  else
    {
    vtkErrorMacro("Vertex coloring requires an undirected, distributed vtkGraph.");
    return 1;
    }

  // Add color array to the output
  output->GetVertexData()->AddArray(colorArray);
  colorArray->Delete();

  return 1;
}

void vtkPBGLVertexColoring::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "BlockSize: " << this->BlockSize << endl;

  os << indent << "ColorArrayName: "
     << (this->ColorArrayName ? this->ColorArrayName : "(none)")
     << endl;
}

//----------------------------------------------------------------------------
int vtkPBGLVertexColoring::FillInputPortInformation(
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
int vtkPBGLVertexColoring::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
    }
  return 1;
}

