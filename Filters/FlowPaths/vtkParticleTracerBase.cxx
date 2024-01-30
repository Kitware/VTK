// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkParticleTracerBase.h"

#include "vtkAbstractParticleWriter.h"
#include "vtkAppendDataSets.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLocatorStrategy.h"
#include "vtkClosestPointStrategy.h"
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSMPTools.h"
#include "vtkSignedCharArray.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTemporalInterpolatedVelocityField.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <mutex>
#ifdef DEBUGPARTICLETRACE
#define Assert(x) assert(x)
#define PRINT(x) cout << __LINE__ << ": " << x << endl;
#else
#define PRINT(x)
#define Assert(x)
#endif

// The 3D cell with the maximum number of points is VTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
#define VTK_MAXIMUM_NUMBER_OF_POINTS 216

VTK_ABI_NAMESPACE_BEGIN
const double vtkParticleTracerBase::Epsilon = 1.0E-12;

using namespace vtkParticleTracerBaseNamespace;
using IDStates = vtkTemporalInterpolatedVelocityField::IDStates;

//------------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkParticleTracerBase, ParticleWriter, vtkAbstractParticleWriter);
vtkCxxSetObjectMacro(vtkParticleTracerBase, Integrator, vtkInitialValueProblemSolver);
vtkCxxSetObjectMacro(vtkParticleTracerBase, Controller, vtkMultiProcessController);

vtkMultiProcessController* vtkParticleTracerBase::GetController()
{
  return this->Controller;
}

// this SetMacro is different than the regular vtkSetMacro
// because it resets the cache as well.
#define ParticleTracerSetMacro(name, type)                                                         \
  void vtkParticleTracerBase::Set##name(type _arg)                                                 \
  {                                                                                                \
    if (this->name == _arg)                                                                        \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
    this->name = _arg;                                                                             \
    this->Modified();                                                                              \
  }
ParticleTracerSetMacro(ComputeVorticity, bool);
ParticleTracerSetMacro(RotationScale, double);
ParticleTracerSetMacro(ForceReinjectionEveryNSteps, int);
ParticleTracerSetMacro(TerminalSpeed, double);
VTK_ABI_NAMESPACE_END

VTK_ABI_NAMESPACE_BEGIN
//------------------------------------------------------------------------------
vtkParticleTracerBase::vtkParticleTracerBase()
{
  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);

  this->ForceReinjectionEveryNSteps = 0;
  this->ReinjectionCounter = 0;
  this->AllFixedGeometry = 1;
  this->MeshOverTime = MeshOverTimeTypes::DIFFERENT;
  this->StaticSeeds = 0;
  this->ComputeVorticity = true;
  this->IgnorePipelineTime = 1;
  this->ParticleWriter = nullptr;
  this->ParticleFileName = nullptr;
  this->EnableParticleWriting = false;
  this->Integrator = nullptr;

  this->RotationScale = 1.0;
  this->MaximumError = 1.0e-6;
  this->TerminalSpeed = vtkParticleTracerBase::Epsilon;
  this->IntegrationStep = 0.5;

  this->Interpolator = vtkSmartPointer<vtkTemporalInterpolatedVelocityField>::New();
  this->SetNumberOfInputPorts(2);

#ifdef JB_H5PART_PARTICLE_OUTPUT
#ifdef _WIN32
  vtkDebugMacro(<< "Setting vtkH5PartWriter");
  vtkH5PartWriter* writer = vtkH5PartWriter::New();
#else
  vtkDebugMacro(<< "Setting vtkXMLParticleWriter");
  vtkXMLParticleWriter* writer = vtkXMLParticleWriter::New();
#endif
  this->SetParticleWriter(writer);
  writer->Delete();
#endif

  this->SetIntegratorType(RUNGE_KUTTA4);
  this->ForceSerialExecution = false;

  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//------------------------------------------------------------------------------
vtkParticleTracerBase::~vtkParticleTracerBase()
{
  this->SetParticleWriter(nullptr);
  this->SetParticleFileName(nullptr);

  this->CachedData[0] = nullptr;
  this->CachedData[1] = nullptr;

  this->SetIntegrator(nullptr);

  this->SetController(nullptr);
}

//------------------------------------------------------------------------------
int vtkParticleTracerBase::FillInputPortInformation(int port, vtkInformation* info)
{
  // port 0 must be a temporal collection of any type
  // the executive should put a temporal collection in when
  // we request multiple time steps.
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  }
  else if (port == 1)
  {
    info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::AddSourceConnection(vtkAlgorithmOutput* input)
{
  this->AddInputConnection(1, input);
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::RemoveAllSources()
{
  this->SetInputConnection(1, nullptr);
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::SetMeshOverTime(int meshOverTime)
{
  if (this->MeshOverTime !=
    (meshOverTime < DIFFERENT ? DIFFERENT
                              : (meshOverTime > SAME_TOPOLOGY ? SAME_TOPOLOGY : meshOverTime)))
  {
    this->MeshOverTime =
      (meshOverTime < DIFFERENT ? DIFFERENT
                                : (meshOverTime > SAME_TOPOLOGY ? SAME_TOPOLOGY : meshOverTime));
    this->Modified();
    // Needed since the value needs to be set at the same time.
    this->Interpolator->SetMeshOverTime(this->MeshOverTime);
  }
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::SetInterpolatorType(int interpolatorType)
{
  if (interpolatorType == INTERPOLATOR_WITH_CELL_LOCATOR)
  {
    // create an interpolator equipped with a cell locator (by default)
    vtkNew<vtkCellLocatorStrategy> strategy;
    this->Interpolator->SetFindCellStrategy(strategy);
  }
  else
  {
    // create an interpolator equipped with a point locator
    auto strategy = vtkSmartPointer<vtkClosestPointStrategy>::New();
    this->Interpolator->SetFindCellStrategy(strategy);
  }
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::SetInterpolatorTypeToDataSetPointLocator()
{
  this->SetInterpolatorType(static_cast<int>(INTERPOLATOR_WITH_DATASET_POINT_LOCATOR));
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::SetInterpolatorTypeToCellLocator()
{
  this->SetInterpolatorType(static_cast<int>(INTERPOLATOR_WITH_CELL_LOCATOR));
}

//------------------------------------------------------------------------------
int vtkParticleTracerBase::InitializeInterpolator()
{
  if (!this->CachedData[0] || !this->CachedData[1])
  {
    vtkErrorMacro("Missing data set to process.");
    return VTK_ERROR;
  }
  // When Multiblock arrays are processed, some may be empty
  // if the first is empty, we won't find the correct vector name
  // so scan until we get one
  vtkSmartPointer<vtkCompositeDataIterator> iterP;
  iterP.TakeReference(this->CachedData[0]->NewIterator());
  iterP->GoToFirstItem();
  const char* vecname = nullptr;
  while (!iterP->IsDoneWithTraversal())
  {
    vtkDataArray* vectors = this->GetInputArrayToProcess(0, iterP->GetCurrentDataObject());
    if (vectors)
    {
      vecname = vectors->GetName();
      break;
    }
    iterP->GoToNextItem();
  }
  if (!vecname)
  {
    vtkErrorMacro(<< "Couldn't find vector array " << vecname);
    return VTK_ERROR;
  }

  // set strategy if needed
  if (this->Interpolator->GetFindCellStrategy() == nullptr)
  {
    // cell locator is the default;
    this->SetInterpolatorTypeToCellLocator();
  }
  this->Interpolator->SelectVectors(vecname);

  vtkDebugMacro(<< "Interpolator using array " << vecname);
  int numValidInputBlocks[2] = { 0, 0 };
  int numTotalInputBlocks[2] = { 0, 0 };
  this->DataReferenceT[0] = this->DataReferenceT[1] = nullptr;
  for (int T = 0; T < 2; T++)
  {
    this->CachedBounds[T].clear();
    // iterate over all blocks of input and cache the bounds information
    // and determine fixed/dynamic mesh status.
    vtkSmartPointer<vtkCompositeDataIterator> anotherIterP;
    anotherIterP.TakeReference(this->CachedData[T]->NewIterator());
    for (anotherIterP->GoToFirstItem(); !anotherIterP->IsDoneWithTraversal();
         anotherIterP->GoToNextItem())
    {
      numTotalInputBlocks[T]++;
      vtkDataSet* inp = vtkDataSet::SafeDownCast(anotherIterP->GetCurrentDataObject());
      if (inp)
      {
        if (inp->GetNumberOfCells() == 0)
        {
          vtkDebugMacro("Skipping an empty dataset");
        }
        else if (!inp->GetPointData()->GetVectors(vecname) && inp->GetNumberOfPoints() > 0)
        {
          vtkDebugMacro("One of the input datasets has no velocity vector.");
        }
        else
        {
          // store the bounding boxes of each local dataset for faster 'point-in-dataset' testing
          bounds bbox;
          inp->GetBounds(&bbox.b[0]);
          this->CachedBounds[T].push_back(bbox);
          // add the dataset to the interpolator
          // We need 2 consecutive time steps. If T == 0, we used the cached one, if not, the
          // current one.
          this->Interpolator->AddDataSetAtTime(
            T, T ? this->GetCurrentTimeStep() : this->CachedTimeStep, inp);
          if (!this->DataReferenceT[T])
          {
            this->DataReferenceT[T] = inp;
          }
          numValidInputBlocks[T]++;
        }
      }
    }
  }
  if (numValidInputBlocks[0] == 0 || numValidInputBlocks[1] == 0)
  {
    vtkErrorMacro("Not enough inputs have been found. Can not execute."
      << numValidInputBlocks[0] << " " << numValidInputBlocks[1]);
    return VTK_ERROR;
  }
  if (numValidInputBlocks[0] != numValidInputBlocks[1] &&
    this->MeshOverTime != MeshOverTimeTypes::DIFFERENT)
  {
    vtkErrorMacro(
      "MeshOverTime is set to STATIC/LINEAR_INTERPOLATION/SAME_TOPOLOGY but the number of "
      "datasets is different between time steps "
      << numValidInputBlocks[0] << " " << numValidInputBlocks[1]);
  }
  vtkDebugMacro("Number of Valid input blocks is " << numValidInputBlocks[0] << " from "
                                                   << numTotalInputBlocks[0]);
  vtkDebugMacro("AllFixedGeometry " << this->AllFixedGeometry);

  // force optimizations if StaticMesh is set.
  this->AllFixedGeometry = this->MeshOverTime == MeshOverTimeTypes::STATIC;
  if (this->MeshOverTime == MeshOverTimeTypes::STATIC)
  {
    vtkDebugMacro("Static Mesh over time optimizations Forced ON");
  }

  this->Interpolator->Initialize(this->CachedData[0], this->CachedData[1]);

  return VTK_OK;
}

//------------------------------------------------------------------------------
std::vector<vtkDataSet*> vtkParticleTracerBase::GetSeedSources(vtkInformationVector* inputVector)
{
  std::vector<vtkDataSet*> seedSources;
  for (int idx = 0, max = inputVector->GetNumberOfInformationObjects(); idx < max; ++idx)
  {
    if (vtkInformation* inInfo = inputVector->GetInformationObject(idx))
    {
      auto datasets = vtkCompositeDataSet::GetDataSets(vtkDataObject::GetData(inInfo));
      seedSources.insert(seedSources.end(), datasets.begin(), datasets.end());
    }
  }
  return seedSources;
}

//------------------------------------------------------------------------------
bool vtkParticleTracerBase::InsideBounds(double point[])
{
  double delta[3] = { 0.0, 0.0, 0.0 };
  for (int t = 0; t < 2; ++t)
  {
    for (size_t i = 0; i < (this->CachedBounds[t].size()); ++i)
    {
      if (vtkMath::PointIsWithinBounds(point, &((this->CachedBounds[t])[i].b[0]), delta))
      {
        return true;
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::TestParticles(ParticleVector& candidates, ParticleVector& passed)
{
  std::vector<int> passedIndices;
  this->TestParticles(candidates, passedIndices);

  for (size_t i = 0; i < passedIndices.size(); i++)
  {
    passed.push_back(candidates[passedIndices[i]]);
  }
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::TestParticles(
  vtkParticleTracerBaseNamespace::ParticleVector& candidates, std::vector<int>& passed)
{
  int i = 0;
  for (ParticleIterator it = candidates.begin(); it != candidates.end(); ++it, ++i)
  {
    ParticleInformation& info = (*it);
    double* pos = &info.CurrentPosition.x[0];
    // if outside bounds, reject instantly
    if (this->InsideBounds(pos))
    {
      // since this is first test, avoid bad cache tests
      this->Interpolator->ClearCache();
      info.LocationState = this->Interpolator->TestPoint(pos);
      if (info.LocationState == IDStates::OUTSIDE_ALL /*|| location==IDStates::OUTSIDE_T0*/)
      {
        // can't really use this particle.
        vtkDebugMacro(<< "TestParticles rejected particle");
      }
      else
      {
        // get the cached ids and datasets from the TestPoint call
        this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
        passed.push_back(i);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::EnqueueParticleToAnotherProcess(ParticleInformation& info)
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() == 1)
  {
    return;
  }

  this->MPISendList.push_back(info);
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::AssignSeedsToProcessors(
  double time, vtkDataSet* source, ParticleVector& localSeedPoints)
{
  ParticleVector candidates;
  //
  // take points from the source object and create a particle list
  //
  vtkIdType numSeeds = source->GetNumberOfPoints();
  candidates.resize(numSeeds);
  auto sourceIds = vtkArrayDownCast<vtkSignedCharArray>(
    source->GetPointData()->GetAbstractArray("ParticleSourceId"));

  for (vtkIdType i = 0; i < numSeeds; i++)
  {
    ParticleInformation& info = candidates[i];
    memcpy(info.CurrentPosition.x, source->GetPoint(i), sizeof(double) * 3);
    info.CurrentPosition.x[3] = time;
    info.LocationState = 0;
    info.CachedCellId[0] = -1;
    info.CachedCellId[1] = -1;
    info.CachedDataSetId[0] = 0;
    info.CachedDataSetId[1] = 0;
    info.InjectedPointId = i;
    info.InjectedStepId = this->ReinjectionCounter;

    info.SourceID = sourceIds->GetValue(i);
    info.TimeStepAge = 0;
    info.rotation = 0.0;
    info.angularVel = 0.0;
    info.time = 0.0;
    info.age = 0.0;
    info.speed = 0.0;
    info.SimulationTime = this->GetCurrentTimeStep();
    info.ErrorCode = 0;
    info.PointId = -1;
  }

  if (!this->Controller || this->Controller->GetNumberOfProcesses() == 1)
  {
    // Gather all Seeds to all processors for classification
    this->TestParticles(candidates, localSeedPoints);
  }
  else
  {
    // Check all Seeds on all processors for classification
    std::vector<int> owningProcess(numSeeds, -1);
    int myRank = this->Controller->GetLocalProcessId();
    ParticleIterator it = candidates.begin();
    for (int i = 0; it != candidates.end(); ++it, ++i)
    {
      ParticleInformation& info = (*it);
      double* pos = info.CurrentPosition.x;
      // if outside bounds, reject instantly
      if (this->InsideBounds(pos))
      {
        // since this is first test, avoid bad cache tests
        this->GetInterpolator()->ClearCache();
        int searchResult = this->GetInterpolator()->TestPoint(pos);
        if (searchResult == IDStates::INSIDE_ALL || searchResult == IDStates::OUTSIDE_T0)
        {
          // this particle is in this process's domain for the latest time step
          owningProcess[i] = myRank;
        }
      }
    }
    std::vector<int> realOwningProcess(numSeeds);
    this->Controller->AllReduce(
      owningProcess.data(), realOwningProcess.data(), numSeeds, vtkCommunicator::MAX_OP);

    for (std::size_t i = 0; i < realOwningProcess.size(); ++i)
    {
      this->InjectedPointIdToProcessId.emplace(candidates[i].InjectedPointId, realOwningProcess[i]);
      if (realOwningProcess[i] == myRank)
      {
        localSeedPoints.push_back(candidates[i]);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool vtkParticleTracerBase::SendReceiveParticles(
  std::vector<vtkIdType>& newReceivedInjectedPointIds)
{
  int numParticles = static_cast<int>(this->MPISendList.size());

  std::vector<int> allNumParticles(this->Controller->GetNumberOfProcesses(), 0);
  // Broadcast and receive size to/from all other processes.
  this->Controller->AllGather(&numParticles, allNumParticles.data(), 1);

  // write the message
  const int typeSize = sizeof(ParticleInformation);

  vtkIdType messageSize = numParticles * typeSize;
  std::vector<char> sendMessage(messageSize, 0);
  for (int i = 0; i < numParticles; i++)
  {
    memcpy(&sendMessage[i * typeSize], &this->MPISendList[i], typeSize);
    auto it = this->MPIRecvList.find(this->MPISendList[i].InjectedPointId);
    if (it != this->MPIRecvList.end())
    {
      this->MPIRecvList.erase(it);
    }
  }

  std::vector<vtkIdType> messageLength(this->Controller->GetNumberOfProcesses(), 0);
  std::vector<vtkIdType> messageOffset(this->Controller->GetNumberOfProcesses() + 1, 0);
  int allMessageSize(0);
  int numAllParticles(0);
  for (int i = 0; i < this->Controller->GetNumberOfProcesses(); ++i)
  {
    numAllParticles += allNumParticles[i];
    messageLength[i] = allNumParticles[i] * typeSize;
    messageOffset[i] = allMessageSize;
    allMessageSize += messageLength[i];
  }
  messageOffset.back() = allMessageSize;

  // receive the message
  std::vector<char> recvMessage(allMessageSize, 0);
  this->Controller->AllGatherV(messageSize > 0 ? sendMessage.data() : nullptr,
    allMessageSize > 0 ? recvMessage.data() : nullptr, messageSize, messageLength.data(),
    messageOffset.data());

  int myRank = this->Controller->GetLocalProcessId();

  // owningProcess is used to make sure that particles that are sent aren't added
  // on multiple processes
  std::vector<vtkIdType> owningProcess(numAllParticles, -1);
  // we automatically ignore particles that we sent
  int ignoreBegin = messageOffset[myRank] / typeSize;
  int ignoreEnd = ignoreBegin + messageLength[myRank] / typeSize;
  for (int i = 0; i < numAllParticles; i++)
  {
    if (i < ignoreBegin || i >= ignoreEnd)
    {
      ParticleInformation tmpParticle;
      memcpy(&tmpParticle, &recvMessage[i * typeSize], typeSize);
      // since this is first test, avoid bad cache tests
      this->GetInterpolator()->ClearCache();
      int searchResult = this->GetInterpolator()->TestPoint(tmpParticle.CurrentPosition.x);
      if (searchResult == IDStates::INSIDE_ALL || searchResult == IDStates::OUTSIDE_T0)
      {
        // this particle is in this process's domain for the latest time step
        owningProcess[i] = myRank;
      }
    }
  }
  std::vector<vtkIdType> realOwningProcess(numAllParticles);
  if (numAllParticles)
  {
    this->Controller->AllReduce(
      owningProcess.data(), realOwningProcess.data(), numAllParticles, vtkCommunicator::MAX_OP);
  }

  // if any value in realOwningProcess array is not -1 then we know
  // that a particle was moved to another process and probably needs
  // to be integrated further
  bool particlesMoved = false; // assume no particles moved

  for (int i = 0; i < numParticles; ++i)
  {
    if (realOwningProcess[i] != -1)
    {
      particlesMoved = true;
    }
  }

  // owningProcess is used to make sure that particles that are sent aren't added
  // on multiple processes

  for (int i = 0; i < numAllParticles; i++)
  {
    if (realOwningProcess[i] == myRank)
    {
      ParticleInformation info;
      memcpy(&info, &recvMessage[i * typeSize], typeSize);
      this->MPIRecvList.emplace(info.InjectedPointId, info);
      newReceivedInjectedPointIds.push_back(info.InjectedPointId);
    }
  }

  this->MPISendList.clear();

  return particlesMoved;
}

//------------------------------------------------------------------------------
bool vtkParticleTracerBase::UpdateParticleListFromOtherProcesses()
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() == 1)
  {
    return false;
  }

  std::vector<vtkIdType> newReceivedInjectedPointIds;
  bool particlesMoved = this->SendReceiveParticles(newReceivedInjectedPointIds);

  for (vtkIdType injectedPointId : newReceivedInjectedPointIds)
  {
    auto& info = this->MPIRecvList.at(injectedPointId);
    info.PointId = -1;
    info.CachedDataSetId[0] = info.CachedDataSetId[1] = -1;
    info.CachedCellId[0] = info.CachedCellId[1] = -1;
    this->ParticleHistories.push_back(info);
  }

  return particlesMoved;
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::UpdateParticleList(ParticleVector& candidates)
{
  int numSeedsNew = static_cast<int>(candidates.size());
  for (int i = 0; i < numSeedsNew; i++)
  {
    // allocate a new particle on the list and get a reference to it
    this->ParticleHistories.push_back(candidates[i]);
  }
  vtkDebugMacro(<< "UpdateParticleList completed with " << this->NumberOfParticles()
                << " particles");
}

VTK_ABI_NAMESPACE_END
//------------------------------------------------------------------------------
namespace vtkParticleTracerBaseNamespace
{
VTK_ABI_NAMESPACE_BEGIN
struct ParticleTracerFunctor
{
  vtkParticleTracerBase* PT;
  double FromTime;
  bool Sequential;

  std::vector<std::list<ParticleInformation>::iterator> ParticleHistories;
  std::atomic<vtkIdType> ParticleCount;
  std::mutex EraseMutex;

  vtkSMPThreadLocal<vtkSmartPointer<vtkInitialValueProblemSolver>> TLIntegrator;
  vtkSMPThreadLocal<vtkSmartPointer<vtkTemporalInterpolatedVelocityField>> TLInterpolator;
  vtkSMPThreadLocal<vtkSmartPointer<vtkDoubleArray>> TLCellVectors;

  ParticleTracerFunctor(vtkParticleTracerBase* pt, double fromTime, bool sequential)
    : PT(pt)
    , FromTime(fromTime)
    , Sequential(sequential)
  {
    this->ParticleCount = 0;
    size_t particleSize = pt->ParticleHistories.size();
    // Copy the particle histories into a vector for O(1) access
    this->ParticleHistories.reserve(particleSize);
    for (auto it = pt->ParticleHistories.begin(), end = pt->ParticleHistories.end(); it != end;
         ++it)
    {
      this->ParticleHistories.push_back(it);
    }
    this->PT->ResizeArrays(static_cast<vtkIdType>(particleSize));
  }

  void Initialize()
  {
    // Some data members of the local output require per-thread initialization.
    auto& interpolator = this->TLInterpolator.Local();
    interpolator.TakeReference(this->PT->Interpolator->NewInstance());
    interpolator->CopyParameters(this->PT->Interpolator);
    auto& integrator = this->TLIntegrator.Local();
    integrator.TakeReference(this->PT->GetIntegrator()->NewInstance());
    integrator->SetFunctionSet(interpolator);
    auto& cellVectors = this->TLCellVectors.Local();
    cellVectors.TakeReference(vtkDoubleArray::New());

    if (this->PT->ComputeVorticity)
    {
      cellVectors->SetNumberOfComponents(3);
      cellVectors->Allocate(3 * VTK_CELL_SIZE);
    }
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& integrator = this->TLIntegrator.Local();
    auto& interpolator = this->TLInterpolator.Local();
    auto& cellVectors = this->TLCellVectors.Local();
    bool isFirst = this->Sequential || vtkSMPTools::GetSingleThread();

    for (vtkIdType i = begin; i < end; ++i)
    {
      if (isFirst)
      {
        this->PT->CheckAbort();
      }
      auto it = this->ParticleHistories[i];
      this->PT->IntegrateParticle(it, this->FromTime, PT->GetCurrentTimeStep(), integrator,
        interpolator, cellVectors, this->ParticleCount, this->EraseMutex, this->Sequential);
      if (this->PT->GetAbortExecute())
      {
        vtkErrorWithObjectMacro(this->PT, "Execute aborted");
        break;
      }
    }
  }

  void Reduce()
  {
    // squeeze possibly extra space
    this->PT->ResizeArrays(this->ParticleCount);
  }
};
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
void vtkParticleTracerBase::ResizeArrays(vtkIdType numTuples)
{
  // resize first so that if you already have data, you don't lose them
  this->OutputCoordinates->Resize(numTuples);
  this->ParticleCellsConnectivity->Resize(numTuples);
  for (int i = 0; i < this->OutputPointData->GetNumberOfArrays(); ++i)
  {
    this->OutputPointData->GetArray(i)->Resize(numTuples);
  }
  // set number number of tuples because resize does not do that
  this->OutputCoordinates->SetNumberOfPoints(numTuples);
  this->ParticleCellsConnectivity->SetNumberOfValues(numTuples);
  this->OutputPointData->SetNumberOfTuples(numTuples);
}

//------------------------------------------------------------------------------
int vtkParticleTracerBase::Initialize(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  std::vector<vtkDataSet*> inputs = vtkCompositeDataSet::GetDataSets<vtkDataSet>(input);

  if (inputs.empty())
  {
    vtkErrorMacro("Empty input");
    return 0;
  }

  // TODO DUPLICATE CODE FIXME
  if (auto composite = vtkCompositeDataSet::SafeDownCast(input))
  {
    this->CachedData[1]->ShallowCopy(composite);
  }
  else
  {
    auto pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
    pds->SetNumberOfPartitions(1);
    pds->SetPartition(0, input);
    this->CachedData[1] = pds;
  }

  this->OutputPointData->InterpolateAllocate(inputs.front()->GetPointData());

  this->ParticleAge->Initialize();
  this->InjectedPointIds->Initialize();
  this->InjectedStepIds->Initialize();
  this->ErrorCodeArray->Initialize();
  this->ParticleSourceIds->Initialize();
  this->ParticleIds->Initialize();

  // Setting up all the relevant arrays for the output
  this->ParticleAge->SetName("ParticleAge");
  this->InjectedPointIds->SetName("InjectedPointId");
  this->InjectedStepIds->SetName("InjectionStepId");
  this->ErrorCodeArray->SetName("ErrorCode");
  this->ParticleSourceIds->SetName("ParticleSourceId");
  this->ParticleIds->SetName("ParticleId");

  if (this->ComputeVorticity)
  {
    this->CellVectors->Initialize();
    this->ParticleVorticity->Initialize();

    this->CellVectors->SetName("CellVectors");
    this->CellVectors->SetNumberOfComponents(3);
    this->CellVectors->Allocate(3 * VTK_CELL_SIZE);
    this->ParticleVorticity->SetName("Vorticity");
    this->ParticleRotation->SetName("Rotation");
    this->ParticleAngularVel->SetName("AngularVelocity");
  }
  this->OutputPointData->AddArray(this->InjectedPointIds);
  this->OutputPointData->AddArray(this->InjectedStepIds);
  this->OutputPointData->AddArray(this->ErrorCodeArray);
  this->OutputPointData->AddArray(this->ParticleAge);
  this->OutputPointData->AddArray(this->ParticleIds);
  this->OutputPointData->AddArray(this->ParticleSourceIds);
  if (this->ComputeVorticity)
  {
    this->OutputPointData->AddArray(this->ParticleVorticity);
    this->OutputPointData->AddArray(this->ParticleRotation);
    this->OutputPointData->AddArray(this->ParticleAngularVel);
  }

  this->InitializeExtraPointDataArrays(this->OutputPointData);

  this->AddRestartSeeds(inputVector);

  this->ParticleHistories.clear();

  auto seedSources = this->GetSeedSources(inputVector[1]);

  // we gotta gather the seeds to all processes so each rank has the same representation of the
  // input seeds
  vtkNew<vtkAppendDataSets> appendSeedsSources;
  int i = -1;
  for (vtkDataSet* ds : seedSources)
  {
    vtkNew<vtkSignedCharArray> sourceIds;
    sourceIds->SetName("ParticleSourceId");
    sourceIds->SetNumberOfValues(ds->GetNumberOfPoints());
    sourceIds->FillValue(++i);
    ds->GetPointData()->AddArray(sourceIds);
    appendSeedsSources->AddInputData(ds);
  }
  appendSeedsSources->MergePointsOn();
  appendSeedsSources->Update();

  std::vector<vtkSmartPointer<vtkDataObject>> gatheredSeeds;

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    this->Controller->AllGather(appendSeedsSources->GetOutputDataObject(0), gatheredSeeds);

    vtkNew<vtkAppendDataSets> appendGatheredSeedSources;
    for (auto& ds : gatheredSeeds)
    {
      appendGatheredSeedSources->AddInputData(ds);
    }
    appendGatheredSeedSources->MergePointsOn();
    appendGatheredSeedSources->Update();

    this->Seeds = vtkSmartPointer<vtkDataSet>(
      vtkDataSet::SafeDownCast(appendGatheredSeedSources->GetOutputDataObject(0)));

    // The reader used by MPI converts vtkSignedCharArray to vtkCharArray. Putting it back together
    // so we stick with a vtkSignedCharArray
    vtkNew<vtkSignedCharArray> sourceIds;
    sourceIds->ShallowCopy(this->Seeds->GetPointData()->GetArray("ParticleSourceId"));
    this->Seeds->GetPointData()->AddArray(sourceIds);
  }
  else
  {
    this->Seeds = vtkSmartPointer<vtkDataSet>(
      vtkDataSet::SafeDownCast(appendSeedsSources->GetOutputDataObject(0)));
  }

  return 1;
}

//------------------------------------------------------------------------------
int vtkParticleTracerBase::Execute(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector*)
{
  if (this->GetCurrentTimeIndex() == 0)
  {
    this->CachedTimeStep = this->GetCurrentTimeStep();
  }

  this->MPIRecvList.clear();

  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  double currentTimeStep = this->GetCurrentTimeStep();
  std::swap(this->CachedData[0], this->CachedData[1]);

  if (auto composite = vtkCompositeDataSet::SafeDownCast(input))
  {
    this->CachedData[1]->ShallowCopy(composite);
  }
  else
  {
    auto pds = vtkSmartPointer<vtkPartitionedDataSet>::New();
    pds->SetNumberOfPartitions(1);
    pds->SetPartition(0, input);
    this->CachedData[1] = pds;
  }

  if (this->InitializeInterpolator() != VTK_OK)
  {
    vtkErrorMacro(<< "InitializeInterpolator failed");
    return 0;
  }

  vtkDebugMacro(<< "Finished allocating point arrays ");

  // Setup some variables
  vtkSmartPointer<vtkInitialValueProblemSolver> integrator;
  integrator.TakeReference(this->GetIntegrator()->NewInstance());
  integrator->SetFunctionSet(this->Interpolator);

  // Perform multiple passes. The number of passes is equal to one more than
  // the maximum times a particle gets migrated between processes.
  // FIXME Not sure if this is what we want? Aren't particles that were not moved across
  // processes being moved again?
  do
  {
    bool sequential = this->ForceSerialExecution || this->ParticleHistories.size() < 100;
    ParticleTracerFunctor particleTracerFunctor(this, this->CachedTimeStep, sequential);
    if (sequential)
    {
      particleTracerFunctor.Initialize();
      particleTracerFunctor(0, this->ParticleHistories.size());
      particleTracerFunctor.Reduce();
    }
    else
    {
      vtkSMPTools::For(0, this->ParticleHistories.size(), particleTracerFunctor);
    }

  } while (this->UpdateParticleListFromOtherProcesses());

  int currentTimeIndex = this->GetCurrentTimeIndex();

  // If we want to reinject seeds (vtkStreaklineFilter needs to do that),
  // we do it here.
  if (!currentTimeIndex ||
    (this->ForceReinjectionEveryNSteps > 0 &&
      !(currentTimeIndex % this->ForceReinjectionEveryNSteps)))
  {
    this->ReinjectionCounter = currentTimeIndex;

    ParticleVector localSeeds;
    this->AssignSeedsToProcessors(currentTimeStep, this->Seeds.GetPointer(), localSeeds);
    this->UpdateParticleList(localSeeds);

    this->ResizeArrays(this->ParticleHistories.size());
    vtkIdType counter = 0;
    for (auto itr = this->ParticleHistories.begin(); itr != this->ParticleHistories.end();
         ++itr, ++counter)
    {
      this->Interpolator->TestPoint(itr->CurrentPosition.x);
      this->Interpolator->GetLastGoodVelocity(itr->velocity);
      itr->speed = vtkMath::Norm(itr->velocity);
      itr->PointId = counter;
      this->SetParticle(*itr, this->Interpolator, this->CellVectors);
    }
  }

  // These hold reference to the inputs. Release them.
  this->DataReferenceT[0] = this->DataReferenceT[1] = nullptr;

  // save some locator building, by re-using them as time progresses
  this->Interpolator->AdvanceOneTimeStep();

  this->CachedTimeStep = this->GetCurrentTimeStep();

  return 1;
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::IntegrateParticle(ParticleListIterator& it, double currentTime,
  double targetTime, vtkInitialValueProblemSolver* integrator,
  vtkTemporalInterpolatedVelocityField* interpolator, vtkDoubleArray* cellVectors,
  std::atomic<vtkIdType>& particleCount, std::mutex& eraseMutex, bool sequential)
{
  double epsilon = (targetTime - currentTime) / 100.0;
  double point1[4], point2[4] = { 0.0, 0.0, 0.0, 0.0 };
  double minStep = 0, maxStep = 0;
  double stepWanted, stepTaken = 0.0;
  int subSteps = 0;

  ParticleInformation& info = *it;
  bool particleGood = true;

  info.ErrorCode = 0;

  // Get the Initial point {x,y,z,t}
  memcpy(point1, &info.CurrentPosition, sizeof(Position));

  if (currentTime == targetTime)
  {
    Assert(point1[3] == currenttime);
  }
  else
  {
    Assert(point1[3] >= (currenttime - epsilon) && point1[3] <= (targettime + epsilon));
    // begin interpolation between available time values, if the particle has
    // a cached cell ID and dataset - try to use it,
    if (this->AllFixedGeometry)
    {
      interpolator->SetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
    }
    else
    {
      interpolator->ClearCache();
    }

    double delT = (targetTime - currentTime) * this->IntegrationStep;
    epsilon = delT * 1E-3;

    while (point1[3] < (targetTime - epsilon))
    {
      // Here begin the real work
      double error = 0;

      // If, with the next step, propagation will be larger than
      // max, reduce it so that it is (approximately) equal to max.
      stepWanted = delT;
      if ((point1[3] + stepWanted) > targetTime)
      {
        stepWanted = targetTime - point1[3];
        maxStep = stepWanted;
      }

      // Calculate the next step using the integrator provided.
      // If the next point is out of bounds, send it to another process
      if (integrator->ComputeNextStep(point1, point2, point1[3], stepWanted, stepTaken, minStep,
            maxStep, this->MaximumError, error) != 0)
      {
        // if the particle is sent, remove it from the list
        info.ErrorCode = 1;
        if (!this->RetryWithPush(info, point1, delT, subSteps, interpolator))
        {
          if (sequential)
          {
            this->EnqueueParticleToAnotherProcess(info);
            this->ParticleHistories.erase(it);
          }
          else
          {
            const std::lock_guard<std::mutex> lock(eraseMutex);
            this->EnqueueParticleToAnotherProcess(info);
            this->ParticleHistories.erase(it);
          }
          particleGood = false;
          break;
        }
        else
        {
          // particle was not sent, retry saved it, so copy info back
          subSteps++;
          memcpy(point1, &info.CurrentPosition, sizeof(Position));
        }
      }
      else // success, increment position/time
      {
        subSteps++;

        // increment the particle time
        point2[3] = point1[3] + stepTaken;
        info.age += stepTaken;
        info.SimulationTime += stepTaken;

        // Point is valid. Insert it.
        memcpy(&info.CurrentPosition, point2, sizeof(Position));
        memcpy(point1, point2, sizeof(Position));
      }

      // If the solver is adaptive and the next time step (delT.Interval)
      // that the solver wants to use is smaller than minStep or larger
      // than maxStep, re-adjust it. This has to be done every step
      // because minStep and maxStep can change depending on the Cell
      // size (unless it is specified in time units)
      if (integrator->IsAdaptive())
      {
        // code removed. Put it back when this is stable
      }
    }

    if (particleGood)
    {
      // The integration succeeded, but check the computed final position
      // is actually inside the domain (the intermediate steps taken inside
      // the integrator were ok, but the final step may just pass out)
      // if it moves out, we can't interpolate scalars, so we must send it away
      info.LocationState = interpolator->TestPoint(info.CurrentPosition.x);
      if (info.LocationState == IDStates::OUTSIDE_ALL)
      {
        info.ErrorCode = 2;
        // if the particle is sent, remove it from the list
        if (sequential)
        {
          this->EnqueueParticleToAnotherProcess(info);
          this->ParticleHistories.erase(it);
        }
        else
        {
          const std::lock_guard<std::mutex> lock(eraseMutex);
          this->EnqueueParticleToAnotherProcess(info);
          this->ParticleHistories.erase(it);
        }
        particleGood = false;
      }
    }

    // Has this particle stagnated
    if (particleGood)
    {
      interpolator->GetLastGoodVelocity(info.velocity);
      info.speed = vtkMath::Norm(info.velocity);
      if (it->speed <= this->TerminalSpeed)
      {
        if (sequential)
        {
          this->ParticleHistories.erase(it);
        }
        else
        {
          const std::lock_guard<std::mutex> lock(eraseMutex);
          this->ParticleHistories.erase(it);
        }
        particleGood = false;
      }
    }
  }

  // We got this far without error :
  // Insert the point into the output
  // Create any new scalars and interpolate existing ones
  // Cache cell ids and datasets
  if (particleGood)
  {
    // store the last Cell Ids and dataset indices for next time particle is updated
    interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSetId);
    info.TimeStepAge += 1;
    info.PointId = particleCount++;
    // Now generate the output geometry and scalars
    this->SetParticle(info, interpolator, cellVectors);
  }
  else
  {
    interpolator->ClearCache();
  }
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ParticleWriter: " << this->ParticleWriter << endl;
  os << indent << "ParticleFileName: " << (this->ParticleFileName ? this->ParticleFileName : "None")
     << endl;
  os << indent << "ForceReinjectionEveryNSteps: " << this->ForceReinjectionEveryNSteps << endl;
  os << indent << "EnableParticleWriting: " << this->EnableParticleWriting << endl;
  os << indent << "IgnorePipelineTime: " << this->IgnorePipelineTime << endl;
  os << indent << "StaticSeeds: " << this->StaticSeeds << endl;
  os << indent << "MeshOverTime: ";
  switch (this->MeshOverTime)
  {
    case MeshOverTimeTypes::DIFFERENT:
      os << "DIFFERENT" << endl;
      break;
    case MeshOverTimeTypes::STATIC:
      os << "STATIC" << endl;
      break;
    case MeshOverTimeTypes::LINEAR_TRANSFORMATION:
      os << "LINEAR_TRANSFORMATION" << endl;
      break;
    case MeshOverTimeTypes::SAME_TOPOLOGY:
      os << "SAME_TOPOLOGY" << endl;
      break;
    default:
      os << "UNKNOWN" << endl;
      break;
  }
}

//------------------------------------------------------------------------------
bool vtkParticleTracerBase::ComputeDomainExitLocation(
  double pos[4], double p2[4], double intersection[4], vtkGenericCell* cell)
{
  double t, pcoords[3];
  int subId;
  if (cell->IntersectWithLine(pos, p2, 1E-3, t, intersection, pcoords, subId) == 0)
  {
    vtkDebugMacro(<< "No cell/domain exit was found");
    return false;
  }
  else
  {
    // We found an intersection on the edge of the cell.
    // Shift it by a small amount to ensure that it crosses over the edge
    // into the adjoining cell.
    for (int i = 0; i < 3; i++)
    {
      intersection[i] = pos[i] + (t + 0.01) * (p2[i] - pos[i]);
    }
    // intersection stored, compute T for intersection
    intersection[3] = pos[3] + (t + 0.01) * (p2[3] - pos[3]);
    return true;
  }
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::SetIntegratorType(int type)
{
  vtkInitialValueProblemSolver* ivp = nullptr;
  switch (type)
  {
    case RUNGE_KUTTA2:
      ivp = vtkRungeKutta2::New();
      break;
    case RUNGE_KUTTA4:
      ivp = vtkRungeKutta4::New();
      break;
    case RUNGE_KUTTA45:
      ivp = vtkRungeKutta45::New();
      break;
    default:
      vtkWarningMacro("Unrecognized integrator type. Keeping old one.");
      break;
  }
  if (ivp)
  {
    this->SetIntegrator(ivp);
    ivp->Delete();
  }
}

//------------------------------------------------------------------------------
int vtkParticleTracerBase::GetIntegratorType()
{
  if (!this->Integrator)
  {
    return NONE;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta2"))
  {
    return RUNGE_KUTTA2;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta4"))
  {
    return RUNGE_KUTTA4;
  }
  if (!strcmp(this->Integrator->GetClassName(), "vtkRungeKutta45"))
  {
    return RUNGE_KUTTA45;
  }
  return UNKNOWN;
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::CalculateVorticity(
  vtkGenericCell* cell, double pcoords[3], vtkDoubleArray* cellVectors, double vorticity[3])
{
  double* cellVel;
  double derivs[VTK_MAXIMUM_NUMBER_OF_POINTS * 3];

  cellVel = cellVectors->GetPointer(0);
  cell->Derivatives(0, pcoords, cellVel, 3, derivs);
  vorticity[0] = derivs[7] - derivs[5];
  vorticity[1] = derivs[2] - derivs[6];
  vorticity[2] = derivs[3] - derivs[1];
}

//------------------------------------------------------------------------------
unsigned int vtkParticleTracerBase::NumberOfParticles()
{
  return static_cast<unsigned int>(this->ParticleHistories.size());
}

//------------------------------------------------------------------------------
vtkTemporalInterpolatedVelocityField* vtkParticleTracerBase::GetInterpolator()
{
  return this->Interpolator;
}

//------------------------------------------------------------------------------
bool vtkParticleTracerBase::RetryWithPush(ParticleInformation& info, double* point1, double delT,
  int substeps, vtkTemporalInterpolatedVelocityField* interpolator)
{
  interpolator->ClearCache();

  info.LocationState = interpolator->TestPoint(point1);

  if (info.LocationState == IDStates::OUTSIDE_ALL)
  {
    // something is wrong, the particle has left the building completely
    // we can't get the last good velocity as it won't be valid
    // send the particle 'as is' and hope it lands in another process
    if (substeps > 0)
    {
      interpolator->GetLastGoodVelocity(info.velocity);
    }
    else
    {
      info.velocity[0] = info.velocity[1] = info.velocity[2] = 0.0;
    }
    info.ErrorCode = 3;
  }
  else if (info.LocationState == IDStates::OUTSIDE_T0)
  {
    // the particle left the volume but can be tested at T2, so use the velocity at T2
    interpolator->GetLastGoodVelocity(info.velocity);
    info.ErrorCode = 4;
  }
  else if (info.LocationState == IDStates::OUTSIDE_T1)
  {
    // the particle left the volume but can be tested at T1, so use the velocity at T1
    interpolator->GetLastGoodVelocity(info.velocity);
    info.ErrorCode = 5;
  }
  else
  {
    // The test returned INSIDE_ALL, so test failed near start of integration,
    interpolator->GetLastGoodVelocity(info.velocity);
  }

  // try adding a one increment push to the particle to get over a rotating/moving boundary
  for (int v = 0; v < 3; v++)
  {
    info.CurrentPosition.x[v] += info.velocity[v] * delT;
  }

  info.CurrentPosition.x[3] += delT;
  info.LocationState = interpolator->TestPoint(info.CurrentPosition.x);
  info.age += delT;
  info.SimulationTime += delT; // = this->GetCurrentTimeValue();

  if (info.LocationState != IDStates::OUTSIDE_ALL)
  {
    // a push helped the particle get back into a dataset,
    info.ErrorCode = 6;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
int vtkParticleTracerBase::Finalize(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
{
  return 1;
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::SetParticle(vtkParticleTracerBaseNamespace::ParticleInformation& info,
  vtkTemporalInterpolatedVelocityField* interpolator, vtkDoubleArray* cellVectors)
{
  const double* coord = info.CurrentPosition.x;
  vtkIdType particleId = info.PointId;
  this->OutputCoordinates->SetPoint(particleId, coord);
  // create the cell
  this->ParticleCellsConnectivity->SetValue(particleId, particleId);
  // set the easy scalars for this particle
  this->InjectedPointIds->SetValue(particleId, info.InjectedPointId);
  this->InjectedStepIds->SetValue(particleId, info.InjectedStepId);
  this->ErrorCodeArray->SetValue(particleId, info.ErrorCode);
  this->ParticleSourceIds->SetValue(particleId, info.SourceID);
  this->ParticleAge->SetValue(particleId, info.age);
  this->ParticleIds->SetValue(particleId, info.InjectedPointId);
  this->SetToExtraPointDataArrays(particleId, info);

  // Interpolate all existing point attributes
  // In principle we always integrate the particle until it reaches Time2
  // - so we don't need to do any interpolation of the scalars
  // between T0 and T1, just fetch the values
  // of the spatially interpolated scalars from T1.
  if (info.LocationState == IDStates::OUTSIDE_T1)
  {
    interpolator->InterpolatePoint(0, this->OutputPointData, particleId);
  }
  else
  {
    interpolator->InterpolatePoint(1, this->OutputPointData, particleId);
  }
  // Compute vorticity
  if (this->ComputeVorticity)
  {
    vtkGenericCell* cell(nullptr);
    double pcoords[3], vorticity[3], weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
    double rotation, omega;
    // have to use T0 if particle is out at T1, otherwise use T1
    if (info.LocationState == IDStates::OUTSIDE_T1)
    {
      interpolator->GetVorticityData(0, pcoords, weights, cell, cellVectors);
    }
    else
    {
      interpolator->GetVorticityData(1, pcoords, weights, cell, cellVectors);
    }

    this->CalculateVorticity(cell, pcoords, cellVectors, vorticity);
    this->ParticleVorticity->SetTuple(particleId, vorticity);
    // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
    if (info.speed != 0.0)
    {
      omega = vtkMath::Dot(vorticity, info.velocity);
      omega /= info.speed;
      omega *= this->RotationScale;
    }
    else
    {
      omega = 0.0;
    }
    this->ParticleAngularVel->SetValue(particleId, omega);
    if (particleId > 0)
    {
      rotation =
        info.rotation + (info.angularVel + omega) / 2 * (info.CurrentPosition.x[3] - info.time);
    }
    else
    {
      rotation = 0.0;
    }
    this->ParticleRotation->SetValue(particleId, rotation);
    info.rotation = rotation;
    info.angularVel = omega;
    info.time = info.CurrentPosition.x[3];
  }
}

//------------------------------------------------------------------------------
bool vtkParticleTracerBase::IsPointDataValid(vtkDataObject* input)
{
  if (!this->Controller || this->Controller->GetNumberOfProcesses() == 1)
  {
    if (vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input))
    {
      std::vector<std::string> arrayNames;
      return this->IsPointDataValid(cdInput, arrayNames);
    }
    // a single data set on a single process will always have consistent point data
    return true;
  }
  else
  {
    int retVal = 1;
    vtkMultiProcessStream stream;
    if (this->Controller->GetLocalProcessId() == 0)
    {
      std::vector<std::string> arrayNames;
      if (vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input))
      {
        retVal = (int)this->IsPointDataValid(cdInput, arrayNames);
      }
      else
      {
        this->GetPointDataArrayNames(vtkDataSet::SafeDownCast(input), arrayNames);
      }
      stream << retVal;
      // only need to send the array names to check if proc 0 has valid point data
      if (retVal == 1)
      {
        stream << (int)arrayNames.size();
        for (std::vector<std::string>::iterator it = arrayNames.begin(); it != arrayNames.end();
             ++it)
        {
          stream << *it;
        }
      }
    }
    this->Controller->Broadcast(stream, 0);
    if (this->Controller->GetLocalProcessId() != 0)
    {
      stream >> retVal;
      if (retVal == 0)
      {
        return false;
      }
      int numArrays;
      stream >> numArrays;
      std::vector<std::string> arrayNames(numArrays);
      for (int i = 0; i < numArrays; i++)
      {
        stream >> arrayNames[i];
      }
      std::vector<std::string> tempNames;
      if (vtkCompositeDataSet* cdInput = vtkCompositeDataSet::SafeDownCast(input))
      {
        retVal = (int)this->IsPointDataValid(cdInput, tempNames);
        if (retVal)
        {
          retVal = std::equal(tempNames.begin(), tempNames.end(), arrayNames.begin()) ? 1 : 0;
        }
      }
      else
      {
        this->GetPointDataArrayNames(vtkDataSet::SafeDownCast(input), tempNames);
        retVal = std::equal(tempNames.begin(), tempNames.end(), arrayNames.begin()) ? 1 : 0;
      }
    }
    else if (retVal == 0)
    {
      return false;
    }
    int tmp = retVal;
    this->Controller->AllReduce(&tmp, &retVal, 1, vtkCommunicator::MIN_OP);

    return (retVal != 0);
  }
}

//------------------------------------------------------------------------------
bool vtkParticleTracerBase::IsPointDataValid(
  vtkCompositeDataSet* input, std::vector<std::string>& arrayNames)
{
  arrayNames.clear();
  vtkCompositeDataIterator* iter = input->NewIterator();
  iter->SkipEmptyNodesOn();
  iter->GoToFirstItem();
  this->GetPointDataArrayNames(vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()), arrayNames);
  for (iter->GoToNextItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    std::vector<std::string> tempNames;
    this->GetPointDataArrayNames(vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()), tempNames);
    if (!std::equal(tempNames.begin(), tempNames.end(), arrayNames.begin()))
    {
      iter->Delete();
      return false;
    }
  }
  iter->Delete();
  return true;
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::GetPointDataArrayNames(
  vtkDataSet* input, std::vector<std::string>& names)
{
  if (input == nullptr)
  {
    names.clear();
    return;
  }
  names.resize(input->GetPointData()->GetNumberOfArrays());
  for (vtkIdType i = 0; i < input->GetPointData()->GetNumberOfArrays(); i++)
  {
    names[i] = input->GetPointData()->GetArrayName(i);
  }
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkParticleTracerBase::GetParticleAge(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkFloatArray>(pd->GetArray("ParticleAge"));
}

//------------------------------------------------------------------------------
vtkSignedCharArray* vtkParticleTracerBase::GetParticleSourceIds(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkSignedCharArray>(pd->GetArray("ParticleSourceId"));
}

//------------------------------------------------------------------------------
vtkIntArray* vtkParticleTracerBase::GetParticleIds(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkIntArray>(pd->GetArray("ParticleId"));
}

//------------------------------------------------------------------------------
vtkIntArray* vtkParticleTracerBase::GetInjectedPointIds(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkIntArray>(pd->GetArray("InjectedPointId"));
}

//------------------------------------------------------------------------------
vtkIntArray* vtkParticleTracerBase::GetInjectedStepIds(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkIntArray>(pd->GetArray("InjectionStepId"));
}

//------------------------------------------------------------------------------
vtkIntArray* vtkParticleTracerBase::GetErrorCodeArr(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkIntArray>(pd->GetArray("ErrorCode"));
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkParticleTracerBase::GetParticleVorticity(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkFloatArray>(pd->GetArray("Vorticity"));
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkParticleTracerBase::GetParticleRotation(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkFloatArray>(pd->GetArray("Rotation"));
}

//------------------------------------------------------------------------------
vtkFloatArray* vtkParticleTracerBase::GetParticleAngularVel(vtkPointData* pd)
{
  return vtkArrayDownCast<vtkFloatArray>(pd->GetArray("AngularVelocity"));
}

//------------------------------------------------------------------------------
void vtkParticleTracerBase::PrintParticleHistories()
{
  cout << "Particle id, ages: " << endl;
  for (ParticleListIterator itr = this->ParticleHistories.begin();
       itr != this->ParticleHistories.end(); itr++)
  {
    ParticleInformation& info(*itr);
    cout << info.InjectedPointId << " " << info.age << " " << endl;
  }
  cout << endl;
}
VTK_ABI_NAMESPACE_END
