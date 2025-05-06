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

class TestAtomicArrayWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, AtomicArrayInOut);
  using ExecutionSignature = void(WorkIndex, _2);
  using InputDomain = _1;

  template <typename AtomicArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& index, const AtomicArrayType& atomicArray) const
  {
    using ValueType = typename AtomicArrayType::ValueType;
    atomicArray.Add(0, static_cast<ValueType>(index));
  }
};

namespace map_whole_array
{

static constexpr viskores::Id ARRAY_SIZE = 10;

struct DoTestAtomicArrayWorklet
{
  using WorkletType = TestAtomicArrayWorklet;

  // This just demonstrates that the WholeArray tags support dynamic arrays.
  VISKORES_CONT
  void CallWorklet(const viskores::cont::UnknownArrayHandle& inOutArray) const
  {
    std::cout << "Create and run dispatcher." << std::endl;
    viskores::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(viskores::cont::ArrayHandleIndex(ARRAY_SIZE),
                      inOutArray.ResetTypes<viskores::cont::AtomicArrayTypeList,
                                            viskores::List<viskores::cont::StorageTagBasic>>());
  }

  template <typename T>
  VISKORES_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    viskores::cont::ArrayHandle<T> inOutHandle = viskores::cont::make_ArrayHandle<T>({ 0 });

    this->CallWorklet(inOutHandle);

    std::cout << "Check result." << std::endl;
    T result = inOutHandle.ReadPortal().Get(0);

    VISKORES_TEST_ASSERT(result == (ARRAY_SIZE * (ARRAY_SIZE - 1)) / 2,
                         "Got wrong summation in atomic array.");
  }
};

void TestWorkletMapFieldExecArgAtomic(viskores::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with AtomicWholeArray on device adapter: " << id.GetName()
            << std::endl;
  viskores::testing::Testing::TryTypes(map_whole_array::DoTestAtomicArrayWorklet(),
                                       viskores::cont::AtomicArrayTypeList());
}

} // anonymous namespace

int UnitTestWorkletMapFieldWholeArrayAtomic(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::RunOnDevice(
    map_whole_array::TestWorkletMapFieldExecArgAtomic, argc, argv);
}
