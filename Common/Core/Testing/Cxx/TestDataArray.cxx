#include "vtkIntArray.h"
#include "vtkDoubleArray.h"

// Define this to run benchmarking tests on some vtkDataArray methods:
#undef BENCHMARK
// #define BENCHMARK

#ifdef BENCHMARK
#include "vtkTimerLog.h"
#include "vtkIdList.h"
#include "vtkNew.h"

#include <iostream>
#include <map>
#include <string>

namespace TestDataArrayPrivate {
typedef std::map<std::string, double> LogType;
LogType log;
const int numBenchmarks = 50;
void insertTimeLog(const std::string &str, double time);
void printTimeLog();
void benchmark();
} // End TestDataArrayPrivate namespace
#endif // BENCHMARK

int TestDataArray(int,char *[])
{
#ifdef BENCHMARK
  for (int i = 0; i < TestDataArrayPrivate::numBenchmarks; ++i)
    {
    TestDataArrayPrivate::benchmark();
    }
  TestDataArrayPrivate::printTimeLog();
  return 0;
#endif // BENCHMARK

  double range[2];
  vtkIntArray* array = vtkIntArray::New();
  array->GetRange( range, 0 );
  if ( range[0] != VTK_DOUBLE_MAX || range[1] != VTK_DOUBLE_MIN )
    {
    cerr
      << "Getting range of empty array failed, min: "
      << range[0] << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
    }
  int cc;
  for ( cc = 0; cc < 10; cc ++ )
    {
    array->InsertNextTuple1(cc);
    }
  array->GetRange( range, 0 ); // Range is now 0-9. Used to check MTimes.
  array->RemoveFirstTuple();
  array->RemoveTuple(3);
  array->RemoveTuple(4);
  array->GetRange( range, 0 );
  if ( range[0] != 0 || range[1] != 9 )
    {
    cerr
      << "Getting range (" << range[0] << "-" << range[1]
      << ") of array not marked as modified caused recomputation of range!";
    array->Delete();
    return 1;
    }
  array->Modified(); // Now mark array so range gets recomputed
  array->GetRange( range, 0 );
  if ( range[0] != 1. || range[1] != 9. )
    {
    cerr
      << "Getting range of array {1,2,3,5,7,8,9} failed, min: "
      << range[0] << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
    }
  array->RemoveLastTuple();
  array->Modified();
  array->GetRange( range, 0 );
  if ( range[0] != 1. || range[1] != 8. )
    {
    cerr
      << "Getting range of array {1,2,3,5,7,8} failed, min: "
      << range[0] << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
    }
  int ca[] = { 1, 2, 3, 5, 7, 8 };
  cout << "Array:";
  for ( cc = 0; cc < array->GetNumberOfTuples(); ++cc )
    {
    if ( array->GetTuple1(cc) != ca[cc] )
      {
      cerr
        << "Problem with array: " << array->GetTuple1(cc)
        << " <> " << ca[cc] << endl;
      array->Delete();
      return 1;
      }
    cout << " " << array->GetTuple1(cc);
    }
  cout << endl;
  array->Delete();

  vtkDoubleArray* farray = vtkDoubleArray::New();
  farray->SetNumberOfComponents(3);
  for ( cc = 0; cc < 10; cc ++ )
    {
    farray->InsertNextTuple3( cc + 0.125, cc + 0.250, cc + 0.375);
    }
  farray->RemoveFirstTuple();
  farray->RemoveTuple(3);
  farray->RemoveTuple(4);
  farray->RemoveLastTuple();
  cout << "Array:";
  for ( cc = 0; cc < farray->GetNumberOfTuples(); ++cc )
    {
    double* fa = farray->GetTuple3(cc);
    double fc[3];
    fc[0] = ca[cc] + .125;
    fc[1] = ca[cc] + .250;
    fc[2] = ca[cc] + .375;
    for (int i = 0; i < 3; i++)
      {
      if (fa[i] != fc[i])
        {
        cerr << "Problem with array: " << fa[i] << " <> " << fc[i] << endl;
        farray->Delete();
        return 1;
        }
      }
    cout << " " << fa[0] << "," << fa[1] << "," << fa[2];
    }
  cout << endl;
  farray->Delete();
  return 0;
}

#ifdef BENCHMARK
namespace TestDataArrayPrivate {
void insertTimeLog(const std::string &str, double time)
{
  if (log.find(str) == log.end())
    {
    log[str] = 0.;
    }
  log[str] += time;
}

void printTimeLog()
{
  for (LogType::const_iterator it = log.begin(), itEnd = log.end(); it != itEnd;
       ++it)
    {
    std::cout << std::setw(35) << std::left << it->first + ": "
              << std::setw(0) << std::right
              << it->second / static_cast<double>(numBenchmarks) << std::endl;
    }
}

void benchmark()
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkDoubleArray> double1;
  vtkNew<vtkDoubleArray> double2;
  vtkNew<vtkDoubleArray> double3;
  vtkNew<vtkIntArray> int1;
  vtkNew<vtkIntArray> int2;
  vtkNew<vtkIntArray> int3;
  double time;

  double1->SetNumberOfComponents(4);
  double1->SetNumberOfTuples(2500000);

  for (vtkIdType i = 0; i < 10000000; ++i)
    {
    double1->SetValue(i, static_cast<double>(i));
    }

  // Deep copy, with/without conversions
  int1->Initialize();
  timer->StartTimer();
  int1->DeepCopy(double1.GetPointer());
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("deep copy 10M double --> int", time);

  double1->Initialize();
  timer->StartTimer();
  double1->DeepCopy(int1.GetPointer());
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("deep copy 10M int --> double", time);

  double2->Initialize();
  timer->StartTimer();
  double2->DeepCopy(double1.GetPointer());
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("deep copy 10M double --> double", time);

  int2->Initialize();
  timer->StartTimer();
  int2->DeepCopy(int1.GetPointer());
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("deep copy 10M int --> int", time);

  // Insert tuple
  double2->Initialize();
  timer->StartTimer();
  for (int i = 0; i < double1->GetNumberOfTuples(); ++i)
    {
    double2->InsertTuple(i, i, double1.GetPointer());
    }
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("insert tuple (double)", time);

  int2->Initialize();
  timer->StartTimer();
  for (int i = 0; i < int1->GetNumberOfTuples(); ++i)
    {
    int2->InsertTuple(i, i, int1.GetPointer());
    }
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("insert tuple (int)", time);

  // Insert next tuple
  double2->Initialize();
  timer->StartTimer();
  for (int i = 0; i < double1->GetNumberOfTuples(); ++i)
    {
    double2->InsertNextTuple(i, double1.GetPointer());
    }
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("insert next tuple (double)", time);

  int2->Initialize();
  timer->StartTimer();
  for (int i = 0; i < int1->GetNumberOfTuples(); ++i)
    {
    int2->InsertNextTuple(i, int1.GetPointer());
    }
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("insert next tuple (int)", time);


  // interpolation
  vtkNew<vtkIdList> ids;
  ids->InsertNextId(4);
  ids->InsertNextId(9);
  ids->InsertNextId(10000);
  ids->InsertNextId(100000);
  ids->InsertNextId(100500);
  ids->InsertNextId(314);
  double weights[6];
  std::fill(weights, weights + 6, 1.0 / 6.0);

  const int numInterps = 100000;
  double3->Initialize();
  timer->StartTimer();
  for (int i = 0; i < numInterps; ++i)
    {
    double3->InterpolateTuple(i, ids.GetPointer(), double1.GetPointer(),
                              weights);
    }
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("interpolate 6 tuples (double)", time);

  int3->Initialize();
  timer->StartTimer();
  for (int i = 0; i < numInterps; ++i)
    {
    int3->InterpolateTuple(i, ids.GetPointer(), int1.GetPointer(), weights);
    }
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("interpolate 6 tuples (int)", time);

  double3->Initialize();
  timer->StartTimer();
  for (int i = 0; i < numInterps; ++i)
    {
    double3->InterpolateTuple(i,
                              500, double1.GetPointer(),
                              700, double2.GetPointer(), 0.25);
    }
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("interpolate 2 arrays (double)", time);

  int3->Initialize();
  timer->StartTimer();
  for (int i = 0; i < numInterps; ++i)
    {
    int3->InterpolateTuple(i,
                           500, int1.GetPointer(),
                           700, int2.GetPointer(), 0.25);
    }
  timer->StopTimer();
  time = timer->GetElapsedTime();
  insertTimeLog("interpolate 2 arrays (int)", time);

  // GetTuples:
  const int numGetTuples = 100000;

  time = 0.;
  for (int i = 0; i < numGetTuples; ++i)
    {
    double3->Initialize();
    double3->SetNumberOfComponents(double1->GetNumberOfComponents());
    double3->SetNumberOfTuples(ids->GetNumberOfIds());
    timer->StartTimer();
    double1->GetTuples(ids.GetPointer(), double3.GetPointer());
    timer->StopTimer();
    time += timer->GetElapsedTime();
    }
  insertTimeLog("get tuples random access (double)", time);

  time = 0.;
  for (int i = 0; i < numGetTuples; ++i)
    {
    int3->Initialize();
    int3->SetNumberOfComponents(int1->GetNumberOfComponents());
    int3->SetNumberOfTuples(ids->GetNumberOfIds());
    timer->StartTimer();
    int1->GetTuples(ids.GetPointer(), int3.GetPointer());
    timer->StopTimer();
    time += timer->GetElapsedTime();
    }
  insertTimeLog("get tuples random access (int)", time);
}
} // end private namespace
#endif // BENCHMARK
