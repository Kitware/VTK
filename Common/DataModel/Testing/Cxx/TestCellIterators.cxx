/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCellIterators.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellIterator.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include <sstream>
#include <string>

// Enable/disable code that helps/hinders profiling.
#undef PROFILE
//#define PROFILE

// Enable benchmarks.
#undef BENCHMARK
//#define BENCHMARK

#ifdef BENCHMARK
#  ifdef PROFILE
#    define NUM_BENCHMARKS 10
#  else // PROFILE
#    define NUM_BENCHMARKS 100
#  endif // PROFILE
#endif // BENCHMARK

//------------------------------------------------------------------------------
// Compare the cell type, point ids, and points in 'grid' with those returned
// in 'iter'.
bool testCellIterator(vtkCellIterator *iter, vtkUnstructuredGrid *grid)
{
  vtkIdType cellId = 0;
  vtkNew<vtkGenericCell> cell;
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal())
    {
    grid->GetCell(cellId, cell.GetPointer());

    if (iter->GetCellType() != cell->GetCellType())
      {
      cerr << "Type mismatch for cell " << cellId << endl;
      return false;
      }

    vtkIdType numPoints = iter->GetNumberOfPoints();
    if (numPoints != cell->GetNumberOfPoints())
      {
      cerr << "Number of points mismatch for cell " << cellId << endl;
      return false;
      }

    for (vtkIdType pointInd = 0; pointInd < numPoints; ++pointInd)
      {
      if (iter->GetPointIds()->GetId(pointInd)
          != cell->PointIds->GetId(pointInd))
        {
        cerr << "Point id mismatch in cell " << cellId << endl;
        return false;
        }

      double iterPoint[3];
      double cellPoint[3];
      iter->GetPoints()->GetPoint(pointInd, iterPoint);
      cell->Points->GetPoint(pointInd, cellPoint);
      if (iterPoint[0] != cellPoint[0] ||
          iterPoint[1] != cellPoint[1] ||
          iterPoint[2] != cellPoint[2] )
        {
        cerr << "Point mismatch in cell " << cellId << endl;
        return false;
        }
      }

    iter->GoToNextCell();
    ++cellId;
    }

  // ensure that we checked all of the cells
  if (cellId != grid->GetNumberOfCells())
    {
    cerr << "Iterator did not cover all cells in the dataset!" << endl;
    return false;
    }

//  cout << "Verified " << cellId << " cells with a " << iter->GetClassName()
//       << "." << endl;
  return true;
}

#define TEST_ITERATOR(iter_, className_) \
  if (std::string(#className_) != std::string(iter->GetClassName())) \
    { \
    cerr << "Unexpected iterator type (expected " #className_ ", got " \
         << iter_->GetClassName() << ")" << endl; \
    return false; \
    } \
  \
  if (!testCellIterator(iter_, grid)) \
    { \
    cerr << #className_ << " test failed." << endl; \
    return false; \
    } \
  \
  if (!testCellIterator(iter_, grid)) \
    { \
    cerr << #className_ << " test failed after rewind." << endl; \
    return false; \
    } \


bool runValidation(vtkUnstructuredGrid *grid)
{
  // vtkDataSetCellIterator:
  vtkCellIterator *iter = grid->vtkDataSet::NewCellIterator();
  TEST_ITERATOR(iter, vtkDataSetCellIterator);
  iter->Delete();

  // vtkPointSetCellIterator:
  iter = grid->vtkPointSet::NewCellIterator();
  TEST_ITERATOR(iter, vtkPointSetCellIterator);
  iter->Delete();

  // vtkUnstructuredGridCellIterator:
  iter = grid->vtkUnstructuredGrid::NewCellIterator();
  TEST_ITERATOR(iter, vtkUnstructuredGridCellIterator);
  iter->Delete();

  return true;
}

// Do-nothing function that ensures arguments passed in will not be compiled
// out. Aggressive optimization will otherwise remove portions of the following
// loops, throwing off the benchmark results:
namespace {
std::stringstream _sink;
template <class Type>
void useData(const Type& data)
{
  _sink << data;
}
} // end anon namespace

// Benchmarking code follows:
#ifdef BENCHMARK
// There are three signatures for each benchmark function:
// - double ()(vtkUnstructuredGrid *)
//   Iterate through cells in an unstructured grid, using raw memory when
//   possible.
// - double ()(vtkUnstructuredGrid *, int)
//   Iterator through cells in an unstructured grid, using API only
// - double ()(vtkCellIterator *)
//   Iterator through all cells available through the iterator.
double benchmarkTypeIteration(vtkUnstructuredGrid *grid)
{
  vtkIdType numCells = grid->GetNumberOfCells();
  vtkUnsignedCharArray *types = grid->GetCellTypesArray();
  unsigned char *ptr = types->GetPointer(0);
  unsigned char range[2] = {VTK_UNSIGNED_CHAR_MAX, VTK_UNSIGNED_CHAR_MIN};

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (int i = 0; i < numCells; ++i)
    {
    range[0] = std::min(range[0], ptr[i]);
    range[1] = std::max(range[1], ptr[i]);
    }
  timer->StopTimer();

  useData(range[0]);
  useData(range[1]);

  return timer->GetElapsedTime();
}

double benchmarkTypeIteration(vtkUnstructuredGrid *grid, int)
{
  vtkIdType numCells = grid->GetNumberOfCells();
  unsigned char tmp;
  unsigned char range[2] = {VTK_UNSIGNED_CHAR_MAX, VTK_UNSIGNED_CHAR_MIN};

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (int i = 0; i < numCells; ++i)
    {
    tmp = static_cast<unsigned char>(grid->GetCellType(i));
    range[0] = std::min(range[0], tmp);
    range[1] = std::max(range[1], tmp);
    }
  timer->StopTimer();

  useData(range[0]);
  useData(range[1]);

  return timer->GetElapsedTime();
}

double benchmarkTypeIteration(vtkCellIterator *iter)
{
  int range[2] = {VTK_UNSIGNED_CHAR_MAX, VTK_UNSIGNED_CHAR_MIN};
  int tmp;

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (iter->InitTraversal(); iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
    tmp = iter->GetCellType();
    range[0] = std::min(range[0], tmp);
    range[1] = std::max(range[1], tmp);
    }
  timer->StopTimer();

  useData(range[0]);
  useData(range[1]);

  return timer->GetElapsedTime();
}

double benchmarkPointIdIteration(vtkUnstructuredGrid *grid)
{
  vtkCellArray *cellArray = grid->GetCells();
  vtkIdType numCells = cellArray->GetNumberOfCells();
  vtkIdType *cellPtr = cellArray->GetPointer();
  vtkIdType range[2] = {VTK_ID_MAX, VTK_ID_MIN};
  vtkIdType cellSize;

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
    cellSize = *(cellPtr++);
    for (vtkIdType pointIdx = 0; pointIdx < cellSize; ++pointIdx)
      {
      range[0] = std::min(range[0], cellPtr[pointIdx]);
      range[1] = std::max(range[1], cellPtr[pointIdx]);
      }
    cellPtr += cellSize;
    }
  timer->StopTimer();

  useData(range[0]);
  useData(range[1]);

  return timer->GetElapsedTime();
}

double benchmarkPointIdIteration(vtkUnstructuredGrid *grid, int)
{
  vtkIdType numCells = grid->GetNumberOfCells();
  vtkIdType range[2] = {VTK_ID_MAX, VTK_ID_MIN};
  vtkIdType cellSize;
  vtkIdList *cellPointIds = vtkIdList::New();
  vtkIdType *cellPtr;

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
    grid->GetCellPoints(cellId, cellPointIds);
    cellSize = cellPointIds->GetNumberOfIds();
    cellPtr = cellPointIds->GetPointer(0);
    for (vtkIdType pointIdx = 0; pointIdx < cellSize; ++pointIdx)
      {
      range[0] = std::min(range[0], cellPtr[pointIdx]);
      range[1] = std::max(range[1], cellPtr[pointIdx]);
      }
    }
  timer->StopTimer();

  useData(range[0]);
  useData(range[1]);

  cellPointIds->Delete();

  return timer->GetElapsedTime();
}

double benchmarkPointIdIteration(vtkCellIterator *iter)
{
  vtkIdType range[2] = {VTK_ID_MAX, VTK_ID_MIN};
  vtkIdType *cellPtr;
  vtkIdType *cellEnd;

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (iter->InitTraversal(); iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
    cellPtr = iter->GetPointIds()->GetPointer(0);
    cellEnd = cellPtr + iter->GetNumberOfPoints();
    while (cellPtr != cellEnd)
      {
      range[0] = std::min(range[0], *cellPtr);
      range[1] = std::max(range[1], *cellPtr);
      ++cellPtr;
      }
    }
  timer->StopTimer();

  useData(range[0]);
  useData(range[1]);

  return timer->GetElapsedTime();
}

double benchmarkPointsIteration(vtkUnstructuredGrid *grid)
{
  vtkCellArray *cellArray = grid->GetCells();
  const vtkIdType numCells = cellArray->GetNumberOfCells();
  vtkIdType *cellPtr = cellArray->GetPointer();
  vtkIdType cellSize;

  vtkPoints *points = grid->GetPoints();
  vtkFloatArray *pointDataArray = vtkFloatArray::SafeDownCast(points->GetData());
  if (!pointDataArray)
    {
    return -1.0;
    }
  float *pointData = pointDataArray->GetPointer(0);
  float *point;
  float dummy[3] = {0.f, 0.f, 0.f};

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
    cellSize = *(cellPtr++);
    for (vtkIdType pointIdx = 0; pointIdx < cellSize; ++pointIdx)
      {
      point = pointData + 3 * cellPtr[pointIdx];
      dummy[0] += point[0];
      dummy[1] += point[1];
      dummy[2] += point[2];
      }
    cellPtr += cellSize;
    }
  timer->StopTimer();

  useData(dummy[0]);
  useData(dummy[1]);
  useData(dummy[2]);

  return timer->GetElapsedTime();
}

double benchmarkPointsIteration(vtkUnstructuredGrid *grid, int)
{
  vtkIdList *pointIds = vtkIdList::New();
  vtkIdType cellSize;
  vtkIdType *cellPtr;

  vtkPoints *points = grid->GetPoints();
  double point[3];
  double dummy[3] = {0.f, 0.f, 0.f};

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  const vtkIdType numCells = grid->GetNumberOfCells();
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
    grid->GetCellPoints(cellId, pointIds);
    cellSize = pointIds->GetNumberOfIds();
    cellPtr = pointIds->GetPointer(0);
    for (vtkIdType pointIdx = 0; pointIdx < cellSize; ++pointIdx)
      {
      points->GetPoint(cellPtr[pointIdx], point);
      dummy[0] += point[0];
      dummy[1] += point[1];
      dummy[2] += point[2];
      }
    }
  timer->StopTimer();

  useData(dummy[0]);
  useData(dummy[1]);
  useData(dummy[2]);

  pointIds->Delete();

  return timer->GetElapsedTime();
}

double benchmarkPointsIteration(vtkCellIterator *iter)
{
  float dummy[3] = {0.f, 0.f, 0.f};

  // Ensure that the call to GetPoints() is at a valid cell:
  iter->InitTraversal();
  if (!iter->IsDoneWithTraversal())
    {
    return -1.0;
    }
  vtkFloatArray *pointArray =
      vtkFloatArray::SafeDownCast(iter->GetPoints()->GetData());
  float *pointsData;
  float *pointsDataEnd;

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (iter->InitTraversal(); iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
    pointsData = pointArray->GetPointer(0);
    pointsDataEnd = pointsData + iter->GetNumberOfPoints();
    while (pointsData < pointsDataEnd)
      {
      dummy[0] += *pointsData++;
      dummy[1] += *pointsData++;
      dummy[2] += *pointsData++;
      }
    }
  timer->StopTimer();

  useData(dummy[0]);
  useData(dummy[1]);
  useData(dummy[2]);

  return timer->GetElapsedTime();
}

double benchmarkCellIteration(vtkUnstructuredGrid *grid)
{
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkIdType numCells = grid->GetNumberOfCells();

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
    grid->GetCell(cellId, cell);
    }
  timer->StopTimer();
  cell->Delete();
  return timer->GetElapsedTime();
}

double benchmarkCellIteration(vtkUnstructuredGrid *grid, int)
{
  // No real difference here....
  return benchmarkCellIteration(grid);
}

double benchmarkCellIteration(vtkCellIterator *it)
{
  vtkGenericCell *cell = vtkGenericCell::New();

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (it->InitTraversal(); it->IsDoneWithTraversal(); it->GoToNextCell())
    {
    it->GetCell(cell);
    }
  timer->StopTimer();
  cell->Delete();
  return timer->GetElapsedTime();
}

double benchmarkPiecewiseIteration(vtkUnstructuredGrid *grid)
{
  // Setup for types:
  vtkUnsignedCharArray *typeArray = grid->GetCellTypesArray();
  unsigned char *typePtr = typeArray->GetPointer(0);
  unsigned char typeRange[2] = {VTK_UNSIGNED_CHAR_MAX, VTK_UNSIGNED_CHAR_MIN};

  // Setup for point ids:
  vtkCellArray *cellArray = grid->GetCells();
  vtkIdType *cellArrayPtr = cellArray->GetPointer();
  vtkIdType ptIdRange[2] = {VTK_ID_MAX, VTK_ID_MIN};
  vtkIdType cellSize;

  // Setup for points:
  vtkPoints *points = grid->GetPoints();
  vtkFloatArray *pointDataArray = vtkFloatArray::SafeDownCast(points->GetData());
  if (!pointDataArray)
    {
    return -1.0;
    }
  float *pointData = pointDataArray->GetPointer(0);
  float *point;
  float dummy[3] = {0.f, 0.f, 0.f};

  // Setup for cells
  vtkGenericCell *cell = vtkGenericCell::New();

  vtkIdType numCells = grid->GetNumberOfCells();
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (int i = 0; i < numCells; ++i)
    {
    // Types:
    typeRange[0] = std::min(typeRange[0], typePtr[i]);
    typeRange[1] = std::max(typeRange[1], typePtr[i]);

    cellSize = *(cellArrayPtr++);
    for (vtkIdType pointIdx = 0; pointIdx < cellSize; ++pointIdx)
      {
      // Point ids:
      ptIdRange[0] = std::min(ptIdRange[0], cellArrayPtr[pointIdx]);
      ptIdRange[1] = std::max(ptIdRange[1], cellArrayPtr[pointIdx]);

      // Points:
      point = pointData + 3 * cellArrayPtr[pointIdx];
      dummy[0] += point[0];
      dummy[1] += point[1];
      dummy[2] += point[2];
      }
    cellArrayPtr += cellSize;

    // Cell:
    grid->GetCell(i, cell);
    }
  timer->StopTimer();

  useData(typeRange[0]);
  useData(typeRange[1]);

  useData(ptIdRange[0]);
  useData(ptIdRange[1]);

  useData(dummy[0]);
  useData(dummy[1]);
  useData(dummy[2]);

  cell->Delete();

  return timer->GetElapsedTime();
}

double benchmarkPiecewiseIteration(vtkUnstructuredGrid *grid, int)
{
  // Setup for type
  unsigned char cellType;
  unsigned char typeRange[2] = {VTK_UNSIGNED_CHAR_MAX, VTK_UNSIGNED_CHAR_MIN};

  // Setup for point ids
  vtkIdType ptIdRange[2] = {VTK_ID_MAX, VTK_ID_MIN};
  vtkIdType cellSize;
  vtkIdList *cellPointIds = vtkIdList::New();
  vtkIdType *cellPtIdPtr;

  // Setup for points
  vtkPoints *points = grid->GetPoints();
  double point[3];
  double dummy[3] = {0.f, 0.f, 0.f};

  // Setup for cells
  vtkGenericCell *cell = vtkGenericCell::New();

  vtkIdType numCells = grid->GetNumberOfCells();
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
    // Cell type
    cellType = static_cast<unsigned char>(grid->GetCellType(cellId));
    typeRange[0] = std::min(typeRange[0], cellType);
    typeRange[1] = std::max(typeRange[1], cellType);

    grid->GetCellPoints(cellId, cellPointIds);
    cellSize = cellPointIds->GetNumberOfIds();
    cellPtIdPtr = cellPointIds->GetPointer(0);
    for (vtkIdType pointIdx = 0; pointIdx < cellSize; ++pointIdx)
      {
      // Point ids:
      ptIdRange[0] = std::min(ptIdRange[0], cellPtIdPtr[pointIdx]);
      ptIdRange[1] = std::max(ptIdRange[1], cellPtIdPtr[pointIdx]);

      // Points:
      points->GetPoint(cellPtIdPtr[pointIdx], point);
      dummy[0] += point[0];
      dummy[1] += point[1];
      dummy[2] += point[2];

      }

    // Cell:
    grid->GetCell(cellId, cell);
    }
  timer->StopTimer();

  useData(typeRange[0]);
  useData(typeRange[1]);

  useData(ptIdRange[0]);
  useData(ptIdRange[1]);

  useData(dummy[0]);
  useData(dummy[1]);
  useData(dummy[2]);

  cellPointIds->Delete();

  return timer->GetElapsedTime();
}

double benchmarkPiecewiseIteration(vtkCellIterator *iter)
{
  // Type setup:
  int typeRange[2] = {VTK_UNSIGNED_CHAR_MAX, VTK_UNSIGNED_CHAR_MIN};

  // Point ids setups:
  vtkIdType ptIdRange[2] = {VTK_ID_MAX, VTK_ID_MIN};
  vtkIdType *cellPtr;
  vtkIdType cellSize;

  // Points setup:
  float dummy[3] = {0.f, 0.f, 0.f};
  float *pointsPtr;

  // Cell setup
  vtkGenericCell *cell = vtkGenericCell::New();

  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
  for (iter->InitTraversal(); iter->IsDoneWithTraversal(); iter->GoToNextCell())
    {
    // Types:
    typeRange[0] = std::min(typeRange[0], iter->GetCellType());
    typeRange[1] = std::max(typeRange[1], iter->GetCellType());

    cellPtr = iter->GetPointIds()->GetPointer(0);
    pointsPtr = static_cast<float*>(iter->GetPoints()->GetVoidPointer(0));
    cellSize = iter->GetPointIds()->GetNumberOfIds();
    while (cellSize-- > 0)
      {
      // Point Ids:
      ptIdRange[0] = std::min(ptIdRange[0], *cellPtr);
      ptIdRange[1] = std::max(ptIdRange[1], *cellPtr);
      ++cellPtr;

      // Points:
      dummy[0] += *pointsPtr++;
      dummy[1] += *pointsPtr++;
      dummy[2] += *pointsPtr++;
      }

    // Cell:
    iter->GetCell(cell);
    }
  timer->StopTimer();

  useData(typeRange[0]);
  useData(typeRange[1]);

  useData(ptIdRange[0]);
  useData(ptIdRange[1]);

  useData(dummy[0]);
  useData(dummy[1]);
  useData(dummy[2]);

  cell->Delete();

  return timer->GetElapsedTime();
}

#define BENCHMARK_ITERATORS(grid_, test_, bench_) \
  if (!runBenchmark(grid_, test_, bench_, bench_, bench_)) \
    { \
    cerr << "Benchmark '" << test_ << "' encountered an error." << endl; \
    return false; \
    }

typedef double (*BenchmarkRefType)(vtkUnstructuredGrid*);
typedef double (*BenchmarkApiType)(vtkUnstructuredGrid*, int);
typedef double (*BenchmarkIterType)(vtkCellIterator*);
bool runBenchmark(vtkUnstructuredGrid *grid, const std::string &test,
                  BenchmarkRefType refBench, BenchmarkApiType apiBench,
                  BenchmarkIterType iterBench)
{
  const int numBenchmarks = NUM_BENCHMARKS;
  double refTime = 0.;
  double apiTime = 0.;
  double dsTime  = 0.;
  double psTime  = 0.;
  double ugTime  = 0.;

  vtkCellIterator *dsIter = grid->vtkDataSet::NewCellIterator();
  vtkCellIterator *psIter = grid->vtkPointSet::NewCellIterator();
  vtkCellIterator *ugIter = grid->NewCellIterator();

  cout << "Testing " << test << " (" << numBenchmarks << " samples):" << endl;

#ifdef PROFILE
  std::string prog;
  prog.resize(12, ' ');
  prog[0] = prog[11] = '|';
#endif // PROFILE

  for (int i = 0; i < numBenchmarks; ++i)
    {
#ifdef PROFILE
    std::fill_n(prog.begin() + 1, i * 10 / numBenchmarks, '=');
    cout << "\rProgress: " << prog << " (" << i << "/" << numBenchmarks << ")"
         << endl;
#endif // PROFILE

    refTime += refBench(grid);
    apiTime += apiBench(grid, 0);
    dsTime  += iterBench(dsIter);
    psTime  += iterBench(psIter);
    ugTime  += iterBench(ugIter);
    }

#ifdef PROFILE
  std::fill_n(prog.begin() + 1, 10, '=');
  cout << "\rProgress: " << prog << " (" << numBenchmarks << "/"
       << numBenchmarks << ")" << endl;
#endif // PROFILE

  refTime /= static_cast<double>(numBenchmarks);
  apiTime /= static_cast<double>(numBenchmarks);
  dsTime  /= static_cast<double>(numBenchmarks);
  psTime  /= static_cast<double>(numBenchmarks);
  ugTime  /= static_cast<double>(numBenchmarks);

  const std::string sep("\t");
  cout << std::setw(8)

       << "\t"
       << "Ref (raw)" << sep
       << "Ref (api)" << sep
       << "DSIter" << sep
       << "PSIter" << sep
       << "UGIter"
       << endl
       << "\t"
       << refTime << sep
       << apiTime << sep
       << dsTime << sep
       << psTime << sep
       << ugTime
       << endl;

  dsIter->Delete();
  psIter->Delete();
  ugIter->Delete();

  return true;
}

bool runBenchmarks(vtkUnstructuredGrid *grid)
{
  BENCHMARK_ITERATORS(grid, "cell type", benchmarkTypeIteration);
  BENCHMARK_ITERATORS(grid, "cell pointId", benchmarkPointIdIteration);
  BENCHMARK_ITERATORS(grid, "cell point", benchmarkPointsIteration);
  BENCHMARK_ITERATORS(grid, "cells", benchmarkCellIteration);
  BENCHMARK_ITERATORS(grid, "piecewise", benchmarkPiecewiseIteration);
  return true;
}
#endif // Benchmark

int TestCellIterators(int argc, char *argv[])
{
  // Load an unstructured grid dataset
  char *fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                         "Data/blowGeom.vtk");
  std::string fileName(fileNameC);
  delete [] fileNameC;

  vtkNew<vtkUnstructuredGridReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkUnstructuredGrid *grid(reader->GetOutput());
  if (!grid)
    {
    cerr << "Error reading file: " << fileName << endl;
    return EXIT_FAILURE;
    }

#ifndef PROFILE
  if (!runValidation(grid))
    {
    return EXIT_FAILURE;
    }
#endif // not PROFILE

#ifdef BENCHMARK
  if (!runBenchmarks(grid))
    {
    return EXIT_FAILURE;
    }

  // Reference _sink to prevent optimizations from interfering with the
  // benchmarks.
  if (_sink.str().size() == 0)
    {
    return EXIT_FAILURE;
    }
#endif // BENCHMARK

  return EXIT_SUCCESS;
}
