/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHDFReaderTransient.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkHDFReader.h"

#include "vtkAppendFilter.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkPointData.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkUnstructuredGrid.h"

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
}

int TestHDFReaderTransient(int argc, char* argv[])
{
  vtkNew<vtkTesting> testUtils;
  testUtils->AddArguments(argc, argv);
  std::string dataRoot = testUtils->GetDataRoot();
  int res = ::TestUGTransient(dataRoot);
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

  template <>
  bool operator()(vtkUnstructuredGrid* lhs, vtkUnstructuredGrid* rhs)
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

private:
  double Tolerance = CHECK_TOLERANCE;
};

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
  if (tRange[0] != 0.0 && tRange[1] != 1.0)
  {
    std::cout << "Time range is incorrect: (0.0, 1.0) != (" << tRange[0] << ", " << tRange[1] << ")"
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
    auto getLHSData = [&](vtkIdType iP) {
      std::array<double, 3> point = { 0 };
      dSet->GetPoint(iP, point.data());
      return ::Sin11T(dSet->GetFieldData()->GetArray("Time")->GetComponent(0, 0), point);
    };
    auto getRHSData = [&](vtkIdType iP) {
      return dSet->GetPointData()->GetArray("Modulator")->GetComponent(iP, 0);
    };

    if (!checks(0, dSet->GetNumberOfPoints(), getLHSData, getRHSData))
    {
      std::cout << "PointData: Failed array checks" << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

}
