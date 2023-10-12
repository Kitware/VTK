// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkHDFReader.h"

#include "vtkAppendDataSets.h"
#include "vtkAppendFilter.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPolyDataReader.h"

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

// analytical functions
template <typename Vec>
double Sin11T(double t, const Vec& point);

// generic workers
struct OpenerWorklet;
struct CheckerWorklet;

// assemblies
int TestUGTransient(const std::string& dataRoot);
int TestImageDataTransient(const std::string& dataRoot);
int TestPolyDataTransient(const std::string& dataRoot);
int TestPolyDataTransientWithOffset(const std::string& dataRoot);

int TestUGTransientWithCache(const std::string& dataRoot);
int TestImageDataTransientWithCache(const std::string& dataRoot);
int TestPolyDataTransientWithCache(const std::string& dataRoot);
}

//------------------------------------------------------------------------------
int TestHDFReaderTransient(int argc, char* argv[])
{
  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();
  int res = ::TestUGTransient(dataRoot);
  res |= ::TestImageDataTransient(dataRoot);
  res |= ::TestPolyDataTransient(dataRoot);
  res |= ::TestPolyDataTransientWithOffset(dataRoot);
  res |= ::TestUGTransientWithCache(dataRoot);
  res |= ::TestImageDataTransientWithCache(dataRoot);
  res |= ::TestPolyDataTransientWithCache(dataRoot);
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
  OpenerWorklet(const std::string& filePath)
  {
    this->Reader->SetFileName(filePath.c_str());
    this->Reader->Update();
  }

  vtkSmartPointer<vtkDataSet> operator()(std::size_t timeStep)
  {
    this->Reader->SetStep(timeStep);
    this->Reader->Update();
    vtkSmartPointer<vtkDataSet> res = this->Reader->GetOutputAsDataSet();
    return res;
  }

  vtkHDFReader* GetReader() { return this->Reader; }

private:
  vtkNew<vtkHDFReader> Reader;
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
int TestUGTransientBase(OpenerWorklet& opener)
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

  for (std::size_t iStep = 0; iStep < 10; ++iStep)
  {
    // Open data at right time
    vtkSmartPointer<vtkDataSet> dSet = opener(iStep);

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
    if (!gChecker(
          vtkUnstructuredGrid::SafeDownCast(refGeometry), vtkUnstructuredGrid::SafeDownCast(dSet)))
    {
      std::cerr << "Geometry: Failed geometry checks." << std::endl;
      return EXIT_FAILURE;
    }

    CheckerWorklet checks(CHECK_TOLERANCE);

    // Point Data checks
    auto getLHSPData = [&](vtkIdType iP) {
      std::array<double, 3> point = { 0 };
      dSet->GetPoint(iP, point.data());
      return ::Sin11T(dSet->GetFieldData()->GetArray("Time")->GetComponent(0, 0), point);
    };
    auto getRHSPData = [&](vtkIdType iP) {
      return dSet->GetPointData()->GetArray("Modulator")->GetComponent(iP, 0);
    };

    if (!checks(0, dSet->GetNumberOfPoints(), getLHSPData, getRHSPData))
    {
      std::cerr << "PointData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestUGTransient(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_sphere.hdf");
  return TestUGTransientBase(opener);
}

//------------------------------------------------------------------------------
int TestUGTransientWithCache(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_sphere.hdf");
  opener.GetReader()->UseCacheOn();
  return TestUGTransientBase(opener);
}

//------------------------------------------------------------------------------
int TestImageDataTransientBase(OpenerWorklet& opener)
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
    vtkSmartPointer<vtkDataSet> dSet = opener(iStep);

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
    auto getLHSPData = [&](vtkIdType iP) {
      auto wave = refGeometry->GetPointData()->GetArray("RTData");
      return dSet->GetFieldData()->GetArray("Time")->GetComponent(0, 0) * wave->GetComponent(iP, 0);
    };
    auto getRHSPData = [&](vtkIdType iP) {
      return dSet->GetPointData()->GetArray("Modulator")->GetComponent(iP, 0);
    };

    if (!checks(0, dSet->GetNumberOfPoints(), getLHSPData, getRHSPData))
    {
      std::cerr << "PointData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }

    // Cell Data checks
    auto getLHSCData = [&](vtkIdType iC) { return iC; };
    auto getRHSCData = [&](vtkIdType iC) {
      return dSet->GetCellData()->GetArray("IDs")->GetComponent(iC, 0);
    };

    if (!checks(0, dSet->GetNumberOfCells(), getLHSCData, getRHSCData))
    {
      std::cerr << "CellData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestImageDataTransient(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_wavelet.hdf");
  return TestImageDataTransientBase(opener);
}

//------------------------------------------------------------------------------
int TestImageDataTransientWithCache(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_wavelet.hdf");
  opener.GetReader()->UseCacheOn();
  return TestImageDataTransientBase(opener);
}

//------------------------------------------------------------------------------
int TestPolyDataTransientBase(
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
    vtkSmartPointer<vtkDataSet> dSet = opener(iStep);

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
int TestPolyDataTransient(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/test_transient_poly_data.hdf");
  return TestPolyDataTransientBase(opener, dataRoot);
}

//------------------------------------------------------------------------------
int TestPolyDataTransientWithCache(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/test_transient_poly_data.hdf");
  opener.GetReader()->UseCacheOn();
  // We should be able to activate the MeshMTime testing once the cache can store
  // the intermediate vtkPoints and vtkCellArrays
  return TestPolyDataTransientBase(opener, dataRoot, false /*testMeshMTime*/);
}

//------------------------------------------------------------------------------
int TestPolyDataTransientWithOffset(const std::string& dataRoot)
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
    vtkSmartPointer<vtkDataSet> dSet = opener(iStep);

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
}
