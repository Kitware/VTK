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
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkMultiThreader.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkSMPTools.h"

#include <cassert>

vtkStandardNewMacro(vtkRandomPool);
vtkCxxSetObjectMacro(vtkRandomPool,Sequence,vtkRandomSequence);

//----------------------------------------------------------------------------
// Static methods to populate a data array.
namespace {

// This method scales all components between (min,max)
template <typename T>
struct PopulateDA
{
  const double *Pool;
  T *Array;
  T Min;
  T Max;

  PopulateDA(const double *pool, T *array, double min, double max) :
    Pool(pool), Array(array)
  {
    this->Min = static_cast<T>(min);
    this->Max = static_cast<T>(max);
  }

  void Initialize()
  {
  }

  void operator() (vtkIdType dataId, vtkIdType endDataId)
  {
    const double *p = this->Pool + dataId;
    T *array = this->Array + dataId;
    double range = static_cast<double>(this->Max - this->Min);

    for ( ; dataId < endDataId; ++dataId, ++array, ++p )
    {
      *array = this->Min + static_cast<T>( *p * range );
    }
  }

  void Reduce()
  {
  }

  static void Execute(const double *pool, T *array, double min, double max,
                      vtkIdType totalSize)
  {
    PopulateDA popDA(pool,array,min,max);
    vtkSMPTools::For(0, totalSize, popDA);
  }
};

// This method scales a selected component between (min,max)
template <typename T>
struct PopulateDAComponent
{
  const double *Pool;
  T *Array;
  int NumComp;
  int CompNum;
  T Min;
  T Max;

  PopulateDAComponent(const double *pool, T *array, double min, double max,
                      int numComp, int compNum) :
    Pool(pool), Array(array), NumComp(numComp), CompNum(compNum)
  {
    this->Min = static_cast<T>(min);
    this->Max = static_cast<T>(max);
  }

  void Initialize()
  {
  }

  void operator() (vtkIdType tupleId, vtkIdType endTupleId)
  {
    int numComp = this->NumComp;
    const double *p = this->Pool + tupleId*numComp + this->CompNum;
    T *array = this->Array + tupleId*numComp + this->CompNum;
    double range = static_cast<double>(this->Max - this->Min);

    for ( ; tupleId < endTupleId; ++tupleId, array+=numComp, p+=numComp )
    {
      *array = this->Min + static_cast<T>( *p * range );
    }
  }

  void Reduce()
  {
  }

  static void Execute(const double *pool, T *array, double min, double max,
                      vtkIdType size, int numComp, int compNum)
  {
    PopulateDAComponent popDAC(pool,array,min,max,numComp,compNum);
    vtkSMPTools::For(0, size, popDAC);
  }
};

} //anonymous namespace

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
  delete [] this->Pool;
}

//----------------------------------------------------------------------------
void vtkRandomPool::
PopulateDataArray(vtkDataArray *da, double minRange, double maxRange)
{
  if ( da == nullptr )
  {
    vtkWarningMacro(<<"Bad request");
    return;
  }

  vtkIdType size = da->GetNumberOfTuples();
  int numComp = da->GetNumberOfComponents();

  this->SetSize(size);
  this->SetNumberOfComponents(numComp);
  const double *pool = this->GeneratePool();
  if ( pool == nullptr )
  {
    return;
  }

  // Now perform the scaling of all components
  void *ptr = da->GetVoidPointer(0);
  switch (da->GetDataType())
  {
    vtkTemplateMacro(PopulateDA<VTK_TT>::
                     Execute(pool, (VTK_TT *)ptr, minRange, maxRange,
                             this->GetTotalSize()));
  }

  // Make sure that the data array is marked modified
  da->Modified();
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

  this->SetSize(size);
  this->SetNumberOfComponents(numComp);
  const double *pool = this->GeneratePool();
  if ( pool == nullptr )
  {
    return;
  }

  // Now perform the scaling for one of the components
  void *ptr = da->GetVoidPointer(0);
  switch (da->GetDataType())
  {
    vtkTemplateMacro(PopulateDAComponent<VTK_TT>::
                     Execute(pool, (VTK_TT *)ptr, minRange, maxRange,
                             size, numComp, compNum));
  }

  // Make sure that the data array is marked modified
  da->Modified();
}

//----------------------------------------------------------------------------
// Support multithreading of sequence generation
struct vtkRandomPoolInfo
{
  vtkIdType NumThreads;
  vtkRandomSequence **Sequencer;
  double *Pool;
  vtkIdType SeqSize;
  vtkIdType SeqChunk;
  vtkRandomSequence *Sequence;

  vtkRandomPoolInfo(double *pool, vtkIdType seqSize, vtkIdType seqChunk,
                    vtkIdType numThreads, vtkRandomSequence *ranSeq) :
    NumThreads(numThreads), Pool(pool), SeqSize(seqSize), SeqChunk(seqChunk),
    Sequence(ranSeq)
  {
    this->Sequencer = new vtkRandomSequence* [numThreads];
    for (vtkIdType i=0; i < numThreads; ++i)
    {
      this->Sequencer[i] = ranSeq->NewInstance();
      assert(this->Sequencer[i] != nullptr);
      this->Sequencer[i]->Initialize(static_cast<vtkTypeUInt32>(i));
    }
  }

  ~vtkRandomPoolInfo()
  {
    for (vtkIdType i=0; i < this->NumThreads; ++i)
    {
      this->Sequencer[i]->Delete();
    }
    delete [] this->Sequencer;
  }

};

//----------------------------------------------------------------------------
// This is the multithreaded piece of random sequence generation.
static VTK_THREAD_RETURN_TYPE vtkRandomPool_ThreadedMethod( void *arg )
{
  // Grab input
  vtkRandomPoolInfo *info;
  int threadId;

  threadId = ((vtkMultiThreader::ThreadInfo *)(arg))->ThreadID;
  info = (vtkRandomPoolInfo *)
    (((vtkMultiThreader::ThreadInfo *)(arg))->UserData);

  // Generate subsequence and place into global sequence in correct spot
  vtkRandomSequence *sequencer = info->Sequencer[threadId];
  double *pool = info->Pool;
  vtkIdType i, start = threadId * info->SeqChunk;
  vtkIdType end = start + info->SeqChunk;
  end = ( end < info->SeqSize ? end : info->SeqSize );
  for ( i=start; i < end; ++i, sequencer->Next())
  {
    pool[i] = sequencer->GetValue();
  }

  return VTK_THREAD_RETURN_VALUE;
}


// ----------------------------------------------------------------------------
// May use threaded sequence generation if the length of the sequence is
// greater than a pre-defined work size.
const double * vtkRandomPool::
GeneratePool()
{
  // Return if generation has already occurred
  if ( this->GenerateTime > this->MTime )
  {
    return this->Pool;
  }

  // Check for valid input and correct if necessary
  this->TotalSize = this->Size * this->NumberOfComponents;
  if ( this->TotalSize <= 0 || this->Sequence == nullptr )
  {
    vtkWarningMacro(<<"Bad pool size");
    this->Size = this->TotalSize = 1000;
    this->NumberOfComponents = 1;
  }
  this->ChunkSize = ( this->ChunkSize < 1000 ? 1000 : this->ChunkSize );
  this->Pool = new double [this->TotalSize];

  // Control the number of threads spawned.
  vtkIdType seqSize = this->TotalSize;
  vtkIdType seqChunk = this->ChunkSize;
  vtkIdType numThreads = (seqSize / seqChunk) + 1;
  vtkRandomSequence *sequencer = this->Sequence;

  // Fast path don't spin up threads
  if ( numThreads == 1 )
  {
    sequencer->Initialize(31415);
    double *p = this->Pool;
    for ( vtkIdType i=0; i < seqSize; ++i, sequencer->Next() )
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
    if ( actualThreads < numThreads) //readjust work load
    {
      numThreads = actualThreads;
    }

    // Now distribute work
    vtkRandomPoolInfo info(this->Pool, seqSize, seqChunk, numThreads, this->Sequence);
    threader->SetSingleMethod( vtkRandomPool_ThreadedMethod, (void *)&info);
    threader->SingleMethodExecute();
  }//spawning threads

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
