// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCPExodusIIInSituReader.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkConeSource.h"
#include "vtkDoubleArray.h"
#include "vtkExodusIIReader.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkSOADataArrayTemplate.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"

// Filters that we test against:
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkExtractGeometry.h"
#include "vtkGlyph3D.h"
#include "vtkWarpScalar.h"
#include "vtkWarpVector.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <sstream>
#include <string>

#include <iostream>

#define FAIL(x)                                                                                    \
  std::cerr << x << std::endl;                                                                     \
  return EXIT_FAILURE;

#define FAILB(x)                                                                                   \
  std::cerr << x << std::endl;                                                                     \
  return false;

bool readExodusCopy(std::string fileName, vtkMultiBlockDataSet* mbds)
{
  // Read file using reference reader
  vtkNew<vtkExodusIIReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->UpdateInformation();

  // Disable extra arrays:
  reader->SetGenerateFileIdArray(0);
  reader->SetGenerateGlobalElementIdArray(0);
  reader->SetGenerateGlobalNodeIdArray(0);
  reader->SetGenerateImplicitElementIdArray(0);
  reader->SetGenerateImplicitNodeIdArray(0);
  reader->SetGenerateObjectIdCellArray(0);

  // Just read the first timestep
  int timeStepRange[2];
  reader->GetTimeStepRange(timeStepRange);
  reader->SetTimeStep(timeStepRange[0]);

  // Include all points in element blocks (including those unused by the block)
  reader->SetSqueezePoints(false);

  // Enable all nodal result (point data) arrays
  int numNodeArrays = reader->GetNumberOfObjectArrays(vtkExodusIIReader::NODAL);
  for (int i = 0; i < numNodeArrays; ++i)
  {
    reader->SetObjectArrayStatus(vtkExodusIIReader::NODAL, i, 1);
  }

  // Enable all element result (cell data) arrays
  int numElementBlockArrays = reader->GetNumberOfObjectArrays(vtkExodusIIReader::ELEM_BLOCK);
  for (int i = 0; i < numElementBlockArrays; ++i)
  {
    reader->SetObjectArrayStatus(vtkExodusIIReader::ELEM_BLOCK, i, 1);
  }

  reader->Update();
  mbds->ShallowCopy(reader->GetOutput());

  return true;
}

vtkUnstructuredGrid* getConnectivityBlock(vtkMultiBlockDataSet* mbds)
{
  vtkUnstructuredGrid* result = nullptr;
  if (vtkDataObject* tmpDO = mbds->GetBlock(0))
  {
    if (vtkMultiBlockDataSet* tmpMBDS = vtkMultiBlockDataSet::SafeDownCast(tmpDO))
    {
      result = vtkUnstructuredGrid::SafeDownCast(tmpMBDS->GetBlock(0));
    }
  }
  return result;
}

// Predicate for std::equal to fuzzy compare floating points.
template <class Scalar>
bool fuzzyEqual(const Scalar& a, const Scalar& b)
{
  return fabs(a - b) < 1e-6;
}

// Add fake scalar and normal data to the dataset
void populateAttributes(vtkDataSet* ref, vtkDataSet* test)
{
  vtkIdType numPoints = ref->GetNumberOfPoints();

  // Create/set scalars for the filters
  vtkNew<vtkDoubleArray> refScalars;
  refScalars->SetName("test-scalars");
  double point[3];
  for (vtkIdType pointId = 0; pointId < numPoints; ++pointId)
  {
    ref->GetPoint(pointId, point);
    refScalars->InsertNextTuple1((sin(point[0] * point[1]) + cos(point[2])));
  }
  vtkNew<vtkAOSDataArrayTemplate<double>> testScalars;
  testScalars->SetName("test-scalars");
  testScalars->SetArray(refScalars->GetPointer(0), numPoints, /*save=*/true);

  ref->GetPointData()->SetScalars(refScalars);
  test->GetPointData()->SetScalars(testScalars);

  // And some fake normals
  vtkNew<vtkAOSDataArrayTemplate<double>> refNormals;
  refNormals->SetName("test-normals");
  refNormals->SetNumberOfComponents(3);
  refNormals->SetNumberOfTuples(numPoints);
  double* testNormalArrayX = new double[numPoints];
  double* testNormalArrayY = new double[numPoints];
  double* testNormalArrayZ = new double[numPoints];
  double normal[3];
  double norm;
  for (vtkIdType pointId = 0; pointId < numPoints; ++pointId)
  {
    ref->GetPoint(pointId, point);
    norm = sqrt(point[0] * point[0] + point[1] * point[1] + point[2] * point[2]);
    if (norm > 1e-5)
    {
      testNormalArrayX[pointId] = normal[0] = (point[1] / norm);
      testNormalArrayY[pointId] = normal[1] = (point[0] / norm);
      testNormalArrayZ[pointId] = normal[2] = (point[2] / norm);
    }
    else
    {
      testNormalArrayX[pointId] = normal[0] = 1.0;
      testNormalArrayY[pointId] = normal[1] = 0.0;
      testNormalArrayZ[pointId] = normal[2] = 0.0;
    }
    refNormals->SetTypedTuple(pointId, normal);
  }
  vtkNew<vtkSOADataArrayTemplate<double>> testNormals;
  testNormals->SetName("test-normals");
  testNormals->SetNumberOfComponents(3);
  testNormals->SetArray(0, testNormalArrayX, numPoints, /*updateMaxId=*/true,
    /*save=*/false, /*deletMethod*/ vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
  testNormals->SetArray(1, testNormalArrayY, numPoints, /*updateMaxId=*/false,
    /*save=*/false, /*deletMethod*/ vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
  testNormals->SetArray(2, testNormalArrayZ, numPoints, /*updateMaxId=*/false,
    /*save=*/false, /*deletMethod*/ vtkAbstractArray::VTK_DATA_ARRAY_DELETE);

  ref->GetPointData()->SetNormals(refNormals);
  test->GetPointData()->SetNormals(testNormals);
}

void testContourFilter(vtkUnstructuredGrid* input, vtkDataSet*& output, double& time)
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(input);
  contour->GenerateValues(2, -0.5, 0.5);
  timer->StartTimer();
  contour->Update();
  timer->StopTimer();
  output = contour->GetOutput();
  output->Register(nullptr);
  time = timer->GetElapsedTime();
}

void testDataSetSurfaceFilter(vtkUnstructuredGrid* input, vtkDataSet*& output, double& time)
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkDataSetSurfaceFilter> extractSurface;
  extractSurface->SetInputData(input);
  extractSurface->SetNonlinearSubdivisionLevel(4);
  timer->StartTimer();
  extractSurface->Update();
  timer->StopTimer();
  output = extractSurface->GetOutput();
  output->Register(nullptr);
  time = timer->GetElapsedTime();
}

void testCutterFilter(vtkUnstructuredGrid* input, vtkDataSet*& output, double& time)
{
  vtkNew<vtkTimerLog> timer;

  // Create plane for testing slicing
  vtkNew<vtkPlane> slicePlane;
  slicePlane->SetOrigin(input->GetCenter());
  slicePlane->SetNormal(1.0, 1.0, 1.0);

  // Cutter (slice, polydata output)
  vtkNew<vtkCutter> cutter;
  cutter->SetInputData(input);
  cutter->SetCutFunction(slicePlane);
  cutter->SetGenerateTriangles(0);
  timer->StartTimer();
  cutter->Update();
  timer->StopTimer();
  output = cutter->GetOutput();
  output->Register(nullptr);
  time = timer->GetElapsedTime();
}

void testExtractGeometryFilter(vtkUnstructuredGrid* input, vtkDataSet*& output, double& time)
{
  vtkNew<vtkTimerLog> timer;

  // Create plane for testing slicing
  vtkNew<vtkPlane> slicePlane;
  slicePlane->SetOrigin(input->GetCenter());
  slicePlane->SetNormal(1.0, 1.0, 1.0);

  vtkNew<vtkExtractGeometry> extract;
  extract->SetInputData(input);
  extract->SetImplicitFunction(slicePlane);
  extract->SetExtractInside(1);
  extract->SetExtractOnlyBoundaryCells(1);
  extract->SetExtractBoundaryCells(1);
  timer->StartTimer();
  extract->Update();
  timer->StopTimer();
  output = extract->GetOutput();
  output->Register(nullptr);
  time = timer->GetElapsedTime();
}

void testGlyph3DFilter(vtkUnstructuredGrid* input, vtkDataSet*& output, double& time)
{
  vtkNew<vtkTimerLog> timer;

  // Create a cone to test glyphing
  vtkNew<vtkConeSource> coneSource;
  coneSource->SetDirection(0.0, 1.0, 0.0);
  coneSource->SetHeight(2.5);
  coneSource->SetCapping(1);
  coneSource->SetRadius(1.25);

  // Glyph3D
  vtkNew<vtkGlyph3D> glypher;
  glypher->SetSourceConnection(coneSource->GetOutputPort());
  glypher->SetInputData(input);
  timer->StartTimer();
  glypher->Update();
  timer->StopTimer();
  output = glypher->GetOutput();
  output->Register(nullptr);
  time = timer->GetElapsedTime();
}

void testWarpScalarFilter(vtkUnstructuredGrid* input, vtkDataSet*& output, double& time)
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkWarpScalar> warpScalar;
  warpScalar->SetInputData(input);
  timer->StartTimer();
  warpScalar->Update();
  timer->StopTimer();
  output = warpScalar->GetOutput();
  output->Register(nullptr);
  time = timer->GetElapsedTime();
}

void testWarpVectorFilter(vtkUnstructuredGrid* input, vtkDataSet*& output, double& time)
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkWarpVector> warpVector;
  warpVector->SetInputData(input);
  warpVector->SetScaleFactor(1.0);
  timer->StartTimer();
  warpVector->Update();
  timer->StopTimer();
  output = warpVector->GetOutput();
  output->Register(nullptr);
  time = timer->GetElapsedTime();
}

void testPipeline(vtkUnstructuredGrid* input, vtkDataSet*& output, double& time)
{
  vtkNew<vtkTimerLog> timer;

  vtkNew<vtkWarpScalar> warpScalar;
  warpScalar->SetInputData(input);

  vtkNew<vtkWarpVector> warpVector;
  warpVector->SetInputConnection(warpScalar->GetOutputPort());
  warpVector->SetScaleFactor(1.0);

  timer->StartTimer();
  warpVector->Update();
  timer->StopTimer();
  output = warpVector->GetOutput();
  output->Register(nullptr);
  time = timer->GetElapsedTime();
}

#define doBenchmark(call_, reset_, timeLog_, repeat_)                                              \
  {                                                                                                \
    timeLog_.clear();                                                                              \
    timeLog_.resize(repeat_, -500);                                                                \
    for (int benchmark = 0; benchmark < (repeat_); ++benchmark)                                    \
    {                                                                                              \
      double& benchmarkTime = timeLog_[benchmark];                                                 \
      call_;                                                                                       \
      if (benchmark + 1 != (repeat_))                                                              \
      {                                                                                            \
        reset_;                                                                                    \
      }                                                                                            \
    }                                                                                              \
  }

// Check that refOutput == testOutput, then delete and clear the outputs.
bool validateFilterOutput(const std::string& name, vtkDataSet*& refOutput, vtkDataSet*& testOutput)
{
  if (refOutput->GetNumberOfPoints() == 0)
  {
    FAILB("Reference " << name << " produced an empty output!")
  }
  if (!vtkTestUtilities::CompareDataObjects(refOutput, testOutput))
  {
    FAILB(name << " output mismatch.")
  }
  std::cout << name << " produced " << refOutput->GetNumberOfPoints() << " points and "
            << refOutput->GetNumberOfCells() << " cells." << std::endl;
  refOutput->Delete();
  refOutput = nullptr;
  testOutput->Delete();
  testOutput = nullptr;
  return true;
}

void printTimingInfo(
  const std::string& name, const std::vector<double>& ref, const std::vector<double>& test)
{
  assert(ref.size() == test.size());
  double refAverage(0.0);
  double testAverage(0.0);
  double refMin(10000.0);
  double testMin(10000.0);
  double refMax(0.0);
  double testMax(0.0);

  for (size_t i = 0; i < ref.size(); ++i)
  {
    refAverage += ref[i];
    testAverage += test[i];
    refMin = std::min(refMin, ref[i]);
    refMax = std::max(refMax, ref[i]);
    testMin = std::min(testMin, test[i]);
    testMax = std::max(testMax, test[i]);
  }
  refAverage /= static_cast<double>(ref.size());
  testAverage /= static_cast<double>(test.size());

  double refStdev(0.0);
  double testStdev(0.0);
  for (size_t i = 0; i < ref.size(); ++i)
  {
    refStdev += (ref[i] - refAverage) * (ref[i] - refAverage);
    testStdev += (test[i] - testAverage) * (test[i] - testAverage);
  }
  refStdev = std::sqrt(refStdev / static_cast<double>(ref.size()));
  testStdev = std::sqrt(testStdev / static_cast<double>(test.size()));

  std::cout << "Timing info for test '" << name << "', " << ref.size() << " sample(s):\n\t"
            << "Average (ref | test | %slowdown): " << std::setprecision(6) << std::setw(9)
            << refAverage << std::setw(0) << " | " << std::setw(9) << testAverage << std::setw(0)
            << " | " << std::setw(9) << ((testAverage / refAverage) - 1.0) * 100 << std::setw(0)
            << "%\n\t"
            << "Std Dev (ref | test): " << std::setw(9) << refStdev << std::setw(0) << " | "
            << std::setw(9) << testStdev << std::setw(0) << "\n\t"
            << "Minimum (ref | test): " << std::setw(9) << refMin << std::setw(0) << " | "
            << std::setw(9) << testMin << std::setw(0) << "\n\t"
            << "Maximum (ref | test): " << std::setw(9) << refMax << std::setw(0) << " | "
            << std::setw(9) << testMax << std::setw(0) << std::endl;
}

bool testFilters(vtkUnstructuredGrid* ref, vtkUnstructuredGrid* test)
{
  std::cout << "Reference: Number of points: " << ref->GetNumberOfPoints() << std::endl;
  std::cout << "Reference: Number of cells:  " << ref->GetNumberOfCells() << std::endl;
  std::cout << "Test: Number of points: " << test->GetNumberOfPoints() << std::endl;
  std::cout << "Test: Number of cells:  " << test->GetNumberOfCells() << std::endl;

  // Number of times to run each benchmark. Don't commit a value greater than
  // 1 to keep the dashboards fast, but this can be increased while benchmarking
  // or profiling particular filters.
  int numBenchmarks = 1;

  // Temporary variables for outputs.
  vtkDataSet* refOutput(nullptr);
  vtkDataSet* testOutput(nullptr);

  //////////////////////////////
  // Actual tests start here: //
  //////////////////////////////

  // Contour filter
  std::vector<double> contourRefTimes;
  std::vector<double> contourTestTimes;
  doBenchmark(testContourFilter(ref, refOutput, benchmarkTime), refOutput->Delete();
              refOutput = nullptr, contourRefTimes, numBenchmarks);
  doBenchmark(testContourFilter(test, testOutput, benchmarkTime), testOutput->Delete();
              testOutput = nullptr, contourTestTimes, numBenchmarks);
  if (!validateFilterOutput("Contour filter", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("contour", contourRefTimes, contourTestTimes);

  // Extract surface
  std::vector<double> dataSetSurfaceRefTimes;
  std::vector<double> dataSetSurfaceTestTimes;
  doBenchmark(testDataSetSurfaceFilter(ref, refOutput, benchmarkTime), refOutput->Delete();
              refOutput = nullptr, dataSetSurfaceRefTimes, numBenchmarks);
  doBenchmark(testDataSetSurfaceFilter(test, testOutput, benchmarkTime), testOutput->Delete();
              testOutput = nullptr, dataSetSurfaceTestTimes, numBenchmarks);
  if (!validateFilterOutput("Data set surface filter", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("dataset surface", dataSetSurfaceRefTimes, dataSetSurfaceTestTimes);

  // Cutter
  std::vector<double> cutterRefTimes;
  std::vector<double> cutterTestTimes;
  doBenchmark(testCutterFilter(ref, refOutput, benchmarkTime), refOutput->Delete();
              refOutput = nullptr, cutterRefTimes, numBenchmarks);
  doBenchmark(testCutterFilter(test, testOutput, benchmarkTime), testOutput->Delete();
              testOutput = nullptr, cutterTestTimes, numBenchmarks);
  if (!validateFilterOutput("Cutter", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("cutter", cutterRefTimes, cutterTestTimes);

  // Extract geometry
  std::vector<double> extractGeometryRefTimes;
  std::vector<double> extractGeometryTestTimes;
  doBenchmark(testExtractGeometryFilter(ref, refOutput, benchmarkTime), refOutput->Delete();
              refOutput = nullptr, extractGeometryRefTimes, numBenchmarks);
  doBenchmark(testExtractGeometryFilter(test, testOutput, benchmarkTime), testOutput->Delete();
              testOutput = nullptr, extractGeometryTestTimes, numBenchmarks);
  if (!validateFilterOutput("Extract geometry", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("extract geometry", extractGeometryRefTimes, extractGeometryTestTimes);

  // Glyph3D
  std::vector<double> glyph3dRefTimes;
  std::vector<double> glyph3dTestTimes;
  doBenchmark(testGlyph3DFilter(ref, refOutput, benchmarkTime), refOutput->Delete();
              refOutput = nullptr, glyph3dRefTimes, numBenchmarks);
  doBenchmark(testGlyph3DFilter(test, testOutput, benchmarkTime), testOutput->Delete();
              testOutput = nullptr, glyph3dTestTimes, numBenchmarks);
  if (!validateFilterOutput("Glyph3D", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("glyph3d", glyph3dRefTimes, glyph3dTestTimes);

  // Warp scalar
  std::vector<double> warpScalarRefTimes;
  std::vector<double> warpScalarTestTimes;
  doBenchmark(testWarpScalarFilter(ref, refOutput, benchmarkTime), refOutput->Delete();
              refOutput = nullptr, warpScalarRefTimes, numBenchmarks);
  doBenchmark(testWarpScalarFilter(test, testOutput, benchmarkTime), testOutput->Delete();
              testOutput = nullptr, warpScalarTestTimes, numBenchmarks);
  if (!validateFilterOutput("Warp scalar", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("warp scalar", warpScalarRefTimes, warpScalarTestTimes);

  // Warp vector
  std::vector<double> warpVectorRefTimes;
  std::vector<double> warpVectorTestTimes;
  doBenchmark(testWarpVectorFilter(ref, refOutput, benchmarkTime), refOutput->Delete();
              refOutput = nullptr, warpVectorRefTimes, numBenchmarks);
  doBenchmark(testWarpVectorFilter(test, testOutput, benchmarkTime), testOutput->Delete();
              testOutput = nullptr, warpVectorTestTimes, numBenchmarks);
  if (!validateFilterOutput("Warp vector", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("warp vector", warpVectorRefTimes, warpVectorTestTimes);

  // mini-mapped pipeline (Warp scalar + vector)
  std::vector<double> pipelineRefTimes;
  std::vector<double> pipelineTestTimes;
  doBenchmark(testPipeline(ref, refOutput, benchmarkTime), refOutput->Delete();
              refOutput = nullptr, pipelineRefTimes, numBenchmarks);
  doBenchmark(testPipeline(test, testOutput, benchmarkTime), testOutput->Delete();
              testOutput = nullptr, pipelineTestTimes, numBenchmarks);
  // Ensure that the mapped test produced a vtkUnstructuredGrid:
  if (!testOutput->IsA("vtkUnstructuredGrid"))
  {
    std::cerr << "Pipeline test did not produce a mapped output object!" << std::endl;
    return false;
  }
  if (!validateFilterOutput("Pipeline test", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("pipeline", pipelineRefTimes, pipelineTestTimes);

  return true;
}

bool testCopies(vtkUnstructuredGrid* test)
{
  vtkNew<vtkUnstructuredGrid> vtkTarget;
  auto mappedTarget = vtkSmartPointer<vtkUnstructuredGrid>::Take(test->NewInstance());

  // No deep copy into test class -- it's read only. Can shallow copy into test
  // class, since it will just share the implementation instance.

  // Deep copy: test --> vtk
  vtkTarget->DeepCopy(test);
  if (!vtkTestUtilities::CompareDataObjects(test, vtkTarget))
  {
    FAILB("Deep copy insitu --> VTK failed.")
  }
  vtkTarget->Reset();

  // Shallow copy: test --> vtk
  vtkTarget->ShallowCopy(test); // Should really deep copy.
  if (!vtkTestUtilities::CompareDataObjects(test, vtkTarget))
  {
    FAILB("Shallow copy insitu --> VTK failed.")
  }

  // Shallow copy: test --> test
  mappedTarget->ShallowCopy(test);
  if (!vtkTestUtilities::CompareDataObjects(test, mappedTarget))
  {
    FAILB("Shallow copy insitu --> insitu failed.")
  }

  return true;
}

void testSaveArrays()
{
  vtkIdType numPoints = 1000;
  vtkNew<vtkAOSDataArrayTemplate<double>> testScalars;
  testScalars->SetName("test-scalars");
  double* testScalarArray = new double[numPoints];
  for (int i = 0; i < numPoints; i++)
  {
    testScalarArray[i] = 1;
  }
  // Call SetExodusScalarArrays a couple of times to make sure
  // we don't free the same memory multiple times. The final call
  // is the one that should actually free the array.
  testScalars->SetArray(testScalarArray, numPoints, /*save=*/true);
  testScalars->SetArray(testScalarArray, numPoints, /*save=*/true);
  testScalars->SetArray(testScalarArray, numPoints, /*save=*/false,
    /*deletMethod*/ vtkAbstractArray::VTK_DATA_ARRAY_DELETE);
}

int TestInSituExodus(int argc, char* argv[])
{
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();

  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/box-noglom.ex2");
  std::string fileName(fileNameC);
  delete[] fileNameC;

  // Read reference copy
  vtkNew<vtkMultiBlockDataSet> refMBDS;
  readExodusCopy(fileName, refMBDS);
  vtkUnstructuredGrid* refGrid(getConnectivityBlock(refMBDS));
  if (!refGrid)
  {
    FAIL("Error retrieving reference element block container.");
  }
  refGrid->GetFieldData()->Initialize(); // in-situ doesn't have field data

  // Read in-situ copy
  vtkNew<vtkCPExodusIIInSituReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkMultiBlockDataSet* testMBDS = reader->GetOutput();
  vtkUnstructuredGrid* grid(getConnectivityBlock(testMBDS));
  if (!grid)
  {
    FAIL("Error retrieving testing element block container.")
  }

  // Compare
  if (!vtkTestUtilities::CompareDataObjects(refGrid, grid))
  {
    FAIL("In-situ data set doesn't match reference data!")
  }

  if (!testCopies(grid))
  {
    FAIL("A copy test failed.")
  }

  populateAttributes(refGrid, grid);

  // Test selected filters
  if (!testFilters(refGrid, grid))
  {
    FAIL("Pipeline test failed!")
  }

  testSaveArrays();

  timer->StopTimer();
  double time = timer->GetElapsedTime();
  std::cout << "Test took " << static_cast<int>(time / 60) << "m " << std::fmod(time, 60.0) << "s."
            << std::endl;
  return EXIT_SUCCESS;
}
