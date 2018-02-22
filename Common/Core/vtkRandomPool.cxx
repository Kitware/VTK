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
#include "vtkDataArray.h"
#include "vtkMersenneTwister.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkSMPTools.h"

vtkStandardNewMacro(vtkRandomPool);
vtkCxxSetObjectMacro(vtkRandomPool,Sequence,vtkRandomSequence);

//----------------------------------------------------------------------------
// Static method to populate a data array. This is done serially now, it could
// be threaded with vtkSMPTools but it may not be worth the overhead.
namespace {

template <typename T>
struct PopulateDA
{
  const double *Pool;
  T *Array;
  T Min;
  T Max;
  int NumComp;
  int CompNum;
  vtkIdType Size;
  vtkIdType TotalSize;

  PopulateDA(const double *pool, T *array, double min, double max,
             vtkIdType size, int numComp, int compNum) :
    Pool(pool), Array(array), Size(size), NumComp(numComp), CompNum(compNum)
  {
    this->Min = static_cast<T>(min);
    this->Max = static_cast<T>(max);
    this->TotalSize = this->Size * this->NumComp;

  }

  void Initialize()
  {
  }

  void operator() (vtkIdType ptId, vtkIdType endPtId)
  {
    for ( ; ptId < endPtId; ++ptId )
    {
    }
  }

  void Reduce()
  {
  }

  static void Execute(const double *pool, T *array, double min, double max,
                      vtkIdType size, int numComp, int compNum)
  {
    PopulateDA popDA(pool,array,min,max,size,numComp,compNum);
    vtkSMPTools::For(0,size, popDA);
  }
};

} //anonymous namespace

// ----------------------------------------------------------------------------
vtkRandomPool::vtkRandomPool()
{
  this->Sequence = vtkMersenneTwister::New();
  this->PoolSize = 0;
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
  delete [] this->Pool;
}

//----------------------------------------------------------------------------
void vtkRandomPool::
PopulateDataArray(vtkDataArray *da, int compNum,
                  double minRange, double maxRange)
{
  if ( da == nullptr )
  {
    vtkWarningMacro(<<"Bad request");
    return;
  }

  vtkIdType size = da->GetNumberOfTuples();
  int numComp = da->GetNumberOfComponents();
  compNum = (compNum < 0 ? 0 : (compNum >= numComp ? numComp-1 : compNum));

  this->SetPoolSize(size);
  this->SetNumberOfComponents(numComp);
  const double *pool = this->GetPool();

  void *ptr = da->GetVoidPointer(0);
  switch (da->GetDataType())
  {
    vtkTemplateMacro(PopulateDA<VTK_TT>::
                     Execute(pool, (VTK_TT *)ptr, minRange, maxRange,
                             size, numComp, compNum));
  }
}

//----------------------------------------------------------------------------
// Support multithreading of sequence generation
struct vtkRandomPoolInfo
{
  vtkIdType NumThreads;
  vtkRandomSequence **Twister;
  double *Pool;
  vtkIdType SeqSize;
  vtkIdType SeqChunk;
  vtkRandomSequence *Sequence;

  vtkRandomPoolInfo(double *pool, vtkIdType seqSize, vtkIdType seqChunk,
                    vtkIdType numThreads, vtkRandomSequence *ranSeq) :
    NumThreads(numThreads), Pool(pool), SeqSize(seqSize), SeqChunk(seqChunk),
    Sequence(ranSeq)
  {
    this->Twister = new vtkRandomSequence* [numThreads];
    for (vtkIdType i=0; i < numThreads; ++i)
    {
      this->Twister[i] = ranSeq->NewInstance();
      this->Twister[i]->Initialize(i);
    }
  }

  ~vtkRandomPoolInfo()
  {
    for (vtkIdType i=0; i < this->NumThreads; ++i)
    {
      this->Twister[i]->Delete();
    }
    delete [] this->Twister;
  }

};

//----------------------------------------------------------------------------
// This is the multithreaded piece of random sequence generation.
static VTK_THREAD_RETURN_TYPE vtkRandomPool_ThreadedMethod( void *arg )
{
  // Grab input
  vtkRandomPoolInfo *info;
  int threadCount, threadId;

  threadId = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
  threadCount = ((vtkMultiThreader::ThreadInfo *)(arg))->NumberOfThreads;
  info = (vtkRandomPoolInfo *)
    (((vtkMultiThreader::ThreadInfo *)(arg))->UserData);

  // Generate subsequence and place into global sequence in correct spot
  vtkRandomSequence *twister = info->Twister[threadId];
  double *pool = info->Pool;
  vtkIdType i, start = threadId * info->SeqChunk;
  vtkIdType end = start + info->SeqChunk;
  end = ( end < info->SeqSize ? end : info->SeqSize );
  for ( i=start; i < end; ++i, twister->Next())
  {
    pool[i] = twister->GetValue();
  }

  return VTK_THREAD_RETURN_VALUE;
}


// ----------------------------------------------------------------------------
// May use threaded sequence generation if the length of the sequence is
// greater than a pre-defined work size.
void vtkRandomPool::
GeneratePool()
{
  // Return if generation has already occured
  if ( this->GenerateTime > this->MTime )
  {
    return;
  }

  // Check for valid input and correct if necessary
  this->TotalSize = this->PoolSize * this->NumberOfComponents;
  if ( this->TotalSize <= 0 || this->Sequence == nullptr )
  {
    vtkWarningMacro(<<"Bad pool size");
    this->PoolSize = this->TotalSize = 1000;
    this->NumberOfComponents = 1;
  }
  this->ChunkSize = ( this->ChunkSize < 1000 ? 1000 : this->ChunkSize );
  this->Pool = new double [this->TotalSize];

  // Control the number of threads spawned.
  vtkIdType seqSize = this->TotalSize;
  vtkIdType seqChunk = this->ChunkSize;
  vtkIdType numThreads = (seqSize / seqChunk) + 1;
  vtkRandomSequence *twister = this->Sequence;

  // Fast path don't spin up threads
  if ( numThreads == 1 )
  {
    twister->Initialize(31415);
    double *p = this->Pool;
    for ( vtkIdType i=0; i < seqSize; ++i, twister->Next() )
    {
      *p++ = twister->GetValue();
    }
  }

  // Otherwise spawn threads to fill in chunks of the sequence.
  else
  {
    vtkNew<vtkMultiThreader> threader;
    threader->SetNumberOfThreads(numThreads);
    vtkIdType actualThreads = threader->GetNumberOfThreads();
    if ( actualThreads < numThreads) //readjust work load
    {
      numThreads = actualThreads;
    }

    // Now distribute work
    vtkRandomPoolInfo info(this->Pool, seqSize, seqChunk, numThreads, this->Sequence);
    threader->SetSingleMethod( vtkRandomPool_ThreadedMethod, (void *)&info);
    threader->SingleMethodExecute();
  }//spawning threads
}

// ----------------------------------------------------------------------------
void vtkRandomPool::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sequence: " << this->Sequence << "\n";
  os << indent << "Pool Size: " << this->PoolSize << "\n";
  os << indent << "Number Of Components: " << this->NumberOfComponents << "\n";
  os << indent << "Chunk Size: " << this->ChunkSize << "\n";

}
