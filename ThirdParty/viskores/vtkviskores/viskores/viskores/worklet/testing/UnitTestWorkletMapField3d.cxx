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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace mapfield3d
{
static constexpr viskores::Id3 SCHEDULE_SIZE = { 10, 10, 10 };
static constexpr viskores::Id ARRAY_SIZE = SCHEDULE_SIZE[0] * SCHEDULE_SIZE[1] * SCHEDULE_SIZE[2];


template <typename PortalType>
struct ExecutionObject
{
  PortalType Portal;
};

template <typename T>
struct ExecutionObjectInterface : public viskores::cont::ExecutionObjectBase
{
  viskores::cont::ArrayHandle<T> Data;
  viskores::Id3 ScheduleRange;

  template <typename Device>
  VISKORES_CONT auto PrepareForExecution(Device device, viskores::cont::Token& token) const
    -> ExecutionObject<decltype(this->Data.PrepareForInput(device, token))>
  {
    return ExecutionObject<decltype(this->Data.PrepareForInput(device, token))>{
      this->Data.PrepareForInput(device, token)
    };
  }

  viskores::Id3 GetRange3d() const { return this->ScheduleRange; }
};
}


namespace viskores
{
namespace exec
{
namespace arg
{
// Fetch for ArrayPortalTex3D when being used for Loads
template <typename PType>
struct Fetch<viskores::exec::arg::FetchTagExecObject,
             viskores::exec::arg::AspectTagDefault,
             mapfield3d::ExecutionObject<PType>>
{
  using ValueType = typename PType::ValueType;
  using PortalType = mapfield3d::ExecutionObject<PType>;

  template <typename ThreadIndicesType>
  VISKORES_EXEC ValueType Load(const ThreadIndicesType& indices, const PortalType& field) const
  {
    return field.Portal.Get(indices.GetInputIndex());
  }

  template <typename ThreadIndicesType>
  VISKORES_EXEC void Store(const ThreadIndicesType&, const PortalType&, const ValueType&) const
  {
  }
};
}
}
}

namespace mapfield3d
{

class TestMapFieldWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(ExecObject, FieldOut, FieldInOut);
  using ExecutionSignature = _3(_1, _2, _3, WorkIndex);

  template <typename T>
  VISKORES_EXEC T operator()(const T& in, T& out, T& inout, viskores::Id workIndex) const
  {
    auto expected = TestValue(workIndex, T()) + T(100);
    if (!test_equal(in, expected))
    {
      this->RaiseError("Got wrong input value.");
    }
    out = static_cast<T>(in - T(100));
    if (!test_equal(inout, TestValue(workIndex, T()) + T(100)))
    {
      this->RaiseError("Got wrong in-out value.");
    }

    // We return the new value of inout. Since _3 is both an arg and return,
    // this tests that the return value is set after updating the arg values.
    return static_cast<T>(inout - T(100));
  }

  template <typename T1, typename T2, typename T3>
  VISKORES_EXEC T3 operator()(const T1&, const T2&, const T3&, viskores::Id) const
  {
    this->RaiseError("Cannot call this worklet with different types.");
    return viskores::TypeTraits<T3>::ZeroInitialization();
  }
};

template <typename T>
inline viskores::Id3 SchedulingRange(const ExecutionObjectInterface<T>& inputDomain)
{
  return inputDomain.GetRange3d();
}

template <typename T>
inline viskores::Id3 SchedulingRange(const ExecutionObjectInterface<T>* const inputDomain)
{
  return inputDomain->GetRange3d();
}


template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    viskores::cont::ArrayHandle<T> inputHandle =
      viskores::cont::make_ArrayHandle(inputArray, ARRAY_SIZE, viskores::CopyFlag::Off);
    viskores::cont::ArrayHandle<T> outputHandleAsPtr;
    viskores::cont::ArrayHandle<T> inoutHandleAsPtr;

    ExecutionObjectInterface<T> inputExecObject;
    inputExecObject.Data = inputHandle;
    inputExecObject.ScheduleRange = SCHEDULE_SIZE;

    viskores::cont::ArrayCopy(inputHandle, inoutHandleAsPtr);

    std::cout << "Create and run dispatchers." << std::endl;
    viskores::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(inputExecObject, &outputHandleAsPtr, &inoutHandleAsPtr);

    std::cout << "Check results." << std::endl;
    CheckPortal(outputHandleAsPtr.ReadPortal());
    CheckPortal(inoutHandleAsPtr.ReadPortal());
  }
};


void TestWorkletMapField3d(viskores::cont::DeviceAdapterId id)
{

  using HandleTypesToTest3D =
    viskores::List<viskores::Id, viskores::Vec2i_32, viskores::FloatDefault, viskores::Vec3f_64>;

  std::cout << "Testing Map Field with 3d types on device adapter: " << id.GetName() << std::endl;

  //need to test with ExecObject that has 3d range
  //need to fetch from ExecObject that has 3d range
  viskores::testing::Testing::TryTypes(mapfield3d::DoTestWorklet<TestMapFieldWorklet>(),
                                       HandleTypesToTest3D());
}

} // mapfield3d namespace



int UnitTestWorkletMapField3d(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::RunOnDevice(
    mapfield3d::TestWorkletMapField3d, argc, argv);
}
