//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <random>
#include <string>

#include <viskores/Math.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/SplineEvaluateRectilinearGrid.h>
#include <viskores/cont/SplineEvaluateUniformGrid.h>

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>

#include <viskores/cont/testing/Testing.h>

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/filter/resampling/Probe.h>


namespace
{

VISKORES_EXEC
viskores::FloatDefault EvaluateNormalizedGyroid(const viskores::Vec3f& point)
{
  //f(x,y,z)=sin(2πx)cos(2πy)+sin(2πy)cos(2πz)+sin(2πz)cos(2πx)
  constexpr viskores::FloatDefault twoPi = viskores::TwoPi<viskores::FloatDefault>();
  return (viskores::Sin(twoPi * point[0]) * viskores::Cos(twoPi * point[1])) +
    (viskores::Sin(twoPi * point[1]) * viskores::Cos(twoPi * point[2])) +
    (viskores::Sin(twoPi * point[2]) * viskores::Cos(twoPi * point[0]));
}

class EvalWorklet : public viskores::worklet::WorkletMapField
{
public:
  EvalWorklet() {}

  using ControlSignature = void(FieldIn pointsIn, ExecObject splineEval, FieldOut results);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename PointType, typename SplineEvalType, typename ResultType>
  VISKORES_EXEC void operator()(const PointType& pointIn,
                                const SplineEvalType& splineEval,
                                ResultType& result) const
  {
    splineEval.Evaluate(pointIn, result);
  }
};

class GenerateData : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn pointsIn, FieldOut results);
  using ExecutionSignature = void(_1, _2);

  VISKORES_EXEC
  void operator()(const viskores::Vec3f& point, viskores::FloatDefault& value) const
  {
    value = EvaluateNormalizedGyroid(point);
  }
};

std::vector<viskores::cont::DataSet> MakeDataSet3D(const std::vector<viskores::Id3>& dims)
{
  viskores::cont::DataSetBuilderUniform builder;
  viskores::Vec3f origin(0.0f, 0.0f, 0.0f);

  std::vector<viskores::cont::DataSet> dataSets;
  for (const auto& d : dims)
  {
    viskores::Vec3f spacing = viskores::Vec3f(1) / (viskores::Vec3f(d) - viskores::Vec3f(1));
    auto ds = builder.Create(d, origin, spacing);

    viskores::cont::Invoker invoker;
    viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
    invoker(GenerateData{}, ds.GetCoordinateSystem(), fieldArray);
    ds.AddPointField("field", fieldArray);
    dataSets.push_back(ds);
  }

  return dataSets;
}

template <typename F>
std::vector<viskores::FloatDefault> fillCoords(viskores::Id N, F g)
{
  std::vector<viskores::FloatDefault> coords(N);
  for (viskores::Id i = 0; i < N; ++i)
  {
    viskores::FloatDefault t = static_cast<viskores::FloatDefault>(i) / (N - 1);
    coords[i] = g(t);
  }
  coords[N - 1] = 1.0f;
  return coords;
}

std::vector<viskores::FloatDefault> fillCoordsExp(viskores::Id N)
{
  std::vector<viskores::FloatDefault> coords;

  coords = fillCoords(N,
                      [](viskores::FloatDefault t)
                      {
                        const viskores::FloatDefault k = 3;
                        return (std::exp(k * t) - 1) / (std::exp(k) - 1);
                      });
  return coords;
}

std::vector<viskores::cont::DataSet> MakeRectDataSet3D(const std::vector<viskores::Id3>& dims)
{
  std::vector<viskores::cont::DataSet> dataSets;

  for (const auto& d : dims)
  {
    for (int i = 0; i < 4; i++)
    {
      viskores::cont::DataSetBuilderRectilinear builder;
      std::vector<viskores::FloatDefault> xcoords, ycoords, zcoords;

      if (i == 0)
      {
        //uniform spacing.
        xcoords = fillCoords(d[0], [](viskores::FloatDefault t) { return t; });
        ycoords = fillCoords(d[1], [](viskores::FloatDefault t) { return t; });
        zcoords = fillCoords(d[2], [](viskores::FloatDefault t) { return t; });
      }
      else if (i == 1)
      {
        //quadratic clustering near 0.
        xcoords = fillCoords(d[0], [](viskores::FloatDefault t) { return t * t; });
        ycoords = fillCoords(d[1], [](viskores::FloatDefault t) { return t * t; });
        zcoords = fillCoords(d[2], [](viskores::FloatDefault t) { return t * t; });
      }
      else if (i == 2)
      {
        //quadratic clustering near 1.
        xcoords = fillCoords(d[0], [](viskores::FloatDefault t) { return 1 - (1 - t) * (1 - t); });
        ycoords = fillCoords(d[1], [](viskores::FloatDefault t) { return 1 - (1 - t) * (1 - t); });
        zcoords = fillCoords(d[2], [](viskores::FloatDefault t) { return 1 - (1 - t) * (1 - t); });
      }
      else if (i == 3)
      {
        //exponential
        xcoords = fillCoordsExp(d[0]);
        ycoords = fillCoordsExp(d[1]);
        zcoords = fillCoordsExp(d[2]);
      }

      auto ds = builder.Create(xcoords, ycoords, zcoords);
      viskores::cont::Invoker invoker;
      viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
      invoker(GenerateData{}, ds.GetCoordinateSystem(), fieldArray);
      ds.AddPointField("field", fieldArray);
      dataSets.push_back(ds);
    }
  }

  return dataSets;
}

viskores::cont::ArrayHandle<viskores::FloatDefault> CreateLinearInterpolationResult(
  const viskores::cont::DataSet& ds,
  const viskores::cont::ArrayHandle<viskores::Vec3f>& points)
{
  viskores::cont::DataSetBuilderExplicit builder;
  viskores::cont::ArrayHandle<viskores::Id> idsArray;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleIndex(points.GetNumberOfValues()),
                            idsArray);

  auto ptDataSet = builder.Create(points, viskores::CellShapeTagVertex(), 1, idsArray);

  viskores::filter::resampling::Probe probe;
  probe.SetActiveField("field");
  probe.SetGeometry(ptDataSet);
  auto output = probe.Execute(ds);

  viskores::cont::ArrayHandle<viskores::FloatDefault> result;
  ds.GetField("field").GetData().AsArrayHandle(result);
  return result;
}

template <typename SplineEvalType>
void CompareToLinear(SplineEvalType& splineEval,
                     const viskores::cont::ArrayHandle<viskores::Vec3f>& points,
                     const viskores::cont::ArrayHandle<viskores::FloatDefault>& expectedValues,
                     const viskores::cont::DataSet& ds)
{
  viskores::cont::Invoker invoke;
  viskores::cont::ArrayHandle<viskores::FloatDefault> results;

  //Compare against linear interpolation.
  auto linearResult = CreateLinearInterpolationResult(ds, points);

  EvalWorklet evalWorklet;
  invoke(evalWorklet, points, splineEval, results);

  auto splinePortal = results.ReadPortal();
  auto linearPortal = linearResult.ReadPortal();
  auto expectedPortal = expectedValues.ReadPortal();
  for (viskores::Id i = 0; i < splinePortal.GetNumberOfValues(); i++)
  {
    auto truth = expectedPortal.Get(i);
    auto linearValue = linearPortal.Get(i);
    auto splineValue = splinePortal.Get(i);

    auto diffLinear = viskores::Abs(truth - linearValue);
    auto diffSpline = viskores::Abs(truth - splineValue);

    // spline interpolation should be better.
    VISKORES_TEST_ASSERT(diffSpline <= diffLinear, "Error in spline interpolation");
  }
}

template <typename SplineEvalType>
void CompareResults(SplineEvalType& splineEval,
                    const viskores::cont::ArrayHandle<viskores::Vec3f>& points,
                    const viskores::cont::ArrayHandle<viskores::FloatDefault>& expectedValues,
                    viskores::Float64 eps)
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> results;
  viskores::cont::Invoker invoke;

  EvalWorklet evalWorklet;
  invoke(evalWorklet, points, splineEval, results);

  auto resultsPortal = results.ReadPortal();
  auto expectedPortal = expectedValues.ReadPortal();

  for (viskores::Id i = 0; i < resultsPortal.GetNumberOfValues(); i++)
  {
    auto diff = viskores::Abs(expectedPortal.Get(i) - resultsPortal.Get(i));
    VISKORES_TEST_ASSERT(diff < eps, "Spline value outside of tolerance.");
  }
}

viskores::cont::ArrayHandle<viskores::Vec3f> CreateRandomVec3f(std::size_t N)
{
  std::mt19937 gen(123);
  std::uniform_real_distribution<float> dist(0.05f, 0.95f);

  std::vector<viskores::Vec3f> v;
  v.reserve(N);

  for (std::size_t i = 0; i < N; ++i)
    v.emplace_back(viskores::Vec3f(dist(gen), dist(gen), dist(gen)));

  return viskores::cont::make_ArrayHandle(v, viskores::CopyFlag::On);
}

void DoSplineEvalTest()
{
  std::vector<viskores::Id3> dims = { { 20, 20, 20 }, { 50, 50, 50 }, { 20, 40, 60 } };

  auto dsUniform = MakeDataSet3D(dims);
  auto dsRect = MakeRectDataSet3D(dims);

  // Compare spline to linear interpolation on 50 points.
  auto pointData = CreateRandomVec3f(50);
  viskores::cont::ArrayHandle<viskores::FloatDefault> expectedValues;
  viskores::cont::Invoker invoker;
  invoker(GenerateData{}, pointData, expectedValues);

  std::cout << "Compare spline to linear." << std::endl;
  std::cout << " --Uniform datasets." << std::endl;
  for (const auto& ds : dsUniform)
  {
    viskores::cont::SplineEvaluateUniformGrid evalUniform(ds, "field");
    CompareToLinear(evalUniform, pointData, expectedValues, ds);
  }

  std::cout << " --Rectilinear datasets." << std::endl;
  for (const auto& ds : dsRect)
  {
    viskores::cont::SplineEvaluateRectilinearGrid evalRect(ds, "field");
    CompareToLinear(evalRect, pointData, expectedValues, ds);
  }

  //compare values for a few points.
  pointData = CreateRandomVec3f(10);
  invoker(GenerateData{}, pointData, expectedValues);

  dsUniform = MakeDataSet3D({ { 100, 100, 100 } });
  dsRect = MakeRectDataSet3D({ { 100, 100, 100 } });

  std::cout << "Compare spline values." << std::endl;
  std::cout << " --Uniform datasets." << std::endl;
  for (const auto& ds : dsUniform)
  {
    viskores::cont::SplineEvaluateUniformGrid evalUniform(ds, "field");
    CompareResults(evalUniform, pointData, expectedValues, 1e-3);
  }

  std::cout << " --Rectilinear datasets." << std::endl;
  for (const auto& ds : dsRect)
  {
    viskores::cont::SplineEvaluateRectilinearGrid evalRect(ds, "field");
    CompareResults(evalRect, pointData, expectedValues, 1e-3);
  }
}
} // anonymous namespace

int UnitTestSplineEvaluate(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoSplineEvalTest, argc, argv);
}
