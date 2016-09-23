/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollectGraph.cxx

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
#include "vtkCollectGraph.h"

#include "vtkCellData.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"

#include <map>
#include <utility>
#include <vector>

vtkStandardNewMacro(vtkCollectGraph);

vtkCxxSetObjectMacro(vtkCollectGraph,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkCollectGraph,SocketController, vtkSocketController);

//----------------------------------------------------------------------------
vtkCollectGraph::vtkCollectGraph()
{
  this->PassThrough = 0;
  this->SocketController = NULL;

  // Default vertex id array.
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "id");

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->OutputType = USE_INPUT_TYPE;
}

//----------------------------------------------------------------------------
vtkCollectGraph::~vtkCollectGraph()
{
  this->SetController(0);
  this->SetSocketController(0);
}

//--------------------------------------------------------------------------
int vtkCollectGraph::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
              outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//--------------------------------------------------------------------------
int vtkCollectGraph::RequestDataObject(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (this->OutputType == USE_INPUT_TYPE)
  {
    return Superclass::RequestDataObject(request, inputVector, outputVector);
  }

  vtkGraph *output = 0;
  if (this->OutputType == DIRECTED_OUTPUT)
  {
    output = vtkDirectedGraph::New();
  }
  else if (this->OutputType == UNDIRECTED_OUTPUT)
  {
    output = vtkUndirectedGraph::New();
  }
  else
  {
    vtkErrorMacro(<<"Invalid output type setting.");
    return 0;
  }
  vtkInformation *info = outputVector->GetInformationObject(0);
  info->Set(vtkDataObject::DATA_OBJECT(), output);
  output->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkCollectGraph::RequestData(
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

  int numProcs, myId;
  int idx;

  if (this->Controller == NULL && this->SocketController == NULL)
  { // Running as a single process.
    output->ShallowCopy(input);
    return 1;
  }

  if (this->Controller == NULL && this->SocketController != NULL)
  { // This is a client.  We assume no data on client for input.
    if ( ! this->PassThrough)
    {
      if (this->OutputType != DIRECTED_OUTPUT &&
          this->OutputType != UNDIRECTED_OUTPUT)
      {
        vtkErrorMacro(<<"OutputType must be set to DIRECTED_OUTPUT or UNDIRECTED_OUTPUT on the client.");
        return 0;
      }
      vtkGraph *g = 0;
      if (this->OutputType == DIRECTED_OUTPUT)
      {
        g = vtkDirectedGraph::New();
      }
      else
      {
        g = vtkUndirectedGraph::New();
      }
      this->SocketController->Receive(g, 1, 121767);
      output->ShallowCopy(g);
      g->Delete();
      g = NULL;
      return 1;
    }
    // If not collected, output will be empty from initialization.
    return 0;
  }

  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

  if (this->PassThrough)
  {
    // Just copy and return (no collection).
    output->ShallowCopy(input);
    return 1;
  }

  // Collect.
  if (myId == 0)
  {
    vtkSmartPointer<vtkMutableDirectedGraph> dirBuilder =
      vtkSmartPointer<vtkMutableDirectedGraph>::New();
    vtkSmartPointer<vtkMutableUndirectedGraph> undirBuilder =
      vtkSmartPointer<vtkMutableUndirectedGraph>::New();

    bool directed = (vtkDirectedGraph::SafeDownCast(input) != 0);

    vtkGraph *builder = 0;
    if (directed)
    {
      builder = dirBuilder;
    }
    else
    {
      builder = undirBuilder;
    }

    vtkDataSetAttributes *wholePointData = builder->GetVertexData();
    vtkPoints *wholePoints = builder->GetPoints();
    wholePointData->CopyAllocate(input->GetVertexData());

    // Get the name of the ID array.
    vtkAbstractArray* ids = this->GetInputAbstractArrayToProcess(0, inputVector);

    if (ids == NULL)
    {
      vtkErrorMacro("The ID array is undefined.");
      return 0;
    }

    if (!ids->IsA("vtkIntArray") && !ids->IsA("vtkStringArray"))
    {
      vtkErrorMacro("The ID array must be an integer or string array but is a " << ids->GetClassName());
      return 0;
    }

    char *idFieldName = ids->GetName();

    // Map from global ids (owner, ownerId pairs) to wholeGraph ids.
    std::map<int, vtkIdType> globalIdMapInt;
    std::map<vtkStdString, vtkIdType> globalIdMapStr;

    // Map from curGraph ids to wholeGraph ids.
    std::vector<vtkIdType> localIdVec;

    // Edge iterator.
    vtkSmartPointer<vtkEdgeListIterator> edges =
      vtkSmartPointer<vtkEdgeListIterator>::New();

    for (idx = 0; idx < numProcs; ++idx)
    {
      vtkGraph* curGraph;
      if (idx == 0)
      {
        curGraph = input;
      }
      else
      {
        if (directed)
        {
          curGraph = vtkDirectedGraph::New();
        }
        else
        {
          curGraph = vtkUndirectedGraph::New();
        }
        this->Controller->Receive(curGraph, idx, 121767);

        // Resize the point data arrays to fit the new data.
        vtkIdType numVertices = directed ? dirBuilder->GetNumberOfVertices() : undirBuilder->GetNumberOfVertices();
        vtkIdType newSize = numVertices + curGraph->GetNumberOfVertices();
        for (vtkIdType i = 0; i < wholePointData->GetNumberOfArrays(); i++)
        {
          vtkAbstractArray *arr = wholePointData->GetAbstractArray(i);
          arr->Resize(newSize);
        }
      }

      vtkAbstractArray *idArr = curGraph->GetVertexData()->GetAbstractArray(idFieldName);
      vtkStringArray *idArrStr = vtkArrayDownCast<vtkStringArray>(idArr);
      vtkIntArray *idArrInt = vtkArrayDownCast<vtkIntArray>(idArr);

      vtkIntArray *ghostLevelsArr = vtkArrayDownCast<vtkIntArray>(
        wholePointData->GetAbstractArray(vtkDataSetAttributes::GhostArrayName()));

      // Add new vertices
      localIdVec.clear();
      vtkIdType numVerts = curGraph->GetNumberOfVertices();
      for (vtkIdType v = 0; v < numVerts; v++)
      {
        vtkStdString globalIdStr = idArrStr ? idArrStr->GetValue(v) : vtkStdString("");
        int globalIdInt = idArrInt ? idArrInt->GetValue(v) : 0;

        double pt[3];
        if ((idArrInt && globalIdMapInt.count(globalIdInt) == 0)
          || (idArrStr && globalIdMapStr.count(globalIdStr) == 0))
        {
          curGraph->GetPoint(v, pt);
          wholePoints->InsertNextPoint(pt);
          vtkIdType id = -1;
          if (directed)
          {
            id = dirBuilder->AddVertex();
          }
          else
          {
            id = undirBuilder->AddVertex();
          }

          // Cannot use CopyData because the arrays may switch order during network transfer.
          // Instead, look up the array name.  This assumes unique array names.
          //wholePointData->CopyData(curGraph->GetPointData(), v, id);
          for (vtkIdType arrIndex = 0; arrIndex < wholePointData->GetNumberOfArrays(); arrIndex++)
          {
            vtkAbstractArray* arr = wholePointData->GetAbstractArray(arrIndex);
            vtkAbstractArray* curArr = curGraph->GetVertexData()->GetAbstractArray(arr->GetName());

            // Always set the ghost levels array to zero.
            if (arr == ghostLevelsArr)
            {
              ghostLevelsArr->InsertNextValue(0);
            }
            else
            {
              arr->InsertNextTuple(v, curArr);
            }
          }

          if (idArrInt)
          {
            globalIdMapInt[globalIdInt] = id;
          }
          else
          {
            globalIdMapStr[globalIdStr] = id;
          }
          localIdVec.push_back(id);
        }
        else
        {
          if (idArrInt)
          {
            localIdVec.push_back(globalIdMapInt[globalIdInt]);
          }
          else
          {
            localIdVec.push_back(globalIdMapStr[globalIdStr]);
          }
        }
      }

      // Add non-ghost edges
      vtkIntArray* edgeGhostLevelsArr = vtkArrayDownCast<vtkIntArray>(
        curGraph->GetEdgeData()->GetAbstractArray(vtkDataSetAttributes::GhostArrayName()));
      curGraph->GetEdges(edges);
      while (edges->HasNext())
      {
        vtkEdgeType e = edges->Next();
        if (edgeGhostLevelsArr == NULL || edgeGhostLevelsArr->GetValue(e.Id) == 0)
        {
          if (directed)
          {
            dirBuilder->AddEdge(localIdVec[e.Source], localIdVec[e.Target]);
          }
          else
          {
            undirBuilder->AddEdge(localIdVec[e.Source], localIdVec[e.Target]);
          }
        }
      }

      if (idx != 0)
      {
        curGraph->Delete();
      }
    }
    undirBuilder->Squeeze();
    dirBuilder->Squeeze();

    if (this->SocketController)
    { // Send collected data onto client.
      this->SocketController->Send(builder, 1, 121767);
      // output will be empty.
    }
    else
    { // No client. Keep the output here.
      output->ShallowCopy(builder);
    }
  }
  else
  {
    this->Controller->Send(input, 0, 121767);
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkCollectGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PassThough: " << this->PassThrough << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketController: (" << this->SocketController << ")\n";
  os << indent << "OutputType: " << this->OutputType << endl;
}
