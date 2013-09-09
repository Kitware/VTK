#include "vtkAtomicInt32.h"
#include "vtkAtomicInt64.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkMultiThreader.h"

static int Total = 0;
static vtkTypeInt64 Total64 = 0;
static vtkAtomicInt32 TotalAtomic;
static vtkAtomicInt64 TotalAtomic64;
static const int Target = 1000000;
static int Values32[Target+2];
static int Values64[Target+2];
static int NumThreads = 5;

static vtkObject* AnObject;

VTK_THREAD_RETURN_TYPE MyFunction(void *)
{
  for (int i=0; i<Target/NumThreads; i++)
    {
    Total++;
    int idx = TotalAtomic.Increment();
    Values32[idx] = 1;

    Total64++;
    idx = TotalAtomic64.Increment();
    Values64[idx] = 1;

    //AnObject->Register(0);
    //AnObject->UnRegister(0);

    AnObject->Modified();
    }

  return VTK_THREAD_RETURN_VALUE;
}

VTK_THREAD_RETURN_TYPE MyFunction2(void *)
{
  for (int i=0; i<Target/NumThreads; i++)
    {
    TotalAtomic.Decrement();

    TotalAtomic64.Decrement();
    }

  return VTK_THREAD_RETURN_VALUE;
}

VTK_THREAD_RETURN_TYPE MyFunction3(void *)
{
  for (int i=0; i<Target/NumThreads; i++)
    {
    int idx = TotalAtomic.Add(1);
    Values32[idx]++;

    idx = TotalAtomic64.Add(1);
    Values64[idx]++;
    }

  return VTK_THREAD_RETURN_VALUE;
}

VTK_THREAD_RETURN_TYPE MyFunction4(void *)
{
  for (int i=0; i<Target/NumThreads; i++)
    {
    TotalAtomic.Increment();
    TotalAtomic.Add(1);
    TotalAtomic.Decrement();
    TotalAtomic.Subtract(1);

    TotalAtomic64.Increment();
    TotalAtomic64.Add(1);
    TotalAtomic64.Decrement();
    TotalAtomic64.Subtract(1);
    }

  return VTK_THREAD_RETURN_VALUE;
}

int TestAtomic(int, char*[])
{
  Total = 0;
  TotalAtomic.Set(0);
  Total64 = 0;
  TotalAtomic64.Set(0);

  AnObject = vtkObject::New();

  //cout << AnObject->GetReferenceCount() << endl;

  int beforeMTime = AnObject->GetMTime();

  for (int i=0; i<Target; i++)
    {
    Values32[i] = 0;
    Values64[i] = 0;
    }

  vtkNew<vtkMultiThreader> mt;
  mt->SetSingleMethod(MyFunction, NULL);
  mt->SetNumberOfThreads(NumThreads);
  mt->SingleMethodExecute();

  mt->SetSingleMethod(MyFunction2, NULL);
  mt->SingleMethodExecute();

  mt->SetSingleMethod(MyFunction3, NULL);
  mt->SingleMethodExecute();

  // Making sure that atomic incr returned unique
  // values each time. We expect all numbers from
  // 1 to Target-1 to be 2.
  if (Values32[0] != 0)
    {
      cout << "Expecting Values32[0] to be 0. Got "
           << Values32[0] << endl;
      return 1;
    }
  if (Values64[0] != 0)
    {
      cout << "Expecting Values64[0] to be 0. Got "
           << Values64[0] << endl;
      return 1;
    }
  for (int i=1; i<Target; i++)
    {
    if (Values32[i] != 2)
      {
      cout << "Expecting Values32[" << i << "] to be 2. Got "
           << Values32[i] << endl;
      return 1;
      }
    if (Values64[i] != 2)
      {
      cout << "Expecting Values64[" << i << "] to be 2. Got "
           << Values64[i] << endl;
      return 1;
      }
    }

  mt->SetSingleMethod(MyFunction4, NULL);
  mt->SingleMethodExecute();

  cout << Total << " " << TotalAtomic.Get() << endl;
  cout << Total64 << " " << TotalAtomic64.Get() << endl;

  //cout << AnObject->GetReferenceCount() << endl;

  cout << "MTime: " << AnObject->GetMTime() << endl;

  if (TotalAtomic.Get() != Target)
    {
    return 1;
    }

  if (TotalAtomic64.Get() != Target)
    {
    return 1;
    }

  if (AnObject->GetReferenceCount() != 1)
    {
    return 1;
    }

  if ((int)AnObject->GetMTime() != Target + beforeMTime + 2)
    {
    return 1;
    }

  AnObject->Delete();
  return 0;
}
