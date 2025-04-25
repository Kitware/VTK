// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObject.h"
#include "vtkHDFReader.h"

#include "vtkAMRBox.h"
#include "vtkAppendDataSets.h"
#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkOverlappingAMR.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPartitionedDataSetReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLUniformGridAMRReader.h"

#include <cstdlib>
#include <map>
#include <vector>

namespace
{
constexpr double CHECK_TOLERANCE = 1e-3;

std::map<int, std::vector<double>> EXPECTED_POINTS_AT_TIMESTEP = {
  { 0,
    { 0, 0, 11.9998, -5.21901, -2.32365, -7.51521, 20.2246, 24.0492, 12.0988, 15.7622, 7.74817,
      16.1954 } },
  { 5,
    { 0, 0, 11.3888, -5.23095, -2.32897, -7.53241, 20.9518, 25.3087, 11.695, 15.8703, 6.71963,
      16.365 } },
  { 10,
    { 0, 0, 11.4393, -4.73392, -2.10768, -6.8167, 21.3814, 26.053, 11.4564, 15.9848, 5.63023,
      16.5446 } }
};

constexpr int EXPECTED_SHAPE_AT_TIMESTEP[3][2] = { { 3, 1 }, { 1, 2 }, { 2, 2 } };

// analytical functions
template <typename Vec>
double Sin11T(double t, const Vec& point);

// generic workers
struct OpenerWorklet;
struct CheckerWorklet;

// assemblies
int TestUGTemporal(const std::string& dataRoot);
int TestImageDataTemporal(const std::string& dataRoot);
int TestPolyDataTemporal(const std::string& dataRoot);
int TestPolyDataTemporalWithOffset(const std::string& dataRoot);
int TestUGTemporalWithCachePartitioned(const std::string& dataRoot);
int TestUGTemporalPartitionedNoCache(const std::string& dataRoot);
int TestImageDataTemporalWithCache(const std::string& dataRoot);
int TestPolyDataTemporalWithCache(const std::string& dataRoot);
int TestPolyDataTemporalFieldData(const std::string& dataRoot);
int TestHyperTreeGridTemporal(const std::string& dataRoot, unsigned int depthLimit);
int TestHyperTreeGridPartitionedTemporal(const std::string& dataRoot);
int TestOverlappingAMRTemporal(const std::string& dataRoot);
int TestOverlappingAMRTemporalLegacy(const std::string& dataRoot);
}

//------------------------------------------------------------------------------
int TestHDFReaderTemporal(int argc, char* argv[])
{
  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();
  int res = ::TestUGTemporal(dataRoot);
  res |= ::TestImageDataTemporal(dataRoot);
  res |= ::TestPolyDataTemporal(dataRoot);
  res |= ::TestPolyDataTemporalWithOffset(dataRoot);
  res |= ::TestUGTemporalPartitionedNoCache(dataRoot);
  res |= ::TestUGTemporalWithCachePartitioned(dataRoot);
  res |= ::TestImageDataTemporalWithCache(dataRoot);
  res |= ::TestPolyDataTemporalWithCache(dataRoot);
  res |= ::TestPolyDataTemporalFieldData(dataRoot);
  res |= ::TestHyperTreeGridTemporal(dataRoot, 3);
  res |= ::TestHyperTreeGridTemporal(dataRoot, 1);
  res |= ::TestHyperTreeGridPartitionedTemporal(dataRoot);
  res |= ::TestOverlappingAMRTemporal(dataRoot);
  res |= ::TestOverlappingAMRTemporalLegacy(dataRoot);

  return res;
}

namespace
{

template <typename Vec>
double Sin11T(double time, const Vec& point)
{
  return std::sin(vtkMath::Pi() * time + point[0] + point[1]);
}

struct OpenerWorklet
{
public:
  OpenerWorklet(const std::string& filePath, bool mergeParts = true)
    : MergeParts(mergeParts)
  {
    this->Reader->SetFileName(filePath.c_str());
    this->Reader->Update();
  }
  vtkSmartPointer<vtkDataObject> operator()(std::size_t timeStep)
  {
    this->Reader->SetStep(timeStep);
    this->Reader->Update();
    if (this->MergeParts)
    {
      return this->MergeBlocksIfNeeded(this->Reader->GetOutputDataObject(0));
    }
    else
    {
      return this->Reader->GetOutputDataObject(0);
    }
  }

  vtkOverlappingAMR* GetDataObjectAsAMR()
  {
    return vtkOverlappingAMR::SafeDownCast(this->Reader->GetOutput());
  }

  void UpdateStep(std::size_t timeStep)
  {
    this->Reader->SetStep(timeStep);
    this->Reader->Update();
  }

  vtkHDFReader* GetReader() { return this->Reader; }

  vtkSmartPointer<vtkDataObject> MergeBlocksIfNeeded(vtkDataObject* data)
  {
    vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(data);

    if (!pds)
    {
      return data; // No merging to do
    }

    vtkNew<vtkAppendDataSets> append;
    append->SetOutputDataSetType(pds->GetPartition(0)->GetDataObjectType());
    for (unsigned int iPiece = 0; iPiece < pds->GetNumberOfPartitions(); ++iPiece)
    {
      append->AddInputData(pds->GetPartition(iPiece));
    }
    append->Update();
    vtkDataObject* merged = append->GetOutputDataObject(0);
    merged->SetFieldData(pds->GetFieldData());
    merged->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),
      data->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP()));
    return merged;
  }

private:
  vtkNew<vtkHDFReader> Reader;
  bool MergeParts = true;
};

struct CheckerWorklet
{
public:
  CheckerWorklet(double tolerance)
    : Tolerance(tolerance)
  {
  }

  template <typename LFunctor, typename RFunctor>
  bool operator()(vtkIdType begin, vtkIdType end, LFunctor LHS, RFunctor RHS)
  {
    for (vtkIdType iter = begin; iter < end; ++iter)
    {
      auto lhs = LHS(iter);
      auto rhs = RHS(iter);
      if (!vtkMathUtilities::FuzzyCompare(
            static_cast<double>(lhs), static_cast<double>(rhs), this->Tolerance))
      {
        std::cerr << "Failed check at " << iter << " with LHS = " << lhs << " != " << rhs
                  << " = RHS" << std::endl;
        return false;
      }
    }
    return true;
  }

private:
  double Tolerance = CHECK_TOLERANCE;
};

struct GeometryCheckerWorklet
{
public:
  GeometryCheckerWorklet(double tolerance)
    : Tolerance(tolerance)
  {
  }

  template <typename DSetT>
  bool operator()(DSetT*, DSetT*)
  {
    std::cerr << "Called unspecialized worker" << std::endl;
    return false;
  }

private:
  double Tolerance = CHECK_TOLERANCE;
};

//------------------------------------------------------------------------------
template <>
bool GeometryCheckerWorklet::operator()(vtkUnstructuredGrid* lhs, vtkUnstructuredGrid* rhs)
{
  CheckerWorklet checks(this->Tolerance);

  // Geometry checks
  auto refRange = vtk::DataArrayValueRange<3>(lhs->GetPoints()->GetData());
  auto getLHSPoints = [&](vtkIdType iPComp) { return refRange[iPComp]; };
  auto hdfRange = vtk::DataArrayValueRange<3>(rhs->GetPoints()->GetData());
  auto getRHSPoints = [&](vtkIdType iPComp) { return hdfRange[iPComp]; };
  if (!checks(0, lhs->GetNumberOfPoints() * 3, getLHSPoints, getRHSPoints))
  {
    std::cerr << "Points: Failed point geometry checks" << std::endl;
    return false;
  }

  auto refConnRange = vtk::DataArrayValueRange<1>(lhs->GetCells()->GetConnectivityArray());
  auto getLHSConn = [&](vtkIdType iConn) { return refConnRange[iConn]; };
  auto hdfConnRange = vtk::DataArrayValueRange<1>(rhs->GetCells()->GetConnectivityArray());
  auto getRHSConn = [&](vtkIdType iConn) { return hdfConnRange[iConn]; };
  if (!checks(0, refConnRange.size(), getLHSConn, getRHSConn))
  {
    std::cerr << "Connectivity: Failed connectivity geometry checks" << std::endl;
    return false;
  }

  auto refOffRange = vtk::DataArrayValueRange<1>(lhs->GetCells()->GetOffsetsArray());
  auto getLHSOff = [&](vtkIdType iOff) { return refOffRange[iOff]; };
  auto hdfOffRange = vtk::DataArrayValueRange<1>(rhs->GetCells()->GetOffsetsArray());
  auto getRHSOff = [&](vtkIdType iOff) { return hdfOffRange[iOff]; };
  if (!checks(0, refOffRange.size(), getLHSOff, getRHSOff))
  {
    std::cerr << "Offsets: Failed offsets geometry checks" << std::endl;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
template <>
bool GeometryCheckerWorklet::operator()(vtkImageData* lhs, vtkImageData* rhs)
{
  const auto lExtent = lhs->GetExtent();
  const auto rExtent = rhs->GetExtent();
  for (vtkIdType iE = 0; iE < 6; ++iE)
  {
    if (lExtent[iE] - rExtent[iE] > this->Tolerance)
    {
      std::cerr << "Extents: Failed extent geometry checks" << std::endl;
      return false;
    }
  }

  const auto lSpacing = lhs->GetSpacing();
  const auto rSpacing = rhs->GetSpacing();
  for (vtkIdType iS = 0; iS < 3; ++iS)
  {
    if (lSpacing[iS] - rSpacing[iS] > this->Tolerance)
    {
      std::cerr << "Spacing: Failed spacing geometry checks" << std::endl;
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
template <>
bool GeometryCheckerWorklet::operator()(vtkPolyData* lhs, vtkPolyData* rhs)
{
  CheckerWorklet checks(this->Tolerance);

  // Geometry checks
  auto refRange = vtk::DataArrayValueRange<3>(lhs->GetPoints()->GetData());
  auto getLHSPoints = [&](vtkIdType iPComp) { return refRange[iPComp]; };
  auto hdfRange = vtk::DataArrayValueRange<3>(rhs->GetPoints()->GetData());
  auto getRHSPoints = [&](vtkIdType iPComp) { return hdfRange[iPComp]; };
  if (!checks(0, lhs->GetNumberOfPoints() * 3, getLHSPoints, getRHSPoints))
  {
    std::cerr << "Points: Failed point geometry checks" << std::endl;
    return false;
  }

  std::map<std::string, std::pair<vtkCellArray*, vtkCellArray*>> topos{
    { "Verts", std::make_pair(lhs->GetVerts(), rhs->GetVerts()) },
    { "Lines", std::make_pair(lhs->GetLines(), rhs->GetLines()) },
    { "Polys", std::make_pair(lhs->GetPolys(), rhs->GetPolys()) },
    { "Strips", std::make_pair(lhs->GetStrips(), rhs->GetStrips()) }
  };
  for (auto& keyVal : topos)
  {
    auto refConnRange = vtk::DataArrayValueRange<1>(keyVal.second.first->GetConnectivityArray());
    auto getLHSConn = [&](vtkIdType iConn) { return refConnRange[iConn]; };
    auto hdfConnRange = vtk::DataArrayValueRange<1>(keyVal.second.second->GetConnectivityArray());
    auto getRHSConn = [&](vtkIdType iConn) { return hdfConnRange[iConn]; };
    if (!checks(0, refConnRange.size(), getLHSConn, getRHSConn))
    {
      std::cerr << "Connectivity: Failed connectivity geometry checks for " << keyVal.first
                << std::endl;
      return false;
    }

    auto refOffRange = vtk::DataArrayValueRange<1>(keyVal.second.first->GetOffsetsArray());
    auto getLHSOff = [&](vtkIdType iOff) { return refOffRange[iOff]; };
    auto hdfOffRange = vtk::DataArrayValueRange<1>(keyVal.second.second->GetOffsetsArray());
    auto getRHSOff = [&](vtkIdType iOff) { return hdfOffRange[iOff]; };
    if (!checks(0, refOffRange.size(), getLHSOff, getRHSOff))
    {
      std::cerr << "Offsets: Failed offsets geometry checks" << std::endl;
      return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
int TestUGTemporalBase(OpenerWorklet& opener, bool testMeshMTime = false)
{
  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Reference Geometry
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(20);
  sphere->Update();
  vtkNew<vtkSphereSource> sphere1;
  std::array<double, 3> center = { 1.0, 1.0, 1.0 };
  sphere1->SetCenter(center.data());
  sphere1->SetThetaResolution(20);
  sphere1->SetPhiResolution(20);
  sphere1->Update();
  vtkNew<vtkAppendFilter> appender;
  appender->AddInputData(sphere->GetOutput());
  appender->AddInputData(sphere1->GetOutput());
  appender->Update();
  vtkDataSet* refGeometry = vtkDataSet::SafeDownCast(appender->GetOutputDataObject(0));
  std::array<vtkMTimeType, 2> meshMTime{ 0, 0 };

  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    vtkSmartPointer<vtkDataSet> dSet = vtkDataSet::SafeDownCast(opener(iStep));
    // Local Time Checks
    double readerTime = opener.GetReader()->GetTimeValue();
    if (!vtkMathUtilities::FuzzyCompare(
          readerTime, static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: TimeValue is wrong: " << readerTime
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    double dataTime = dSet->GetInformation()->Get(vtkDataObject::DATA_TIME_STEP());
    if (readerTime != dataTime)
    {
      std::cerr << "Output DATA_TIME_STEP is wrong: " << dataTime << " != " << readerTime
                << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = dSet->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    GeometryCheckerWorklet gChecker(CHECK_TOLERANCE);
    if (!gChecker(
          vtkUnstructuredGrid::SafeDownCast(refGeometry), vtkUnstructuredGrid::SafeDownCast(dSet)))
    {
      std::cerr << "Geometry: Failed geometry checks." << std::endl;
      return EXIT_FAILURE;
    }

    CheckerWorklet checks(CHECK_TOLERANCE);

    // Point Data checks
    auto getLHSPData = [&](vtkIdType iP)
    {
      std::array<double, 3> point = { 0 };
      dSet->GetPoint(iP, point.data());
      return ::Sin11T(dSet->GetFieldData()->GetArray("Time")->GetComponent(0, 0), point);
    };
    auto getRHSPData = [&](vtkIdType iP)
    { return dSet->GetPointData()->GetArray("Modulator")->GetComponent(iP, 0); };

    if (!checks(0, dSet->GetNumberOfPoints(), getLHSPData, getRHSPData))
    {
      std::cerr << "PointData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }

    meshMTime[1] = meshMTime[0];
    meshMTime[0] = vtkUnstructuredGrid::SafeDownCast(dSet)->GetMeshMTime();
    if (testMeshMTime && (iStep > 0 && iStep < 10))
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestUGTemporalPartitioned(
  OpenerWorklet& opener, const std::string& dataRoot, bool testMeshMTime = false)
{
  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<vtkMTimeType, 2> meshMTime{ 0, 0 };
  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    vtkSmartPointer<vtkPartitionedDataSet> dSet =
      vtkPartitionedDataSet::SafeDownCast(opener(iStep));

    // Reference Geometry
    vtkNew<vtkXMLPartitionedDataSetReader> refReader;
    refReader->SetFileName((dataRoot + "/Data/hdf_transient_partitioned_ug_twin/transient_sphere_" +
      std::to_string(iStep) + ".vtpd")
                             .c_str());
    vtkPartitionedDataSet* refGeometry =
      vtkPartitionedDataSet::SafeDownCast(refReader->GetOutputDataObject(0));
    refReader->Update();

    // Local Time Checks
    if (!vtkMathUtilities::FuzzyCompare(
          opener.GetReader()->GetTimeValue(), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: TimeValue is wrong: " << opener.GetReader()->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = dSet->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    unsigned int dSetPartitionNb = dSet->GetNumberOfPartitions();
    unsigned int refGeometryPartitionNb = refGeometry->GetNumberOfPartitions();

    if (dSetPartitionNb != refGeometryPartitionNb)
    {
      std::cerr << "The number of partitions of the data is wrong :" << dSetPartitionNb
                << " should be " << refGeometryPartitionNb << std::endl;
      return EXIT_FAILURE;
    }

    int maxMeshMTimePartition = -1;

    for (unsigned int i = 0; i < refGeometryPartitionNb; i++)
    {
      auto refPartition = vtkUnstructuredGrid::SafeDownCast(refGeometry->GetPartition(i));
      auto dSetPartition = vtkUnstructuredGrid::SafeDownCast(dSet->GetPartition(i));

      if (testMeshMTime)
      {
        maxMeshMTimePartition = std::max(
          static_cast<int>(vtkUnstructuredGrid::SafeDownCast(dSetPartition)->GetMeshMTime()),
          maxMeshMTimePartition);
      }

      GeometryCheckerWorklet gChecker(CHECK_TOLERANCE);
      if (!gChecker(refPartition, dSetPartition))
      {
        std::cerr << "Geometry: Failed geometry checks." << std::endl;
        return EXIT_FAILURE;
      }

      CheckerWorklet checks(CHECK_TOLERANCE);

      // Point Data checks
      auto getLHSPData = [&](vtkIdType iP)
      {
        std::array<double, 3> point = { 0 };
        dSetPartition->GetPoint(iP, point.data());
        return ::Sin11T(dSet->GetFieldData()->GetArray("Time")->GetComponent(0, 0), point);
      };
      auto getRHSPData = [&](vtkIdType iP)
      { return dSetPartition->GetPointData()->GetArray("Modulator")->GetComponent(iP, 0); };

      if (!checks(0, dSetPartition->GetNumberOfPoints(), getLHSPData, getRHSPData))
      {
        std::cerr << "PointData: Failed array checks" << std::endl;
        return EXIT_FAILURE;
      }
    }

    meshMTime[1] = meshMTime[0];
    meshMTime[0] = maxMeshMTimePartition;
    if (testMeshMTime && (iStep > 0 && iStep < 10))
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << " at timestep :" << iStep << std::endl;
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestUGTemporalPartitionedNoCache(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_sphere.hdf", false);
  return TestUGTemporalPartitioned(opener, dataRoot, false);
}

//------------------------------------------------------------------------------
int TestUGTemporal(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_sphere.hdf");
  return TestUGTemporalBase(opener);
}

//------------------------------------------------------------------------------
int TestUGTemporalWithCachePartitioned(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_sphere.hdf", false);
  opener.GetReader()->UseCacheOn();
  return TestUGTemporalPartitioned(opener, dataRoot, true);
}

//------------------------------------------------------------------------------
int TestImageDataTemporalBase(OpenerWorklet& opener)
{
  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  // Reference Geometry
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->Update();
  vtkDataSet* refGeometry = vtkDataSet::SafeDownCast(wavelet->GetOutputDataObject(0));

  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    vtkSmartPointer<vtkDataSet> dSet = vtkDataSet::SafeDownCast(opener(iStep));

    // Local Time Checks
    if (!vtkMathUtilities::FuzzyCompare(
          opener.GetReader()->GetTimeValue(), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: Time Value is wrong: " << opener.GetReader()->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = dSet->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    GeometryCheckerWorklet gChecker(CHECK_TOLERANCE);
    if (!gChecker(vtkImageData::SafeDownCast(refGeometry), vtkImageData::SafeDownCast(dSet)))
    {
      std::cerr << "Geometry: Failed geometry checks." << std::endl;
      return EXIT_FAILURE;
    }

    CheckerWorklet checks(CHECK_TOLERANCE);

    // Point Data checks
    auto getLHSPData = [&](vtkIdType iP)
    {
      auto wave = refGeometry->GetPointData()->GetArray("RTData");
      return dSet->GetFieldData()->GetArray("Time")->GetComponent(0, 0) * wave->GetComponent(iP, 0);
    };
    auto getRHSPData = [&](vtkIdType iP)
    { return dSet->GetPointData()->GetArray("Modulator")->GetComponent(iP, 0); };

    if (!checks(0, dSet->GetNumberOfPoints(), getLHSPData, getRHSPData))
    {
      std::cerr << "PointData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }

    // Cell Data checks
    auto getLHSCData = [&](vtkIdType iC) { return iC; };
    auto getRHSCData = [&](vtkIdType iC)
    { return dSet->GetCellData()->GetArray("IDs")->GetComponent(iC, 0); };

    if (!checks(0, dSet->GetNumberOfCells(), getLHSCData, getRHSCData))
    {
      std::cerr << "CellData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestImageDataTemporal(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_wavelet.hdf");
  return TestImageDataTemporalBase(opener);
}

//------------------------------------------------------------------------------
int TestImageDataTemporalWithCache(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_wavelet.hdf");
  opener.GetReader()->UseCacheOn();
  return TestImageDataTemporalBase(opener);
}

//------------------------------------------------------------------------------
int TestPolyDataTemporalBase(
  OpenerWorklet& opener, const std::string& dataRoot, bool testMeshMTime = false)
{
  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<vtkMTimeType, 2> meshMTime;
  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    vtkSmartPointer<vtkDataSet> dSet = vtkDataSet::SafeDownCast(opener(iStep));

    // Reference Geometry
    vtkNew<vtkXMLPolyDataReader> refReader;
    refReader->SetFileName(
      (dataRoot + "/Data/hdf_transient_poly_data_twin/hdf_transient_poly_data_twin_00" +
        std::to_string(iStep) + ".vtp")
        .c_str());
    refReader->Update();

    vtkDataSet* refGeometry = vtkDataSet::SafeDownCast(refReader->GetOutputDataObject(0));

    // Local Time Checks
    if (!vtkMathUtilities::FuzzyCompare(
          opener.GetReader()->GetTimeValue(), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: TimeValue is wrong: " << opener.GetReader()->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = dSet->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    GeometryCheckerWorklet gChecker(CHECK_TOLERANCE);
    if (!gChecker(vtkPolyData::SafeDownCast(refGeometry), vtkPolyData::SafeDownCast(dSet)))
    {
      std::cerr << "Geometry: Failed geometry checks." << std::endl;
      return EXIT_FAILURE;
    }

    meshMTime[1] = meshMTime[0];
    meshMTime[0] = vtkPolyData::SafeDownCast(dSet)->GetMeshMTime();
    if (testMeshMTime && (iStep > 0 && iStep < 6))
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }

    CheckerWorklet checks(CHECK_TOLERANCE);

    // Point Data checks
    auto lhsPRange = vtk::DataArrayValueRange<3>(refGeometry->GetPointData()->GetArray("Warping"));
    auto getLHSPData = [&](vtkIdType iC) { return lhsPRange[iC]; };
    auto rhsPRange = vtk::DataArrayValueRange<3>(dSet->GetPointData()->GetArray("Warping"));
    auto getRHSPData = [&](vtkIdType iC) { return rhsPRange[iC]; };

    if (!checks(0, dSet->GetNumberOfPoints() * 3, getLHSPData, getRHSPData))
    {
      std::cerr << "PointData: Failed array checks at step " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    // Cell Data checks
    auto lhsCRange = vtk::DataArrayValueRange<1>(refGeometry->GetCellData()->GetArray("Materials"));
    auto getLHSCData = [&](vtkIdType iC) { return lhsCRange[iC]; };
    auto rhsCRange = vtk::DataArrayValueRange<1>(dSet->GetCellData()->GetArray("Materials"));
    auto getRHSCData = [&](vtkIdType iC) { return rhsCRange[iC]; };

    if (!checks(0, dSet->GetNumberOfCells(), getLHSCData, getRHSCData))
    {
      std::cerr << "CellData: Failed array checks at step " << iStep << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPolyDataTemporalPartitionedWithCache(
  OpenerWorklet& opener, const std::string& dataRoot, bool testMeshMTime = false)
{
  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::array<int, 2> meshMTime{ 0, 0 };
  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    vtkSmartPointer<vtkPartitionedDataSet> dSet =
      vtkPartitionedDataSet::SafeDownCast(opener(iStep));

    // Reference Geometry
    vtkNew<vtkXMLPartitionedDataSetReader> refReader;
    refReader->SetFileName(
      (dataRoot + "/Data/hdf_transient_partitioned_poly_data_twin/transient_sphere_" +
        std::to_string(iStep) + ".vtpd")
        .c_str());
    refReader->Update();

    vtkPartitionedDataSet* refGeometry =
      vtkPartitionedDataSet::SafeDownCast(refReader->GetOutputDataObject(0));

    unsigned int dSetPartitionNb = dSet->GetNumberOfPartitions();
    unsigned int refGeometryPartitionNb = refGeometry->GetNumberOfPartitions();

    if (dSetPartitionNb != refGeometryPartitionNb)
    {
      std::cerr << "The number of partitions of the data is wrong :" << dSetPartitionNb
                << " should be " << refGeometryPartitionNb << std::endl;
      return EXIT_FAILURE;
    }

    int maxMeshMTimePartition = -1;
    for (unsigned int i = 0; i < refGeometryPartitionNb; i++)
    {
      auto refPartition = vtkPolyData::SafeDownCast(refGeometry->GetPartition(i));
      auto dSetPartition = vtkPolyData::SafeDownCast(dSet->GetPartition(i));

      GeometryCheckerWorklet gChecker(CHECK_TOLERANCE);
      if (!gChecker(refPartition, dSetPartition))
      {
        std::cerr << "Geometry: Failed geometry checks for partition : " << i << std::endl;
        return EXIT_FAILURE;
      }

      maxMeshMTimePartition =
        std::max(static_cast<int>(vtkPolyData::SafeDownCast(dSetPartition)->GetMeshMTime()),
          maxMeshMTimePartition);

      CheckerWorklet checks(CHECK_TOLERANCE);

      // Point Data checks
      auto lhsPRange =
        vtk::DataArrayValueRange<3>(refPartition->GetPointData()->GetArray("Warping"));
      auto getLHSPData = [&](vtkIdType iC) { return lhsPRange[iC]; };
      auto rhsPRange =
        vtk::DataArrayValueRange<3>(dSetPartition->GetPointData()->GetArray("Warping"));
      auto getRHSPData = [&](vtkIdType iC) { return rhsPRange[iC]; };
      if (!checks(0, dSetPartition->GetNumberOfPoints() * 3, getLHSPData, getRHSPData))
      {
        std::cerr << "PointData: Failed array checks at step " << iStep << " for partition :" << i
                  << std::endl;
        return EXIT_FAILURE;
      }

      // Cell Data checks
      auto lhsCRange =
        vtk::DataArrayValueRange<1>(refPartition->GetCellData()->GetArray("Materials"));
      auto getLHSCData = [&](vtkIdType iC) { return lhsCRange[iC]; };
      auto rhsCRange =
        vtk::DataArrayValueRange<1>(dSetPartition->GetCellData()->GetArray("Materials"));
      auto getRHSCData = [&](vtkIdType iC) { return rhsCRange[iC]; };

      if (!checks(0, dSetPartition->GetNumberOfCells(), getLHSCData, getRHSCData))
      {
        std::cerr << "CellData: Failed array checks at step " << iStep << std::endl;
        return EXIT_FAILURE;
      }
    }
    meshMTime[0] = meshMTime[1];
    meshMTime[1] = maxMeshMTimePartition;
    if (testMeshMTime && (iStep > 0 && iStep < 6))
    {
      if (meshMTime[0] != meshMTime[1])
      {
        std::cerr << "MTime: Failed MeshMTime check - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
    else if (testMeshMTime && (iStep == 6))
    {
      if (meshMTime[0] == meshMTime[1])
      {
        std::cerr << "MTime: Failed MeshMTime shouldn't be equal - previous = " << meshMTime[1]
                  << " while current = " << meshMTime[0] << std::endl;
        return EXIT_FAILURE;
      }
    }
    // Local Time Checks
    if (!vtkMathUtilities::FuzzyCompare(
          opener.GetReader()->GetTimeValue(), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "Property: TimeValue is wrong: " << opener.GetReader()->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = dSet->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cerr << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cerr << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPolyDataTemporal(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/test_transient_poly_data.hdf");
  return TestPolyDataTemporalBase(opener, dataRoot);
}

//------------------------------------------------------------------------------
int TestPolyDataTemporalWithCache(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/test_transient_poly_data.hdf", false);
  opener.GetReader()->UseCacheOn();

  // We should be able to activate the MeshMTime testing once the cache can store
  // the intermediate vtkPoints and vtkCellArrays
  return TestPolyDataTemporalPartitionedWithCache(opener, dataRoot, true /*testMeshMTime*/);
}

//------------------------------------------------------------------------------
int TestPolyDataTemporalWithOffset(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/test_transient_poly_data_offset.vtkhdf");

  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 12)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 12 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.719948, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.719948) != (" << tRange[0] << ", " << tRange[1]
              << ")" << std::endl;
    return EXIT_FAILURE;
  }

  for (int iStep = 0; iStep < 12; iStep += 5)
  {
    // Open data at right time
    vtkSmartPointer<vtkDataSet> dSet = vtkDataSet::SafeDownCast(opener(iStep));

    int it = 0;
    for (vtkIdType id = 0; id < dSet->GetNumberOfPoints(); id += 500)
    {
      double* pts = dSet->GetPoint(id);

      double expected_valueX = EXPECTED_POINTS_AT_TIMESTEP[iStep][it * 3];
      double expected_valueY = EXPECTED_POINTS_AT_TIMESTEP[iStep][it * 3 + 1];
      double expected_valueZ = EXPECTED_POINTS_AT_TIMESTEP[iStep][it * 3 + 2];

      bool sameOnX = vtkMathUtilities::FuzzyCompare(pts[0], expected_valueX, CHECK_TOLERANCE);
      bool sameOnY = vtkMathUtilities::FuzzyCompare(pts[1], expected_valueY, CHECK_TOLERANCE);
      bool sameOnZ = vtkMathUtilities::FuzzyCompare(pts[2], expected_valueZ, CHECK_TOLERANCE);

      if (!sameOnX || !sameOnY || !sameOnZ)
      {
        std::cerr << "Expected point value {" << expected_valueX << "," << expected_valueY << ","
                  << expected_valueZ << "} but got {" << pts[0] << "," << pts[1] << "," << pts[2]
                  << "}." << std::endl;
        return EXIT_FAILURE;
      }

      it++;
    }

    auto* polyData = vtkPolyData::SafeDownCast(dSet);
    if (!polyData)
    {
      std::cerr << "The data isn't a polydata." << std::endl;
      return EXIT_FAILURE;
    }

    double range[2] = { 0, 0 };
    vtkCellArray* polys = polyData->GetPolys();
    polys->GetOffsetsArray()->GetRange(range);

    if (range[0] != 0.0 || range[1] != 10080)
    {
      std::cerr << "Expected range for the offset array to be between 0 and 10080 but got ["
                << range[0] << "," << range[1] << "]" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPolyDataTemporalFieldData(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/test_transient_poly_data_field_data.vtkhdf", false);

  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  for (int iStep = 0; iStep < 10; iStep++)
  {
    // Open data at right time
    vtkSmartPointer<vtkDataSet> dSet = vtkDataSet::SafeDownCast(opener(iStep));

    auto* polyData = vtkPolyData::SafeDownCast(dSet);
    if (!polyData)
    {
      std::cerr << "The data isn't a polydata." << std::endl;
      return EXIT_FAILURE;
    }

    vtkFieldData* fdData = polyData->GetFieldData();
    if (!fdData)
    {
      std::cerr << "The data should contains field data." << std::endl;
      return EXIT_FAILURE;
    }
    vtkAbstractArray* testArray = fdData->GetAbstractArray("Test");
    if (!testArray)
    {
      std::cerr << "The data should contains field data a field data array \"Test\"." << std::endl;
      return EXIT_FAILURE;
    }

    int expectedNbComponents = EXPECTED_SHAPE_AT_TIMESTEP[iStep % 3][0];
    int expectedNbTuples = EXPECTED_SHAPE_AT_TIMESTEP[iStep % 3][1];
    if (testArray->GetNumberOfComponents() != expectedNbComponents ||
      testArray->GetNumberOfTuples() != expectedNbTuples)
    {
      std::cerr << "The field data's shape doesn't match the expected (" << expectedNbComponents
                << ", " << expectedNbTuples << ") for step " << iStep << ", instead got ("
                << testArray->GetNumberOfComponents() << ", " << testArray->GetNumberOfTuples()
                << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestHyperTreeGridTemporal(const std::string& dataRoot, unsigned int depthLimit)
{
  OpenerWorklet opener(dataRoot + "/Data/vtkHDF/temporal_htg.hdf");

  // Generic Time data checks
  constexpr vtkIdType numberOfSteps = 5;
  if (opener.GetReader()->GetNumberOfSteps() != numberOfSteps)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << numberOfSteps << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], (numberOfSteps - 1) * 0.1, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, " << (numberOfSteps - 1) * 0.1 << ") != ("
              << tRange[0] << ", " << tRange[1] << ")" << std::endl;
    return EXIT_FAILURE;
  }

  // Create HTG Source to compare data to.
  const std::array descriptors = { "....", ".R.. | ....", "RR.. | .... ....", "RR.. | .... ....",
    "RRRR | .... R... .... .... | ...." };
  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetBranchFactor(2);
  htgSource->SetDimensions(3, 3, 1);
  htgSource->SetMaxDepth(depthLimit);

  opener.GetReader()->SetMaximumLevelsToReadByDefaultForAMR(depthLimit);

  for (int iStep = 0; iStep < numberOfSteps; iStep++)
  {
    // Open data at right time
    vtkSmartPointer<vtkDataObject> dSet = vtkDataObject::SafeDownCast(opener(iStep));

    htgSource->SetDescriptor(descriptors[iStep]);
    htgSource->Update();
    vtkHyperTreeGrid* expectedHTG = htgSource->GetHyperTreeGridOutput();
    vtkHyperTreeGrid* readHTG = vtkHyperTreeGrid::SafeDownCast(dSet);

    // Generated HTG Source is not temporal, so it will not have a time field array
    vtkNew<vtkFieldData> field;
    readHTG->SetFieldData(field);

    if (!vtkTestUtilities::CompareDataObjects(expectedHTG, readHTG))
    {
      std::cerr << "HyperTreeGrids are not the same for time step " << iStep << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestHyperTreeGridPartitionedTemporal(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/vtkHDF/multipiece_temporal_htg.hdf", false);

  // Generic Time data checks
  constexpr vtkIdType numberOfSteps = 2;
  if (opener.GetReader()->GetNumberOfSteps() != numberOfSteps)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << numberOfSteps << std::endl;
    return EXIT_FAILURE;
  }

  // Create HTG Source to compare data to.
  const std::array descriptorsPart1 = {
    "... .R. ... ... ... | ....",
    "... RRR ... ... ... | .... ...R .... | ....",
  };
  const std::array descriptorsPart2 = {
    "... ... ... .R. ... | ....",
    "... ... ... .RR ... | .... ....",
  };
  const std::array masksPart1 = {
    "111 111 111 000 000 | 1111",
    "111 111 111 000 000 | 1111 1111 1111 | 1111",
  };
  const std::array masksPart2 = { "000 000 000 111 111 | 1111", "000 000 000 111 111 | 1111 1111" };

  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetBranchFactor(2);
  htgSource->SetDimensions(6, 4, 1);
  htgSource->SetMaxDepth(3);
  htgSource->SetUseMask(true);

  for (int iStep = 0; iStep < numberOfSteps; iStep++)
  {
    // Open data at right time
    vtkSmartPointer<vtkDataObject> dSet = vtkDataObject::SafeDownCast(opener(iStep));
    vtkPartitionedDataSet* pds = vtkPartitionedDataSet::SafeDownCast(dSet);

    htgSource->SetDescriptor(descriptorsPart1[iStep]);
    htgSource->SetMask(masksPart1[iStep]);
    htgSource->Update();
    vtkHyperTreeGrid* expectedHTG = htgSource->GetHyperTreeGridOutput();
    vtkHyperTreeGrid* readHTG = vtkHyperTreeGrid::SafeDownCast(pds->GetPartitionAsDataObject(0));

    // Generated HTG Source is not temporal, so it will not have a time field array
    vtkNew<vtkFieldData> field;
    readHTG->SetFieldData(field);

    if (!vtkTestUtilities::CompareDataObjects(expectedHTG, readHTG))
    {
      std::cerr << "HyperTreeGrids are not the same for part 0 of time step " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    htgSource->SetDescriptor(descriptorsPart2[iStep]);
    htgSource->SetMask(masksPart2[iStep]);
    htgSource->Update();
    expectedHTG = htgSource->GetHyperTreeGridOutput();
    readHTG = vtkHyperTreeGrid::SafeDownCast(pds->GetPartitionAsDataObject(1));
    readHTG->SetFieldData(field);
    if (!vtkTestUtilities::CompareDataObjects(expectedHTG, readHTG))
    {
      std::cerr << "HyperTreeGrids are not the same for part 1 of time step " << iStep << std::endl;
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestOverlappingAMRTemporalBase(OpenerWorklet& opener, const std::string& dataRoot)
{
  // Generic Time data checks
  vtkIdType nbSteps = 3;
  if (opener.GetReader()->GetNumberOfSteps() != nbSteps)
  {
    std::cerr << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << nbSteps << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 1.0, CHECK_TOLERANCE))
  {
    std::cerr << "Time range is incorrect: (0.0, 1.0) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

  for (vtkIdType iStep = 0; iStep < nbSteps; iStep++)
  {
    // Open data at right time
    opener.UpdateStep(iStep);
    auto dSet = opener.GetDataObjectAsAMR();

    vtkNew<vtkXMLUniformGridAMRReader> outputReader;
    std::string expectedFileName = dataRoot +
      "/Data/vtkHDF/Transient/transient_expected_overlapping_amr_" + std::to_string(iStep) +
      ".vthb";
    outputReader->SetFileName(expectedFileName.c_str());
    outputReader->SetMaximumLevelsToReadByDefault(0);
    outputReader->Update();
    auto expectedData = vtkOverlappingAMR::SafeDownCast(outputReader->GetOutput());

    if (dSet == nullptr || expectedData == nullptr)
    {
      std::cerr << "Input dataset is empty at timestep " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    unsigned int numLevels = dSet->GetNumberOfLevels();
    if (numLevels != expectedData->GetNumberOfLevels())
    {
      std::cerr << "Expected " << expectedData->GetNumberOfLevels() << "levels but got "
                << numLevels << std::endl;
      return EXIT_FAILURE;
    }

    auto origin = dSet->GetOrigin();
    auto expectedOrigin = expectedData->GetOrigin();
    bool wrongOriginX =
      !vtkMathUtilities::FuzzyCompare(origin[0], expectedOrigin[0], CHECK_TOLERANCE);
    bool wrongOriginY =
      !vtkMathUtilities::FuzzyCompare(origin[1], expectedOrigin[1], CHECK_TOLERANCE);
    bool wrongOriginZ =
      !vtkMathUtilities::FuzzyCompare(origin[2], expectedOrigin[2], CHECK_TOLERANCE);

    if (wrongOriginX || wrongOriginY || wrongOriginZ)
    {
      std::cerr << "Wrong origin, it should be {" << expectedOrigin[0] << "," << expectedOrigin[1]
                << "," << expectedOrigin[2] << "} but got {" << origin[0] << "," << origin[1] << ","
                << origin[2] << "}." << std::endl;
      return EXIT_FAILURE;
    }

    for (unsigned int levelIndex = 0; levelIndex < expectedData->GetNumberOfLevels(); ++levelIndex)
    {
      if (dSet->GetNumberOfDataSets(levelIndex) != expectedData->GetNumberOfDataSets(levelIndex))
      {
        std::cerr << "Number of datasets does not match for level " << levelIndex
                  << ". Expected: " << expectedData->GetNumberOfDataSets(0)
                  << " got: " << dSet->GetNumberOfDataSets(0) << std::endl;
        return EXIT_FAILURE;
      }

      for (unsigned int datasetIndex = 0;
           datasetIndex < expectedData->GetNumberOfDataSets(levelIndex); ++datasetIndex)
      {
        auto dataset = dSet->GetDataSet(levelIndex, datasetIndex);
        auto expectedDataset = expectedData->GetDataSet(levelIndex, datasetIndex);
        if (!vtkTestUtilities::CompareDataObjects(dataset, expectedDataset))
        {
          std::cerr << "Datasets does not match for level " << levelIndex << " dataset "
                    << datasetIndex << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestOverlappingAMRTemporal(const std::string& dataRoot)
{
  std::string filePath = "/Data/vtkHDF/test_temporal_overlapping_amr.vtkhdf";
  auto worklet = OpenerWorklet(dataRoot + filePath);
  return ::TestOverlappingAMRTemporalBase(worklet, dataRoot);
}

//------------------------------------------------------------------------------
// Ensures retro-compatibility with the VTKHDF specification v2.2 which has a typo in the
// Point/Cell/FieldDataOffset name arrays.
int TestOverlappingAMRTemporalLegacy(const std::string& dataRoot)
{
  std::string filePath = "/Data/vtkHDF/test_temporal_overlapping_amr_version_2_2.vtkhdf";
  auto worklet = OpenerWorklet(dataRoot + filePath);
  return ::TestOverlappingAMRTemporalBase(worklet, dataRoot);
}
}
