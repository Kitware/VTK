/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRandomPool.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkRandomPool.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkMath.h"
#include "vtkMersenneTwister.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkMultiThreader.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"

#include <algorithm>
#include <cassert>

vtkStandardNewMacro(vtkRandomPool);
vtkCxxSetObjectMacro(vtkRandomPool, Sequence, vtkRandomSequence);

//----------------------------------------------------------------------------
// Static methods to populate a data array.
namespace
{

// This method scales all components between (min,max)
template <typename ArrayT>
struct PopulateDA
{
  using T = vtk::GetAPIType<ArrayT>;
  const double* Pool;
  ArrayT* Array;
  T Min;
  T Max;

  PopulateDA(const double* pool, ArrayT* array, double min, double max)
    : Pool(pool)
    , Array(array)
  {
    this->Min = static_cast<T>(min);
    this->Max = static_cast<T>(max);
  }

  void Initialize() {}

  void operator()(vtkIdType dataId, vtkIdType endDataId)
  {
    const double* pool = this->Pool + dataId;
    const double* poolEnd = this->Pool + endDataId;
    const double range = static_cast<double>(this->Max - this->Min);

    auto output = vtk::DataArrayValueRange(this->Array, dataId, endDataId);

    std::transform(pool, poolEnd, output.begin(),
      [&](const double p) -> T { return this->Min + static_cast<T>(p * range); });
  }

  void Reduce() {}
};

struct PopulateLauncher
{
  template <typename ArrayT>
  void operator()(ArrayT* array, const double* pool, double min, double max) const
  {
    PopulateDA<ArrayT> popDA{ pool, array, min, max };
    vtkSMPTools::For(0, array->GetNumberOfValues(), popDA);
  }
};

// This method scales a selected component between (min,max)
template <typename ArrayT>
struct PopulateDAComponent
{
  using T = vtk::GetAPIType<ArrayT>;

  const double* Pool;
  ArrayT* Array;
  int CompNum;
  T Min;
  T Max;

  PopulateDAComponent(const double* pool, ArrayT* array, double min, double max, int compNum)
    : Pool(pool)
    , Array(array)
    , CompNum(compNum)
  {
    this->Min = static_cast<T>(min);
    this->Max = static_cast<T>(max);
  }

  void Initialize() {}

  void operator()(vtkIdType tupleId, vtkIdType endTupleId)
  {
    const int numComp = this->Array->GetNumberOfComponents();
    const double range = static_cast<double>(this->Max - this->Min);

    const vtkIdType valueId = tupleId * numComp + this->CompNum;
    const vtkIdType endValueId = endTupleId * numComp;

    const double* poolIter = this->Pool + valueId;
    const double* poolEnd = this->Pool + endValueId;

    auto data = vtk::DataArrayValueRange(this->Array, valueId, endValueId);
    auto dataIter = data.begin();

    for (; poolIter < poolEnd; dataIter += numComp, poolIter += numComp)
    {
      *dataIter = this->Min + static_cast<T>(*poolIter * range);
    }
  }

  void Reduce() {}
};

struct PopulateDAComponentLauncher
{
  template <typename ArrayT>
  void operator()(ArrayT* array, const double* pool, double min, double max, int compNum)
  {
    PopulateDAComponent<ArrayT> popDAC{ pool, array, min, max, compNum };
    vtkSMPTools::For(0, array->GetNumberOfTuples(), popDAC);
  }
};

} // anonymous namespace

// ----------------------------------------------------------------------------
vtkRandomPool::vtkRandomPool()
{
  this->Sequence = vtkMinimalStandardRandomSequence::New();
  this->Size = 0;
  this->NumberOfComponents = 1;
  this->ChunkSize = 10000;

  this->TotalSize = 0;
  this->Pool = nullptr;

  // Ensure that the modified time > generate time
  this->GenerateTime.Modified();
  this->Modified();
}

// ----------------------------------------------------------------------------
vtkRandomPool::~vtkRandomPool()
{
  this->SetSequence(nullptr);
  delete[] this->Pool;
}

//----------------------------------------------------------------------------
void vtkRandomPool::PopulateDataArray(vtkDataArray* da, double minRange, double maxRange)
{
  if (da == nullptr)
  {
    vtkWarningMacro(<< "Bad request");
    return;
  }

  vtkIdType size = da->GetNumberOfTuples();
  int numComp = da->GetNumberOfComponents();

  this->SetSize(size);
  this->SetNumberOfComponents(numComp);
  const double* pool = this->GeneratePool();
  if (pool == nullptr)
  {
    return;
  }

  // Now perform the scaling of all components
  using Dispatcher = vtkArrayDispatch::Dispatch;
  PopulateLauncher worker;
  if (!Dispatcher::Execute(da, worker, pool, minRange, maxRange))
  { // Fallback for unknown array types:
    worker(da, pool, minRange, maxRange);
  }

  // Make sure that the data array is marked modified
  da->Modified();
}

//----------------------------------------------------------------------------
void vtkRandomPool::PopulateDataArray(
  vtkDataArray* da, int compNum, double minRange, double maxRange)
{
  if (da == nullptr)
  {
    vtkWarningMacro(<< "Bad request");
    return;
  }

  vtkIdType size = da->GetNumberOfTuples();
  int numComp = da->GetNumberOfComponents();
  compNum = (compNum < 0 ? 0 : (compNum >= numComp ? numComp - 1 : compNum));

  this->SetSize(size);
  this->SetNumberOfComponents(numComp);
  const double* pool = this->GeneratePool();
  if (pool == nullptr)
  {
    return;
  }

  // Now perform the scaling for one of the components
  using Dispatcher = vtkArrayDispatch::Dispatch;
  PopulateDAComponentLauncher worker;
  if (!Dispatcher::Execute(da, worker, pool, minRange, maxRange, compNum))
  { // fallback
    worker(da, pool, minRange, maxRange, compNum);
  }

  // Make sure that the data array is marked modified
  da->Modified();
}

//----------------------------------------------------------------------------
// Support multithreading of sequence generation
struct vtkRandomPoolInfo
{
  vtkIdType NumThreads;
  vtkRandomSequence** Sequencer;
  double* Pool;
  vtkIdType SeqSize;
  vtkIdType SeqChunk;
  vtkRandomSequence* Sequence;

  vtkRandomPoolInfo(double* pool, vtkIdType seqSize, vtkIdType seqChunk, vtkIdType numThreads,
    vtkRandomSequence* ranSeq)
    : NumThreads(numThreads)
    , Pool(pool)
    , SeqSize(seqSize)
    , SeqChunk(seqChunk)
    , Sequence(ranSeq)
  {
    this->Sequencer = new vtkRandomSequence*[numThreads];
    for (vtkIdType i = 0; i < numThreads; ++i)
    {
      this->Sequencer[i] = ranSeq->NewInstance();
      assert(this->Sequencer[i] != nullptr);
      this->Sequencer[i]->Initialize(static_cast<vtkTypeUInt32>(i));
    }
  }

  ~vtkRandomPoolInfo()
  {
    for (vtkIdType i = 0; i < this->NumThreads; ++i)
    {
      this->Sequencer[i]->Delete();
    }
    delete[] this->Sequencer;
  }
};

//----------------------------------------------------------------------------
// This is the multithreaded piece of random sequence generation.
static VTK_THREAD_RETURN_TYPE vtkRandomPool_ThreadedMethod(void* arg)
{
  // Grab input
  vtkRandomPoolInfo* info;
  int threadId;

  threadId = ((vtkMultiThreader::ThreadInfo*)(arg))->ThreadID;
  info = (vtkRandomPoolInfo*)(((vtkMultiThreader::ThreadInfo*)(arg))->UserData);

  // Generate subsequence and place into global sequence in correct spot
  vtkRandomSequence* sequencer = info->Sequencer[threadId];
  double* pool = info->Pool;
  vtkIdType i, start = threadId * info->SeqChunk;
  vtkIdType end = start + info->SeqChunk;
  end = (end < info->SeqSize ? end : info->SeqSize);
  for (i = start; i < end; ++i, sequencer->Next())
  {
    pool[i] = sequencer->GetValue();
  }

  return VTK_THREAD_RETURN_VALUE;
}

// ----------------------------------------------------------------------------
// May use threaded sequence generation if the length of the sequence is
// greater than a pre-defined work size.
const double* vtkRandomPool::GeneratePool()
{
  // Return if generation has already occurred
  if (this->GenerateTime > this->MTime)
  {
    return this->Pool;
  }

  // Check for valid input and correct if necessary
  this->TotalSize = this->Size * this->NumberOfComponents;
  if (this->TotalSize <= 0 || this->Sequence == nullptr)
  {
    vtkWarningMacro(<< "Bad pool size");
    this->Size = this->TotalSize = 1000;
    this->NumberOfComponents = 1;
  }
  this->ChunkSize = (this->ChunkSize < 1000 ? 1000 : this->ChunkSize);
  this->Pool = new double[this->TotalSize];

  // Control the number of threads spawned.
  vtkIdType seqSize = this->TotalSize;
  vtkIdType seqChunk = this->ChunkSize;
  vtkIdType numThreads = (seqSize / seqChunk) + 1;
  vtkRandomSequence* sequencer = this->Sequence;

  // Fast path don't spin up threads
  if (numThreads == 1)
  {
    sequencer->Initialize(31415);
    double* p = this->Pool;
    for (vtkIdType i = 0; i < seqSize; ++i, sequencer->Next())
    {
      *p++ = sequencer->GetValue();
    }
  }

  // Otherwise spawn threads to fill in chunks of the sequence.
  else
  {
    vtkNew<vtkMultiThreader> threader;
    threader->SetNumberOfThreads(numThreads);
    vtkIdType actualThreads = threader->GetNumberOfThreads();
    if (actualThreads < numThreads) // readjust work load
    {
      numThreads = actualThreads;
    }

    // Now distribute work
    vtkRandomPoolInfo info(this->Pool, seqSize, seqChunk, numThreads, this->Sequence);
    threader->SetSingleMethod(vtkRandomPool_ThreadedMethod, (void*)&info);
    threader->SingleMethodExecute();
  } // spawning threads

  // Update generation time
  this->GenerateTime.Modified();
  return this->Pool;
}

// ----------------------------------------------------------------------------
void vtkRandomPool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sequence: " << this->Sequence << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "Number Of Components: " << this->NumberOfComponents << "\n";
  os << indent << "Chunk Size: " << this->ChunkSize << "\n";
}
