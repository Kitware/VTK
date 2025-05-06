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
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/UncertainArrayHandle.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

class TestWholeArrayWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn, WholeArrayInOut, WholeArrayOut);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3);

  template <typename InPortalType, typename InOutPortalType, typename OutPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& index,
                                const InPortalType& inPortal,
                                const InOutPortalType& inOutPortal,
                                const OutPortalType& outPortal) const
  {
    using inT = typename InPortalType::ValueType;
    if (!test_equal(inPortal.Get(index), TestValue(index, inT())))
    {
      this->RaiseError("Got wrong input value.");
    }

    using inOutT = typename InOutPortalType::ValueType;
    if (!test_equal(inOutPortal.Get(index), TestValue(index, inOutT()) + inOutT(100)))
    {
      this->RaiseError("Got wrong input/output value.");
    }
    inOutPortal.Set(index, TestValue(index, inOutT()));

    using outT = typename OutPortalType::ValueType;
    outPortal.Set(index, TestValue(index, outT()));
  }
};

namespace map_whole_array
{

static constexpr viskores::Id ARRAY_SIZE = 10;

struct DoTestWholeArrayWorklet
{
  using WorkletType = TestWholeArrayWorklet;

  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inArray[ARRAY_SIZE];
    T inOutArray[ARRAY_SIZE];

    for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inArray[index] = TestValue(index, T());
      inOutArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    viskores::cont::ArrayHandle<T> inHandle =
      viskores::cont::make_ArrayHandle(inArray, ARRAY_SIZE, viskores::CopyFlag::On);
    viskores::cont::ArrayHandle<T> inOutHandle =
      viskores::cont::make_ArrayHandle(inOutArray, ARRAY_SIZE, viskores::CopyFlag::On);
    viskores::cont::ArrayHandle<T> outHandle;
    // Output arrays must be preallocated.
    outHandle.Allocate(ARRAY_SIZE);

    viskores::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(
      viskores::cont::UnknownArrayHandle(inHandle)
        .ResetTypes<viskores::List<T>, viskores::List<VISKORES_DEFAULT_STORAGE_TAG>>(),
      viskores::cont::UnknownArrayHandle(inOutHandle)
        .ResetTypes<viskores::List<T>, viskores::List<VISKORES_DEFAULT_STORAGE_TAG>>(),
      viskores::cont::UnknownArrayHandle(outHandle)
        .ResetTypes<viskores::List<T>, viskores::List<VISKORES_DEFAULT_STORAGE_TAG>>());

    std::cout << "Check result." << std::endl;
    CheckPortal(inOutHandle.ReadPortal());
    CheckPortal(outHandle.ReadPortal());
  }
};

void TestWorkletMapFieldExecArg(viskores::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with WholeArray on device adapter: " << id.GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  viskores::testing::Testing::TryTypes(map_whole_array::DoTestWholeArrayWorklet(),
                                       viskores::TypeListCommon());
}

} // anonymous namespace

int UnitTestWorkletMapFieldWholeArray(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::RunOnDevice(
    map_whole_array::TestWorkletMapFieldExecArg, argc, argv);
}
