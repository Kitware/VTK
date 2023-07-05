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

namespace
{
constexpr double CHECK_TOLERANCE = 1e-3;

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
}

int TestHDFReaderTransient(int argc, char* argv[])
{
  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();
  int res = ::TestUGTransient(dataRoot);
  res |= ::TestImageDataTransient(dataRoot);
  res |= ::TestPolyDataTransient(dataRoot);
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
        std::cout << "Failed check at " << iter << " with LHS = " << lhs << " != " << rhs
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
    std::cout << "Called unspecialized worker" << std::endl;
    return false;
  }

private:
  double Tolerance = CHECK_TOLERANCE;
};

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
    std::cout << "Points: Failed point geometry checks" << std::endl;
    return false;
  }

  auto refConnRange = vtk::DataArrayValueRange<1>(lhs->GetCells()->GetConnectivityArray());
  auto getLHSConn = [&](vtkIdType iConn) { return refConnRange[iConn]; };
  auto hdfConnRange = vtk::DataArrayValueRange<1>(rhs->GetCells()->GetConnectivityArray());
  auto getRHSConn = [&](vtkIdType iConn) { return hdfConnRange[iConn]; };
  if (!checks(0, refConnRange.size(), getLHSConn, getRHSConn))
  {
    std::cout << "Connectivity: Failed connectivity geometry checks" << std::endl;
    return false;
  }

  auto refOffRange = vtk::DataArrayValueRange<1>(lhs->GetCells()->GetOffsetsArray());
  auto getLHSOff = [&](vtkIdType iOff) { return refOffRange[iOff]; };
  auto hdfOffRange = vtk::DataArrayValueRange<1>(rhs->GetCells()->GetOffsetsArray());
  auto getRHSOff = [&](vtkIdType iOff) { return hdfOffRange[iOff]; };
  if (!checks(0, refOffRange.size(), getLHSOff, getRHSOff))
  {
    std::cout << "Offsets: Failed offsets geometry checks" << std::endl;
    return false;
  }
  return true;
}

template <>
bool GeometryCheckerWorklet::operator()(vtkImageData* lhs, vtkImageData* rhs)
{
  const auto lExtent = lhs->GetExtent();
  const auto rExtent = rhs->GetExtent();
  for (vtkIdType iE = 0; iE < 6; ++iE)
  {
    if (lExtent[iE] - rExtent[iE] > this->Tolerance)
    {
      std::cout << "Extents: Failed extent geometry checks" << std::endl;
      return false;
    }
  }

  const auto lSpacing = lhs->GetSpacing();
  const auto rSpacing = rhs->GetSpacing();
  for (vtkIdType iS = 0; iS < 3; ++iS)
  {
    if (lSpacing[iS] - rSpacing[iS] > this->Tolerance)
    {
      std::cout << "Spacing: Failed spacing geometry checks" << std::endl;
      return false;
    }
  }
  return true;
}

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
    std::cout << "Points: Failed point geometry checks" << std::endl;
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
      std::cout << "Connectivity: Failed connectivity geometry checks for " << keyVal.first
                << std::endl;
      return false;
    }

    auto refOffRange = vtk::DataArrayValueRange<1>(keyVal.second.first->GetOffsetsArray());
    auto getLHSOff = [&](vtkIdType iOff) { return refOffRange[iOff]; };
    auto hdfOffRange = vtk::DataArrayValueRange<1>(keyVal.second.second->GetOffsetsArray());
    auto getRHSOff = [&](vtkIdType iOff) { return hdfOffRange[iOff]; };
    if (!checks(0, refOffRange.size(), getLHSOff, getRHSOff))
    {
      std::cout << "Offsets: Failed offsets geometry checks" << std::endl;
      return false;
    }
  }
  return true;
}

int TestUGTransient(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_sphere.hdf");

  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cout << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cout << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
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
      std::cout << "Property: TimeValue is wrong: " << opener.GetReader()->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = dSet->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cout << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cout << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    GeometryCheckerWorklet gChecker(CHECK_TOLERANCE);
    if (!gChecker(
          vtkUnstructuredGrid::SafeDownCast(refGeometry), vtkUnstructuredGrid::SafeDownCast(dSet)))
    {
      std::cout << "Geometry: Failed geometry checks." << std::endl;
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
      std::cout << "PointData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestImageDataTransient(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/transient_wavelet.hdf");

  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cout << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cout << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
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
      std::cout << "Property: Time Value is wrong: " << opener.GetReader()->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = dSet->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cout << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cout << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    GeometryCheckerWorklet gChecker(CHECK_TOLERANCE);
    if (!gChecker(vtkImageData::SafeDownCast(refGeometry), vtkImageData::SafeDownCast(dSet)))
    {
      std::cout << "Geometry: Failed geometry checks." << std::endl;
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
      std::cout << "PointData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }

    // Cell Data checks
    auto getLHSCData = [&](vtkIdType iC) { return iC; };
    auto getRHSCData = [&](vtkIdType iC) {
      return dSet->GetCellData()->GetArray("IDs")->GetComponent(iC, 0);
    };

    if (!checks(0, dSet->GetNumberOfCells(), getLHSCData, getRHSCData))
    {
      std::cout << "CellData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

int TestPolyDataTransient(const std::string& dataRoot)
{
  OpenerWorklet opener(dataRoot + "/Data/test_transient_poly_data.hdf");

  // Generic Time data checks
  if (opener.GetReader()->GetNumberOfSteps() != 10)
  {
    std::cout << "Number of time steps is not correct: " << opener.GetReader()->GetNumberOfSteps()
              << " != " << 10 << std::endl;
    return EXIT_FAILURE;
  }

  auto tRange = opener.GetReader()->GetTimeRange();
  if (!vtkMathUtilities::FuzzyCompare(tRange[0], 0.0, CHECK_TOLERANCE) ||
    !vtkMathUtilities::FuzzyCompare(tRange[1], 0.9, CHECK_TOLERANCE))
  {
    std::cout << "Time range is incorrect: (0.0, 0.9) != (" << tRange[0] << ", " << tRange[1] << ")"
              << std::endl;
    return EXIT_FAILURE;
  }

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
      std::cout << "Property: TimeValue is wrong: " << opener.GetReader()->GetTimeValue()
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    auto timeArr = dSet->GetFieldData()->GetArray("Time");
    if (!timeArr)
    {
      std::cout << "No Time array in FieldData" << std::endl;
      return EXIT_FAILURE;
    }

    if (!vtkMathUtilities::FuzzyCompare(
          timeArr->GetComponent(0, 0), static_cast<double>(iStep) / 10, CHECK_TOLERANCE))
    {
      std::cout << "FieldData: Time value is wrong: " << timeArr->GetComponent(0, 0)
                << " != " << static_cast<double>(iStep) / 10 << std::endl;
      return EXIT_FAILURE;
    }

    GeometryCheckerWorklet gChecker(CHECK_TOLERANCE);
    if (!gChecker(vtkPolyData::SafeDownCast(refGeometry), vtkPolyData::SafeDownCast(dSet)))
    {
      std::cout << "Geometry: Failed geometry checks." << std::endl;
      return EXIT_FAILURE;
    }

    CheckerWorklet checks(CHECK_TOLERANCE);

    // Point Data checks
    auto lhsPRange = vtk::DataArrayValueRange<3>(refGeometry->GetPointData()->GetArray("Warping"));
    auto getLHSPData = [&](vtkIdType iC) { return lhsPRange[iC]; };
    auto rhsPRange = vtk::DataArrayValueRange<3>(dSet->GetPointData()->GetArray("Warping"));
    auto getRHSPData = [&](vtkIdType iC) { return rhsPRange[iC]; };

    if (!checks(0, dSet->GetNumberOfPoints() * 3, getLHSPData, getRHSPData))
    {
      std::cout << "PointData: Failed array checks at step " << iStep << std::endl;
      return EXIT_FAILURE;
    }

    // Cell Data checks
    auto lhsCRange = vtk::DataArrayValueRange<1>(refGeometry->GetCellData()->GetArray("Materials"));
    auto getLHSCData = [&](vtkIdType iC) { return lhsCRange[iC]; };
    auto rhsCRange = vtk::DataArrayValueRange<1>(dSet->GetCellData()->GetArray("Materials"));
    auto getRHSCData = [&](vtkIdType iC) { return rhsCRange[iC]; };

    if (!checks(0, dSet->GetNumberOfCells(), getLHSCData, getRHSCData))
    {
      std::cout << "CellData: Failed array checks at step " << iStep << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

}
