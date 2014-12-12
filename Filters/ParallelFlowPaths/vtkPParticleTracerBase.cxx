/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkParticleTracerBase.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPParticleTracerBase.h"

#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include <cassert>

#include <algorithm>

using namespace vtkParticleTracerBaseNamespace;

vtkPParticleTracerBase::vtkPParticleTracerBase()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//---------------------------------------------------------------------------
vtkPParticleTracerBase::~vtkPParticleTracerBase()
{
  this->SetController(NULL);
  this->SetParticleWriter(NULL);
}

//---------------------------------------------------------------------------
vtkPolyData* vtkPParticleTracerBase::Execute(vtkInformationVector** inputVector)
{
  vtkDebugMacro(<< "Clear MPI send list ");
  this->MPISendList.clear();
  this->Tail.clear();

  return vtkParticleTracerBase::Execute(inputVector);
}

//---------------------------------------------------------------------------
bool vtkPParticleTracerBase::SendParticleToAnotherProcess(
  ParticleInformation &info, ParticleInformation &previousInfo,
  vtkPointData* pd)
{
  assert(info.PointId>=0); //the particle must have already been added;

  RemoteParticleInfo remoteInfo;

  remoteInfo.Current = info;

  remoteInfo.Previous = previousInfo;
  remoteInfo.PreviousPD = vtkSmartPointer<vtkPointData>::New();
  remoteInfo.PreviousPD->CopyAllocate(this->ProtoPD);

  //only copy those that correspond to the original data fields
  for(int i=0; i <this->ProtoPD->GetNumberOfArrays(); i++)
    {
    char* arrName = this->ProtoPD->GetArray(i)->GetName();
    vtkDataArray* arrFrom = pd->GetArray(arrName);
    vtkDataArray* arrTo = remoteInfo.PreviousPD->GetArray(i);
    assert(arrFrom->GetNumberOfComponents()==arrTo->GetNumberOfComponents());
    arrTo->InsertNextTuple(arrFrom->GetTuple(info.PointId));
    }

  double eps = (this->GetCacheDataTime(1)-this->GetCacheDataTime(0))/100;
  if (info.CurrentPosition.x[3]<(this->GetCacheDataTime(0)-eps) ||
      info.CurrentPosition.x[3]>(this->GetCacheDataTime(1)+eps))
    {
    vtkErrorMacro(<< "Unexpected time value in MPISendList - expected ("
                   << this->GetCacheDataTime(0) << "-" << this->GetCacheDataTime(1) << ") got "
                   << info.CurrentPosition.x[3]);
    }

  if (this->MPISendList.capacity()<(this->MPISendList.size()+1))
    {
    this->MPISendList.reserve(static_cast<int>(this->MPISendList.size()*1.5));
    }
  this->MPISendList.push_back(remoteInfo);
  return true;
}

//---------------------------------------------------------------------------
void vtkPParticleTracerBase::AssignSeedsToProcessors(
  double t, vtkDataSet *source, int sourceID, int ptId,
  ParticleVector &localSeedPoints, int &localAssignedCount)
{
  if(!this->Controller)
    {
    return Superclass::AssignSeedsToProcessors(t, source, sourceID, ptId,
                                               localSeedPoints, localAssignedCount);
    }
  ParticleVector candidates;
  //
  // take points from the source object and create a particle list
  //
  int numSeeds = source->GetNumberOfPoints();
  candidates.resize(numSeeds);
  //
  for (int i=0; i<numSeeds; i++)
    {
    ParticleInformation &info = candidates[i];
    memcpy(&(info.CurrentPosition.x[0]), source->GetPoint(i), sizeof(double)*3);
    info.CurrentPosition.x[3] = t;
    info.LocationState        = 0;
    info.CachedCellId[0]      =-1;
    info.CachedCellId[1]      =-1;
    info.CachedDataSetId[0]   = 0;
    info.CachedDataSetId[1]   = 0;
    info.SourceID             = sourceID;
    info.InjectedPointId      = i+ptId;
    info.InjectedStepId       = this->GetReinjectionCounter();
    info.TimeStepAge          = 0;
    info.UniqueParticleId     =-1;
    info.rotation             = 0.0;
    info.angularVel           = 0.0;
    info.time                 = 0.0;
    info.age                  = 0.0;
    info.speed                = 0.0;
    info.SimulationTime       = this->GetCurrentTimeValue();
    info.ErrorCode            = 0;
  }
  //
  // Gather all Seeds to all processors for classification
  //
  // TODO : can we just use the same array here for send and receive
  ParticleVector allCandidates;
  this->TestParticles(candidates, localSeedPoints, localAssignedCount);
  int TotalAssigned = 0;
  this->Controller->Reduce(&localAssignedCount, &TotalAssigned, 1, vtkCommunicator::SUM_OP, 0);

  // Assign unique identifiers taking into account uneven distribution
  // across processes and seeds which were rejected
  this->AssignUniqueIds(localSeedPoints);
}

//---------------------------------------------------------------------------
void vtkPParticleTracerBase::AssignUniqueIds(
  vtkParticleTracerBaseNamespace::ParticleVector &localSeedPoints)
{
  if(!this->Controller)
    {
    return Superclass::AssignUniqueIds(localSeedPoints);
    }

  vtkIdType particleCountOffset = 0;
  vtkIdType numParticles = localSeedPoints.size();

  if (this->Controller->GetNumberOfProcesses()>1)
    {
    // everyone starts with the master index
    this->Controller->Broadcast(&this->UniqueIdCounter, 1, 0);
    // setup arrays used by the AllGather call.
    std::vector<vtkIdType> recvNumParticles(this->Controller->GetNumberOfProcesses(), 0);
    // Broadcast and receive count to/from all other processes.
    this->Controller->AllGather(&numParticles, &recvNumParticles[0], 1);
    // Each process is allocating a certain number.
    // start our indices from sum[0,this->Rank](numparticles)
    for (int i=0; i<this->Controller->GetLocalProcessId(); ++i)
      {
      particleCountOffset += recvNumParticles[i];
      }
    for (vtkIdType i=0; i<numParticles; i++)
      {
      localSeedPoints[i].UniqueParticleId =
        this->UniqueIdCounter + particleCountOffset + i;
      }
    for (int i=0; i<this->Controller->GetNumberOfProcesses(); ++i)
      {
      this->UniqueIdCounter += recvNumParticles[i];
      }
    }
  else {
    for (vtkIdType i=0; i<numParticles; i++)
      {
      localSeedPoints[i].UniqueParticleId =
        this->UniqueIdCounter + particleCountOffset + i;
      }
    this->UniqueIdCounter += numParticles;
  }
}

//---------------------------------------------------------------------------
void vtkPParticleTracerBase::SendReceiveParticles(RemoteParticleVector &sParticles,
                                                  RemoteParticleVector &rParticles)
{
  int numParticles = static_cast<int>(sParticles.size());
  std::vector<int> allNumParticles(this->Controller->GetNumberOfProcesses(), 0);
  // Broadcast and receive size to/from all other processes.
  this->Controller->AllGather(&numParticles, &allNumParticles[0], 1);

  // write the message
  const int size1 = sizeof(ParticleInformation);
  const int nArrays = this->ProtoPD->GetNumberOfArrays();
  unsigned int typeSize = 2*size1;
  for(int i=0; i<this->ProtoPD->GetNumberOfArrays();i++)
    {
    typeSize+= this->ProtoPD->GetArray(i)->GetNumberOfComponents()*sizeof(double);
    }

  vtkIdType messageSize = numParticles*typeSize;
  std::vector<char> sendMessage(messageSize,0);
  for(int i=0; i<numParticles; i++)
    {
    memcpy(&sendMessage[i*typeSize],        &sParticles[i].Current, size1);
    memcpy(&sendMessage[i*typeSize]+size1 , &sParticles[i].Previous, size1);

    vtkPointData* pd = sParticles[i].PreviousPD;
    char* data = &sendMessage[i*typeSize] + 2*size1;
    for(int j=0; j<nArrays;j++)
      {
      vtkDataArray* arr = pd->GetArray(j);
      assert(arr->GetNumberOfTuples()==1);
      int numComponents = arr->GetNumberOfComponents();
      double* y = arr->GetTuple(0);
      int dataSize = sizeof(double)*numComponents;
      memcpy(data, y, dataSize);
      data+=dataSize;
      }
    }

  std::vector<vtkIdType> messageLength(this->Controller->GetNumberOfProcesses(), 0);
  std::vector<vtkIdType> messageOffset(this->Controller->GetNumberOfProcesses(), 0);
  int allMessageSize(0);
  int numAllParticles(0);
  for (int i=0; i<this->Controller->GetNumberOfProcesses(); ++i)
    {
    numAllParticles+= allNumParticles[i];
    messageLength[i] = allNumParticles[i]*typeSize;
    messageOffset[i] =allMessageSize;
    allMessageSize+= messageLength[i];
    }

  //receive the message

  std::vector<char> recvMessage(allMessageSize,0);
  this->Controller->AllGatherV(messageSize>0?  &sendMessage[0] : NULL,
                               allMessageSize>0? &recvMessage[0] : NULL,
                               messageSize, &messageLength[0],
                               &messageOffset[0]);

  //read the message
  rParticles.resize(numAllParticles);
  for(int i=0; i<numAllParticles; i++)
    {
    memcpy(&rParticles[i].Current,  &recvMessage[i*typeSize],     size1);
    memcpy(&rParticles[i].Previous, &recvMessage[i*typeSize]+size1,size1);

    rParticles[i].PreviousPD = vtkSmartPointer<vtkPointData>::New();
    rParticles[i].PreviousPD->CopyAllocate(this->ProtoPD);
    vtkPointData* pd = rParticles[i].PreviousPD;
    char* data = &recvMessage[i*typeSize] + 2*size1;
    for(int j=0; j<nArrays;j++)
      {
      vtkDataArray* arr = pd->GetArray(j);
      int numComponents = arr->GetNumberOfComponents();
      int dataSize = sizeof(double)*numComponents;
      std::vector<double> xi(numComponents);
      memcpy(&xi[0], data, dataSize);
      arr->InsertNextTuple(&xi[0]);
      data+=dataSize;
      }
    }

  std::vector<RemoteParticleInfo>::iterator first =
    rParticles.begin() + messageOffset[this->Controller->GetLocalProcessId()]/typeSize;
  std::vector<RemoteParticleInfo>::iterator last =
    first + messageLength[this->Controller->GetLocalProcessId()]/typeSize;
  rParticles.erase(first, last);
  // don't want the ones that we sent away
  this->MPISendList.clear();
}

//---------------------------------------------------------------------------
int vtkPParticleTracerBase::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  if (sourceInfo)
    {
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
                    0);
    sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
                    1);
    }
  return Superclass::RequestUpdateExtent(request,inputVector,outputVector);
}

//---------------------------------------------------------------------------
int vtkPParticleTracerBase::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int rvalue = this->Superclass::RequestData(request, inputVector, outputVector);
  if(this->Controller)
    {
    this->Controller->Barrier();
    }

  return rvalue;
}

//---------------------------------------------------------------------------
void vtkPParticleTracerBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Controller: " << this->Controller << endl;
}

//---------------------------------------------------------------------------
void vtkPParticleTracerBase::UpdateParticleListFromOtherProcesses()
{
  if(!this->Controller)
    {
    return;
    }
  RemoteParticleVector received;

  this->SendReceiveParticles(this->MPISendList, received);

  std::vector<int> candidatesIndices;

  // the Particle lists will grow if any are received
  // so we must be very careful with our iterators
  //  this->TransmitReceiveParticles(this->MPISendList, received, true);
  // classify all the ones we received
  if (received.size()>0)
    {
    ParticleVector receivedParticles;
    for(unsigned int i=0; i<received.size();i++)
      {
      receivedParticles.push_back(received[i].Current);
      }

    this->TestParticles(receivedParticles, candidatesIndices);
    }
  int numCandidates =static_cast<int>(candidatesIndices.size());

  //increment particle ids
  for(int i=0; i<numCandidates;i++)
    {
    RemoteParticleInfo& info(received[candidatesIndices[i]]);
    info.Current.UniqueParticleId++;
    info.Previous.UniqueParticleId++;
    }

// Now update our main list with the ones we are keeping
  for (int i=0; i<numCandidates; i++)
    {
    RemoteParticleInfo& info(received[candidatesIndices[i]]);
    info.Current.PointId = -1;
    info.Current.CachedDataSetId[0] = info.Current.CachedDataSetId[1] = -1;
    info.Current.CachedCellId[0] = info.Current.CachedCellId[1] = -1;
    info.Previous.CachedDataSetId[0] = info.Previous.CachedDataSetId[1] = -1;
    info.Previous.CachedCellId[0] = info.Previous.CachedCellId[1] = -1;
    this->Tail.push_back(info);
    this->ParticleHistories.push_back(info.Current);
    }
}

//---------------------------------------------------------------------------
bool vtkPParticleTracerBase::IsPointDataValid(vtkDataObject* input)
{
  if(this->Controller->GetNumberOfProcesses() == 1)
    {
    return this->Superclass::IsPointDataValid(input);
    }
  int retVal = 1;
  vtkMultiProcessStream stream;
  if(this->Controller->GetLocalProcessId() == 0)
    {
    std::vector<std::string> arrayNames;
    if(vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input))
      {
      retVal = (int) this->Superclass::IsPointDataValid(cdInput, arrayNames);
      }
    else
      {
      this->GetPointDataArrayNames(vtkDataSet::SafeDownCast(input), arrayNames);
      }
    stream << retVal;
    // only need to send the array names to check if proc 0 has valid point data
    if(retVal == 1)
      {
      stream << (int)arrayNames.size();
      for(std::vector<std::string>::iterator it=arrayNames.begin();
          it!=arrayNames.end();it++)
        {
        stream << *it;
        }
      }
    }
  this->Controller->Broadcast(stream, 0);
  if(this->Controller->GetLocalProcessId() != 0)
    {
    stream >> retVal;
    if(retVal == 0)
      {
      return false;
      }
    int numArrays;
    stream >> numArrays;
    std::vector<std::string> arrayNames(numArrays);
    for(int i=0;i<numArrays;i++)
      {
      stream >> arrayNames[i];
      }
    std::vector<std::string> tempNames;
    if(vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input))
      {
      retVal = (int) this->Superclass::IsPointDataValid(cdInput, tempNames);
      if(retVal)
        {
        retVal = (std::equal(tempNames.begin(), tempNames.end(), arrayNames.begin()) == true ?
                  1 : 0);
        }
      }
    else
      {
      this->GetPointDataArrayNames(vtkDataSet::SafeDownCast(input), tempNames);
      retVal = (std::equal(tempNames.begin(), tempNames.end(), arrayNames.begin()) == true ?
                1 : 0);
      }
    }
  else if(retVal == 0)
    {
    return false;
    }
  int tmp = retVal;
  this->Controller->AllReduce(&tmp, &retVal, 1, vtkCommunicator::MIN_OP);

  return (retVal != 0);
}

//---------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkPParticleTracerBase, Controller, vtkMultiProcessController);
//---------------------------------------------------------------------------
