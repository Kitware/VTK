/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLCollectGraph.cxx

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
#include "vtkPBGLCollectGraph.h"

#if !defined(VTK_LEGACY_REMOVE)

#include "vtkBoostGraphAdapter.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDirectedGraph.h"
#include "vtkDistributedGraphHelper.h"
#include "vtkEdgeListIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPBGLDistributedGraphHelper.h"
#include "vtkPBGLGraphAdapter.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkSystemIncludes.h"
#include "vtkUndirectedGraph.h"
#include "vtkVariant.h"
#include "vtkVariantBoostSerialization.h"

#include <boost/mpi/collectives/all_gather.hpp>
#include <boost/mpi/collectives/gather.hpp>
#include <boost/mpi/collectives/all_reduce.hpp>
#include <boost/serialization/vector.hpp>

#include <utility> // for pair
#include <numeric> // for accumulate, partial_sum
#include <functional> // for plus

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

using namespace boost;

vtkStandardNewMacro(vtkPBGLCollectGraph);


// Constructor/Destructor
vtkPBGLCollectGraph::vtkPBGLCollectGraph()
{
  // Default values for the origin vertex
  this->TargetProcessor = 0;
  this->ReplicateGraph  = false;
  this->CopyVertexData  = true;
  this->CopyEdgeData    = true;
  this->CreateOriginProcessArray = false;
  this->OriginProcessArrayName = NULL;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  VTK_LEGACY_BODY(vtkPBGLCollectGraph::vtkPBGLCollectGraph, "VTK 6.2");
}


//----------------------------------------------------------------------------
vtkPBGLCollectGraph::~vtkPBGLCollectGraph()
{
}


//----------------------------------------------------------------------------
int vtkPBGLCollectGraph::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation * inInfo  = inputVector[0]->GetInformationObject(0);
  vtkInformation * outInfo = outputVector->GetInformationObject(0);

  // get the input
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  bool isDirected = vtkDirectedGraph::SafeDownCast(input) != 0;

  // Create a mutable graph of the appropriate type.
  vtkSmartPointer<vtkMutableDirectedGraph> dirBuilder =
    vtkSmartPointer<vtkMutableDirectedGraph>::New();
  vtkSmartPointer<vtkMutableUndirectedGraph> undirBuilder =
    vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  vtkDistributedGraphHelper *helper = input->GetDistributedGraphHelper();
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
    vtkErrorMacro("Can only collect Parallel BGL distributed graph");
    return 1;
  }

  // Set up the origin process array
  VTK_CREATE(vtkIdTypeArray, VertexProcessSourceArray);
  VTK_CREATE(vtkIdTypeArray, EdgeProcessSourceArray);
  if(this->CreateOriginProcessArray)
  {
    if(OriginProcessArrayName)
    {
      VertexProcessSourceArray->SetName(this->OriginProcessArrayName);
      EdgeProcessSourceArray->SetName(this->OriginProcessArrayName);
    }
    else
    {
      VertexProcessSourceArray->SetName("ProcessorID");
      EdgeProcessSourceArray->SetName("ProcessorID");
    }
  }

  int myRank = input->GetInformation()->Get(vtkDataObject::DATA_PIECE_NUMBER());
  int numProcs
    = input->GetInformation()->Get(vtkDataObject::DATA_NUMBER_OF_PIECES());

  // Get the Boost.MPI communicator from the input graph.
  boost::mpi::communicator comm = communicator(pbglHelper->GetProcessGroup());

  // Determine the number of vertices stored on each processor.
  std::vector<vtkIdType> numVerticesPerProcessor(numProcs);
  boost::mpi::all_gather(comm, input->GetNumberOfVertices(),
                         numVerticesPerProcessor);

  // Determine the total number of vertices stored on each processor
  vtkIdType totalNumVertices
    = std::accumulate(numVerticesPerProcessor.begin(),
                         numVerticesPerProcessor.end(),
                         vtkIdType(0));

  // Determine the ID of the first vertex provided by each rank.
  std::vector<vtkIdType> vertexOffsets(numProcs+1);
  std::partial_sum(numVerticesPerProcessor.begin(),
                      numVerticesPerProcessor.end(),
                      vertexOffsets.begin()+1);

  // Collect and add vertices to the resulting graph.
  vtkDataSetAttributes *distribVertexData = input->GetVertexData();
  if (distribVertexData->GetNumberOfArrays() > 0 && this->CopyVertexData)
  {
    int numArrays = distribVertexData->GetNumberOfArrays();

    // Get the arrays we'll be reading from.
    std::vector<vtkAbstractArray*> arrays(numArrays);
    for (int arrayIndex = 0; arrayIndex < numArrays; ++arrayIndex)
    {
      arrays[arrayIndex] = distribVertexData->GetAbstractArray(arrayIndex);
    }

    // Serialize and communicate all vertices and their properties.
    std::vector<std::vector<vtkVariant> > allVertexProperties;
    {
      // Serialize all of the vertex attributes from the local vertices.
      vtkIdType myNumVertices = numVerticesPerProcessor[myRank];
      std::vector<vtkVariant> myVertexProperties(myNumVertices * numArrays);
      vtkIdType vertIndex;

      for (vertIndex = 0; vertIndex < myNumVertices; ++vertIndex)
      {
        for (int arrayIndex = 0; arrayIndex < numArrays; ++arrayIndex)
        {
          myVertexProperties[vertIndex * numArrays + arrayIndex]
              = arrays[arrayIndex]->GetVariantValue(vertIndex);
        }
      }

      // Any attribute arrays flagged as containing distributed ids should be
      // converted to appropriate serialized values.
      for(int arrayIndex=0; arrayIndex<numArrays; ++arrayIndex)
      {
        if(arrays[arrayIndex]->GetInformation()->Get(vtkDistributedGraphHelper::DISTRIBUTEDVERTEXIDS()))
        {
          // TODO: Currently we just assume DistributedIds are vtkIdType
          //       values.  This should be generalized to support other
          //       integer types that might be valid.  (vtkIdType should be
          //       okay for now since the GetVertexOwner() and GetVertexIndex()
          //       methods take a vtkIdType vale. (i.e., TemplateMacro <egad!>)
          vtkIdTypeArray * distributedIdArray = vtkArrayDownCast<vtkIdTypeArray>( arrays[arrayIndex]);
          if(distributedIdArray)
          {
            for(vtkIdType vertIndex=0; vertIndex < myNumVertices; ++vertIndex)
            {
              vtkIdType value   = distributedIdArray->GetValue(vertIndex);
              vtkIdType owner   = helper->GetVertexOwner(value);
              vtkIdType index   = helper->GetVertexIndex(value);
              vtkIdType localId = vertexOffsets[owner]+index;
              myVertexProperties[vertIndex*numArrays+arrayIndex] = localId;
            }
          }
          else
          {
            // Print a warning message if the distributed vertex id array
            // isn't a vtkIdType array.
            vtkStdString arrayName(arrays[arrayIndex]->GetName());
            vtkWarningMacro(<< "Array '" << arrayName.c_str() << "' is flagged "
                            << "as a DISTRIBUTEDVERTEXID array but is not "
                            << "a vtkIdTypeArray." );
          }
        }
      }

      // Communicate this data.
      if (this->ReplicateGraph)
      {
        // Everyone receives all of the vertex properties
        boost::mpi::all_gather(comm, myVertexProperties, allVertexProperties);
      }
      else
      {
        // Only the target processor receives the vertex properties
        boost::mpi::gather(comm, myVertexProperties, allVertexProperties,
                           this->TargetProcessor);
      }

      // Note: local storage for myVertexProperties is deallocated here
    }

    if (this->ReplicateGraph || myRank == this->TargetProcessor)
    {
      // Copy the structure of the vertex data attributes.
      vtkDataSetAttributes *outputVertexData;
      if (isDirected)
      {
        outputVertexData = dirBuilder->GetVertexData();
      }
      else
      {
        outputVertexData = undirBuilder->GetVertexData();
      }
      this->CopyStructureOfDataSetAttributes(distribVertexData,
                                             outputVertexData,
                                             totalNumVertices);

      // Add all of the vertices, in blocks, from rank 0 up to the
      // last processor.
      vtkSmartPointer<vtkVariantArray> propArray
        = vtkSmartPointer<vtkVariantArray>::New();
      for (int origin = 0; origin < numProcs; ++origin)
      {
        for (vtkIdType vertIndex = 0;
             vertIndex < numVerticesPerProcessor[origin];
             ++vertIndex)
        {
          propArray->SetArray
            (&allVertexProperties[origin][vertIndex * numArrays],
             numArrays, /*save=*/1);
          if (isDirected)
          {
            vtkIdType id = dirBuilder->AddVertex(propArray);
          }
          else
          {
            vtkIdType id = undirBuilder->AddVertex(propArray);
          }
        }

        // Clear out the memory storing the serialized form of the
        // vertex properties from this source processor.
        std::vector<vtkVariant>().swap(allVertexProperties[origin]);
      }
    }
  }
  else
  {
    // No need to exchange data: just add the vertices
    if (this->ReplicateGraph || myRank == this->TargetProcessor)
    {
      for (vtkIdType vertIndex = 0; vertIndex < totalNumVertices; ++vertIndex)
      {
        if (isDirected)
        {
          dirBuilder->AddVertex();
        }
        else
        {
          undirBuilder->AddVertex();
        }
      }
    }
  }

  // Collect and add edges to the resulting graph.
  vtkDataSetAttributes *distribEdgeData = input->GetEdgeData();
  if (distribEdgeData->GetNumberOfArrays() > 0 && this->CopyEdgeData)
  {
    typedef boost::mpi::packed_iarchive::buffer_type mpi_buffer_type;
    int numArrays = distribEdgeData->GetNumberOfArrays();

    // Get the arrays we'll be reading from.
    std::vector<vtkAbstractArray*> arrays(numArrays);
    for (int arrayIndex = 0; arrayIndex < numArrays; ++arrayIndex)
    {
      arrays[arrayIndex] = distribEdgeData->GetAbstractArray(arrayIndex);
    }

    // Serialize and communicate the end points and attributes of the
    // edges.
    std::vector<mpi_buffer_type> allEdgesBuffers;

    {
      // Create a buffer into which we'll pack the edge data.
      mpi_buffer_type myEdgesBuffer;
      boost::mpi::packed_oarchive out(comm, myEdgesBuffer);

      // Serialize all of the local edges and their properties.
      vtkIdType myNumEdges = input->GetNumberOfEdges();
      out << myNumEdges;
      vtkSmartPointer<vtkEdgeListIterator> edges
        = vtkSmartPointer<vtkEdgeListIterator>::New();
      input->GetEdges(edges);
      while (edges->HasNext())
      {
        vtkEdgeType edge = edges->Next();

        // Serialize source, target with global IDs
        vtkIdType source
          = vertexOffsets[pbglHelper->GetVertexOwner(edge.Source)]
          + pbglHelper->GetVertexIndex(edge.Source);
        vtkIdType target
          = vertexOffsets[pbglHelper->GetVertexOwner(edge.Target)]
          + pbglHelper->GetVertexIndex(edge.Target);
        out << source << target;

        // Serialize properties
        for (int arrayIndex = 0; arrayIndex < numArrays; ++arrayIndex)
        {
          vtkIdType edgeIndex = pbglHelper->GetEdgeIndex(edge.Id);
          vtkVariant value = arrays[arrayIndex]->GetVariantValue(edgeIndex);
          out << value;
        }
      }

      if (this->ReplicateGraph)
      {
        // Everyone receives all of the edges
        boost::mpi::all_gather(comm, myEdgesBuffer, allEdgesBuffers);
      }
      else
      {
        // Only the target processor receives the vertex properties
        boost::mpi::gather(comm, myEdgesBuffer, allEdgesBuffers,
                           this->TargetProcessor);
      }

      // Note: local storage for myEdgesBuffer is deallocated here.
    }

    vtkIdType totalNumEdges
      = boost::mpi::all_reduce(comm, input->GetNumberOfEdges(),
                               std::plus<vtkIdType>());

    if (this->ReplicateGraph || myRank == this->TargetProcessor)
    {
      // Copy the structure of the edge data attributes.
      vtkDataSetAttributes *outputEdgeData;
      if (isDirected)
      {
        outputEdgeData = dirBuilder->GetEdgeData();
      }
      else
      {
        outputEdgeData = undirBuilder->GetEdgeData();
      }
      this->CopyStructureOfDataSetAttributes(distribEdgeData,
                                             outputEdgeData,
                                             totalNumEdges);

      // Add all of the edges, in blocks, from rank 0 up to the last
      // processor.
      vtkSmartPointer<vtkVariantArray> propArray
        = vtkSmartPointer<vtkVariantArray>::New();
      propArray->SetNumberOfTuples(numArrays);

      for (int origin = 0; origin < numProcs; ++origin)
      {
        // Extract the edges and properties
        boost::mpi::packed_iarchive in(comm, allEdgesBuffers[origin]);
        vtkIdType numEdges;
        in >> numEdges;

        for (vtkIdType e = 0; e < numEdges; ++e)
        {
          // Extract source, target
          vtkIdType source, target;
          in >> source >> target;

          // Extract properties.
          for (int arrayIndex = 0; arrayIndex < numArrays; ++arrayIndex)
          {
            in >> *propArray->GetPointer(arrayIndex);
          }

          // Add the edge.
          if (isDirected)
          {
            vtkEdgeType newEdge = dirBuilder->AddEdge(source, target, &*propArray);
            if(CreateOriginProcessArray)
            {
              EdgeProcessSourceArray->InsertNextValue(origin);
            }
          }
          else
          {
            vtkEdgeType newEdge = undirBuilder->AddEdge(source, target, &*propArray);
            if(CreateOriginProcessArray)
            {
              EdgeProcessSourceArray->InsertNextValue(origin);
            }
          }
        }

        // Clear out the memory storing the serialized form of the
        // edge data from this source processor.
        mpi_buffer_type().swap(allEdgesBuffers[origin]);
      }
    }
  }
  else
  {
    // Serialize and communicate just the endpoints of the edges.
    typedef std::pair<vtkIdType, vtkIdType> vtkIdPair;
    std::vector<std::vector<vtkIdPair> > allEdges;
    {
      std::vector<vtkIdPair> myEdges;
      myEdges.reserve(input->GetNumberOfEdges());

      // Serialize all of the local edges.
      vtkSmartPointer<vtkEdgeListIterator> edges
        = vtkSmartPointer<vtkEdgeListIterator>::New();
      input->GetEdges(edges);
      while (edges->HasNext())
      {
        vtkEdgeType edge = edges->Next();
        myEdges.push_back
          (vtkIdPair
            (vertexOffsets[pbglHelper->GetVertexOwner(edge.Source)]
             + pbglHelper->GetVertexIndex(edge.Source),
             vertexOffsets[pbglHelper->GetVertexOwner(edge.Target)]
             + pbglHelper->GetVertexIndex(edge.Target)));
      }

      if (this->ReplicateGraph)
      {
        // Everyone receives all of the edges
        boost::mpi::all_gather(comm, myEdges, allEdges);
      }
      else
      {
        // Only the target processor receives the vertex properties
        boost::mpi::gather(comm, myEdges, allEdges,
                           this->TargetProcessor);
      }

      // Note: local storage for myEdges is deallocated here.
    }

    if (this->ReplicateGraph || myRank == this->TargetProcessor)
    {
      // Add all of the edges, in blocks, from rank 0 up to the last
      // processor.
      for (int origin = 0; origin < numProcs; ++origin)
      {
        // Add the edges
        for (std::vector<vtkIdPair>::iterator e = allEdges[origin].begin(),
                                             e_end = allEdges[origin].end();
             e != e_end; ++e)
        {
          if (isDirected)
          {
            vtkEdgeType edge = dirBuilder->AddEdge(e->first, e->second);
          }
          else
          {
            vtkEdgeType edge = undirBuilder->AddEdge(e->first, e->second);
          }
        }

        // Clear out the memory storing the serialized form of the
        // edge data from this source processor.
        std::vector<vtkIdPair>().swap(allEdges[origin]);
      }
    }
  }

  // Set the vertex ProcessorID array if requested.
  if(CreateOriginProcessArray)
  {
    VertexProcessSourceArray->SetNumberOfTuples(totalNumVertices);
    for(vtkIdType pid=0; pid<numProcs; pid++)
    {
      for(vtkIdType vid=vertexOffsets[pid]; vid<vertexOffsets[pid+1]; vid++)
      {
        VertexProcessSourceArray->SetValue(vid, pid);
      }
    }
    if(isDirected)
    {
      dirBuilder->GetVertexData()->AddArray(VertexProcessSourceArray);
      dirBuilder->GetEdgeData()->AddArray(EdgeProcessSourceArray);
    }
    else
    {
      undirBuilder->GetVertexData()->AddArray(VertexProcessSourceArray);
      undirBuilder->GetEdgeData()->AddArray(EdgeProcessSourceArray);
    }
  }

  // Copy the structure into the output.
  vtkGraph *output = vtkGraph::GetData(outputVector);
  if (isDirected)
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

  return 1;
}


//----------------------------------------------------------------------------
void vtkPBGLCollectGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TargetProcessor: " << this->TargetProcessor << endl;

  os << indent << "ReplicateGraph: "
     << (this->ReplicateGraph ? "on" : "off") << endl;

  os << indent << "CopyVertexData: "
     << (this->CopyVertexData ? "on" : "off") << endl;

  os << indent << "CopyEdgeData: "
     << (this->CopyEdgeData ? "on" : "off") << endl;

  os << indent << "CreateOriginProcessArray: "
     << (this->CreateOriginProcessArray ? "on" : "off") << endl;

  os << indent << "OriginProcessArrayName: "
     << (this->OriginProcessArrayName ? this->OriginProcessArrayName
                                      : "ProcessorID")
     << endl;
}


//----------------------------------------------------------------------------
int vtkPBGLCollectGraph::FillInputPortInformation(
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
int vtkPBGLCollectGraph::FillOutputPortInformation(
  int port, vtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkGraph");
  }
  return 1;
}


//----------------------------------------------------------------------------
void vtkPBGLCollectGraph::CopyStructureOfDataSetAttributes
       (vtkDataSetAttributes *inAttrs,
        vtkDataSetAttributes *outAttrs,
        vtkIdType numberOfTuples)
{
  int numArrays = inAttrs->GetNumberOfArrays();

  // Build the output arrays.
  outAttrs->AllocateArrays(numArrays);
  for (int arrayIndex = 0; arrayIndex < numArrays; ++arrayIndex)
  {
    // Build an array of the appropriate type.
    vtkAbstractArray *fromArray = inAttrs->GetAbstractArray(arrayIndex);
    vtkAbstractArray *toArray
      = vtkAbstractArray::CreateArray(fromArray->GetDataType());

    // Allocate the array and set its name.
    toArray->SetNumberOfComponents(fromArray->GetNumberOfComponents());
    toArray->SetNumberOfTuples(numberOfTuples);
    toArray->SetName(fromArray->GetName());

    // Add the array to the vertex data of the output graph.
    outAttrs->AddArray(toArray);
    int attribute = inAttrs->IsArrayAnAttribute(arrayIndex);
    if (attribute >= 0)
    {
      outAttrs->SetActiveAttribute(arrayIndex, attribute);
    }
  }
}

#endif //VTK_LEGACY_REMOVE
