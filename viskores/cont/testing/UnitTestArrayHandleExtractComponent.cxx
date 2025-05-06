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
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleExtractComponent.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template <typename ValueType>
struct ExtractComponentTests
{
  using InputArray = viskores::cont::ArrayHandle<viskores::Vec<ValueType, 4>>;
  using ExtractArray = viskores::cont::ArrayHandleExtractComponent<InputArray>;
  using ReferenceComponentArray = viskores::cont::ArrayHandleCounting<ValueType>;
  using ReferenceCompositeArray =
    typename viskores::cont::ArrayHandleCompositeVector<ReferenceComponentArray,
                                                        ReferenceComponentArray,
                                                        ReferenceComponentArray,
                                                        ReferenceComponentArray>;

  // This is used to build a ArrayHandleExtractComponent's internal array.
  ReferenceCompositeArray RefComposite;

  void ConstructReferenceArray()
  {
    // Build the Ref array
    const viskores::Id numValues = 32;
    ReferenceComponentArray c1 =
      viskores::cont::make_ArrayHandleCounting<ValueType>(3, 2, numValues);
    ReferenceComponentArray c2 =
      viskores::cont::make_ArrayHandleCounting<ValueType>(2, 3, numValues);
    ReferenceComponentArray c3 =
      viskores::cont::make_ArrayHandleCounting<ValueType>(4, 4, numValues);
    ReferenceComponentArray c4 =
      viskores::cont::make_ArrayHandleCounting<ValueType>(1, 3, numValues);

    this->RefComposite = viskores::cont::make_ArrayHandleCompositeVector(c1, c2, c3, c4);
  }

  InputArray BuildInputArray() const
  {
    InputArray result;
    viskores::cont::ArrayCopyDevice(this->RefComposite, result);
    return result;
  }

  void SanityCheck(viskores::IdComponent component) const
  {
    InputArray composite = this->BuildInputArray();
    ExtractArray extract(composite, component);

    VISKORES_TEST_ASSERT(composite.GetNumberOfValues() == extract.GetNumberOfValues(),
                         "Number of values in copied ExtractComponent array does not match input.");
  }

  void ReadTestComponentExtraction(viskores::IdComponent component) const
  {
    // Test that the expected values are read from an ExtractComponent array.
    InputArray composite = this->BuildInputArray();
    ExtractArray extract(composite, component);

    // Test reading the data back in the control env:
    this->ValidateReadTestArray(extract, component);

    // Copy the extract array in the execution environment to test reading:
    viskores::cont::ArrayHandle<ValueType> execCopy;
    viskores::cont::ArrayCopy(extract, execCopy);
    this->ValidateReadTestArray(execCopy, component);
  }

  template <typename ArrayHandleType>
  void ValidateReadTestArray(ArrayHandleType testArray, viskores::IdComponent component) const
  {
    using RefVectorType = typename ReferenceCompositeArray::ValueType;
    using Traits = viskores::VecTraits<RefVectorType>;

    auto testPortal = testArray.ReadPortal();
    auto refPortal = this->RefComposite.ReadPortal();

    VISKORES_TEST_ASSERT(testPortal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                         "Number of values in read test output do not match input.");

    for (viskores::Id i = 0; i < testPortal.GetNumberOfValues(); ++i)
    {
      VISKORES_TEST_ASSERT(
        test_equal(testPortal.Get(i), Traits::GetComponent(refPortal.Get(i), component), 0.),
        "Value mismatch in read test.");
    }
  }

  // Doubles the specified component
  struct WriteTestWorklet : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn referenceArray, FieldOut componentArray);

    viskores::IdComponent Component;

    VISKORES_CONT WriteTestWorklet(viskores::IdComponent component)
      : Component(component)
    {
    }

    template <typename RefType, typename ComponentType>
    VISKORES_EXEC void operator()(const RefType& reference, ComponentType& outComponent) const
    {
      outComponent = viskores::VecTraits<RefType>::GetComponent(reference, this->Component) * 2;
    }
  };

  void WriteTestComponentExtraction(viskores::IdComponent component) const
  {
    // Control test:
    {
      InputArray composite = this->BuildInputArray();
      ExtractArray extract(composite, component);

      {
        auto refPortal = this->RefComposite.ReadPortal();
        auto outPortal = extract.WritePortal();
        for (viskores::Id i = 0; i < extract.GetNumberOfValues(); ++i)
        {
          auto ref = refPortal.Get(i);
          using Traits = viskores::VecTraits<decltype(ref)>;
          outPortal.Set(i, Traits::GetComponent(ref, component) * 2);
        }
      }

      this->ValidateWriteTestArray(composite, component);
    }

    // Exec test:
    {
      InputArray composite = this->BuildInputArray();
      ExtractArray extract(composite, component);

      viskores::cont::Invoker{}(WriteTestWorklet(component), this->RefComposite, extract);

      this->ValidateWriteTestArray(composite, component);
    }
  }

  void ValidateWriteTestArray(InputArray testArray, viskores::IdComponent component) const
  {
    using VectorType = typename ReferenceCompositeArray::ValueType;
    using Traits = viskores::VecTraits<VectorType>;

    // Check that the indicated component is twice the reference value.
    auto refPortal = this->RefComposite.ReadPortal();
    auto portal = testArray.ReadPortal();

    VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                         "Number of values in write test output do not match input.");

    for (viskores::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      auto value = portal.Get(i);
      auto refValue = refPortal.Get(i);
      Traits::SetComponent(refValue, component, Traits::GetComponent(refValue, component) * 2);

      VISKORES_TEST_ASSERT(test_equal(refValue, value, 0.), "Value mismatch in write test.");
    }
  }

  void TestComponent(viskores::IdComponent component) const
  {
    this->SanityCheck(component);
    this->ReadTestComponentExtraction(component);
    this->WriteTestComponentExtraction(component);
  }

  void operator()()
  {
    this->ConstructReferenceArray();

    this->TestComponent(0);
    this->TestComponent(1);
    this->TestComponent(2);
    this->TestComponent(3);
  }
};

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    ExtractComponentTests<ValueType>()();
  }
};

void TestArrayHandleExtractComponent()
{
  using TestTypes =
    viskores::List<viskores::Int32, viskores::Int64, viskores::Float32, viskores::Float64>;
  viskores::testing::Testing::TryTypes(ArgToTemplateType(), TestTypes());
}

} // end anon namespace

int UnitTestArrayHandleExtractComponent(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestArrayHandleExtractComponent, argc, argv);
}
