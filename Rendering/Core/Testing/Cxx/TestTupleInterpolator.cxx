// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME Test of vtkTupleInterpolator
// .SECTION Description

#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkTupleInterpolator.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <iterator>

namespace
{
constexpr double epsilon = 1e-12;

struct InitializerListAsCArray
{
  std::vector<double> Data;

  // Adds the initializer list into `Data` and returns a pointer to the location
  // of the newly added values.
  double* PointerTo(std::initializer_list<double> in)
  {
    std::size_t i = this->Data.size();
    std::copy(in.begin(), in.end(), std::back_inserter(this->Data));
    return &this->Data[i];
  }
};
}

// Test if the interpolation is correct on a few data
//------------------------------------------------------------------------------
bool TestTupleInterpolatorInterpolateTuple()
{
  bool retVal = EXIT_SUCCESS;

  vtkNew<vtkTupleInterpolator> linearInterp;
  vtkNew<vtkTupleInterpolator> splinInterp;

  linearInterp->SetInterpolationTypeToLinear();
  splinInterp->SetInterpolationTypeToSpline();

  linearInterp->SetNumberOfComponents(1);
  splinInterp->SetNumberOfComponents(1);

  double u = 11.5;
  linearInterp->AddTuple(2, &u);
  splinInterp->AddTuple(2, &u);

  u = 23.9;
  linearInterp->AddTuple(6, &u);
  splinInterp->AddTuple(6, &u);

  u = 18;
  splinInterp->AddTuple(10, &u);

  // testing the linear interpolation
  double a = (23.9 - 11.5) / (6 - 2);
  double b = 11.5 - 2 * a;
  for (int i = 0; i < 25; ++i)
  {
    double time = 2 + static_cast<double>(i) / 25 * (6 - 2);
    double res1 = a * time + b;
    double res2 = 0;
    linearInterp->InterpolateTuple(time, &res2);
    if (!vtkMathUtilities::FuzzyCompare(res1, res2, epsilon))
    {
      retVal = EXIT_FAILURE;
      break;
    }
  }

  // expected result
  double res1 = 12.19114375;
  double res2 = 23.577553906249999;

  double time = 2.6;
  splinInterp->InterpolateTuple(time, &u);
  if (!vtkMathUtilities::FuzzyCompare(res1, u, epsilon))
  {
    retVal = EXIT_FAILURE;
  }

  time = 6.9;
  splinInterp->InterpolateTuple(time, &u);
  if (!vtkMathUtilities::FuzzyCompare(res2, u, epsilon))
  {
    retVal = EXIT_FAILURE;
  }

  return retVal;
}

// Test if the FillFromData is correct
//------------------------------------------------------------------------------
bool TestTupleInterpolatorFillFromData()
{
  bool retVal = EXIT_SUCCESS;

  constexpr unsigned int Ndata = 20;
  constexpr unsigned int Ndim = 3;

  // Allocate memory
  std::array<double, Ndata> time;
  std::array<double*, Ndata> tuples;
  std::array<double, Ndim> currentTuple;
  std::array<double, Ndim> lin;
  std::array<double, Ndim> linGT;
  std::array<double, Ndim> splin;
  std::array<double, Ndim> splinGT;

  for (unsigned int i = 0; i < Ndata; ++i)
  {
    tuples[i] = new double[Ndim];
  }

  // fill the data
  for (unsigned int i = 0; i < Ndata; ++i)
  {
    // t from 1 to 3
    time[i] = 1.0 + static_cast<double>(i) / Ndata * (3 - 1);
    tuples[i][0] = std::exp(time[i]);
    tuples[i][1] = std::log(time[i]);
    tuples[i][2] = std::cos(time[i]);
  }

  // Interpolator initialized with FillFromData
  vtkNew<vtkTupleInterpolator> interpLinear;
  interpLinear->SetInterpolationTypeToLinear();
  interpLinear->SetNumberOfComponents(Ndim);
  interpLinear->FillFromData(Ndata, time.data(), tuples.data());

  vtkNew<vtkTupleInterpolator> interpSpline;
  interpSpline->SetInterpolationTypeToSpline();
  interpSpline->SetNumberOfComponents(Ndim);
  interpSpline->FillFromData(Ndata, time.data(), tuples.data());

  // interpolator initialize with addPoint
  // Considered as the ground truth
  vtkNew<vtkTupleInterpolator> interpLinearGroundTruth;
  interpLinearGroundTruth->SetInterpolationTypeToLinear();
  interpLinearGroundTruth->SetNumberOfComponents(Ndim);

  vtkNew<vtkTupleInterpolator> interpSplineGroundTruth;
  interpSplineGroundTruth->SetInterpolationTypeToSpline();
  interpSplineGroundTruth->SetNumberOfComponents(Ndim);

  for (unsigned int i = 0; i < Ndata; ++i)
  {
    std::copy(tuples[i], tuples[i] + Ndim, currentTuple.begin());
    interpLinearGroundTruth->AddTuple(time[i], currentTuple.data());
    interpSplineGroundTruth->AddTuple(time[i], currentTuple.data());
  }

  double timeDense = 0;
  // Now, we compare the results obtained with the interpolator
  // filled with FillFromData and the one considered as GroundTruth
  for (unsigned int i = 0; i < 10 * Ndata; ++i)
  {
    // From 1 to 3
    timeDense = 1.0 + static_cast<double>(i) / (10.0 * Ndata) * (3 - 1);
    interpLinear->InterpolateTuple(timeDense, lin.data());
    interpLinearGroundTruth->InterpolateTuple(timeDense, linGT.data());
    interpSpline->InterpolateTuple(timeDense, splin.data());
    interpSplineGroundTruth->InterpolateTuple(timeDense, splinGT.data());

    for (unsigned int j = 0; j < Ndim; ++j)
    {
      if (!vtkMathUtilities::FuzzyCompare(lin[j], linGT[j], epsilon))
      {
        retVal = EXIT_FAILURE;
      }
      if (!vtkMathUtilities::FuzzyCompare(splin[j], splinGT[j], epsilon))
      {
        retVal = EXIT_FAILURE;
      }
    }
  }

  // Release memory
  for (unsigned int i = 0; i < Ndata; ++i)
  {
    delete[] tuples[i];
  }

  return retVal;
}

// Test if GetTimedTuples is correct
int TestTupleInterpolatorSetGetTimedTuples(int type)
{
  std::cout << "Run TestTupleInterpolatorSetGetTimedTuples with interpolation type=" << type
            << '\n';
  InitializerListAsCArray storage;
  bool success = true;
  vtkNew<vtkTupleInterpolator> interpolator;
  interpolator->SetNumberOfComponents(3);
  interpolator->SetInterpolationType(type);
  interpolator->AddTuple(0., storage.PointerTo({ 1., 2., 3. }));
  interpolator->AddTuple(4., storage.PointerTo({ 5., 6., 7. }));
  auto timedTuples = interpolator->GetTimedTuples();
  if (timedTuples.size() != 8)
  {
    success = false;
    std::cerr << "In line " << __LINE__ << ", number of values in GetTimedTuples is not 8\n";
  }
  for (std::size_t i = 0; i < timedTuples.size(); ++i)
  {
    if (timedTuples[i] != i)
    {
      success = false;
      std::cerr << "In line " << __LINE__ << ", expected " << i << " at timedTuples[" << i
                << "], got " << timedTuples[i] << "\n";
    }
  }
  interpolator->SetTimedTuples({ 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0, 0.0 });
  {
    auto interpResult = interpolator->InterpolateTuple(7.0);
    if (interpResult[0] != 6.0 || interpResult[1] != 5.0 || interpResult[2] != 4.0)
    {
      success = false;
      std::cerr << "In line " << __LINE__ << ", expected InterpolateTuple(7.0) = {6,5,4}, got "
                << interpResult[0] << "," << interpResult[1] << "," << interpResult[2] << '\n';
    }
    interpResult = interpolator->InterpolateTuple(3.0);
    if (interpResult[0] != 2.0 || interpResult[1] != 1.0 || interpResult[2] != 0.0)
    {
      success = false;
      std::cerr << "In line " << __LINE__ << ", expected InterpolateTuple(7.0) = {2,1,0}, got "
                << interpResult[0] << "," << interpResult[1] << "," << interpResult[2] << '\n';
    }
  }
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

//------------------------------------------------------------------------------
int TestTupleInterpolator(int, char*[])
{
  // Store up any errors, return non-zero if something fails.
  int retVal = EXIT_SUCCESS;

  retVal += TestTupleInterpolatorInterpolateTuple();
  retVal += TestTupleInterpolatorFillFromData();
  retVal += TestTupleInterpolatorSetGetTimedTuples(vtkTupleInterpolator::INTERPOLATION_TYPE_LINEAR);
  retVal += TestTupleInterpolatorSetGetTimedTuples(vtkTupleInterpolator::INTERPOLATION_TYPE_SPLINE);

  return retVal;
}
