/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkTemporalStreamTracer.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalStreamTracer.h"
#include "vtkPTemporalStreamTracer.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSmartPointer.h"
#include "vtkTemporalInterpolatedVelocityField.h"
#include "vtkOutputWindow.h"
#include "vtkAbstractParticleWriter.h"
#include "vtkToolkits.h"
#include <cassert>
#include "vtkMPIController.h"

using namespace vtkTemporalStreamTracerNamespace;

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkPTemporalStreamTracer);
vtkCxxSetObjectMacro(vtkPTemporalStreamTracer, Controller, vtkMultiProcessController);
//---------------------------------------------------------------------------
vtkPTemporalStreamTracer::vtkPTemporalStreamTracer()
{
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}
//---------------------------------------------------------------------------
vtkPTemporalStreamTracer::~vtkPTemporalStreamTracer()
{
  this->SetController(NULL);
  this->SetParticleWriter(NULL);
}
//---------------------------------------------------------------------------
void vtkPTemporalStreamTracer::AssignSeedsToProcessors(
  vtkDataSet *source, int sourceID, int ptId,
  ParticleVector &LocalSeedPoints, int &LocalAssignedCount)
{
  if(!this->Controller)
    {
    return Superclass::AssignSeedsToProcessors(source, sourceID, ptId,
                                               LocalSeedPoints, LocalAssignedCount);
    }

  ParticleVector candidates;
  //
  // take points from the source object and create a particle list
  //
  int numSeeds = source->GetNumberOfPoints();
#ifndef NDEBUG
  int numTested = numSeeds;
#endif
  candidates.resize(numSeeds);
  //
  for (int i=0; i<numSeeds; i++) {
    ParticleInformation &info = candidates[i];
    memcpy(&(info.CurrentPosition.x[0]), source->GetPoint(i), sizeof(double)*3);
    info.CurrentPosition.x[3] = this->CurrentTimeSteps[0];
    info.LocationState        = 0;
    info.CachedCellId[0]      =-1;
    info.CachedCellId[1]      =-1;
    info.CachedDataSetId[0]   = 0;
    info.CachedDataSetId[1]   = 0;
    info.SourceID             = sourceID;
    info.InjectedPointId      = i+ptId;
    info.InjectedStepId       = this->ReinjectionCounter;
    info.TimeStepAge          = 0;
    info.UniqueParticleId     =-1;
    info.rotation             = 0.0;
    info.angularVel           = 0.0;
    info.time                 = 0.0;
    info.age                  = 0.0;
    info.speed                = 0.0;
    info.ErrorCode            = 0;
  }
  //
  // Gather all Seeds to all processors for classification
  //
  // TODO : can we just use the same array here for send and receive
  ParticleVector allCandidates;
  if (this->UpdateNumPieces>1) {
    // Gather all seed particles to all processes
    this->TransmitReceiveParticles(candidates, allCandidates, false);
#ifndef NDEBUG
    numTested = static_cast<int>(allCandidates.size());
#endif
    vtkDebugMacro(<< "Local Particles " << numSeeds << " TransmitReceive Total " << numTested);
    // Test to see which ones belong to us
    this->TestParticles(allCandidates, LocalSeedPoints, LocalAssignedCount);
  }
  else {
#ifndef NDEBUG
    numTested = static_cast<int>(candidates.size());
#endif
    this->TestParticles(candidates, LocalSeedPoints, LocalAssignedCount);
  }
  int TotalAssigned = 0;
  this->Controller->Reduce(&LocalAssignedCount, &TotalAssigned, 1, vtkCommunicator::SUM_OP, 0);

  // Assign unique identifiers taking into account uneven distribution
  // across processes and seeds which were rejected
  this->AssignUniqueIds(LocalSeedPoints);
  //
  vtkDebugMacro(<< "Tested " << numTested << " LocallyAssigned " << LocalAssignedCount);
  if (this->UpdatePiece==0) {
    vtkDebugMacro(<< "Total Assigned to all processes " << TotalAssigned);
  }
}
//---------------------------------------------------------------------------
void vtkPTemporalStreamTracer::AssignUniqueIds(
  vtkTemporalStreamTracerNamespace::ParticleVector &LocalSeedPoints)
{
  if(!this->Controller)
    {
    return Superclass::AssignUniqueIds(LocalSeedPoints);
    }

  vtkIdType ParticleCountOffset = 0;
  vtkIdType numParticles = LocalSeedPoints.size();
  if (this->UpdateNumPieces>1) {
    vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
      this->Controller->GetCommunicator());
    if (com == 0) {
      vtkErrorMacro("MPICommunicator needed for this operation.");
      return;
    }
    // everyone starts with the master index
    com->Broadcast(&this->UniqueIdCounter, 1, 0);
//    vtkErrorMacro("UniqueIdCounter " << this->UniqueIdCounter);
    // setup arrays used by the AllGather call.
    std::vector<vtkIdType> recvNumParticles(this->UpdateNumPieces, 0);
    // Broadcast and receive count to/from all other processes.
    com->AllGather(&numParticles, &recvNumParticles[0], 1);
    // Each process is allocating a certain number.
    // start our indices from sum[0,this->UpdatePiece](numparticles)
    for (int i=0; i<this->UpdatePiece; ++i) {
      ParticleCountOffset += recvNumParticles[i];
    }
    for (vtkIdType i=0; i<numParticles; i++) {
      LocalSeedPoints[i].UniqueParticleId =
        this->UniqueIdCounter + ParticleCountOffset + i;
    }
    for (int i=0; i<this->UpdateNumPieces; ++i) {
      this->UniqueIdCounter += recvNumParticles[i];
    }
  }
  else {
    for (vtkIdType i=0; i<numParticles; i++) {
      LocalSeedPoints[i].UniqueParticleId =
        this->UniqueIdCounter + ParticleCountOffset + i;
    }
    this->UniqueIdCounter += numParticles;
  }
}
//---------------------------------------------------------------------------
void vtkPTemporalStreamTracer::TransmitReceiveParticles(
  ParticleVector &sending, ParticleVector &received, bool removeself)
{
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
    this->Controller->GetCommunicator());
  if (com == 0) {
    vtkErrorMacro("MPICommunicator needed for this operation.");
    return;
  }
  //
  // We must allocate buffers for all processor particles
  //
  vtkIdType OurParticles = sending.size();
  vtkIdType TotalParticles = 0;
  // setup arrays used by the AllGatherV call.
  std::vector<vtkIdType> recvLengths(this->UpdateNumPieces, 0);
  std::vector<vtkIdType> recvOffsets(this->UpdateNumPieces, 0);
  // Broadcast and receive size to/from all other processes.
  com->AllGather(&OurParticles, &recvLengths[0], 1);
  // Compute the displacements.
  const vtkIdType TypeSize = sizeof(ParticleInformation);
  for (int i=0; i<this->UpdateNumPieces; ++i)
  {
    //  << i << ": " << recvLengths[i] << "   ";
    recvOffsets[i] = TotalParticles*TypeSize;
    TotalParticles += recvLengths[i];
    recvLengths[i] *= TypeSize;
  }
  //  << '\n';
  // Allocate the space for all particles
  received.resize(TotalParticles);
  if (TotalParticles==0) return;
  // Gather the data from all procs.
  char *sendbuf = (char*) ((sending.size()>0) ? &(sending[0]) : NULL);
  char *recvbuf = (char*) (&(received[0]));
  com->AllGatherV(sendbuf, recvbuf,
    OurParticles*TypeSize, &recvLengths[0], &recvOffsets[0]);
  // Now all particles from all processors are in one big array
  // remove any from ourself that we have already tested
  if (removeself) {
    std::vector<ParticleInformation>::iterator first =
      received.begin() + recvOffsets[this->UpdatePiece]/TypeSize;
    std::vector<ParticleInformation>::iterator last =
      first + recvLengths[this->UpdatePiece]/TypeSize;
    received.erase(first, last);
  }
  if (received.size()>0) {
    TotalParticles=TotalParticles; // brkpnt
  }
}
//---------------------------------------------------------------------------
int vtkPTemporalStreamTracer::RequestData(
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
void vtkPTemporalStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Controller: " << this->Controller << endl;
}
//---------------------------------------------------------------------------
void vtkPTemporalStreamTracer::AddParticleToMPISendList(ParticleInformation &info)
{
  double eps = (this->CurrentTimeSteps[1]-this->CurrentTimeSteps[0])/100;
  if (info.CurrentPosition.x[3]<(this->CurrentTimeSteps[0]-eps) ||
      info.CurrentPosition.x[3]>(this->CurrentTimeSteps[1]+eps)) {
    vtkDebugMacro(<< "Unexpected time value in MPISendList - expected ("
      << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got "
      << info.CurrentPosition.x[3]);
  }
  if (this->MPISendList.capacity()<(this->MPISendList.size()+1)) {
    this->MPISendList.reserve(static_cast<int>(this->MPISendList.size()*1.5));
  }
  this->MPISendList.push_back(info);
}
