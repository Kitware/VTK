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

#include <typeinfo>
#include <viskores/VecVariable.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/flow/worklet/Field.h>
#include <viskores/filter/flow/worklet/ParticleAdvection.h>
#include <viskores/filter/flow/worklet/Particles.h>
#include <viskores/filter/flow/worklet/Stepper.h>
#include <viskores/filter/flow/worklet/TemporalGridEvaluators.h>

template <typename ScalarType>
viskores::cont::DataSet CreateUniformDataSet(const viskores::Bounds& bounds,
                                             const viskores::Id3& dims)
{
  viskores::Vec<ScalarType, 3> origin(static_cast<ScalarType>(bounds.X.Min),
                                      static_cast<ScalarType>(bounds.Y.Min),
                                      static_cast<ScalarType>(bounds.Z.Min));
  viskores::Vec<ScalarType, 3> spacing(
    static_cast<ScalarType>(bounds.X.Length()) / static_cast<ScalarType>((dims[0] - 1)),
    static_cast<ScalarType>(bounds.Y.Length()) / static_cast<ScalarType>((dims[1] - 1)),
    static_cast<ScalarType>(bounds.Z.Length()) / static_cast<ScalarType>((dims[2] - 1)));

  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);
  return ds;
}

class TestEvaluatorWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputPoint,
                                ExecObject evaluator,
                                FieldOut validity,
                                FieldOut outputPoint);

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename EvaluatorType>
  VISKORES_EXEC void operator()(viskores::Particle& pointIn,
                                const EvaluatorType& evaluator,
                                viskores::worklet::flow::GridEvaluatorStatus& status,
                                viskores::Vec3f& pointOut) const
  {
    viskores::VecVariable<viskores::Vec3f, 2> values;
    status = evaluator.Evaluate(pointIn.GetPosition(), 0.5f, values);
    if (values.GetNumberOfComponents() > 0)
      pointOut = values[0];
  }
};

template <typename EvalType>
void ValidateEvaluator(const EvalType& eval,
                       const viskores::cont::ArrayHandle<viskores::Particle>& pointIns,
                       const viskores::cont::ArrayHandle<viskores::Vec3f>& validity,
                       const std::string& msg)
{
  using EvalTester = TestEvaluatorWorklet;
  using EvalTesterDispatcher = viskores::worklet::DispatcherMapField<EvalTester>;
  using Status = viskores::worklet::flow::GridEvaluatorStatus;

  EvalTester evalTester;
  EvalTesterDispatcher evalTesterDispatcher(evalTester);
  viskores::Id numPoints = pointIns.GetNumberOfValues();
  viskores::cont::ArrayHandle<Status> evalStatus;
  viskores::cont::ArrayHandle<viskores::Vec3f> evalResults;
  evalTesterDispatcher.Invoke(pointIns, eval, evalStatus, evalResults);
  auto statusPortal = evalStatus.ReadPortal();
  auto resultsPortal = evalResults.ReadPortal();
  auto validityPortal = validity.ReadPortal();
  for (viskores::Id index = 0; index < numPoints; index++)
  {
    Status status = statusPortal.Get(index);
    viskores::Vec3f result = resultsPortal.Get(index);
    viskores::Vec3f expected = validityPortal.Get(index);
    VISKORES_TEST_ASSERT(status.CheckOk(), "Error in evaluator for " + msg);
    VISKORES_TEST_ASSERT(result == expected, "Error in evaluator result for " + msg);
  }
}

template <typename ScalarType>
void CreateConstantVectorField(viskores::Id num,
                               const viskores::Vec<ScalarType, 3>& vec,
                               viskores::cont::ArrayHandle<viskores::Vec<ScalarType, 3>>& vecField)
{
  viskores::cont::ArrayHandleConstant<viskores::Vec<ScalarType, 3>> vecConst;
  vecConst = viskores::cont::make_ArrayHandleConstant(vec, num);
  viskores::cont::ArrayCopy(vecConst, vecField);
}

viskores::Vec3f RandomPt(const viskores::Bounds& bounds)
{
  viskores::FloatDefault rx =
    static_cast<viskores::FloatDefault>(rand()) / static_cast<viskores::FloatDefault>(RAND_MAX);
  viskores::FloatDefault ry =
    static_cast<viskores::FloatDefault>(rand()) / static_cast<viskores::FloatDefault>(RAND_MAX);
  viskores::FloatDefault rz =
    static_cast<viskores::FloatDefault>(rand()) / static_cast<viskores::FloatDefault>(RAND_MAX);

  viskores::Vec3f p;
  p[0] = static_cast<viskores::FloatDefault>(bounds.X.Min + rx * bounds.X.Length());
  p[1] = static_cast<viskores::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length());
  p[2] = static_cast<viskores::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length());
  return p;
}

void GeneratePoints(const viskores::Id numOfEntries,
                    const viskores::Bounds& bounds,
                    viskores::cont::ArrayHandle<viskores::Particle>& pointIns)
{
  pointIns.Allocate(numOfEntries);
  auto writePortal = pointIns.WritePortal();
  for (viskores::Id index = 0; index < numOfEntries; index++)
  {
    viskores::Particle particle(RandomPt(bounds), index);
    writePortal.Set(index, particle);
  }
}

void GenerateValidity(const viskores::Id numOfEntries,
                      viskores::cont::ArrayHandle<viskores::Vec3f>& validity,
                      const viskores::Vec3f& vecOne,
                      const viskores::Vec3f& vecTwo)
{
  validity.Allocate(numOfEntries);
  auto writePortal = validity.WritePortal();
  for (viskores::Id index = 0; index < numOfEntries; index++)
  {
    viskores::Vec3f value = 0.5f * vecOne + (1.0f - 0.5f) * vecTwo;
    writePortal.Set(index, value);
  }
}

void TestTemporalEvaluators()
{
  using ScalarType = viskores::FloatDefault;
  using PointType = viskores::Vec<ScalarType, 3>;
  using FieldHandle = viskores::cont::ArrayHandle<PointType>;
  using FieldType = viskores::worklet::flow::VelocityField<FieldHandle>;
  using EvalType = viskores::worklet::flow::GridEvaluator<FieldType>;
  using TemporalEvalType = viskores::worklet::flow::TemporalGridEvaluator<FieldType>;

  // Create Datasets
  viskores::Id3 dims(5, 5, 5);
  viskores::Bounds bounds(0, 10, 0, 10, 0, 10);
  viskores::cont::DataSet sliceOne = CreateUniformDataSet<ScalarType>(bounds, dims);
  viskores::cont::DataSet sliceTwo = CreateUniformDataSet<ScalarType>(bounds, dims);

  // Create Vector Field
  PointType X(1, 0, 0);
  PointType Z(0, 0, 1);
  viskores::cont::ArrayHandle<PointType> alongX, alongZ;
  CreateConstantVectorField(125, X, alongX);
  CreateConstantVectorField(125, Z, alongZ);
  FieldType velocityX(alongX);
  FieldType velocityZ(alongZ);

  // Send them to test
  EvalType evalOne(sliceOne.GetCoordinateSystem(), sliceOne.GetCellSet(), velocityX);
  EvalType evalTwo(sliceTwo.GetCoordinateSystem(), sliceTwo.GetCellSet(), velocityZ);

  // Test data : populate with meaningful values
  viskores::Id numValues = 10;
  viskores::cont::ArrayHandle<viskores::Particle> pointIns;
  viskores::cont::ArrayHandle<viskores::Vec3f> validity;
  GeneratePoints(numValues, bounds, pointIns);
  GenerateValidity(numValues, validity, X, Z);

  viskores::FloatDefault timeOne(0.0f), timeTwo(1.0f);
  TemporalEvalType gridEval(evalOne, timeOne, evalTwo, timeTwo);
  ValidateEvaluator(gridEval, pointIns, validity, "grid evaluator");
}

void TestTemporalAdvection()
{
  TestTemporalEvaluators();
}

int UnitTestWorkletTemporalAdvection(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestTemporalAdvection, argc, argv);
}
