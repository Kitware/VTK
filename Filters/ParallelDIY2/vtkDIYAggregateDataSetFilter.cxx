/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYAggregateDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "vtkDIYAggregateDataSetFilter.h"

#include "vtkCellData.h"
#include "vtkExtentTranslator.h"
#include "vtkExtractGrid.h"
#include "vtkExtractRectilinearGrid.h"
#include "vtkExtractVOI.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPI.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLStructuredGridWriter.h"

#include <map>
#include <set>

// clang-format off
#include "vtk_diy2.h" // must include this before any diy header
#include VTK_DIY2(diy/assigner.hpp)
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/reduce.hpp)
#include VTK_DIY2(diy/partners/swap.hpp)
#include VTK_DIY2(diy/decomposition.hpp)
// clang-format on

vtkStandardNewMacro(vtkDIYAggregateDataSetFilter);

namespace
{
//----------------------------------------------------------------------------
inline diy::mpi::communicator GetDiyCommunicator(vtkMPIController* controller)
{
  vtkMPICommunicator* vtkcomm = vtkMPICommunicator::SafeDownCast(controller->GetCommunicator());
  return diy::mpi::communicator(*vtkcomm->GetMPIComm()->GetHandle());
}

struct Block
{ // if we got more sophisticated with our use of DIY we'd take advantage of this
  // Block struct but for now we just leave it in as is.
  // The full input to the filter
  // vtkSmartPointer<vtkDataSet> OriginalPiece;
  // If we have non-empty output, these are the pieces that will be combined into FinalPiece
  // std::vector<vtkSmartPointer<vtkDataSet> > ReceivedPieces;
  // The full output from the filter
  vtkSmartPointer<vtkDataSet> FinalPiece;
};
}

//-----------------------------------------------------------------------------
vtkDIYAggregateDataSetFilter::vtkDIYAggregateDataSetFilter()
{
  this->OutputInitialized = false;
}

//-----------------------------------------------------------------------------
vtkDIYAggregateDataSetFilter::~vtkDIYAggregateDataSetFilter() = default;

//-----------------------------------------------------------------------------
int vtkDIYAggregateDataSetFilter::RequestInformation(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  if (inputInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
  {
    int wholeExtent[6];
    inputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    // Overwrite the whole extent if there's an input whole extent is set. This is needed
    // for distributed structured data.
    outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);
  }

  // We assume that whoever sets up the input handles  partitioned data properly.
  // For structured data, this means setting up WHOLE_EXTENT as above. For
  // unstructured data, nothing special is required
  outputInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkDIYAggregateDataSetFilter::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = nullptr;
  vtkDataSet* output = vtkDataSet::GetData(outputVector, 0);

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    input = vtkDataSet::GetData(inputVector[0], 0);
  }

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  int numberOfProcesses = controller->GetNumberOfProcesses();
  int myRank = controller->GetLocalProcessId();
  if (numberOfProcesses == this->NumberOfTargetProcesses)
  {
    if (input)
    {
      output->ShallowCopy(input);
    }
    return 1;
  }

  if (input->IsA("vtkUnstructuredGrid") || input->IsA("vtkPolyData"))
  {
    // the superclass handles unstructured grids and polydata
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }
  // mark that the output grid hasn't been touched yet
  this->OutputInitialized = false;
  // DIY bounds are really just based on extents
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  int wholeExtent[6] = { 0, -1, 0, -1, 0, -1 }; // empty by default
  outputInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  int outputExtent[6] = { 0, -1, 0, -1, 0, -1 }; // empty by default

  vtkNew<vtkExtentTranslator> extentTranslator;
  int targetProcessId = this->GetTargetProcessId(myRank, numberOfProcesses);
  if (targetProcessId != -1)
  {
    extentTranslator->PieceToExtentThreadSafe(targetProcessId, this->GetNumberOfTargetProcesses(),
      0, wholeExtent, outputExtent, vtkExtentTranslator::BLOCK_MODE, 0);
  }

  if (vtkImageData* idOutput = vtkImageData::SafeDownCast(output))
  {
    idOutput->SetExtent(outputExtent);
  }
  else if (vtkRectilinearGrid* rgOutput = vtkRectilinearGrid::SafeDownCast(output))
  {
    rgOutput->SetExtent(outputExtent);
  }
  else if (vtkStructuredGrid* sgOutput = vtkStructuredGrid::SafeDownCast(output))
  {
    sgOutput->SetExtent(outputExtent);
  }

  // map from the process rank to the serialized datasets that we'll be sending out via DIY
  std::map<int, std::string> serializedDataSets;
  int dimensions[3] = { 0, 0, 0 };
  for (int i = 0; i < 3; i++)
  {
    if (wholeExtent[2 * i] < wholeExtent[2 * i + 1])
    {
      dimensions[i] = 1;
    }
  }
  int inputExtent[6];
  this->GetExtent(input, inputExtent);
  for (int proc = 0; proc < numberOfProcesses; proc++)
  {
    targetProcessId = this->GetTargetProcessId(proc, numberOfProcesses);
    if (targetProcessId != -1)
    {
      int targetProcessOutputExtent[6];
      extentTranslator->PieceToExtentThreadSafe(targetProcessId, this->GetNumberOfTargetProcesses(),
        0, wholeExtent, targetProcessOutputExtent, vtkExtentTranslator::BLOCK_MODE, 0);
      int overlappingExtent[6];
      if (this->DoExtentsOverlap(
            inputExtent, targetProcessOutputExtent, dimensions, overlappingExtent))
      {
        if (output->IsA("vtkImageData"))
        {
          vtkNew<vtkExtractVOI> imageDataExtractVOI;
          imageDataExtractVOI->SetVOI(overlappingExtent);
          imageDataExtractVOI->SetInputDataObject(input);
          if (proc == myRank)
          {
            imageDataExtractVOI->Update();
            this->ExtractDataSetInformation(imageDataExtractVOI->GetOutput(), output);
          }
          else
          {
            vtkNew<vtkXMLImageDataWriter> writer;
            writer->SetInputConnection(imageDataExtractVOI->GetOutputPort());
            writer->WriteToOutputStringOn();
            writer->Write();
            serializedDataSets[proc] = writer->GetOutputString();
          }
        }
        else if (output->IsA("vtkRectilinearGrid"))
        {
          vtkNew<vtkExtractRectilinearGrid> rectilinearGridExtractVOI;
          rectilinearGridExtractVOI->SetVOI(overlappingExtent);
          rectilinearGridExtractVOI->SetInputDataObject(input);
          if (proc == myRank)
          {
            rectilinearGridExtractVOI->Update();
            this->ExtractDataSetInformation(rectilinearGridExtractVOI->GetOutput(), output);
          }
          else
          {
            vtkNew<vtkXMLRectilinearGridWriter> writer;
            writer->SetInputConnection(rectilinearGridExtractVOI->GetOutputPort());
            writer->WriteToOutputStringOn();
            writer->Write();
            serializedDataSets[proc] = writer->GetOutputString();
          }
        }
        else if (output->IsA("vtkStructuredGrid"))
        {
          vtkNew<vtkExtractGrid> structuredGridExtractVOI;
          structuredGridExtractVOI->SetVOI(overlappingExtent);
          structuredGridExtractVOI->SetInputDataObject(input);
          if (proc == myRank)
          {
            structuredGridExtractVOI->Update();
            this->ExtractDataSetInformation(structuredGridExtractVOI->GetOutput(), output);
          }
          else
          {
            vtkNew<vtkXMLStructuredGridWriter> writer;
            writer->SetInputConnection(structuredGridExtractVOI->GetOutputPort());
            writer->WriteToOutputStringOn();
            writer->Write();
            serializedDataSets[proc] = writer->GetOutputString();
          }
        }
      }
    }
  }
  std::vector<std::string> receivedDataSets;
  int retVal =
    this->MoveData(inputExtent, wholeExtent, outputExtent, serializedDataSets, receivedDataSets);
  // if we want to try using DIY to move the data we would just use the line below. when it was
  // tested
  // before there was an issue with the serialized imagedata string with DIY so due to time
  // constraints
  // we did an implementation with just using direct MPI data movement instead and left the DIY
  // around
  // in case others wanted to try using that without starting from scratch.
  // int retVal = this->MoveDataWithDIY(inputExtent, wholeExtent, outputExtent, serializedDataSets,
  // output);

  for (auto it : receivedDataSets)
  {
    vtkSmartPointer<vtkDataSet> tempDataSet = nullptr;

    if (output->IsA("vtkImageData"))
    {
      vtkNew<vtkXMLImageDataReader> reader;
      reader->ReadFromInputStringOn();
      reader->SetInputString(it);
      reader->Update();
      tempDataSet = reader->GetOutput();
    }
    else if (output->IsA("vtkRectilinearGrid"))
    {
      vtkNew<vtkXMLRectilinearGridReader> reader;
      reader->ReadFromInputStringOn();
      reader->SetInputString(it);
      reader->Update();
      tempDataSet = reader->GetOutput();
    }
    else if (output->IsA("vtkStructuredGrid"))
    {
      vtkNew<vtkXMLStructuredGridReader> reader;
      reader->ReadFromInputStringOn();
      reader->SetInputString(it);
      reader->Update();
      tempDataSet = reader->GetOutput();
    }
    else
    {
      vtkErrorMacro("Cannot handle dataset type " << output->GetClassName());
      return 0;
    }

    this->ExtractDataSetInformation(tempDataSet, output);
  }

  return retVal;
}

//-----------------------------------------------------------------------------
int vtkDIYAggregateDataSetFilter::MoveDataWithDIY(int inputExtent[6], int wholeExtent[6],
  int outputExtent[6], std::map<int, std::string>& serializedDataSets,
  std::vector<std::string>& receivedDataSets)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  int myRank = controller->GetLocalProcessId();
  vtkNew<vtkIdList> processesIReceiveFrom;
  this->ComputeProcessesIReceiveFrom(inputExtent, wholeExtent, outputExtent, processesIReceiveFrom);

  diy::mpi::communicator comm = GetDiyCommunicator(vtkMPIController::SafeDownCast(controller));

  diy::Master master(comm, 1);
  diy::RoundRobinAssigner assigner(comm.size(), comm.size());

  Block block;

  diy::Link* link = new diy::Link; // master will delete this automatically

  for (auto it : serializedDataSets)
  { // processes I send data to
    diy::BlockID neighbor;
    neighbor.gid = it.first;
    neighbor.proc = assigner.rank(neighbor.gid);
    link->add_neighbor(neighbor);
  }
  for (vtkIdType i = 0; i < processesIReceiveFrom->GetNumberOfIds(); i++)
  { // processes I receive data from
    diy::BlockID neighbor;
    neighbor.gid = processesIReceiveFrom->GetId(i);
    neighbor.proc = assigner.rank(neighbor.gid);
    link->add_neighbor(neighbor);
  }

  master.add(myRank, &block, link);

  int counter = 0;
  for (auto it : serializedDataSets)
  { // processes I send data to
    master.proxy(0).enqueue(master.proxy(0).link()->target(counter), it.second);
    counter++;
  }
  master.exchange(); // does the communication

  const diy::Master::ProxyWithLink proxyWithLink = master.proxy(0);

  std::vector<int> in;
  proxyWithLink.incoming(in);
  receivedDataSets.clear();
  for (size_t i = 0; i < in.size(); ++i)
  {
    if (proxyWithLink.incoming(in[i]))
    {
      receivedDataSets.push_back("");
      proxyWithLink.dequeue(in[i], receivedDataSets.back());
      // now deserialize...
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkDIYAggregateDataSetFilter::MoveData(int inputExtent[6], int wholeExtent[6],
  int outputExtent[6], std::map<int, std::string>& serializedDataSets,
  std::vector<std::string>& receivedDataSets)
{
  vtkMPIController* controller =
    vtkMPIController::SafeDownCast(vtkMultiProcessController::GetGlobalController());

  vtkNew<vtkIdList> processesIReceiveFrom;
  this->ComputeProcessesIReceiveFrom(inputExtent, wholeExtent, outputExtent, processesIReceiveFrom);

  // map to keep track of the size of data I receive from each process
  std::vector<int> receiveSizes(processesIReceiveFrom->GetNumberOfIds(), 0);
  std::vector<vtkMPICommunicator::Request> sizeReceiveRequests(
    processesIReceiveFrom->GetNumberOfIds());
  for (vtkIdType i = 0; i < processesIReceiveFrom->GetNumberOfIds(); i++)
  {
    controller->NoBlockReceive(
      receiveSizes.data() + i, 1, processesIReceiveFrom->GetId(i), 9318, sizeReceiveRequests[i]);
  }

  std::vector<vtkMPICommunicator::Request> sizeSendRequests(serializedDataSets.size());
  int counter = 0;
  for (auto it : serializedDataSets)
  {
    int size = static_cast<int>(it.second.size());
    controller->NoBlockSend(&size, 1, it.first, 9318, sizeSendRequests[counter]);
    counter++;
  }

  controller->WaitAll(static_cast<int>(sizeReceiveRequests.size()), sizeReceiveRequests.data());
  std::vector<vtkMPICommunicator::Request> dataReceiveRequests(
    processesIReceiveFrom->GetNumberOfIds());
  std::vector<unsigned char*> dataArrays;
  for (vtkIdType i = 0; i < processesIReceiveFrom->GetNumberOfIds(); i++)
  {
    int size = receiveSizes[i];
    dataArrays.push_back(new unsigned char[size + 1]);
    dataArrays.back()[size] = '\0';
    controller->NoBlockReceive(
      dataArrays.back(), size, processesIReceiveFrom->GetId(i), 9319, dataReceiveRequests[i]);
  }

  std::vector<vtkMPICommunicator::Request> dataSendRequests(serializedDataSets.size());
  counter = 0;
  // deal with problems with not being able to get direct access to the string's memory.
  // in the future we may want to look at ways to make this more memory efficient. for
  // now it's not too bad though in that it really only has 2 copies of the sent data
  // to a single process since it clears out the string after it copies it over
  // to sendData.
  std::vector<std::vector<unsigned char> > sendData(serializedDataSets.size());
  for (auto it : serializedDataSets)
  {
    int size = static_cast<int>(it.second.size());
    sendData[counter].resize(size);
    for (int i = 0; i < size; i++)
    {
      sendData[counter][i] = static_cast<unsigned char>(it.second[i]);
    }
    it.second.clear(); // clear out the data
    controller->NoBlockSend(
      sendData[counter].data(), size, it.first, 9319, dataSendRequests[counter]);
    counter++;
  }
  controller->WaitAll(static_cast<int>(dataReceiveRequests.size()), dataReceiveRequests.data());

  receivedDataSets.resize(processesIReceiveFrom->GetNumberOfIds());
  for (vtkIdType i = 0; i < processesIReceiveFrom->GetNumberOfIds(); i++)
  {
    receivedDataSets[i].assign(reinterpret_cast<const char*>(dataArrays[i]), receiveSizes[i]);
    if (receivedDataSets[i].size() != static_cast<size_t>(receiveSizes[i]))
    {
      vtkErrorMacro("Problem deserializing dataset onto target process. Data from "
        << processesIReceiveFrom->GetId(i) << " should be size " << receiveSizes[i]
        << " but is size " << receivedDataSets[i].size());
      return 0;
    }
    delete[] dataArrays[i];
  }

  // wait on messages to make sure that we don't interfere with any future use of this filter
  controller->WaitAll(static_cast<int>(sizeSendRequests.size()), sizeSendRequests.data());
  controller->WaitAll(static_cast<int>(dataSendRequests.size()), dataSendRequests.data());

  return 1;
}

//-----------------------------------------------------------------------------
void vtkDIYAggregateDataSetFilter::ComputeProcessesIReceiveFrom(
  int inputExtent[6], int wholeExtent[6], int outputExtent[6], vtkIdList* processesIReceiveFrom)
{
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  int myRank = controller->GetLocalProcessId();
  int numberOfProcesses = controller->GetNumberOfProcesses();
  int dimensions[3] = { 0, 0, 0 };
  processesIReceiveFrom->SetNumberOfIds(0);
  for (int i = 0; i < 3; i++)
  {
    if (wholeExtent[2 * i] < wholeExtent[2 * i + 1])
    {
      dimensions[i] = 1;
    }
  }
  // share the input extents so that we can figure out who we receive from
  int tempInputExtent[6];
  std::copy(inputExtent, inputExtent + 6, tempInputExtent);
  std::vector<int> inputExtentsGlobal(6 * numberOfProcesses);
  controller->AllGather(tempInputExtent, inputExtentsGlobal.data(), 6);
  if (this->GetTargetProcessId(myRank, numberOfProcesses) != -1)
  {
    for (int proc = 0; proc < numberOfProcesses; proc++)
    {
      if (proc != myRank &&
        this->DoExtentsOverlap(outputExtent, &inputExtentsGlobal[proc * 6], dimensions, nullptr))
      {
        processesIReceiveFrom->InsertNextId(proc);
      }
    }
  }
}

//-----------------------------------------------------------------------------
int vtkDIYAggregateDataSetFilter::GetTargetProcessId(int sourceProcessId, int numberOfProcesses)
{
  if (this->GetNumberOfTargetProcesses() == 1)
  {
    return (sourceProcessId == 0 ? 0 : -1);
  }
  int spacing = numberOfProcesses / this->GetNumberOfTargetProcesses();
  if ((sourceProcessId + 1) % spacing == 0)
  {
    return sourceProcessId / spacing;
  }
  return -1;
}

//-----------------------------------------------------------------------------
bool vtkDIYAggregateDataSetFilter::DoExtentsOverlap(
  int extent1[6], int extent2[6], int dimensions[3], int* overlappingExtent)
{
  for (int i = 0; i < 3; i++)
  {
    if (dimensions[i] != 0)
    {
      if (extent1[2 * i] >= extent2[2 * i + 1] || extent1[2 * i + 1] <= extent2[2 * i])
      {
        return false;
      }
    }
    if (overlappingExtent)
    {
      overlappingExtent[2 * i] = std::max(extent1[2 * i], extent2[2 * i]);
      overlappingExtent[2 * i + 1] = std::min(extent1[2 * i + 1], extent2[2 * i + 1]);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
void vtkDIYAggregateDataSetFilter::GetExtent(vtkDataSet* dataSet, int extent[6])
{
  if (vtkImageData* id = vtkImageData::SafeDownCast(dataSet))
  {
    id->GetExtent(extent);
  }
  else if (vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(dataSet))
  {
    rg->GetExtent(extent);
  }
  else if (vtkStructuredGrid* sg = vtkStructuredGrid::SafeDownCast(dataSet))
  {
    sg->GetExtent(extent);
  }
  else
  {
    vtkErrorMacro("Unknown grid type " << dataSet->GetClassName());
  }
}

//-----------------------------------------------------------------------------
void vtkDIYAggregateDataSetFilter::ExtractDataSetInformation(vtkDataSet* source, vtkDataSet* target)
{
  if (!source)
  {
    return;
  }
  int sourceExtent[6], targetExtent[6];
  this->GetExtent(source, sourceExtent);
  this->GetExtent(target, targetExtent);
  if (!this->OutputInitialized)
  {
    target->GetFieldData()->ShallowCopy(source->GetFieldData());
    if (vtkImageData* idSource = vtkImageData::SafeDownCast(source))
    {
      vtkImageData* idTarget = vtkImageData::SafeDownCast(target);
      idTarget->SetOrigin(idSource->GetOrigin());
      idTarget->SetSpacing(idSource->GetSpacing());
    }
    else if (vtkRectilinearGrid* rgSource = vtkRectilinearGrid::SafeDownCast(source))
    {
      vtkRectilinearGrid* rgTarget = vtkRectilinearGrid::SafeDownCast(target);
      vtkDataArray* xCoordinates = rgSource->GetXCoordinates()->NewInstance();
      rgTarget->SetXCoordinates(xCoordinates);
      xCoordinates->SetNumberOfTuples(targetExtent[1] - targetExtent[0] + 1);
      xCoordinates->FastDelete();
      vtkDataArray* yCoordinates = rgSource->GetYCoordinates()->NewInstance();
      rgTarget->SetYCoordinates(yCoordinates);
      yCoordinates->SetNumberOfTuples(targetExtent[3] - targetExtent[2] + 1);
      yCoordinates->FastDelete();
      vtkDataArray* zCoordinates = rgSource->GetZCoordinates()->NewInstance();
      rgTarget->SetZCoordinates(zCoordinates);
      zCoordinates->SetNumberOfTuples(targetExtent[5] - targetExtent[4] + 1);
      zCoordinates->FastDelete();
    }
    else if (vtkStructuredGrid* sgSource = vtkStructuredGrid::SafeDownCast(source))
    {
      vtkStructuredGrid* sgTarget = vtkStructuredGrid::SafeDownCast(target);
      vtkNew<vtkPoints> points;
      points->SetDataType(sgSource->GetPoints()->GetDataType());
      points->SetNumberOfPoints(vtkStructuredData::GetNumberOfPoints(targetExtent));
      sgTarget->SetPoints(points);
    }
    else
    {
      vtkErrorMacro("Unknown dataset type " << source->GetClassName());
      return;
    }
  }

  if (vtkRectilinearGrid* rgSource = vtkRectilinearGrid::SafeDownCast(source))
  {
    vtkRectilinearGrid* rgTarget = vtkRectilinearGrid::SafeDownCast(target);
    vtkDataArray* sourceXCoordinates = rgSource->GetXCoordinates();
    vtkDataArray* targetXCoordinates = rgTarget->GetXCoordinates();
    this->ExtractRectilinearGridCoordinates(
      sourceExtent, targetExtent, sourceXCoordinates, targetXCoordinates);
    vtkDataArray* sourceYCoordinates = rgSource->GetYCoordinates();
    vtkDataArray* targetYCoordinates = rgTarget->GetYCoordinates();
    this->ExtractRectilinearGridCoordinates(
      sourceExtent + 2, targetExtent + 2, sourceYCoordinates, targetYCoordinates);
    vtkDataArray* sourceZCoordinates = rgSource->GetZCoordinates();
    vtkDataArray* targetZCoordinates = rgTarget->GetZCoordinates();
    this->ExtractRectilinearGridCoordinates(
      sourceExtent + 4, targetExtent + 4, sourceZCoordinates, targetZCoordinates);
  }
  else if (vtkStructuredGrid* sgSource = vtkStructuredGrid::SafeDownCast(source))
  {
    vtkStructuredGrid* sgTarget = vtkStructuredGrid::SafeDownCast(target);
    vtkPoints* sourcePoints = sgSource->GetPoints();
    vtkPoints* targetPoints = sgTarget->GetPoints();
    for (int k = sourceExtent[4]; k <= sourceExtent[5]; k++)
    {
      if (targetExtent[4] <= k && k <= targetExtent[5])
      {
        for (int j = sourceExtent[2]; j <= sourceExtent[3]; j++)
        {
          if (targetExtent[2] <= j + sourceExtent[2] && j <= targetExtent[3])
          {
            for (int i = sourceExtent[0]; i <= sourceExtent[1]; i++)
            {
              if (targetExtent[0] <= i && i <= targetExtent[1])
              {
                int ijk[3] = { i, j, k };
                vtkIdType sourcePointId =
                  vtkStructuredData::ComputePointIdForExtent(sourceExtent, ijk);
                vtkIdType targetPointId =
                  vtkStructuredData::ComputePointIdForExtent(targetExtent, ijk);
                double coord[3];
                sourcePoints->GetPoint(sourcePointId, coord);
                targetPoints->SetPoint(targetPointId, coord);
              }
            }
          }
        }
      }
    }
  }

  if (this->OutputInitialized)
  {
    target->GetPointData()->SetupForCopy(source->GetPointData());
    target->GetCellData()->SetupForCopy(source->GetCellData());
  }
  else
  {
    target->GetPointData()->CopyAllocate(source->GetPointData());
    target->GetCellData()->CopyAllocate(source->GetCellData());
  }
  target->GetPointData()->CopyStructuredData(
    source->GetPointData(), sourceExtent, targetExtent, !this->OutputInitialized);
  sourceExtent[1]--;
  sourceExtent[3]--;
  sourceExtent[5]--;
  targetExtent[1]--;
  targetExtent[3]--;
  targetExtent[5]--;
  target->GetCellData()->CopyStructuredData(
    source->GetCellData(), sourceExtent, targetExtent, !this->OutputInitialized);

  this->OutputInitialized = true;
}

//-----------------------------------------------------------------------------
void vtkDIYAggregateDataSetFilter::ExtractRectilinearGridCoordinates(int* sourceExtent,
  int* targetExtent, vtkDataArray* sourceCoordinates, vtkDataArray* targetCoordinates)
{
  for (int i = sourceExtent[0]; i <= sourceExtent[1]; i++)
  {
    if (targetExtent[0] <= i && i <= targetExtent[1])
    {
      targetCoordinates->SetTuple1(
        i - targetExtent[0], sourceCoordinates->GetTuple1(i - sourceExtent[0]));
    }
  }
}

//-----------------------------------------------------------------------------
void vtkDIYAggregateDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OutputInitialized: " << this->OutputInitialized << endl;
}
