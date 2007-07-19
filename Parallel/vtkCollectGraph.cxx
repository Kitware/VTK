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
#include "vtkCollectGraph.h"

#include "vtkCellData.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"

#include <vtksys/stl/map>
#include <vtksys/stl/utility>
#include <vtksys/stl/vector>

using vtksys_stl::map;
using vtksys_stl::pair;
using vtksys_stl::vector;

vtkCxxRevisionMacro(vtkCollectGraph, "1.2");
vtkStandardNewMacro(vtkCollectGraph);

vtkCxxSetObjectMacro(vtkCollectGraph,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkCollectGraph,SocketController, vtkSocketController);

//----------------------------------------------------------------------------
vtkCollectGraph::vtkCollectGraph()
{
  this->PassThrough = 0;
  this->SocketController = NULL;

  // Default vertex id array.
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_NONE, "id");

  // Controller keeps a reference to this object as well.
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());  
}

//----------------------------------------------------------------------------
vtkCollectGraph::~vtkCollectGraph()
{
  this->SetController(0);
  this->SetSocketController(0);
}

//----------------------------------------------------------------------------
int vtkCollectGraph::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  return 1;
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
  
//----------------------------------------------------------------------------
int vtkCollectGraph::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
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
      vtkGraph* table = NULL;;
      table = vtkGraph::New();
      this->SocketController->Receive(table, 1, 121767);
      output->ShallowCopy(table);
      table->Delete();
      table = NULL;
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
    vtkGraph* wholeGraph = vtkGraph::New();
    wholeGraph->SetDirected(input->GetDirected());

    vtkPointData* wholePointData = wholeGraph->GetPointData();
    wholePointData->CopyAllocate(input->GetPointData());

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

    char* idFieldName = ids->GetName();

    // Map from global ids (owner, ownerId pairs) to wholeGraph ids.
    map<int, vtkIdType> globalIdMapInt;
    map<vtkStdString, vtkIdType> globalIdMapStr;

    // Map from curGraph ids to wholeGraph ids.
    vector<vtkIdType> localIdVec;

    for (idx = 0; idx < numProcs; ++idx)
      {
      vtkGraph* curGraph;
      if (idx == 0)
        {
        curGraph = input;
        }
      else
        {
        curGraph = vtkGraph::New();
        this->Controller->Receive(curGraph, idx, 121767);

        // Resize the point data arrays to fit the new data.
        vtkIdType newSize = wholeGraph->GetNumberOfVertices() + curGraph->GetNumberOfVertices();
        for (vtkIdType i = 0; i < wholePointData->GetNumberOfArrays(); i++)
          {
          vtkAbstractArray* arr = wholePointData->GetAbstractArray(i);
          arr->Resize(newSize);
          }
        }

      vtkAbstractArray* idArr = curGraph->GetVertexData()->GetAbstractArray(idFieldName);
      vtkStringArray* idArrStr = vtkStringArray::SafeDownCast(idArr);
      vtkIntArray* idArrInt = vtkIntArray::SafeDownCast(idArr);

      vtkIntArray* ghostLevelsArr = vtkIntArray::SafeDownCast(
        wholePointData->GetAbstractArray("vtkGhostLevels"));

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
          wholeGraph->GetPoints()->InsertNextPoint(pt);
          vtkIdType id = wholeGraph->AddVertex();

          // Cannot use CopyData because the arrays may switch order during network transfer.
          // Instead, look up the array name.  This assumes unique array names.
          //wholePointData->CopyData(curGraph->GetPointData(), v, id);
          for (vtkIdType arrIndex = 0; arrIndex < wholePointData->GetNumberOfArrays(); arrIndex++)
            {
            vtkAbstractArray* arr = wholePointData->GetAbstractArray(arrIndex);
            vtkAbstractArray* curArr = curGraph->GetPointData()->GetAbstractArray(arr->GetName());

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
      vtkIntArray* edgeGhostLevelsArr = vtkIntArray::SafeDownCast(
        curGraph->GetEdgeData()->GetAbstractArray("vtkGhostLevels"));
      vtkIdType numEdges = curGraph->GetNumberOfEdges();
      for (vtkIdType e = 0; e < numEdges; e++)
        {
        if (edgeGhostLevelsArr == NULL || edgeGhostLevelsArr->GetValue(e) == 0)
          {
          vtkIdType source = curGraph->GetSourceVertex(e);
          vtkIdType target = curGraph->GetTargetVertex(e);
          wholeGraph->AddEdge(localIdVec[source], localIdVec[target]);
          }
        }

      if (idx != 0)
        {
        curGraph->Delete();
        }
      }
    wholeGraph->Squeeze();

    if (this->SocketController)
      { // Send collected data onto client.
      this->SocketController->Send(wholeGraph, 1, 121767);
      // output will be empty.
      }
    else
      { // No client. Keep the output here.
      output->ShallowCopy(wholeGraph);
      }
    wholeGraph->Delete();
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
}
