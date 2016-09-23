/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCPExodusIIElementBlock.h"
#include "vtkCPExodusIIInSituReader.h"
#include "vtkCPExodusIINodalCoordinatesTemplate.h"
#include "vtkCPExodusIIResultsArrayTemplate.h"

#include "vtkCellData.h"
#include "vtkCellIterator.h"
#include "vtkConeSource.h"
#include "vtkDoubleArray.h"
#include "vtkExodusIIReader.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"

// Filters that we test against:
#include "vtkContourFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkCutter.h"
#include "vtkExtractGeometry.h"
#include "vtkGlyph3D.h"
#include "vtkWarpScalar.h"
#include "vtkWarpVector.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <sstream>

// Define this to work around "glommed" point/cell data in the reference data.
#undef GLOM_WORKAROUND
//#define GLOM_WORKAROUND

#define FAIL(x)        \
  cerr << x << endl;   \
  return EXIT_FAILURE;

#define FAILB(x)       \
  cerr << x << endl;   \
  return false;

bool readExodusCopy(std::string fileName, vtkMultiBlockDataSet *mbds)
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
  int numElementBlockArrays =
      reader->GetNumberOfObjectArrays(vtkExodusIIReader::ELEM_BLOCK);
  for (int i = 0; i < numElementBlockArrays; ++i)
  {
    reader->SetObjectArrayStatus(vtkExodusIIReader::ELEM_BLOCK, i, 1);
  }

  reader->Update();
  mbds->ShallowCopy(reader->GetOutput());

  return true;
}

vtkUnstructuredGridBase* getConnectivityBlock(vtkMultiBlockDataSet *mbds)
{
  vtkUnstructuredGridBase *result = NULL;
   if (vtkDataObject *tmpDO = mbds->GetBlock(0))
   {
     if (vtkMultiBlockDataSet *tmpMBDS =
         vtkMultiBlockDataSet::SafeDownCast(tmpDO))
     {
       result = vtkUnstructuredGridBase::SafeDownCast(tmpMBDS->GetBlock(0));
     }
   }
   return result;
}

// Predicate for std::equal to fuzzy compare floating points.
template <class Scalar> bool fuzzyEqual(const Scalar &a, const Scalar &b)
{
  return fabs(a - b) < 1e-6;
}

bool compareDataSets(vtkDataSet *ref, vtkDataSet *test)
{
  // Compare number of points
  vtkIdType refNumPoints = ref->GetNumberOfPoints();
  vtkIdType testNumPoints = test->GetNumberOfPoints();
  if (refNumPoints != testNumPoints)
  {
    FAILB("Number of points do not match (" << refNumPoints << ", "
          << testNumPoints << ").")
  }

  // Compare coordinate data
  double refPoint[3] = {0., 0., 0.};
  double testPoint[3] = {0., 0., 0.};
  for (vtkIdType pointId = 0; pointId < testNumPoints; ++pointId)
  {
    ref->GetPoint(pointId, refPoint);
    test->GetPoint(pointId, testPoint);
    if (fabs(refPoint[0] - testPoint[0]) > 1e-5
        || fabs(refPoint[1] - testPoint[1]) > 1e-5
        || fabs(refPoint[2] - testPoint[2]) > 1e-5)
    {
      FAILB("Point mismatch at point index: " << pointId
            << "\n\tExpected: " << refPoint[0] << " " << refPoint[1] << " "
            << refPoint[2]
            << "\n\tActual: " << testPoint[0] << " " << testPoint[1] << " "
            << testPoint[2])
    }
  }

  // Compare point data
  // Number of point data arrays may not match -- the reference reader
  // "gloms" multi-component arrays together, while the in-situ doesn't (yet?).
  vtkPointData *refPointData = ref->GetPointData();
  vtkPointData *testPointData = test->GetPointData();
  int refNumPointDataArrays = refPointData->GetNumberOfArrays();
  int testNumPointDataArrays = testPointData->GetNumberOfArrays();
  if (refNumPointDataArrays != testNumPointDataArrays)
  {
#ifdef GLOM_WORKAROUND
    cerr << "Warning: "
            "Point data array count mismatch. This may not be an error, as "
            "the reference data combines multicomponent arrays. "
            << "Reference: " << refNumPointDataArrays
            << " Actual: " << testNumPointDataArrays
            << endl;
#else
    FAILB("Point data array count mismatch. This may not be an error, as "
          "the reference data combines multicomponent arrays. "
          << "Reference: " << refNumPointDataArrays
          << " Actual: " << testNumPointDataArrays
          << " Define GLOM_WORKAROUND in " << __FILE__ << " to treat this "
          << "message as a warning.")
#endif
  }
  for (int arrayIndex = 0; arrayIndex < testNumPointDataArrays; ++arrayIndex)
  {
    vtkDataArray *testArray = testPointData->GetArray(arrayIndex);
    const char *arrayName = testArray->GetName();
    vtkDataArray *refArray = refPointData->GetArray(arrayName);
    if (refArray == NULL)
    {
#ifdef GLOM_WORKAROUND
      cerr << "Warning: "
           << "Testing point data array '" << arrayName
           << "' does not exist in the reference data set. This may not be an "
           << "error if the reference data has probably made this into a "
           << "multicomponent array. "
           << endl;
      continue;
#else
      FAILB("Testing point data array '" << arrayName
            << "' does not exist in the reference data set. This may not be an "
            << "error if the reference data has probably made this into a "
            << "multicomponent array. "
            << " Define GLOM_WORKAROUND in " << __FILE__ << " to treat this "
            << "message as a warning.")
#endif
    }

    int refNumComponents = refArray->GetNumberOfComponents();
    int testNumComponents = testArray->GetNumberOfComponents();
    if (refNumComponents != testNumComponents)
    {
      FAILB("Number of components mismatch for point data array '"
            << arrayName << "'")
    }

    vtkIdType refNumTuples = refArray->GetNumberOfTuples();
    vtkIdType testNumTuples = testArray->GetNumberOfTuples();
    if (refNumTuples != testNumTuples)
    {
      FAILB("Number of tuples mismatch for point data array '"
            << arrayName << "'")
    }

    std::vector<double> refTuple(refNumComponents);
    std::vector<double> testTuple(testNumComponents);
    for (vtkIdType i = 0; i < testNumTuples; ++i)
    {
      refArray->GetTuple(i, &refTuple[0]);
      testArray->GetTuple(i, &testTuple[0]);
      if (!std::equal(refTuple.begin(), refTuple.end(), testTuple.begin(),
                     fuzzyEqual<double>))
      {
        std::stringstream refString;
        std::stringstream testString;
        for (int comp = 0; comp < refNumComponents; ++comp)
        {
          refString << refTuple[comp] << " ";
          testString << testTuple[comp] << " ";
        }
        FAILB("Tuple mismatch for point data array '" << arrayName
              << "' at tuple index: " << i << "\n"
              << "Expected:\n\t" << refString.str() << "\n"
              << "Actual:\n\t" << testString.str());
      }
    }
  }

  // Compare number of cells
  vtkIdType refNumCells = ref->GetNumberOfCells();
  vtkIdType testNumCells = test->GetNumberOfCells();
  if (refNumCells != testNumCells)
  {
    FAILB("Number of cells do not match (" << refNumCells << ", "
          << testNumCells << ").")
  }

  // Compare connectivity data
  vtkNew<vtkGenericCell> refCell;
  vtkNew<vtkGenericCell> testCell;

  // Test out the iterators, too:
  vtkSmartPointer<vtkCellIterator> refCellIter =
      vtkSmartPointer<vtkCellIterator>::Take(ref->NewCellIterator());
  vtkSmartPointer<vtkCellIterator> testCellIter =
      vtkSmartPointer<vtkCellIterator>::Take(test->NewCellIterator());

  for (vtkIdType cellId = 0;
       cellId < testNumCells &&
       !refCellIter->IsDoneWithTraversal() &&
       !testCellIter->IsDoneWithTraversal();
       ++cellId, refCellIter->GoToNextCell(), testCellIter->GoToNextCell())
  {
    // Lookup cells in iterators:
    refCellIter->GetCell(refCell.GetPointer());
    testCellIter->GetCell(testCell.GetPointer());

    if (refCell->GetCellType() != testCell->GetCellType())
    {
      FAILB("Cell types do not match!")
    }
    refNumPoints = refCell->GetNumberOfPoints();
    testNumPoints = testCell->GetNumberOfPoints();
    if (refNumPoints != testNumPoints)
    {
      FAILB("Number of cell points do not match (" << refNumPoints << ", "
            << testNumPoints << ") for cellId " << cellId)
    }

    for (vtkIdType pointId = 0; pointId < testNumPoints; ++pointId)
    {
      if (refCell->GetPointId(pointId) != testCell->GetPointId(pointId))
      {
        FAILB("Point id mismatch in cellId " << cellId)
      }
      refCell->Points->GetPoint(pointId, refPoint);
      testCell->Points->GetPoint(pointId, testPoint);
      if (fabs(refPoint[0] - testPoint[0]) > 1e-5
          || fabs(refPoint[1] - testPoint[1]) > 1e-5
          || fabs(refPoint[2] - testPoint[2]) > 1e-5)
      {
        FAILB("Point mismatch in cellId " << cellId
              << "\n\tExpected: " << refPoint[0] << " " << refPoint[1] << " "
              << refPoint[2]
              << "\n\tActual: " << testPoint[0] << " " << testPoint[1] << " "
              << testPoint[2])
      }
    }
  }

  // Verify that all cells were checked
  if (!refCellIter->IsDoneWithTraversal() ||
      !testCellIter->IsDoneWithTraversal())
  {
    FAILB("Did not finish traversing all cells (an iterator is still valid).")
  }

  // Compare cell data
  // Number of cell data arrays probably won't match -- the reference reader
  // "gloms" multi-component arrays together, while the in-situ doesn't (yet?).
  vtkCellData *refCellData = ref->GetCellData();
  vtkCellData *testCellData = test->GetCellData();
  int refNumCellDataArrays = refCellData->GetNumberOfArrays();
  int testNumCellDataArrays = testCellData->GetNumberOfArrays();
  if (refNumCellDataArrays != testNumCellDataArrays)
  {
#ifdef GLOM_WORKAROUND
    cerr << "Warning: "
         << "Cell data array count mismatch. This may not be an error, as "
            "the reference data combines multicomponent arrays. "
         << "Reference: " << refNumCellDataArrays
         << " Actual: " << testNumCellDataArrays
         << endl;
#else
    FAILB("Cell data array count mismatch. This may not be an error, as "
          "the reference data combines multicomponent arrays. "
          << "Reference: " << refNumCellDataArrays
          << " Actual: " << testNumCellDataArrays
          << " Define GLOM_WORKAROUND in " << __FILE__ << " to treat this "
          << "message as a warning.")
#endif
  }
  for (int arrayIndex = 0; arrayIndex < testNumCellDataArrays; ++arrayIndex)
  {
    vtkDataArray *testArray = testCellData->GetArray(arrayIndex);
    const char *arrayName = testArray->GetName();
    vtkDataArray *refArray = refCellData->GetArray(arrayName);
    if (refArray == NULL)
    {
#ifdef GLOM_WORKAROUND
      cerr << "Warning: "
           << "Testing cell data array '" << arrayName
           << "' does not exist in the reference data set. But it's cool -- "
           << "the reference data has probably made this into a multicomponent "
           << "array."
           << endl;
         continue;
#else
      FAILB("Testing cell data array '" << arrayName
            << "' does not exist in the reference data set. But it's cool -- "
            << "the reference data has probably made this into a multicomponent "
            << "array."
            << " Define GLOM_WORKAROUND in " << __FILE__ << " to treat this "
            << "message as a warning.")
#endif
    }

    int refNumComponents = refArray->GetNumberOfComponents();
    int testNumComponents = testArray->GetNumberOfComponents();
    if (refNumComponents != testNumComponents)
    {
      FAILB("Number of components mismatch for cell data array '"
            << arrayName << "'")
    }

    vtkIdType refNumTuples = refArray->GetNumberOfTuples();
    vtkIdType testNumTuples = testArray->GetNumberOfTuples();
    if (refNumTuples != testNumTuples)
    {
      FAILB("Number of tuples mismatch for cell data array '"
            << arrayName << "'")
    }

    std::vector<double> refTuple(refNumComponents);
    std::vector<double> testTuple(testNumComponents);
    for (vtkIdType i = 0; i < testNumTuples; ++i)
    {
      refArray->GetTuple(i, &refTuple[0]);
      testArray->GetTuple(i, &testTuple[0]);
      if (!std::equal(refTuple.begin(), refTuple.end(), testTuple.begin(),
                     fuzzyEqual<double>))
      {
        std::stringstream refString;
        std::stringstream testString;
        for (int comp = 0; comp < refNumComponents; ++comp)
        {
          refString << refTuple[comp] << " ";
          testString << testTuple[comp] << " ";
        }
        FAILB("Tuple mismatch for cell data array '" << arrayName
              << "' at tuple index: " << i << "\n"
              << "Expected:\n\t" << refString.str() << "\n"
              << "Actual:\n\t" << testString.str());
      }
    }
  }

  return true;
}

// Add fake scalar and normal data to the dataset
void populateAttributes(vtkDataSet *ref, vtkDataSet *test)
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
  vtkNew<vtkCPExodusIIResultsArrayTemplate<double> > testScalars;
  testScalars->SetName("test-scalars");
  double *testScalarArray = new double[numPoints];
  memcpy(testScalarArray, refScalars->GetVoidPointer(0),
         numPoints * sizeof(double));
  testScalars->SetExodusScalarArrays(std::vector<double*>(1, testScalarArray),
                                     numPoints);

  ref->GetPointData()->SetScalars(refScalars.GetPointer());
  test->GetPointData()->SetScalars(testScalars.GetPointer());

  // And some fake normals
  vtkNew<vtkFloatArray> refNormals;
  refNormals->SetName("test-normals");
  refNormals->SetNumberOfComponents(3);
  refNormals->SetNumberOfTuples(numPoints);
  double *testNormalArrayX = new double[numPoints];
  double *testNormalArrayY = new double[numPoints];
  double *testNormalArrayZ = new double[numPoints];
  double normal[3];
  double norm;
  for (vtkIdType pointId = 0; pointId < numPoints; ++pointId)
  {
    ref->GetPoint(pointId, point);
    norm = sqrt(point[0]*point[0] + point[1]*point[1] + point[2]*point[2]);
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
    refNormals->SetTuple(pointId, normal);
  }
  vtkNew<vtkCPExodusIIResultsArrayTemplate<double> > testNormals;
  testNormals->SetName("test-normals");
  std::vector<double*> testNormalVector;
  testNormalVector.push_back(testNormalArrayX);
  testNormalVector.push_back(testNormalArrayY);
  testNormalVector.push_back(testNormalArrayZ);
  testNormals->SetExodusScalarArrays(testNormalVector, numPoints);

  ref->GetPointData()->SetNormals(refNormals.GetPointer());
  test->GetPointData()->SetNormals(testNormals.GetPointer());
}

void testContourFilter(vtkUnstructuredGridBase *input,
                       vtkDataSet *&output,
                       double &time)
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkContourFilter> contour;
  contour->SetInputData(input);
  contour->GenerateValues(2, -0.5, 0.5);
  timer->StartTimer();
  contour->Update();
  timer->StopTimer();
  output = contour->GetOutput();
  output->Register(NULL);
  time = timer->GetElapsedTime();
}

void testDataSetSurfaceFilter(vtkUnstructuredGridBase *input,
                              vtkDataSet *&output,
                              double &time)
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkDataSetSurfaceFilter> extractSurface;
  extractSurface->SetInputData(input);
  extractSurface->SetNonlinearSubdivisionLevel(4);
  timer->StartTimer();
  extractSurface->Update();
  timer->StopTimer();
  output = extractSurface->GetOutput();
  output->Register(NULL);
  time = timer->GetElapsedTime();
}

void testCutterFilter(vtkUnstructuredGridBase *input,
                      vtkDataSet *&output,
                      double &time)
{
  vtkNew<vtkTimerLog> timer;

  // Create plane for testing slicing
  vtkNew<vtkPlane> slicePlane;
  slicePlane->SetOrigin(input->GetCenter());
  slicePlane->SetNormal(1.0, 1.0, 1.0);

  // Cutter (slice, polydata output)
  vtkNew<vtkCutter> cutter;
  cutter->SetInputData(input);
  cutter->SetCutFunction(slicePlane.GetPointer());
  cutter->SetGenerateTriangles(0);
  timer->StartTimer();
  cutter->Update();
  timer->StopTimer();
  output = cutter->GetOutput();
  output->Register(NULL);
  time = timer->GetElapsedTime();
}

void testExtractGeometryFilter(vtkUnstructuredGridBase *input,
                               vtkDataSet *&output,
                               double &time)
{
  vtkNew<vtkTimerLog> timer;

  // Create plane for testing slicing
  vtkNew<vtkPlane> slicePlane;
  slicePlane->SetOrigin(input->GetCenter());
  slicePlane->SetNormal(1.0, 1.0, 1.0);

  vtkNew<vtkExtractGeometry> extract;
  extract->SetInputData(input);
  extract->SetImplicitFunction(slicePlane.GetPointer());
  extract->SetExtractInside(1);
  extract->SetExtractOnlyBoundaryCells(1);
  extract->SetExtractBoundaryCells(1);
  timer->StartTimer();
  extract->Update();
  timer->StopTimer();
  output = extract->GetOutput();
  output->Register(NULL);
  time = timer->GetElapsedTime();
}

void testGlyph3DFilter(vtkUnstructuredGridBase *input,
                       vtkDataSet *&output,
                       double &time)
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
  output->Register(NULL);
  time = timer->GetElapsedTime();
}

void testWarpScalarFilter(vtkUnstructuredGridBase *input,
                          vtkDataSet *&output,
                          double &time)
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkWarpScalar> warpScalar;
  warpScalar->SetInputData(input);
  timer->StartTimer();
  warpScalar->Update();
  timer->StopTimer();
  output = warpScalar->GetOutput();
  output->Register(NULL);
  time = timer->GetElapsedTime();
}

void testWarpVectorFilter(vtkUnstructuredGridBase *input,
                          vtkDataSet *&output,
                          double &time)
{
  vtkNew<vtkTimerLog> timer;
  vtkNew<vtkWarpVector> warpVector;
  warpVector->SetInputData(input);
  warpVector->SetScaleFactor(1.0);
  timer->StartTimer();
  warpVector->Update();
  timer->StopTimer();
  output = warpVector->GetOutput();
  output->Register(NULL);
  time = timer->GetElapsedTime();
}

void testPipeline(vtkUnstructuredGridBase *input, vtkDataSet *&output,
                  double &time)
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
  output->Register(NULL);
  time = timer->GetElapsedTime();
}

#define doBenchmark(call_, reset_, timeLog_, repeat_) \
{ \
  timeLog_.clear(); \
  timeLog_.resize(repeat_, -500); \
  for (int benchmark = 0; benchmark < (repeat_); ++benchmark) \
  { \
    double &benchmarkTime = timeLog_[benchmark]; \
    call_; \
    if (benchmark + 1 != (repeat_)) \
    { \
      reset_; \
    } \
  } \
}

// Check that refOutput == testOutput, then delete and clear the outputs.
bool validateFilterOutput(const std::string &name,
                          vtkDataSet *&refOutput, vtkDataSet *&testOutput)
{
  if (refOutput->GetNumberOfPoints() == 0)
  {
    FAILB("Reference " << name << " produced an empty output!")
  }
  if (!compareDataSets(refOutput, testOutput))
  {
    FAILB(name << " output mismatch.")
  }
  cout << name << " produced " << refOutput->GetNumberOfPoints()
       << " points and " << refOutput->GetNumberOfCells() << " cells." << endl;
  refOutput->Delete();
  refOutput = NULL;
  testOutput->Delete();
  testOutput = NULL;
  return true;
}

void printTimingInfo(const std::string &name,
                     const std::vector<double> &ref,
                     const std::vector<double> &test)
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
  refAverage  /= static_cast<double>(ref.size());
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

  cout << "Timing info for test '" << name << "', "
       << ref.size() << " sample(s):\n\t"
       << "Average (ref | test | %slowdown): "
       << std::setprecision(6)
       << std::setw(9) << refAverage
       << std::setw(0) << " | "
       << std::setw(9) << testAverage
       << std::setw(0) << " | "
       << std::setw(9) << ((testAverage / refAverage) - 1.0) * 100
       << std::setw(0) << "%\n\t"
       << "Std Dev (ref | test): "
       << std::setw(9) << refStdev
       << std::setw(0) << " | "
       << std::setw(9) << testStdev
       << std::setw(0) << "\n\t"
       << "Minimum (ref | test): "
       << std::setw(9) << refMin
       << std::setw(0) << " | "
       << std::setw(9) << testMin
       << std::setw(0) << "\n\t"
       << "Maximum (ref | test): "
       << std::setw(9) << refMax
       << std::setw(0) << " | "
       << std::setw(9) << testMax
       << std::setw(0) << endl;
}

// The test to run while profiling or benchmarking:
#define CURRENT_TEST testContourFilter

// Define this to profile a particular filter (see testFilters(...)).
#undef PROFILE
//#define PROFILE CURRENT_TEST

// Define this to benchmark a particular filter (see testFilters(...)).
#undef BENCHMARK
//#define BENCHMARK CURRENT_TEST

bool testFilters(vtkUnstructuredGridBase *ref,
                 vtkUnstructuredGridBase *test)
{
  cout << "Number of points: " << ref->GetNumberOfPoints() << endl;
  cout << "Number of cells:  " << ref->GetNumberOfCells() << endl;

  // Number of times to run each benchmark. Don't commit a value greater than
  // 1 to keep the dashboards fast, but this can be increased while benchmarking
  // or profiling particular filters.
  int numBenchmarks = 1;

  // Temporary variables for outputs.
  vtkDataSet *refOutput(NULL);
  vtkDataSet *testOutput(NULL);

#ifdef PROFILE
  // Profiling, multirun:
  std::vector<double> profileTimes;
  doBenchmark(PROFILE(test, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              profileTimes, numBenchmarks);
  return true;
#endif

#ifdef BENCHMARK
  // Benchmarking:
  std::vector<double> benchmarkRefTimes;
  std::vector<double> benchmarkTestTimes;
  doBenchmark(BENCHMARK(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              benchmarkRefTimes, numBenchmarks);
  doBenchmark(BENCHMARK(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              benchmarkTestTimes, numBenchmarks);
  if (!validateFilterOutput("Benchmark:", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("Benchmark", benchmarkRefTimes, benchmarkTestTimes);
  return true;
#endif

  //////////////////////////////
  // Actual tests start here: //
  //////////////////////////////

  // Contour filter
  std::vector<double> contourRefTimes;
  std::vector<double> contourTestTimes;
  doBenchmark(testContourFilter(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              contourRefTimes, numBenchmarks);
  doBenchmark(testContourFilter(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              contourTestTimes, numBenchmarks);
  if (!validateFilterOutput("Contour filter", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("contour", contourRefTimes, contourTestTimes);

  // Extract surface
  std::vector<double> dataSetSurfaceRefTimes;
  std::vector<double> dataSetSurfaceTestTimes;
  doBenchmark(testDataSetSurfaceFilter(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              dataSetSurfaceRefTimes, numBenchmarks);
  doBenchmark(testDataSetSurfaceFilter(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              dataSetSurfaceTestTimes, numBenchmarks);
  if (!validateFilterOutput("Data set surface filter", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("dataset surface", dataSetSurfaceRefTimes,
                  dataSetSurfaceTestTimes);

  // Cutter
  std::vector<double> cutterRefTimes;
  std::vector<double> cutterTestTimes;
  doBenchmark(testCutterFilter(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              cutterRefTimes, numBenchmarks);
  doBenchmark(testCutterFilter(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              cutterTestTimes, numBenchmarks);
  if (!validateFilterOutput("Cutter", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("cutter", cutterRefTimes, cutterTestTimes);

  // Extract geometry
  std::vector<double> extractGeometryRefTimes;
  std::vector<double> extractGeometryTestTimes;
  doBenchmark(testExtractGeometryFilter(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              extractGeometryRefTimes, numBenchmarks);
  doBenchmark(testExtractGeometryFilter(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              extractGeometryTestTimes, numBenchmarks);
  if (!validateFilterOutput("Extract geometry", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("extract geometry", extractGeometryRefTimes,
                  extractGeometryTestTimes);

  // Glyph3D
  std::vector<double> glyph3dRefTimes;
  std::vector<double> glyph3dTestTimes;
  doBenchmark(testGlyph3DFilter(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              glyph3dRefTimes, numBenchmarks);
  doBenchmark(testGlyph3DFilter(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              glyph3dTestTimes, numBenchmarks);
  if (!validateFilterOutput("Glyph3D", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("glyph3d", glyph3dRefTimes, glyph3dTestTimes);

  // Warp scalar
  std::vector<double> warpScalarRefTimes;
  std::vector<double> warpScalarTestTimes;
  doBenchmark(testWarpScalarFilter(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              warpScalarRefTimes, numBenchmarks);
  doBenchmark(testWarpScalarFilter(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              warpScalarTestTimes, numBenchmarks);
  if (!validateFilterOutput("Warp scalar", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("warp scalar", warpScalarRefTimes, warpScalarTestTimes);

  // Warp vector
  std::vector<double> warpVectorRefTimes;
  std::vector<double> warpVectorTestTimes;
  doBenchmark(testWarpVectorFilter(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              warpVectorRefTimes, numBenchmarks);
  doBenchmark(testWarpVectorFilter(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              warpVectorTestTimes, numBenchmarks);
  if (!validateFilterOutput("Warp vector", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("warp vector", warpVectorRefTimes, warpVectorTestTimes);

  // mini-mapped pipeline (Warp scalar + vector)
  std::vector<double> pipelineRefTimes;
  std::vector<double> pipelineTestTimes;
  doBenchmark(testPipeline(ref, refOutput, benchmarkTime),
              refOutput->Delete(); refOutput = NULL,
              pipelineRefTimes, numBenchmarks);
  doBenchmark(testPipeline(test, testOutput, benchmarkTime),
              testOutput->Delete(); testOutput = NULL,
              pipelineTestTimes, numBenchmarks);
  // Ensure that the mapped test produced a mapped output:
  if (!testOutput->IsA("vtkCPExodusIIElementBlock"))
  {
    cerr << "Pipeline test did not produce a mapped output object!" << endl;
    return false;
  }
  if (!validateFilterOutput("Pipeline test", refOutput, testOutput))
  {
    return false;
  }
  printTimingInfo("pipeline", pipelineRefTimes, pipelineTestTimes);

  return true;
}

bool testCopies(vtkUnstructuredGridBase *test)
{
  vtkNew<vtkUnstructuredGrid> vtkTarget;
  vtkSmartPointer<vtkUnstructuredGridBase> mappedTarget =
      vtkSmartPointer<vtkUnstructuredGridBase>::Take(test->NewInstance());

  // No deep copy into test class -- it's read only. Can shallow copy into test
  // class, since it will just share the implementation instance.

  // Deep copy: test --> vtk
  vtkTarget->DeepCopy(test);
  if (!compareDataSets(test, vtkTarget.GetPointer()))
  {
    FAILB("Deep copy insitu --> VTK failed.")
  }
  vtkTarget->Reset();

  // Shallow copy: test --> vtk
  vtkTarget->ShallowCopy(test); // Should really deep copy.
  if (!compareDataSets(test, vtkTarget.GetPointer()))
  {
    FAILB("Shallow copy insitu --> VTK failed.")
  }
  vtkTarget->Reset();

  // Shallow copy: test --> test
  mappedTarget->ShallowCopy(test);
  if (!compareDataSets(test, mappedTarget))
  {
    FAILB("Shallow copy insitu --> insitu failed.")
  }
  mappedTarget->Initialize();

  return true;
}

void testSaveArrays()
{
  vtkIdType numPoints = 1000;
  vtkNew<vtkCPExodusIIResultsArrayTemplate<double> > testScalars;
  testScalars->SetName("test-scalars");
  double *testScalarArray = new double[numPoints];
  for(int i=0;i<numPoints;i++)
  {
    testScalarArray[i] = 1;
  }
  // Call SetExodusScalarArrays a couple of times to make sure
  // we don't free the same memory multiple times. The final call
  // is the one that should actually free the array.
  testScalars->SetExodusScalarArrays(std::vector<double*>(1, testScalarArray),
                                     numPoints, true);
  testScalars->SetExodusScalarArrays(std::vector<double*>(1, testScalarArray),
                                     numPoints, true);
  testScalars->SetExodusScalarArrays(std::vector<double*>(1, testScalarArray),
                                     numPoints, false);
}

int TestInSituExodus(int argc, char *argv[])
{
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();

  char *fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                         "Data/box-noglom.ex2");
  std::string fileName(fileNameC);
  delete [] fileNameC;

  // Read reference copy
  vtkNew<vtkMultiBlockDataSet> refMBDS;
  readExodusCopy(fileName, refMBDS.GetPointer());
  vtkUnstructuredGridBase *refGrid(getConnectivityBlock(refMBDS.GetPointer()));
  if (!refGrid)
  {
    FAIL("Error retrieving reference element block container.");
  }

  // Read in-situ copy
  vtkNew<vtkCPExodusIIInSituReader> reader;
  reader->SetFileName(fileName.c_str());
  reader->Update();
  vtkMultiBlockDataSet *testMBDS = reader->GetOutput();
  vtkUnstructuredGridBase *grid(getConnectivityBlock(testMBDS));
  if (!grid)
  {
    FAIL("Error retrieving testing element block container.")
  }

#ifndef PROFILE // These just add noise during profiling:
  // Compare
  if (!compareDataSets(refGrid, grid))
  {
    FAIL("In-situ data set doesn't match reference data!")
  }

  if (!testCopies(grid))
  {
    FAIL("A copy test failed.")
  }
#endif

  populateAttributes(refGrid, grid);

  // Test selected filters
  if (!testFilters(refGrid, grid))
  {
    FAIL("Pipeline test failed!")
  }

  testSaveArrays();

  timer->StopTimer();
  double time = timer->GetElapsedTime();
  cout << "Test took " << static_cast<int>(time / 60) << "m "
       << std::fmod(time, 60.0) << "s." << endl;
  return EXIT_SUCCESS;
}
