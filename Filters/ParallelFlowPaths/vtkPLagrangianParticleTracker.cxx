/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLagrangianParticleTracker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPLagrangianParticleTracker.h"

#include "vtkAppendFilter.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLagrangianBasicIntegrationModel.h"
#include "vtkLagrangianParticle.h"
#include "vtkLagrangianThreadedData.h"
#include "vtkLongLongArray.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"

#include <vector>

#define LAGRANGIAN_PARTICLE_TAG 621
#define LAGRANGIAN_RANG_FLAG_TAG 622
#define LAGRANGIAN_ARRAY_TAG 623
#define LAGRANGIAN_PARTICLE_ID_TAG 624
#define LAGRANGIAN_PARTICLE_CONTROL_TAG 625

// Class used to serialize and stream a particle
class MessageStream
{
public:
  MessageStream(int BufferSize)
    : Size(BufferSize)
  {
    this->Data.resize(Size);
    this->Head = Data.data();
    this->count = 0;
  }

  ~MessageStream() = default;

  int GetSize() { return this->Size; }

  template <class T>
  MessageStream& operator<<(T t)
  {
    size_t size = sizeof(T);
    char* value = reinterpret_cast<char*>(&t);
    for (size_t i = 0; i < size; i++)
    {
      *this->Head++ = *value++;
    }
    return *this;
  }

  template <class T>
  MessageStream& operator>>(T& t)
  {
    size_t size = sizeof(T);
    t = *reinterpret_cast<T*>(this->Head);
    this->Head += size;
    return *this;
  }

  char* GetRawData() { return this->Data.data(); }
  int GetLength() { return this->Head - this->Data.data(); }

  void Reset() { this->Head = this->Data.data(); }
  int count;

private:
  MessageStream(const MessageStream&) = delete;
  void operator=(const MessageStream&) = delete;

  std::vector<char> Data;
  char* Head;
  int Size;
};

// A singleton class used by each rank to stream particle with another
// It sends particle to all other ranks and can receive particle
// from any other rank.
class ParticleStreamManager
{
public:
  ParticleStreamManager(vtkMPIController* controller, vtkPointData* seedData,
    vtkLagrangianBasicIntegrationModel* model, const vtkBoundingBox* bounds)
  {
    // Initialize Members
    this->Controller = controller;
    this->SeedData = seedData;

    // Gather bounds and initialize requests
    std::vector<double> allBounds(6 * this->Controller->GetNumberOfProcesses(), 0);
    double nodeBounds[6];
    bounds->GetBounds(nodeBounds);
    this->Controller->AllGather(nodeBounds, &allBounds[0], 6);
    for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
    {
      vtkBoundingBox box;
      box.AddBounds(&allBounds[i * 6]);
      this->Boxes.push_back(box);
    }

    // Compute StreamSize for one particle
    // This is strongly linked to Send and Receive code
    this->StreamSize = 2 * sizeof(int) + 2 * sizeof(double) + 4 * sizeof(vtkIdType) + sizeof(int) +
      2 * sizeof(bool) +
      3 * (model->GetNumberOfIndependentVariables() + model->GetNumberOfTrackedUserData()) *
        sizeof(double);
    for (int i = 0; i < seedData->GetNumberOfArrays(); i++)
    {
      vtkDataArray* array = seedData->GetArray(i);
      this->StreamSize += array->GetNumberOfComponents() * sizeof(double);
    }

    // Initialize Streams
    this->ReceiveStream = new MessageStream(this->StreamSize);

    this->SendCounter = 0;
  }

  ~ParticleStreamManager()
  {
    for (size_t i = 0; i < this->SendRequests.size(); i++)
    {
      this->SendRequests[i].first->Wait();
    }
    this->CleanSendRequests();

    // Delete  receive stream
    delete this->ReceiveStream;
  }

  // Method to send a particle to others ranks
  // if particle contained in bounds
  void SendParticle(vtkLagrangianParticle* particle)
  {
    // Serialize particle
    // This is strongly linked to Constructor and Receive code

    MessageStream* sendStream = new MessageStream(this->StreamSize);
    *sendStream << particle->GetSeedId();
    *sendStream << particle->GetId();
    *sendStream << particle->GetParentId();
    *sendStream << particle->GetNumberOfVariables();
    *sendStream << static_cast<int>(particle->GetTrackedUserData().size());
    *sendStream << particle->GetNumberOfSteps();
    *sendStream << particle->GetIntegrationTime();
    *sendStream << particle->GetPrevIntegrationTime();
    *sendStream << particle->GetUserFlag();
    *sendStream << particle->GetPInsertPreviousPosition();
    *sendStream << particle->GetPManualShift();

    double* prev = particle->GetPrevEquationVariables();
    double* curr = particle->GetEquationVariables();
    double* next = particle->GetNextEquationVariables();
    for (int i = 0; i < particle->GetNumberOfVariables(); i++)
    {
      *sendStream << prev[i];
      *sendStream << curr[i];
      *sendStream << next[i];
    }

    for (auto data : particle->GetPrevTrackedUserData())
    {
      *sendStream << data;
    }
    for (auto data : particle->GetTrackedUserData())
    {
      *sendStream << data;
    }
    for (auto data : particle->GetNextTrackedUserData())
    {
      *sendStream << data;
    }

    for (int i = 0; i < particle->GetSeedData()->GetNumberOfArrays(); i++)
    {
      vtkDataArray* array = particle->GetSeedData()->GetArray(i);
      double* tuple = array->GetTuple(particle->GetSeedArrayTupleIndex());
      for (int j = 0; j < array->GetNumberOfComponents(); j++)
      {
        *sendStream << tuple[j];
      }
    }

    // clean out old requests & sendStreams
    this->CleanSendRequests();

    // Send to other ranks
    for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
    {
      if (i == this->Controller->GetLocalProcessId())
      {
        continue;
      }
      if (particle->GetPManualShift() || this->Boxes[i].ContainsPoint(particle->GetPosition()))
      {
        ++sendStream->count; // increment counter on message
        this->SendRequests.emplace_back(new vtkMPICommunicator::Request, sendStream);
        this->Controller->NoBlockSend(sendStream->GetRawData(), this->StreamSize, i,
          LAGRANGIAN_PARTICLE_TAG, *this->SendRequests.back().first);
        ++this->SendCounter;
      }
    }
  }

  // Method to receive and deserialize a particle from any other rank
  bool ReceiveParticleIfAny(vtkLagrangianParticle*& particle, int& source)
  {
    int probe;
    if (this->Controller->Iprobe(
          vtkMultiProcessController::ANY_SOURCE, LAGRANGIAN_PARTICLE_TAG, &probe, &source) &&
      probe)
    {
      this->ReceiveStream->Reset();
      this->Controller->Receive(
        this->ReceiveStream->GetRawData(), this->StreamSize, source, LAGRANGIAN_PARTICLE_TAG);
      // Deserialize particle
      // This is strongly linked to Constructor and Send method
      int nVar, userFlag, nTrackedUserData;
      vtkIdType seedId, particleId, parentId, nSteps;
      double iTime, prevITime;
      bool pInsertPrevious, pManualShift;
      *this->ReceiveStream >> seedId;
      *this->ReceiveStream >> particleId;
      *this->ReceiveStream >> parentId;
      *this->ReceiveStream >> nVar;
      *this->ReceiveStream >> nTrackedUserData;
      *this->ReceiveStream >> nSteps;
      *this->ReceiveStream >> iTime;
      *this->ReceiveStream >> prevITime;
      *this->ReceiveStream >> userFlag;
      *this->ReceiveStream >> pInsertPrevious;
      *this->ReceiveStream >> pManualShift;

      // Create a particle with out of range seedData
      particle = vtkLagrangianParticle::NewInstance(nVar, seedId, particleId,
        this->SeedData->GetNumberOfTuples(), iTime, this->SeedData, nTrackedUserData, nSteps,
        prevITime);
      particle->SetParentId(parentId);
      particle->SetUserFlag(userFlag);
      particle->SetPInsertPreviousPosition(pInsertPrevious);
      particle->SetPManualShift(pManualShift);
      double* prev = particle->GetPrevEquationVariables();
      double* curr = particle->GetEquationVariables();
      double* next = particle->GetNextEquationVariables();
      for (int i = 0; i < nVar; i++)
      {
        *this->ReceiveStream >> prev[i];
        *this->ReceiveStream >> curr[i];
        *this->ReceiveStream >> next[i];
      }

      std::vector<double>& prevTracked = particle->GetPrevTrackedUserData();
      for (auto& var : prevTracked)
      {
        *this->ReceiveStream >> var;
      }
      std::vector<double>& tracked = particle->GetTrackedUserData();
      for (auto& var : tracked)
      {
        *this->ReceiveStream >> var;
      }
      std::vector<double>& nextTracked = particle->GetNextTrackedUserData();
      for (auto& var : nextTracked)
      {
        *this->ReceiveStream >> var;
      }

      // Recover the correct seed data values and write them into the seedData
      // So particle seed data become correct
      for (int i = 0; i < this->SeedData->GetNumberOfArrays(); i++)
      {
        vtkDataArray* array = this->SeedData->GetArray(i);
        int numComponents = array->GetNumberOfComponents();
        std::vector<double> xi(numComponents);
        for (int j = 0; j < numComponents; j++)
        {
          *this->ReceiveStream >> xi[j];
        }
        array->InsertNextTuple(&xi[0]);
      }
      return true;
    }
    return false;
  }

  void CleanSendRequests()
  {
    auto it = SendRequests.begin();
    while (it != SendRequests.end())
    {
      if (it->first->Test())
      {
        delete it->first;    // delete Request
        --it->second->count; // decrement counter
        if (it->second->count == 0)
        {
          // delete the SendStream
          delete it->second;
        }
        it = SendRequests.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  int GetSendCounter() { return this->SendCounter; }

private:
  vtkMPIController* Controller;
  int StreamSize;
  int SendCounter; // Total number of particles sent
  MessageStream* ReceiveStream;
  vtkPointData* SeedData;
  ParticleStreamManager(const ParticleStreamManager&) {}
  std::vector<vtkBoundingBox> Boxes;
  std::vector<std::pair<vtkMPICommunicator::Request*, MessageStream*>> SendRequests;
};

// A singleton class used by each rank to send particle id and valid status to another rank
// It sends to other ranks and can receive it from any other rank.
class ParticleIdManager
{
public:
  ParticleIdManager(vtkMPIController* controller)
  {
    // Initialize Members
    this->Controller = controller;

    // Compute StreamSize
    // This is strongly linked to Send and Receive code
    this->StreamSize = sizeof(vtkIdType) + sizeof(bool);

    // Initialize Streams
    this->ReceiveStream = new MessageStream(this->StreamSize);

    this->ReceivedCounter = 0;
  }

  ~ParticleIdManager()
  {
    for (size_t i = 0; i < this->SendRequests.size(); i++)
    {
      this->SendRequests[i].first->Wait();
    }
    this->CleanSendRequests();

    // Delete  receive stream
    delete this->ReceiveStream;
  }

  // Method to send a particle id to others ranks
  void SendParticleId(vtkIdType id, bool valid, int sendToRank)
  {
    // This is strongly linked to Constructor and Receive code
    MessageStream* sendStream = new MessageStream(this->StreamSize);
    *sendStream << id << valid;

    // clean out old requests & sendStreams
    this->CleanSendRequests();

    // Send to sendToRank
    ++sendStream->count; // increment counter on message
    this->SendRequests.emplace_back(new vtkMPICommunicator::Request, sendStream);
    this->Controller->NoBlockSend(sendStream->GetRawData(), this->StreamSize, sendToRank,
      LAGRANGIAN_PARTICLE_ID_TAG, *this->SendRequests.back().first);
  }

  // Method to receive a particle id from any other rank
  bool ReceiveParticleIdIfAny(vtkIdType& id, bool& valid)
  {
    int probe, source;
    if (this->Controller->Iprobe(
          vtkMultiProcessController::ANY_SOURCE, LAGRANGIAN_PARTICLE_ID_TAG, &probe, &source) &&
      probe)
    {
      this->ReceiveStream->Reset();
      this->Controller->Receive(
        this->ReceiveStream->GetRawData(), this->StreamSize, source, LAGRANGIAN_PARTICLE_ID_TAG);

      *this->ReceiveStream >> id >> valid;
      ++this->ReceivedCounter;
      return true;
    }
    return false;
  }

  void CleanSendRequests()
  {
    auto it = SendRequests.begin();
    while (it != SendRequests.end())
    {
      if (it->first->Test())
      {
        delete it->first;    // delete Request
        --it->second->count; // decrement counter
        if (it->second->count == 0)
        {
          // delete the SendStream
          delete it->second;
        }
        it = SendRequests.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  int GetReceivedCounter() { return this->ReceivedCounter; }

private:
  vtkMPIController* Controller;
  int StreamSize;
  int ReceivedCounter; // Total number of particlesIds recieved
  MessageStream* ReceiveStream;
  ParticleIdManager(const ParticleIdManager&) {}
  std::vector<std::pair<vtkMPICommunicator::Request*, MessageStream*>> SendRequests;
};

// a class used to manage the feed of particles using GetGlobalStatus(status) function
//  input a local partition 'status' and outputs the globalStatus
//  status = 0 - INACTIVE - particle queue is empty and all sent particles have been confirmed as
//  being recieved status = 1 - ACTIVE - either the particle queue has particles or we are waiting
//  on confirmation of pariticles
//               being recieved.
//  - each rank updates master when its status changes
//  globalStatus is 0 when all paritition are INACTIVE and 1 if at least one partition is ACTIVE.
class ParticleFeedManager
{
public:
  ParticleFeedManager(vtkMPIController* controller)
  {
    this->Controller = controller;

    this->RankStates.resize(this->Controller->GetNumberOfProcesses() - 1, 1);
    this->GlobalStatus = 1;
    this->CurrentStatus = 1;
  }

  void MasterUpdateRankStatus()
  {
    // only called on master process - receive any updated status from other ranks
    int probe, source;

    while (this->Controller->Iprobe(
             vtkMultiProcessController::ANY_SOURCE, LAGRANGIAN_RANG_FLAG_TAG, &probe, &source) &&
      probe)
    {
      this->Controller->Receive(&this->RankStates[source - 1], 1, source, LAGRANGIAN_RANG_FLAG_TAG);
    }
  }

  void RankSendStatus(int status)
  {
    // Send an updated status if it has changed
    if (status != this->CurrentStatus)
    {
      this->CurrentStatus = status;
      std::shared_ptr<vtkMPICommunicator::Request> sendRequest(new vtkMPICommunicator::Request);
      this->Controller->NoBlockSend(
        &this->CurrentStatus, 1, 0, LAGRANGIAN_RANG_FLAG_TAG, *sendRequest);
      this->SendRequests.emplace_back(sendRequest);
    }
  }

  void MasterSendGlobalStatus()
  {
    // no active particles - send terminate instruction to other ranks
    for (int p = 1; p < this->Controller->GetNumberOfProcesses(); ++p)
    {
      std::shared_ptr<vtkMPICommunicator::Request> sendRequest(new vtkMPICommunicator::Request);
      this->Controller->NoBlockSend(
        &this->GlobalStatus, 1, p, LAGRANGIAN_PARTICLE_CONTROL_TAG, *sendRequest);
      this->SendRequests.emplace_back(sendRequest);
    }
  }

  void RankReceiveGlobalStatus()
  {
    // check for change in globalStatus from master
    int probe, source;
    while (this->Controller->Iprobe(0, LAGRANGIAN_PARTICLE_CONTROL_TAG, &probe, &source) && probe)
    {
      this->Controller->Receive(&this->GlobalStatus, 1, source, LAGRANGIAN_PARTICLE_CONTROL_TAG);
    }
  }

  int GetGlobalStatus(int status)
  {
    if (this->Controller->GetLocalProcessId() == 0)
    {
      this->CurrentStatus = status;

      // master process - receive any updated counters from other ranks
      this->MasterUpdateRankStatus();

      // determine globalStatus across all partitions
      this->GlobalStatus = this->CurrentStatus;
      for (auto state : this->RankStates)
      {
        this->GlobalStatus = this->GlobalStatus || state;
      }

      // if everything has finished send message to all ranks
      if (this->GlobalStatus == 0)
      {
        this->MasterSendGlobalStatus();
      }
    }
    else
    {
      // check for update to global status
      this->RankReceiveGlobalStatus();

      // send status to master
      this->RankSendStatus(status);
    }

    return this->GlobalStatus;
  }

private:
  vtkMPIController* Controller;
  int GlobalStatus;
  int CurrentStatus; // current status of rank
  std::vector<int> RankStates;
  std::vector<std::shared_ptr<vtkMPICommunicator::Request>> SendRequests;
};

vtkStandardNewMacro(vtkPLagrangianParticleTracker);

//------------------------------------------------------------------------------
vtkPLagrangianParticleTracker::vtkPLagrangianParticleTracker()
  : Controller(vtkMPIController::SafeDownCast(vtkMultiProcessController::GetGlobalController()))
  , StreamManager(nullptr)
  , TransferredParticleIdManager(nullptr)
  , FeedManager(nullptr)
{
  // To get a correct progress update
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->IntegratedParticleCounterIncrement = this->Controller->GetNumberOfProcesses();
  }
}

//------------------------------------------------------------------------------
vtkPLagrangianParticleTracker::~vtkPLagrangianParticleTracker()
{
  delete StreamManager;
  delete TransferredParticleIdManager;
  delete FeedManager;
}

//------------------------------------------------------------------------------
int vtkPLagrangianParticleTracker::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int ghostLevel = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  if (info)
  {
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
  }

  vtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  if (sourceInfo)
  {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
  }

  vtkInformation* surfaceInfo = inputVector[2]->GetInformationObject(0);
  if (surfaceInfo)
  {
    surfaceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
    surfaceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
    surfaceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevel);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkPLagrangianParticleTracker::GenerateParticles(const vtkBoundingBox* bounds,
  vtkDataSet* seeds, vtkDataArray* initialVelocities, vtkDataArray* initialIntegrationTimes,
  vtkPointData* seedData, int nVar, std::queue<vtkLagrangianParticle*>& particles)
{
  // Generate particle
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->ParticleCounter = this->Controller->GetLocalProcessId();

    // delete potential remaining managers
    delete StreamManager;
    delete TransferredParticleIdManager;
    delete FeedManager;

    // Reduce SeedData Arrays
    int nArrays = seedData->GetNumberOfArrays();
    int actualNArrays;
    int rank = this->Controller->GetLocalProcessId();
    int dummyRank = -1;
    int fullArrayRank;

    // Recover maximum number of arrays
    this->Controller->AllReduce(&nArrays, &actualNArrays, 1, vtkCommunicator::MAX_OP);
    if (actualNArrays != nArrays)
    {
      // This rank does not have the maximum number of arrays
      if (nArrays != 0)
      {
        // this rank have an incorrect number of arrays, not supposed to happen
        vtkErrorMacro("Something went wrong with seed data arrays, discarding arrays");
        for (int i = nArrays - 1; i >= 0; i--)
        {
          seedData->RemoveArray(i);
        }
      }

      // Rank without any seeds, does not have access to the structure of
      // seeds pointData.
      // Recover this information from another rank.
      this->Controller->AllReduce(&dummyRank, &fullArrayRank, 1, vtkCommunicator::MAX_OP);
      int source, size;
      char type;
      int probe = false;
      while (!probe)
      {
        // Wait for the arrays metadata to be sent
        this->Controller->Iprobe(
          fullArrayRank, LAGRANGIAN_ARRAY_TAG, &probe, &source, &type, &size);
      }
      MessageStream stream(size);
      // Receive arrays metadata
      this->Controller->Receive(stream.GetRawData(), size, source, LAGRANGIAN_ARRAY_TAG);
      for (int i = 0; i < actualNArrays; i++)
      {
        // Create arrays according to metadata
        int dataType, nComponents, nameLen, compNameLen;
        stream >> dataType;
        vtkDataArray* array = vtkDataArray::CreateDataArray(dataType);
        stream >> nComponents;
        array->SetNumberOfComponents(nComponents);
        stream >> nameLen;
        std::vector<char> name(nameLen + 1, 0);
        for (int l = 0; l < nameLen; l++)
        {
          stream >> name[l];
        }
        array->SetName(&name[0]);
        for (int idComp = 0; idComp < nComponents; idComp++)
        {
          stream >> compNameLen;
          if (compNameLen > 0)
          {
            std::vector<char> compName(compNameLen + 1, 0);
            for (int compLength = 0; compLength < compNameLen; compLength++)
            {
              stream >> compName[compLength];
            }
            array->SetComponentName(idComp, &compName[0]);
          }
        }
        seedData->AddArray(array);
        array->Delete();
      }
    }
    else
    {
      // This rank contains the correct number of arrays
      this->Controller->AllReduce(&rank, &fullArrayRank, 1, vtkCommunicator::MAX_OP);

      // Select the highest rank containing arrays to be the one to be right about arrays metadata
      if (fullArrayRank == rank)
      {
        // Generate arrays metadata
        int streamSize = 0;
        streamSize += nArrays * 3 * sizeof(int);
        // nArrays * (Datatype + nComponents + strlen(name));
        for (int i = 0; i < nArrays; i++)
        {
          vtkDataArray* array = seedData->GetArray(i);
          const char* name = array->GetName();
          streamSize += static_cast<int>(strlen(name)); // name
          int nComp = array->GetNumberOfComponents();
          for (int idComp = 0; idComp < nComp; idComp++)
          {
            streamSize += sizeof(int);
            const char* compName = array->GetComponentName(idComp);
            if (compName)
            {
              streamSize += static_cast<int>(strlen(compName));
            }
          }
        }
        MessageStream stream(streamSize);
        for (int i = 0; i < nArrays; i++)
        {
          vtkDataArray* array = seedData->GetArray(i);
          stream << array->GetDataType();
          stream << array->GetNumberOfComponents();
          const char* name = array->GetName();
          int nameLen = static_cast<int>(strlen(name));
          stream << nameLen;
          for (int l = 0; l < nameLen; l++)
          {
            stream << name[l];
          }
          for (int idComp = 0; idComp < array->GetNumberOfComponents(); idComp++)
          {
            const char* compName = array->GetComponentName(idComp);
            int compNameLen = 0;
            if (compName)
            {
              compNameLen = static_cast<int>(strlen(compName));
              stream << compNameLen;
              for (int compLength = 0; compLength < compNameLen; compLength++)
              {
                stream << compName[compLength];
              }
            }
            else
            {
              stream << compNameLen;
            }
          }
        }

        // Send to arrays metadata to all other ranks
        for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
        {
          if (i == this->Controller->GetLocalProcessId())
          {
            continue;
          }
          this->Controller->Send(stream.GetRawData(), streamSize, i, LAGRANGIAN_ARRAY_TAG);
        }
      }
      else
      {
        // Other ranks containing correct number of arrays, check metadata is correct
        char type;
        int source, size;
        int probe = false;
        while (!probe)
        {
          // Wait for array metadata
          this->Controller->Iprobe(
            fullArrayRank, LAGRANGIAN_ARRAY_TAG, &probe, &source, &type, &size);
        }
        MessageStream stream(size);
        // Receive array metadata
        this->Controller->Receive(stream.GetRawData(), size, source, LAGRANGIAN_ARRAY_TAG);
        // Check data arrays
        for (int i = 0; i < nArrays; i++)
        {
          vtkDataArray* array = seedData->GetArray(i);
          int dataType, nComponents, nameLen, compNameLen;
          stream >> dataType;
          if (dataType != array->GetDataType())
          {
            vtkErrorMacro("Incoherent dataType between nodes, results may be invalid");
          }
          stream >> nComponents;
          if (nComponents != array->GetNumberOfComponents())
          {
            vtkErrorMacro("Incoherent number of components between nodes, "
                          "results may be invalid");
          }
          const char* localName = array->GetName();
          stream >> nameLen;
          std::vector<char> name(nameLen + 1, 0);
          for (int l = 0; l < nameLen; l++)
          {
            stream >> name[l];
          }
          if (strcmp(&name[0], localName) != 0)
          {
            vtkErrorMacro("Incoherent array names between nodes, "
                          "results may be invalid");
          }
          for (int idComp = 0; idComp < nComponents; idComp++)
          {
            stream >> compNameLen;
            const char* localCompName = array->GetComponentName(idComp);
            std::vector<char> compName(compNameLen + 1, 0);
            for (int compLength = 0; compLength < compNameLen; compLength++)
            {
              stream >> compName[compLength];
            }
            if (localCompName && strcmp(&compName[0], localCompName) != 0)
            {
              vtkErrorMacro("Incoherent array component names between nodes, "
                            "results may be invalid");
            }
          }
        }
      }
    }

    // Create managers
    this->StreamManager =
      new ParticleStreamManager(this->Controller, seedData, this->IntegrationModel, bounds);
    this->TransferredParticleIdManager = new ParticleIdManager(this->Controller);
    this->FeedManager = new ParticleFeedManager(this->Controller);

    // Generate particle and distribute the ones not in domain to other nodes
    for (vtkIdType i = 0; i < seeds->GetNumberOfPoints(); i++)
    {
      double position[3];
      seeds->GetPoint(i, position);
      double initialIntegrationTime =
        initialIntegrationTimes ? initialIntegrationTimes->GetTuple1(i) : 0;
      vtkIdType particleId = this->GetNewParticleId();
      vtkLagrangianParticle* particle = new vtkLagrangianParticle(nVar, particleId, particleId, i,
        initialIntegrationTime, seedData, this->IntegrationModel->GetNumberOfTrackedUserData());
      memcpy(particle->GetPosition(), position, 3 * sizeof(double));
      initialVelocities->GetTuple(i, particle->GetVelocity());
      particle->SetThreadedData(this->SerialThreadedData);
      this->IntegrationModel->InitializeParticle(particle);
      if (this->IntegrationModel->FindInLocators(particle->GetPosition(), particle))
      {
        particles.push(particle);
      }
      else
      {
        this->StreamManager->SendParticle(particle);
        delete particle;
      }
    }
    this->Controller->Barrier();
    this->ReceiveParticles(particles);
  }
  else
  {
    this->Superclass::GenerateParticles(
      bounds, seeds, initialVelocities, initialIntegrationTimes, seedData, nVar, particles);
  }
}

//------------------------------------------------------------------------------
void vtkPLagrangianParticleTracker::GetParticleFeed(
  std::queue<vtkLagrangianParticle*>& particleQueue)
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() <= 1)
  {
    return;
  }

  // local parition status 0 = parition inactive,  1 = active
  int status;

  do
  {
    // receive particles from other partitions
    this->ReceiveParticles(particleQueue);

    // check for receipt of sent particles
    this->ReceiveTransferredParticleIds();

    // determine local status - active if queue is busy or we are waiting for receipt of sent
    // pariticles
    status = !particleQueue.empty() ||
      this->StreamManager->GetSendCounter() !=
        this->TransferredParticleIdManager->GetReceivedCounter();
  } while (this->FeedManager->GetGlobalStatus(status) && particleQueue.empty());
}

//------------------------------------------------------------------------------
int vtkPLagrangianParticleTracker::Integrate(vtkInitialValueProblemSolver* integrator,
  vtkLagrangianParticle* particle, std::queue<vtkLagrangianParticle*>& particleQueue,
  vtkPolyData* particlePathsOutput, vtkPolyLine* particlePath, vtkDataObject* interactionOutput)
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    if (this->GenerateParticlePathsOutput && particle->GetPInsertPreviousPosition())
    {
      // This is a particle from another rank, store a duplicated previous point
      this->InsertPathOutputPoint(particle, particlePathsOutput, particlePath->GetPointIds(), true);
      particle->SetPInsertPreviousPosition(false);
    }
  }

  int ret = this->vtkLagrangianParticleTracker::Integrate(
    integrator, particle, particleQueue, particlePathsOutput, particlePath, interactionOutput);

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    if (particle->GetTermination() == vtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_DOMAIN)
    {
      if (!particle->GetPManualShift())
      {
        particle->SetPInsertPreviousPosition(true);
      }

      // Stream out of domain particles
      std::lock_guard<std::mutex> guard(this->StreamManagerMutex);
      this->StreamManager->SendParticle(particle);
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
void vtkPLagrangianParticleTracker::ReceiveTransferredParticleIds()
{
  vtkIdType id;
  bool valid;
  while (this->TransferredParticleIdManager->ReceiveParticleIdIfAny(id, valid))
  {
    if (valid)
    {
      // Delete transferred particle without calling
      // ParticleAboutToBeDeleted
      auto iter = this->OutOfDomainParticleMap.find(id);
      if (iter != this->OutOfDomainParticleMap.end())
      {
        iter->second->SetTermination(vtkLagrangianParticle::PARTICLE_TERMINATION_TRANSFERRED);
        this->Superclass::DeleteParticle(iter->second);
        this->OutOfDomainParticleMap.erase(iter);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkPLagrangianParticleTracker::ReceiveParticles(
  std::queue<vtkLagrangianParticle*>& particleQueue)
{
  vtkLagrangianParticle* receivedParticle;
  int source = -1;

  while (this->StreamManager->ReceiveParticleIfAny(receivedParticle, source))
  {
    receivedParticle->SetThreadedData(this->SerialThreadedData);

    // Check for manual shift
    if (receivedParticle->GetPManualShift())
    {
      this->IntegrationModel->ParallelManualShift(receivedParticle);
      receivedParticle->SetPManualShift(false);
    }
    // Receive all particles
    bool valid =
      this->IntegrationModel->FindInLocators(receivedParticle->GetPosition(), receivedParticle);

    // Inform source rank that it was received
    this->TransferredParticleIdManager->SendParticleId(receivedParticle->GetId(), valid, source);

    if (valid)
    {
      particleQueue.push(receivedParticle);
    }
    else
    {
      delete receivedParticle;
    }
  }
}

//------------------------------------------------------------------------------
bool vtkPLagrangianParticleTracker::FinalizeOutputs(
  vtkPolyData* particlePathsOutput, vtkDataObject* interactionOutput)
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    // Cleanly delete remaining out of domain particles
    for (auto iter : this->OutOfDomainParticleMap)
    {
      this->Superclass::DeleteParticle(iter.second);
    }
    this->OutOfDomainParticleMap.clear();

    if (this->GenerateParticlePathsOutput)
    {
      // Construct array with all non outofdomains ids and terminations
      vtkNew<vtkLongLongArray> idTermination;
      vtkNew<vtkLongLongArray> allIdTermination;
      idTermination->Allocate(particlePathsOutput->GetNumberOfCells());
      idTermination->SetNumberOfComponents(2);
      vtkIntArray* terminations =
        vtkIntArray::SafeDownCast(particlePathsOutput->GetCellData()->GetArray("Termination"));
      vtkLongLongArray* ids =
        vtkLongLongArray::SafeDownCast(particlePathsOutput->GetCellData()->GetArray("Id"));
      for (int i = 0; i < particlePathsOutput->GetNumberOfCells(); i++)
      {
        if (terminations->GetValue(i) != vtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_DOMAIN)
        {
          idTermination->InsertNextTuple2(ids->GetValue(i), terminations->GetValue(i));
        }
      }
      idTermination->Squeeze();

      // AllGather it
      this->Controller->AllGatherV(idTermination, allIdTermination);

      // Modify current terminations
      for (vtkIdType i = 0; i < allIdTermination->GetNumberOfTuples(); i++)
      {
        vtkIdType id = allIdTermination->GetTuple2(i)[0];
        for (vtkIdType j = 0; j < particlePathsOutput->GetNumberOfCells(); j++)
        {
          if (ids->GetValue(j) == id)
          {
            terminations->SetTuple1(j, allIdTermination->GetTuple2(i)[1]);
          }
        }
      }
    }
  }
  return this->Superclass::FinalizeOutputs(particlePathsOutput, interactionOutput);
}

//------------------------------------------------------------------------------
bool vtkPLagrangianParticleTracker::UpdateSurfaceCacheIfNeeded(vtkDataObject*& surfaces)
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    // Update local cache and reduce cache status
    int localCacheUpdated = this->Superclass::UpdateSurfaceCacheIfNeeded(surfaces);
    int maxLocalCacheUpdated;
    this->Controller->AllReduce(
      &localCacheUpdated, &maxLocalCacheUpdated, 1, vtkCommunicator::MAX_OP);

    if (!maxLocalCacheUpdated)
    {
      // Cache is still valid, use already reduced surface
      if (vtkDataSet::SafeDownCast(surfaces))
      {
        surfaces = this->TmpSurfaceInput;
      }
      else // if (vtkCompositeDataSet::SafeDownCast(surfaces))
      {
        surfaces = this->TmpSurfaceInputMB;
      }
      return false;
    }

    // Local cache has been updated, update temporary reduced surface
    // In Parallel, reduce surfaces on rank 0, which then broadcast them to all ranks.

    // Recover all surfaces on rank 0
    std::vector<vtkSmartPointer<vtkDataObject>> allSurfaces;
    this->Controller->Gather(surfaces, allSurfaces, 0);

    // Manager dataset case
    if (vtkDataSet::SafeDownCast(surfaces))
    {
      if (this->Controller->GetLocalProcessId() == 0)
      {
        // Rank 0 append all dataset together
        vtkNew<vtkAppendFilter> append;
        for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
        {
          vtkDataSet* ds = vtkDataSet::SafeDownCast(allSurfaces[i]);
          if (ds)
          {
            append->AddInputData(ds);
          }
        }
        append->Update();
        this->TmpSurfaceInput->ShallowCopy(append->GetOutput());
      }

      // Broadcast resulting UnstructuredGrid
      this->Controller->Broadcast(this->TmpSurfaceInput, 0);
      surfaces = this->TmpSurfaceInput;
    }

    // Composite case
    else if (vtkCompositeDataSet::SafeDownCast(surfaces))
    {
      if (this->Controller->GetLocalProcessId() == 0)
      {
        // Rank 0 reconstruct Composite tree
        vtkCompositeDataSet* mb = vtkCompositeDataSet::SafeDownCast(surfaces);
        this->TmpSurfaceInputMB->ShallowCopy(mb);
        vtkCompositeDataIterator* iter = mb->NewIterator();
        iter->SkipEmptyNodesOff();
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          // Rank 0 append all dataset together
          vtkNew<vtkAppendFilter> append;
          for (int i = 0; i < this->Controller->GetNumberOfProcesses(); i++)
          {
            vtkCompositeDataSet* localMb = vtkCompositeDataSet::SafeDownCast(allSurfaces[i]);
            vtkDataSet* ds = vtkDataSet::SafeDownCast(localMb->GetDataSet(iter));
            if (ds)
            {
              append->AddInputData(ds);
            }
          }
          append->Update();
          this->TmpSurfaceInputMB->SetDataSet(iter, append->GetOutput());
        }
        iter->Delete();
      }
      // Broadcast resulting Composite
      this->Controller->Broadcast(this->TmpSurfaceInputMB, 0);
      surfaces = this->TmpSurfaceInputMB;
    }
    else
    {
      vtkErrorMacro("Unrecognized surface.");
    }
    return true;
  }
  else
  {
    return this->Superclass::UpdateSurfaceCacheIfNeeded(surfaces);
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkPLagrangianParticleTracker::GetNewParticleId()
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    vtkIdType id = this->ParticleCounter;
    this->ParticleCounter += this->Controller->GetNumberOfProcesses();
    return id;
  }
  return this->Superclass::GetNewParticleId();
}

//------------------------------------------------------------------------------
void vtkPLagrangianParticleTracker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkPLagrangianParticleTracker::DeleteParticle(vtkLagrangianParticle* particle)
{
  if (particle->GetTermination() != vtkLagrangianParticle::PARTICLE_TERMINATION_OUT_OF_DOMAIN)
  {
    this->Superclass::DeleteParticle(particle);
  }
  else
  {
    // store the particle to be deleted later
    std::lock_guard<std::mutex> guard(this->OutOfDomainParticleMapMutex);
    this->OutOfDomainParticleMap[particle->GetId()] = particle;
  }
}
