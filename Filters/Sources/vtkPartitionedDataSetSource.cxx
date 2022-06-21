/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPartitionedDataSetSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPartitionedDataSetSource.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkParametricFunctionSource.h"
#include "vtkParametricKlein.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"

#include <algorithm>
#include <numeric>
#include <typeinfo>
#include <vector>

namespace
{

enum NUM_PARTITIONS : signed int
{
  MULTIPLE_PARTITIONS = -1,
  NO_PARTITIONS = 0
};

/*
 * Generate allocations for the given ranks taking in consideration that:
 *
 * - Some ranks might not accept any partitions
 * - Some ranks might accept a finite amount of partitions.
 * - Some ranks might accept a any multiplicity of partitions.
 *
 */
std::vector<int> GenerateAllocations(const std::vector<int>& allocs, const int numPartitions)
{
  std::vector<int> partsPerRank(allocs);

  const int partsAllocated = std::accumulate(allocs.begin(), allocs.end(), 0,
    [](int a, int b) { return (b == NUM_PARTITIONS::MULTIPLE_PARTITIONS) ? a : a + b; });

  const int partsToAlloc = std::max(numPartitions - partsAllocated, 0);

  if (partsToAlloc > 0)
  {
    // Make a vector with iters of partitions to be alloc
    std::vector<std::vector<int>::iterator> ranksToAllocIters;
    auto it = partsPerRank.begin();
    while (partsPerRank.end() !=
      (it = find(partsPerRank.begin(), partsPerRank.end(), NUM_PARTITIONS::MULTIPLE_PARTITIONS)))
    {
      *it = 0; // Initialize to 0
      ranksToAllocIters.push_back(it);
    }

    // Schedule blocks in a round-robin fashion
    const size_t ranksToAllocSize = ranksToAllocIters.size();
    for (size_t i = 0; i < static_cast<size_t>(partsToAlloc); ++i)
    {
      ++(*ranksToAllocIters[i % ranksToAllocSize]);
    }
  }

  if (std::accumulate(partsPerRank.begin(), partsPerRank.end(), 0) != numPartitions)
  {
    vtkLogF(ERROR, "GenerateAllocations generated partitions != given numPartitions");
  }

  return partsPerRank;
}

// returns [start, end].
std::pair<int, int> GetRange(const int rank, const std::vector<int>& parts)
{
  std::pair<int, int> result(0, parts[0]);
  if (rank == 0)
  {
    return result;
  }
  for (int cc = 1; cc <= rank; ++cc)
  {
    result.first = result.second;
    result.second += parts[cc];
  }
  return result;
}

}

vtkStandardNewMacro(vtkPartitionedDataSetSource);
vtkCxxSetObjectMacro(vtkPartitionedDataSetSource, ParametricFunction, vtkParametricFunction);

//----------------------------------------------------------------------------
vtkPartitionedDataSetSource::vtkPartitionedDataSetSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // Default Parametric Function
  vtkNew<vtkParametricKlein> pklein;
  this->SetParametricFunction(pklein.Get());
}

//----------------------------------------------------------------------------
vtkPartitionedDataSetSource::~vtkPartitionedDataSetSource()
{
  this->SetParametricFunction(nullptr);
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetSource::EnableRank(int rank)
{
  auto it = this->Allocations.find(rank);
  if (it == this->Allocations.end() || it->second != NUM_PARTITIONS::MULTIPLE_PARTITIONS)
  {
    this->Allocations[rank] = NUM_PARTITIONS::MULTIPLE_PARTITIONS;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetSource::DisableRank(int rank)
{
  auto it = this->Allocations.find(rank);
  if (it == this->Allocations.end() || it->second != NUM_PARTITIONS::NO_PARTITIONS)
  {
    this->Allocations[rank] = NUM_PARTITIONS::NO_PARTITIONS;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetSource::EnableAllRanks()
{
  if (!this->RanksEnabledByDefault)
  {
    this->RanksEnabledByDefault = true;
    this->Modified();
  }

  if (!this->Allocations.empty())
  {
    this->Allocations.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetSource::DisableAllRanks()
{
  if (this->RanksEnabledByDefault)
  {
    this->RanksEnabledByDefault = false;
    this->Modified();
  }

  if (!this->Allocations.empty())
  {
    this->Allocations.clear();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkPartitionedDataSetSource::IsEnabledRank(int rank)
{
  auto it = this->Allocations.find(rank);

  // By default return the default partitioning policy
  if (it == this->Allocations.end())
  {
    return RanksEnabledByDefault;
  }

  return it->second == NUM_PARTITIONS::MULTIPLE_PARTITIONS;
}

//----------------------------------------------------------------------------
void vtkPartitionedDataSetSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ParametricFunction: "
     << (this->ParametricFunction ? this->ParametricFunction->GetClassName() : "(nullptr)") << endl;
}

//------------------------------------------------------------------------------
int vtkPartitionedDataSetSource::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  auto outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPartitionedDataSetSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  // We must meet these preconditions to continue this method
  if (!this->GetParametricFunction())
  {
    vtkLogF(WARNING, "RequestData aborted since ParametricFunction is missing");
    return 1;
  }

  if (!this->RanksEnabledByDefault && this->Allocations.empty())
  {
    return 1;
  }

  auto outInfo = outputVector->GetInformationObject(0);
  auto pds = vtkPartitionedDataSet::GetData(outInfo);
  const int rank = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  const int numRanks = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  auto* function = this->GetParametricFunction();
  function->JoinVOff();
  function->JoinUOff();

  vtkNew<vtkParametricFunctionSource> source;
  source->SetOutputPointsPrecision(vtkAlgorithm::DOUBLE_PRECISION);
  source->SetParametricFunction(function);
  source->SetScalarModeToV();

  // We set our default policy here
  auto defaultAlloc = (this->RanksEnabledByDefault) ? NUM_PARTITIONS::MULTIPLE_PARTITIONS
                                                    : NUM_PARTITIONS::NO_PARTITIONS;

  std::vector<int> allocs(numRanks, defaultAlloc);
  for (auto& kv : this->Allocations)
  {
    if (kv.first < static_cast<int>(allocs.size()))
    {
      allocs[kv.first] = kv.second;
    }
  }

  const int numberOfPartitions = (this->NumberOfPartitions > 0)
    ? this->NumberOfPartitions
    : std::count(allocs.begin(), allocs.end(), NUM_PARTITIONS::MULTIPLE_PARTITIONS);

  if (numberOfPartitions <= 0)
  {
    return 1;
  }

  const auto partsPerRank = ::GenerateAllocations(allocs, numberOfPartitions);
  const auto range = ::GetRange(rank, partsPerRank);

  const double deltaV = function->GetMaximumV() / numberOfPartitions;
  for (int idx = 0, partition = range.first; partition < range.second; ++partition, ++idx)
  {
    function->SetMinimumV(partition * deltaV);
    function->SetMaximumV((partition + 1) * deltaV);
    vtkLogF(TRACE, "min=%f max=%f", function->GetMinimumV(), function->GetMaximumV());

    source->Update();

    vtkNew<vtkPolyData> clone;
    clone->ShallowCopy(source->GetOutputDataObject(0));

    vtkNew<vtkIntArray> partId;
    partId->SetName("PartitionId");
    partId->SetNumberOfTuples(clone->GetNumberOfPoints());
    partId->FillValue(partition);
    clone->GetPointData()->AddArray(partId);
    pds->SetPartition(idx, clone);
  }

  return 1;
}
