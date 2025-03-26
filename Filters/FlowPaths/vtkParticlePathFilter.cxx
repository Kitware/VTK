// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkParticlePathFilter.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkParticlePathFilter);

namespace
{
constexpr int TAG = 45902;

//------------------------------------------------------------------------------
void FillCellArrays(vtkCellArray* verts, vtkCellArray* lines,
  const std::map<vtkIdType, std::vector<vtkIdType>>& paths)
{
  using ArrayType64 = vtkCellArray::ArrayType64;
  vtkNew<ArrayType64> vertsConnectivity, vertsOffsets, linesConnectivity, linesOffsets;
  vertsOffsets->InsertNextValue(0);
  linesOffsets->InsertNextValue(0);
  vtkIdType nverts = 0, nlines = 0;

  for (const auto& pair : paths)
  {
    const auto& path = pair.second;
    auto insertNextCell = [&path](ArrayType64* connectivity, ArrayType64* offsets, vtkIdType& n) {
      for (vtkIdType id = n; id < n + static_cast<vtkIdType>(path.size()); ++id)
      {
        connectivity->InsertNextValue(id);
      }
      n += path.size();
      offsets->InsertNextValue(n);
    };

    if (path.size() == 1)
    {
      insertNextCell(vertsConnectivity, vertsOffsets, nverts);
    }
    else
    {
      insertNextCell(linesConnectivity, linesOffsets, nlines);
    }
  }

  verts->SetData(vertsOffsets, vertsConnectivity);
  lines->SetData(linesOffsets, linesConnectivity);
}
} // anonymous namespace

//------------------------------------------------------------------------------
int vtkParticlePathFilter::Initialize(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int retVal = this->Superclass::Initialize(request, inputVector, outputVector);

  this->Paths.clear();

  this->Points = vtkSmartPointer<vtkPointSet>::New();
  vtkNew<vtkPoints> points;
  this->Points->SetPoints(points);
  this->Points->GetPointData()->CopyAllocate(this->OutputPointData);

  return retVal;
}

//------------------------------------------------------------------------------
int vtkParticlePathFilter::Execute(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int retVal = this->Superclass::Execute(request, inputVector, outputVector);

  // First, for every particle that we receive, we need to ask the original rank for its Path data
  // so we can reconstruct the paths.
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    int myRank = this->Controller ? this->Controller->GetLocalProcessId() : 0;

    vtkIdType startId = this->Points->GetNumberOfPoints();
    vtkIdType endId = startId;

    // We map points using InjectedPointId
    vtkIdType nParticlesSentLocal = static_cast<vtkIdType>(this->MPIRecvList.size());
    std::vector<vtkIdType> particleRequests(nParticlesSentLocal);
    vtkIdType counter = 0;
    for (const auto& pair : this->MPIRecvList)
    {
      const auto& info = pair.second;
      particleRequests[counter++] = info.InjectedPointId;
    }

    std::vector<vtkIdType> allNumParticles(this->Controller->GetNumberOfProcesses());

    this->Controller->AllGather(&nParticlesSentLocal, allNumParticles.data(), 1);

    vtkIdType nParticlesSent = 0;
    std::vector<vtkIdType> offsets(this->Controller->GetNumberOfProcesses() + 1);
    offsets.front() = 0;

    for (std::size_t i = 0; i < allNumParticles.size(); ++i)
    {
      offsets[i + 1] = offsets[i] + allNumParticles[i];
      nParticlesSent += allNumParticles[i];
    }

    std::vector<vtkIdType> allParticleRequests(nParticlesSent);

    // We share with everyone the particles that we require. The processes owning the relevant Paths
    // data will know what we want and we can exchange data.
    this->Controller->AllGatherV(particleRequests.data(), allParticleRequests.data(),
      nParticlesSentLocal, allNumParticles.data(), offsets.data());

    std::vector<vtkNew<vtkIdList>> sendLists(this->Controller->GetNumberOfProcesses());
    counter = 0;
    int rank = 0;

    while (rank < this->Controller->GetNumberOfProcesses())
    {
      if (allParticleRequests.empty())
      {
        break;
      }
      if (rank == myRank)
      {
        // skipping ourselves
        counter = offsets[++rank];
        continue;
      }
      if (counter == offsets[rank + 1])
      {
        // we finished current rank, moving on
        ++rank;
        continue;
      }

      vtkIdType injectedPointId = allParticleRequests[counter];
      auto it = this->Paths.find(injectedPointId);
      auto& sendList = sendLists[rank];
      if (it != this->Paths.end())
      {
        for (vtkIdType pointId : it->second)
        {
          sendList->InsertNextId(pointId);
          this->UnusedIndices.push(pointId);
        }
        // don't forget to erase the path, we do not own it anymore.
        this->Paths.erase(it);
      }

      if (++counter == offsets[rank + 1])
      {
        // seems redundant, but we need to account for the case where
        // ranks have no data to share earlier. Here we actually move the counter forward.
        ++rank;
      }
    }

    for (rank = 0; rank < this->Controller->GetNumberOfProcesses(); ++rank)
    {
      // Let's construct a poly data to send with all the data needed to reconstruct the requested
      // paths.
      const auto& sendList = sendLists[rank];

      vtkNew<vtkPolyData> ps;
      vtkNew<vtkPoints> points;
      vtkPointData* pd = ps->GetPointData();

      points->SetNumberOfPoints(sendList->GetNumberOfIds());
      points->GetData()->InsertTuplesStartingAt(0, sendList, this->Points->GetPoints()->GetData());
      pd->CopyAllocate(this->Points->GetPointData(), sendList->GetNumberOfIds());
      pd->CopyData(this->Points->GetPointData(), sendList);
      ps->SetPoints(points);

      this->Controller->Send(ps, rank, TAG);
    }

    // We send points to other processe and receive new ones. We could replace the data from the
    // sent points by data from received points or / and current particles. We would need to keep
    // track of available slots in a container and prioritize flushing it.
    for (rank = 0; rank < this->Controller->GetNumberOfProcesses(); ++rank)
    {
      // Receiving the polydata containing the paths we requested.
      vtkNew<vtkPolyData> ps;
      this->Controller->Receive(ps, rank, TAG);

      if (!ps->GetNumberOfPoints())
      {
        continue;
      }

      auto injectedPointIdArray =
        vtkArrayDownCast<vtkIdTypeArray>(ps->GetPointData()->GetAbstractArray("InjectedPointId"));

      // We use injectedPointIdArray to map the received paths to the paths we hold locally.
      for (vtkIdType pointId = 0; pointId < ps->GetNumberOfPoints(); ++pointId)
      {
        vtkIdType id = [this, &endId] {
          if (this->UnusedIndices.empty())
          {
            return endId++;
          }
          vtkIdType result = this->UnusedIndices.top();
          this->UnusedIndices.pop();
          return result;
        }();
        vtkIdType injectedPointId = injectedPointIdArray->GetValue(pointId);
        this->Paths[injectedPointId].push_back(id);
      }

      vtkIdType recvNPoints = ps->GetNumberOfPoints();
      if (!recvNPoints)
      {
        continue;
      }
      if (!this->Points->GetPointData()->GetNumberOfTuples())
      {
        this->Points->GetPointData()->CopyAllocate(ps->GetPointData(), recvNPoints);
      }
      this->Points->GetPointData()->CopyData(ps->GetPointData(), startId, recvNPoints, 0);
      this->Points->GetPoints()->InsertPoints(startId, recvNPoints, 0, ps->GetPoints());

      startId = this->Points->GetNumberOfPoints();
    }
  }

  // From there on, we have all the past data for the paths for which we hold a currently living
  // particle. We just need to add the particles to the relevant path.
  vtkIdType startId = this->Points->GetNumberOfPoints();
  vtkIdType n = this->OutputCoordinates->GetNumberOfPoints();
  vtkIdType endId = startId;

  this->Points->GetPoints()->InsertPoints(startId, n, 0, this->OutputCoordinates);

  for (auto& particle : this->ParticleHistories)
  {
    this->Paths[particle.InjectedPointId].push_back(endId++);
  }

  this->Points->GetPointData()->CopyData(this->OutputPointData, startId, n, 0);

  return retVal;
}

//------------------------------------------------------------------------------
int vtkParticlePathFilter::Finalize(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  int retVal = this->Superclass::Finalize(request, inputVector, outputVector);

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  auto output = vtkPolyData::SafeDownCast(vtkDataObject::GetData(outInfo));
  output->Initialize();

  if (!this->Points->GetNumberOfPoints())
  {
    // Nothing to do in this process
    return retVal;
  }

  std::size_t nPoints = 0;
  for (auto& pair : this->Paths)
  {
    nPoints += pair.second.size();
  }

  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(nPoints);

  vtkNew<vtkIdList> mapping;
  mapping->SetNumberOfIds(points->GetNumberOfPoints());
  vtkIdType i = -1;

  // We create a mapping from the indexing in Points to the polydata we actually want to output.
  for (auto& pair : this->Paths)
  {
    const auto& path = pair.second;
    for (vtkIdType pointId : path)
    {
      mapping->SetId(++i, pointId);
    }
  }

  output->GetPointData()->CopyAllocate(this->Points->GetPointData(), mapping->GetNumberOfIds());
  output->GetPointData()->CopyData(this->Points->GetPointData(), mapping);
  points->GetData()->InsertTuplesStartingAt(0, mapping, this->Points->GetPoints()->GetData());
  output->SetPoints(points);

  vtkNew<vtkCellArray> verts, lines;

  FillCellArrays(verts, lines, this->Paths);

  output->SetVerts(verts);
  output->SetLines(lines);

  return retVal;
}

VTK_ABI_NAMESPACE_END
