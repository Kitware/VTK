// Tests the performance of prefix sum / exclusive scan.
#include "vtkIdTypeArray.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkNew.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkTimerLog.h"

#include <cassert>
#include <iostream>
#include <random> // random generation of values

// Use system <random> - create a simple convenience class. This generates
// random values [0,64).
struct SMPScanRandomValues
{
  std::mt19937 RNG;
  std::uniform_int_distribution<int> Dist;
  SMPScanRandomValues() { this->Dist.param(decltype(this->Dist)::param_type(0, 64)); }
  void Seed(vtkIdType s) { this->RNG.seed(s); }
  vtkIdType Next() { return this->Dist(RNG); }
};

int TestSMPScan(int, char*[])
{
  // Some default values
  int NVals = 20000;

  // Create an array in which inplace prefix sum will be computed.
  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfTuples(NVals);
  vtkIdType* offsetsPtr = offsets->GetPointer(0);

  // Populate the array with pseudo-random integral values
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  vtkSMPThreadLocal<SMPScanRandomValues> LocalGenerator;
  vtkSMPTools::For(0, NVals,
    [offsetsPtr, &LocalGenerator](vtkIdType id, vtkIdType endId)
    {
      for (; id < endId; ++id)
      {
        auto& localGen = LocalGenerator.Local();
        localGen.Seed(id);
        vtkIdType val = localGen.Next();
        offsetsPtr[id] = val;
      }
    }); // end lambda
  timer->StopTimer();
  double time = timer->GetElapsedTime();
  std::cout << "Time to generate values: " << time << "\n";

  // Copy the input for the SMP scan operation.
  vtkNew<vtkIdTypeArray> smpCopy;
  smpCopy->SetNumberOfTuples(NVals);
  smpCopy->DeepCopy(offsets);

  // Compute the prefix sum sequentially.
  timer->StartTimer();
  vtkIdType val, sum = 0;
  vtkIdType serRet = *(offsets->GetPointer(0) + NVals - 1);
  for (int id = 0; id < NVals; ++id)
  {
    val = offsetsPtr[id];
    offsetsPtr[id] = sum;
    sum += val;
  }
  serRet += *(offsets->GetPointer(0) + NVals - 1);
  timer->StopTimer();
  time = timer->GetElapsedTime();
  std::cout << "Time to sequentially sum values: " << time << "\n";

  // Test vtkSMPTools version
  timer->StartTimer();
  vtkIdType* smpPtr = smpCopy->GetPointer(0);
  vtkIdType smpRet = vtkSMPTools::ExclusiveScan(smpPtr, smpPtr + NVals, (vtkIdType)0);
  timer->StopTimer();
  time = timer->GetElapsedTime();

  std::cout << "Time to SMPTools scan: " << time << "\n";

  vtkIdType serSum = *(offsets->GetPointer(0) + NVals - 1);
  vtkIdType smpSum = *(smpCopy->GetPointer(0) + NVals - 1);
  std::cout << "Serial sum: " << serSum << endl;
  std::cout << "SMP sum: " << smpSum << endl;

  std::cout << "Serial return: " << serRet << endl;
  std::cout << "SMP return: " << smpRet << endl;

  assert(serSum == smpSum);
  assert(serRet == smpRet);

  return EXIT_SUCCESS;
}
