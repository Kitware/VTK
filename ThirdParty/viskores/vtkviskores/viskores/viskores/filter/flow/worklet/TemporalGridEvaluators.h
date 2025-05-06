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

#ifndef viskores_filter_flow_worklet_TemporalGridEvaluators_h
#define viskores_filter_flow_worklet_TemporalGridEvaluators_h

#include <viskores/filter/flow/worklet/GridEvaluatorStatus.h>
#include <viskores/filter/flow/worklet/GridEvaluators.h>

namespace viskores
{
namespace worklet
{
namespace flow
{

template <typename FieldType>
class ExecutionTemporalGridEvaluator
{
private:
  using GridEvaluator = viskores::worklet::flow::GridEvaluator<FieldType>;
  using ExecutionGridEvaluator = viskores::worklet::flow::ExecutionGridEvaluator<FieldType>;

public:
  VISKORES_CONT
  ExecutionTemporalGridEvaluator() = default;

  VISKORES_CONT
  ExecutionTemporalGridEvaluator(const GridEvaluator& evaluatorOne,
                                 const viskores::FloatDefault timeOne,
                                 const GridEvaluator& evaluatorTwo,
                                 const viskores::FloatDefault timeTwo,
                                 viskores::cont::DeviceAdapterId device,
                                 viskores::cont::Token& token)
    : EvaluatorOne(evaluatorOne.PrepareForExecution(device, token))
    , EvaluatorTwo(evaluatorTwo.PrepareForExecution(device, token))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
    , TimeDiff(timeTwo - timeOne)
  {
  }

  template <typename Point>
  VISKORES_EXEC bool IsWithinSpatialBoundary(const Point point) const
  {
    return this->EvaluatorOne.IsWithinSpatialBoundary(point) &&
      this->EvaluatorTwo.IsWithinSpatialBoundary(point);
  }

  VISKORES_EXEC
  bool IsWithinTemporalBoundary(const viskores::FloatDefault time) const
  {
    return time >= this->TimeOne && time <= this->TimeTwo;
  }

  VISKORES_EXEC
  viskores::Bounds GetSpatialBoundary() const { return this->EvaluatorTwo.GetSpatialBoundary(); }

  VISKORES_EXEC_CONT
  viskores::FloatDefault GetTemporalBoundary(viskores::Id direction) const
  {
    return direction > 0 ? this->TimeTwo : this->TimeOne;
  }

  template <typename Point>
  VISKORES_EXEC GridEvaluatorStatus Evaluate(const Point& particle,
                                             viskores::FloatDefault time,
                                             viskores::VecVariable<Point, 2>& out) const
  {
    // Validate time is in bounds for the current two slices.
    GridEvaluatorStatus status;

    if (!(time >= TimeOne && time <= TimeTwo))
    {
      status.SetFail();
      status.SetTemporalBounds();
      return status;
    }

    viskores::VecVariable<Point, 2> e1, e2;
    status = this->EvaluatorOne.Evaluate(particle, time, e1);
    if (status.CheckFail())
      return status;
    status = this->EvaluatorTwo.Evaluate(particle, time, e2);
    if (status.CheckFail())
      return status;

    // LERP between the two values of calculated fields to obtain the new value
    viskores::FloatDefault proportion = (time - this->TimeOne) / this->TimeDiff;
    VISKORES_ASSERT(e1.GetNumberOfComponents() != 0 &&
                    e1.GetNumberOfComponents() == e2.GetNumberOfComponents());
    out = viskores::VecVariable<Point, 2>{};
    for (viskores::IdComponent index = 0; index < e1.GetNumberOfComponents(); ++index)
      out.Append(viskores::Lerp(e1[index], e2[index], proportion));

    status.SetOk();
    return status;
  }

private:
  ExecutionGridEvaluator EvaluatorOne;
  ExecutionGridEvaluator EvaluatorTwo;
  viskores::FloatDefault TimeOne;
  viskores::FloatDefault TimeTwo;
  viskores::FloatDefault TimeDiff;
};

template <typename FieldType>
class TemporalGridEvaluator : public viskores::cont::ExecutionObjectBase
{
private:
  using GridEvaluator = viskores::worklet::flow::GridEvaluator<FieldType>;

public:
  VISKORES_CONT TemporalGridEvaluator() = default;

  VISKORES_CONT TemporalGridEvaluator(const viskores::cont::DataSet& ds1,
                                      const viskores::FloatDefault t1,
                                      const FieldType& field1,
                                      const viskores::cont::DataSet& ds2,
                                      const viskores::FloatDefault t2,
                                      const FieldType& field2)
    : EvaluatorOne(GridEvaluator(ds1, field1))
    , EvaluatorTwo(GridEvaluator(ds2, field2))
    , TimeOne(t1)
    , TimeTwo(t2)

  {
  }


  VISKORES_CONT TemporalGridEvaluator(GridEvaluator& evaluatorOne,
                                      const viskores::FloatDefault timeOne,
                                      GridEvaluator& evaluatorTwo,
                                      const viskores::FloatDefault timeTwo)
    : EvaluatorOne(evaluatorOne)
    , EvaluatorTwo(evaluatorTwo)
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  VISKORES_CONT TemporalGridEvaluator(const viskores::cont::CoordinateSystem& coordinatesOne,
                                      const viskores::cont::UnknownCellSet& cellsetOne,
                                      const FieldType& fieldOne,
                                      const viskores::FloatDefault timeOne,
                                      const viskores::cont::CoordinateSystem& coordinatesTwo,
                                      const viskores::cont::UnknownCellSet& cellsetTwo,
                                      const FieldType& fieldTwo,
                                      const viskores::FloatDefault timeTwo)
    : EvaluatorOne(GridEvaluator(coordinatesOne, cellsetOne, fieldOne))
    , EvaluatorTwo(GridEvaluator(coordinatesTwo, cellsetTwo, fieldTwo))
    , TimeOne(timeOne)
    , TimeTwo(timeTwo)
  {
  }

  VISKORES_CONT ExecutionTemporalGridEvaluator<FieldType> PrepareForExecution(
    viskores::cont::DeviceAdapterId device,
    viskores::cont::Token& token) const
  {
    return ExecutionTemporalGridEvaluator<FieldType>(
      this->EvaluatorOne, this->TimeOne, this->EvaluatorTwo, this->TimeTwo, device, token);
  }

private:
  GridEvaluator EvaluatorOne;
  GridEvaluator EvaluatorTwo;
  viskores::FloatDefault TimeOne;
  viskores::FloatDefault TimeTwo;
};

}
}
} //viskores::worklet::flow

#endif // viskores_filter_flow_worklet_TemporalGridEvaluators_h
