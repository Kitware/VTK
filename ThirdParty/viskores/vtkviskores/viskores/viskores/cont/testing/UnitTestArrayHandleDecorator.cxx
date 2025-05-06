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

#include <viskores/cont/ArrayHandleDecorator.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>

#include <viskores/BinaryOperators.h>

#include <viskores/cont/testing/Testing.h>

// not anonymous because nvcc sometimes complains about "unused" methods (that
// are definitely being used)
namespace ut_decorator
{

struct DecoratorTests
{
  static constexpr viskores::Id ARRAY_SIZE = 10;

  // Decorator implementation that demonstrates how to write invertible functors
  // that combine three array handles with complex access logic. The resulting
  // ArrayHandleDecorator can be both read from and written to.
  //
  // Constructs functors that take three portals.
  //
  // The first portal's values are accessed in reverse order.
  // The second portal's values are accessed in normal order.
  // The third portal's values are accessed via ((idx + 3) % size).
  //
  // Functor will return the max of the first two added to the third.
  //
  // InverseFunctor will update the third portal such that the Functor would
  // return the indicated value.
  struct InvertibleDecorImpl
  {

    // The functor used for reading data from the three portals.
    template <typename Portal1Type, typename Portal2Type, typename Portal3Type>
    struct Functor
    {
      using ValueType = typename Portal1Type::ValueType;

      Portal1Type Portal1;
      Portal2Type Portal2;
      Portal3Type Portal3;

      VISKORES_EXEC_CONT ValueType operator()(viskores::Id idx) const
      {
        const auto idx1 = this->Portal1.GetNumberOfValues() - idx - 1;
        const auto idx2 = idx;
        const auto idx3 = (idx + 3) % this->Portal3.GetNumberOfValues();

        const auto v1 = this->Portal1.Get(idx1);
        const auto v2 = this->Portal2.Get(idx2);
        const auto v3 = this->Portal3.Get(idx3);

        return viskores::Max(v1, v2) + v3;
      }
    };

    // The functor used for writing. Only Portal3 is written to, the other
    // portals may be read-only.
    template <typename Portal1Type, typename Portal2Type, typename Portal3Type>
    struct InverseFunctor
    {
      using ValueType = typename Portal1Type::ValueType;

      Portal1Type Portal1;
      Portal2Type Portal2;
      Portal3Type Portal3;

      VISKORES_EXEC_CONT void operator()(viskores::Id idx, const ValueType& vIn) const
      {
        const auto v1 = this->Portal1.Get(this->Portal1.GetNumberOfValues() - idx - 1);
        const auto v2 = this->Portal2.Get(idx);
        const auto vNew = static_cast<ValueType>(vIn - viskores::Max(v1, v2));
        this->Portal3.Set((idx + 3) % this->Portal3.GetNumberOfValues(), vNew);
      }
    };

    // Factory function that takes 3 portals as input and creates an instance
    // of Functor with them. Variadic template parameters are used here, but are
    // not necessary.
    template <typename... PortalTs>
    Functor<typename std::decay<PortalTs>::type...> CreateFunctor(PortalTs&&... portals) const
    {
      VISKORES_STATIC_ASSERT(sizeof...(PortalTs) == 3);
      return { std::forward<PortalTs>(portals)... };
    }

    // Factory function that takes 3 portals as input and creates an instance
    // of InverseFunctor with them. Variadic template parameters are used here,
    // but are not necessary.
    template <typename... PortalTs>
    InverseFunctor<typename std::decay<PortalTs>::type...> CreateInverseFunctor(
      PortalTs&&... portals) const
    {
      VISKORES_STATIC_ASSERT(sizeof...(PortalTs) == 3);
      return { std::forward<PortalTs>(portals)... };
    }
  };

  // Same as above, but cannot be inverted. The resulting ArrayHandleDecorator
  // will be read-only.
  struct NonInvertibleDecorImpl
  {
    template <typename Portal1Type, typename Portal2Type, typename Portal3Type>
    struct Functor
    {
      using ValueType = typename Portal1Type::ValueType;

      Portal1Type Portal1;
      Portal2Type Portal2;
      Portal3Type Portal3;

      VISKORES_EXEC_CONT ValueType operator()(viskores::Id idx) const
      {
        const auto v1 = this->Portal1.Get(this->Portal1.GetNumberOfValues() - idx - 1);
        const auto v2 = this->Portal2.Get(idx);
        const auto v3 = this->Portal3.Get((idx + 3) % this->Portal3.GetNumberOfValues());
        return viskores::Max(v1, v2) + v3;
      }
    };

    template <typename... PortalTs>
    Functor<typename std::decay<PortalTs>::type...> CreateFunctor(PortalTs&&... portals) const
    {
      VISKORES_STATIC_ASSERT(sizeof...(PortalTs) == 3);
      return { std::forward<PortalTs>(portals)... };
    }
  };

  // Decorator implementation that demonstrates how to create functors that
  // hold custom state. Here, the functors have a customizable Operation
  // member.
  //
  // This implementation is used to create a read-only ArrayHandleDecorator
  // that combines the values in two other ArrayHandles using an arbitrary
  // binary operation (e.g. viskores::Maximum, viskores::Add, etc).
  template <typename ValueType, typename OperationType>
  struct BinaryOperationDecorImpl
  {
    OperationType Operation;

    // The functor use to read values. Note that it holds extra state in
    // addition to the portals.
    template <typename Portal1Type, typename Portal2Type>
    struct Functor
    {
      Portal1Type Portal1;
      Portal2Type Portal2;
      OperationType Operation;

      VISKORES_EXEC_CONT
      ValueType operator()(viskores::Id idx) const
      {
        return this->Operation(static_cast<ValueType>(this->Portal1.Get(idx)),
                               static_cast<ValueType>(this->Portal2.Get(idx)));
      }
    };

    // A non-variadic example of a factory function to produce a functor. This
    // is where the extra state is passed into the functor.
    template <typename P1T, typename P2T>
    Functor<P1T, P2T> CreateFunctor(P1T p1, P2T p2) const
    {
      return { p1, p2, this->Operation };
    }
  };

  // Decorator implementation that reverses the ScanExtended operation.
  //
  // The resulting ArrayHandleDecorator will take an array produced by the
  // ScanExtended algorithm and return the original ScanExtended input.
  //
  // Some interesting things about this:
  // - The ArrayHandleDecorator's ValueType will not be the same as the
  //   ScanPortal's ValueType. The Decorator ValueType is determined by the
  //   return type of Functor::operator().
  // - The ScanPortal has more values than the ArrayHandleDecorator. The
  //   number of values the ArrayHandleDecorator should hold is set during
  //   construction and may differ from the arrays it holds.
  template <typename ValueType>
  struct ScanExtendedToNumIndicesDecorImpl
  {
    template <typename ScanPortalType>
    struct Functor
    {
      ScanPortalType ScanPortal;

      VISKORES_EXEC_CONT
      ValueType operator()(viskores::Id idx) const
      {
        return static_cast<ValueType>(this->ScanPortal.Get(idx + 1) - this->ScanPortal.Get(idx));
      }
    };

    template <typename ScanPortalType>
    Functor<ScanPortalType> CreateFunctor(ScanPortalType portal) const
    {
      return { portal };
    }
  };

  // Decorator implementation that combines two source arrays using the formula
  // `[source1] * 10 + [source2]` and supports resizing.
  template <typename ValueType>
  struct DecompositionDecorImpl
  {
    template <typename Portal1T, typename Portal2T>
    struct Functor
    {
      Portal1T Portal1;
      Portal2T Portal2;

      VISKORES_EXEC_CONT
      ValueType operator()(viskores::Id idx) const
      {
        return static_cast<ValueType>(this->Portal1.Get(idx) * 10 + this->Portal2.Get(idx));
      }
    };

    template <typename Portal1T, typename Portal2T>
    struct InverseFunctor
    {
      Portal1T Portal1;
      Portal2T Portal2;

      VISKORES_EXEC_CONT
      void operator()(viskores::Id idx, const ValueType& val) const
      {
        this->Portal1.Set(idx, static_cast<ValueType>(std::floor(val / 10)));
        this->Portal2.Set(idx, static_cast<ValueType>(std::fmod(val, 10)));
      }
    };

    template <typename Portal1T, typename Portal2T>
    VISKORES_CONT Functor<typename std::decay<Portal1T>::type, typename std::decay<Portal2T>::type>
    CreateFunctor(Portal1T&& p1, Portal2T&& p2) const
    {
      return { std::forward<Portal1T>(p1), std::forward<Portal2T>(p2) };
    }

    template <typename Portal1T, typename Portal2T>
    VISKORES_CONT
      InverseFunctor<typename std::decay<Portal1T>::type, typename std::decay<Portal2T>::type>
      CreateInverseFunctor(Portal1T&& p1, Portal2T&& p2) const
    {
      return { std::forward<Portal1T>(p1), std::forward<Portal2T>(p2) };
    }

    // Resize methods:
    template <typename Array1T, typename Array2T>
    VISKORES_CONT void AllocateSourceArrays(viskores::Id numVals,
                                            viskores::CopyFlag preserve,
                                            viskores::cont::Token& token,
                                            Array1T&& array1,
                                            Array2T&& array2) const
    {
      array1.Allocate(numVals, preserve, token);
      array2.Allocate(numVals, preserve, token);
    }
  };

  template <typename ValueType>
  void InversionTest() const
  {
    auto ah1 = viskores::cont::make_ArrayHandleCounting(ValueType{ 0 }, ValueType{ 2 }, ARRAY_SIZE);
    auto ah2 = viskores::cont::make_ArrayHandleConstant(ValueType{ ARRAY_SIZE }, ARRAY_SIZE);
    viskores::cont::ArrayHandle<ValueType> ah3;
    ah3.AllocateAndFill(ARRAY_SIZE, ValueType{ ARRAY_SIZE / 2 });

    auto ah3Const =
      viskores::cont::make_ArrayHandleConstant(ValueType{ ARRAY_SIZE / 2 }, ARRAY_SIZE);

    { // Has a writable handle and an invertible functor:
      auto ahInv =
        viskores::cont::make_ArrayHandleDecorator(ARRAY_SIZE, InvertibleDecorImpl{}, ah1, ah2, ah3);
      VISKORES_TEST_ASSERT(viskores::cont::internal::IsWritableArrayHandle<decltype(ahInv)>::value);
    }

    { // Has no writable handles and an invertible functor:
      auto ahNInv = viskores::cont::make_ArrayHandleDecorator(
        ARRAY_SIZE, InvertibleDecorImpl{}, ah1, ah2, ah3Const);
      VISKORES_TEST_ASSERT(
        !viskores::cont::internal::IsWritableArrayHandle<decltype(ahNInv)>::value);
    }

    { // Has writable handles, but the functor cannot be inverted:
      auto ahNInv = viskores::cont::make_ArrayHandleDecorator(
        ARRAY_SIZE, NonInvertibleDecorImpl{}, ah1, ah2, ah3);
      VISKORES_TEST_ASSERT(
        !viskores::cont::internal::IsWritableArrayHandle<decltype(ahNInv)>::value);
    }

    { // Has no writable handles and the functor cannot be inverted:
      auto ahNInv = viskores::cont::make_ArrayHandleDecorator(
        ARRAY_SIZE, NonInvertibleDecorImpl{}, ah1, ah2, ah3Const);
      VISKORES_TEST_ASSERT(
        !viskores::cont::internal::IsWritableArrayHandle<decltype(ahNInv)>::value);
    }

    { // Test reading/writing to an invertible handle:
      // Copy ah3 since we'll be modifying it:
      viskores::cont::ArrayHandle<ValueType> ah3Copy;
      viskores::cont::ArrayCopy(ah3, ah3Copy);

      auto ahDecor = viskores::cont::make_ArrayHandleDecorator(
        ARRAY_SIZE, InvertibleDecorImpl{}, ah1, ah2, ah3Copy);

      {
        auto portalDecor = ahDecor.ReadPortal();
        VISKORES_TEST_ASSERT(ahDecor.GetNumberOfValues() == ARRAY_SIZE);
        VISKORES_TEST_ASSERT(ahDecor.GetNumberOfComponentsFlat() ==
                             viskores::VecFlat<ValueType>::NUM_COMPONENTS);
        VISKORES_TEST_ASSERT(portalDecor.GetNumberOfValues() == ARRAY_SIZE);
        VISKORES_TEST_ASSERT(portalDecor.Get(0) == ValueType{ 23 });
        VISKORES_TEST_ASSERT(portalDecor.Get(1) == ValueType{ 21 });
        VISKORES_TEST_ASSERT(portalDecor.Get(2) == ValueType{ 19 });
        VISKORES_TEST_ASSERT(portalDecor.Get(3) == ValueType{ 17 });
        VISKORES_TEST_ASSERT(portalDecor.Get(4) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalDecor.Get(5) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalDecor.Get(6) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalDecor.Get(7) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalDecor.Get(8) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalDecor.Get(9) == ValueType{ 15 });
      }

      // Copy a constant array into the decorator. This should modify ah3Copy.
      viskores::cont::ArrayCopyDevice(
        viskores::cont::make_ArrayHandleConstant(ValueType{ 25 }, ARRAY_SIZE), ahDecor);

      { // Accessing portal should give all 25s:
        auto portalDecor = ahDecor.ReadPortal();
        VISKORES_TEST_ASSERT(ahDecor.GetNumberOfValues() == ARRAY_SIZE);
        VISKORES_TEST_ASSERT(ahDecor.GetNumberOfComponentsFlat() ==
                             viskores::VecFlat<ValueType>::NUM_COMPONENTS);
        VISKORES_TEST_ASSERT(portalDecor.GetNumberOfValues() == ARRAY_SIZE);
        VISKORES_TEST_ASSERT(portalDecor.Get(0) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(1) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(2) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(3) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(4) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(5) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(6) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(7) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(8) == ValueType{ 25 });
        VISKORES_TEST_ASSERT(portalDecor.Get(9) == ValueType{ 25 });
      }

      { // ah3Copy should have updated values:
        auto portalAH3Copy = ah3Copy.ReadPortal();
        VISKORES_TEST_ASSERT(ahDecor.GetNumberOfValues() == ARRAY_SIZE);
        VISKORES_TEST_ASSERT(ahDecor.GetNumberOfComponentsFlat() ==
                             viskores::VecFlat<ValueType>::NUM_COMPONENTS);
        VISKORES_TEST_ASSERT(portalAH3Copy.GetNumberOfValues() == ARRAY_SIZE);
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(0) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(1) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(2) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(3) == ValueType{ 7 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(4) == ValueType{ 9 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(5) == ValueType{ 11 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(6) == ValueType{ 13 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(7) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(8) == ValueType{ 15 });
        VISKORES_TEST_ASSERT(portalAH3Copy.Get(9) == ValueType{ 15 });
      }
    }
  }

  template <typename ValueType, typename OperationType>
  void BinaryOperatorTest() const
  {
    auto ahCount =
      viskores::cont::make_ArrayHandleCounting(ValueType{ 0 }, ValueType{ 1 }, ARRAY_SIZE);
    auto ahConst =
      viskores::cont::make_ArrayHandleConstant(ValueType{ ARRAY_SIZE / 2 }, ARRAY_SIZE);

    const OperationType op;
    BinaryOperationDecorImpl<ValueType, OperationType> impl{ op };

    auto decorArray = viskores::cont::make_ArrayHandleDecorator(ARRAY_SIZE, impl, ahCount, ahConst);

    {
      auto decorPortal = decorArray.ReadPortal();
      auto countPortal = ahCount.ReadPortal();
      auto constPortal = ahConst.ReadPortal();
      for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        VISKORES_TEST_ASSERT(decorPortal.Get(i) == op(countPortal.Get(i), constPortal.Get(i)));
      }
    }

    viskores::cont::ArrayHandle<ValueType> copiedInExec;
    viskores::cont::ArrayCopyDevice(decorArray, copiedInExec);
    {
      auto copiedPortal = copiedInExec.ReadPortal();
      auto countPortal = ahCount.ReadPortal();
      auto constPortal = ahConst.ReadPortal();
      for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        VISKORES_TEST_ASSERT(copiedPortal.Get(i) == op(countPortal.Get(i), constPortal.Get(i)));
      }
    }
  }


  template <typename ValueType>
  void ScanExtendedToNumIndicesTest() const
  {
    auto numIndicesOrig =
      viskores::cont::make_ArrayHandleCounting(ValueType{ 0 }, ValueType{ 1 }, ARRAY_SIZE);
    viskores::cont::ArrayHandle<viskores::Id> scan;
    viskores::cont::Algorithm::ScanExtended(
      viskores::cont::make_ArrayHandleCast<viskores::Id>(numIndicesOrig), scan);

    // Some interesting things to notice:
    // - `numIndicesDecor` will have `ARRAY_SIZE` entries, while `scan` has
    //   `ARRAY_SIZE + 1`.
    // - `numIndicesDecor` uses the current function scope `ValueType`, since
    //   that is what the functor from the implementation class returns. `scan`
    //   uses `viskores::Id`.
    auto numIndicesDecor = viskores::cont::make_ArrayHandleDecorator(
      ARRAY_SIZE, ScanExtendedToNumIndicesDecorImpl<ValueType>{}, scan);

    {
      auto origPortal = numIndicesOrig.ReadPortal();
      auto decorPortal = numIndicesDecor.ReadPortal();
      VISKORES_STATIC_ASSERT(VISKORES_PASS_COMMAS(
        std::is_same<decltype(origPortal.Get(0)), decltype(decorPortal.Get(0))>::value));
      VISKORES_TEST_ASSERT(origPortal.GetNumberOfValues() == decorPortal.GetNumberOfValues());
      for (viskores::Id i = 0; i < origPortal.GetNumberOfValues(); ++i)
      {
        VISKORES_TEST_ASSERT(origPortal.Get(i) == decorPortal.Get(i));
      }
    }
  }

  template <typename ValueType>
  void DecompositionTest() const
  {
    viskores::cont::ArrayHandle<ValueType> a1;
    viskores::cont::ArrayHandle<ValueType> a2;
    auto decor =
      viskores::cont::make_ArrayHandleDecorator(0, DecompositionDecorImpl<ValueType>{}, a1, a2);

    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 0);

    decor.Allocate(5);
    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 5);
    {
      auto decorPortal = decor.WritePortal();
      decorPortal.Set(0, 13);
      decorPortal.Set(1, 8);
      decorPortal.Set(2, 43);
      decorPortal.Set(3, 92);
      decorPortal.Set(4, 117);
    }

    VISKORES_TEST_ASSERT(a1.GetNumberOfValues() == 5);
    {
      auto a1Portal = a1.ReadPortal();
      VISKORES_TEST_ASSERT(test_equal(a1Portal.Get(0), 1));
      VISKORES_TEST_ASSERT(test_equal(a1Portal.Get(1), 0));
      VISKORES_TEST_ASSERT(test_equal(a1Portal.Get(2), 4));
      VISKORES_TEST_ASSERT(test_equal(a1Portal.Get(3), 9));
      VISKORES_TEST_ASSERT(test_equal(a1Portal.Get(4), 11));
    }

    VISKORES_TEST_ASSERT(a2.GetNumberOfValues() == 5);
    {
      auto a2Portal = a2.ReadPortal();
      VISKORES_TEST_ASSERT(test_equal(a2Portal.Get(0), 3));
      VISKORES_TEST_ASSERT(test_equal(a2Portal.Get(1), 8));
      VISKORES_TEST_ASSERT(test_equal(a2Portal.Get(2), 3));
      VISKORES_TEST_ASSERT(test_equal(a2Portal.Get(3), 2));
      VISKORES_TEST_ASSERT(test_equal(a2Portal.Get(4), 7));
    }

    decor.Allocate(3, viskores::CopyFlag::On);
    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 3);
    {
      auto decorPortal = decor.ReadPortal();
      VISKORES_TEST_ASSERT(test_equal(decorPortal.Get(0), 13));
      VISKORES_TEST_ASSERT(test_equal(decorPortal.Get(1), 8));
      VISKORES_TEST_ASSERT(test_equal(decorPortal.Get(2), 43));
    }

    VISKORES_TEST_ASSERT(a1.GetNumberOfValues() == 3);
    {
      auto a1Portal = a1.ReadPortal();
      VISKORES_TEST_ASSERT(test_equal(a1Portal.Get(0), 1));
      VISKORES_TEST_ASSERT(test_equal(a1Portal.Get(1), 0));
      VISKORES_TEST_ASSERT(test_equal(a1Portal.Get(2), 4));
    }

    VISKORES_TEST_ASSERT(a2.GetNumberOfValues() == 3);
    {
      auto a2Portal = a2.ReadPortal();
      VISKORES_TEST_ASSERT(test_equal(a2Portal.Get(0), 3));
      VISKORES_TEST_ASSERT(test_equal(a2Portal.Get(1), 8));
      VISKORES_TEST_ASSERT(test_equal(a2Portal.Get(2), 3));
    }
  }

  template <typename ValueType>
  void operator()(const ValueType) const
  {
    this->InversionTest<ValueType>();

    this->BinaryOperatorTest<ValueType, viskores::Maximum>();
    this->BinaryOperatorTest<ValueType, viskores::Minimum>();
    this->BinaryOperatorTest<ValueType, viskores::Add>();
    this->BinaryOperatorTest<ValueType, viskores::Subtract>();
    this->BinaryOperatorTest<ValueType, viskores::Multiply>();

    this->ScanExtendedToNumIndicesTest<ValueType>();

    this->DecompositionTest<ValueType>();
  }
};

// ArrayHandleDecorator that implements AllocateSourceArrays, thus allowing
// it to be resized.
struct ResizableDecorImpl
{
  // We don't actually read/write from this, so use a dummy functor:
  struct Functor
  {
    VISKORES_EXEC_CONT viskores::Id operator()(viskores::Id) const { return 0; }
  };

  template <typename... PortalTs>
  VISKORES_CONT Functor CreateFunctor(PortalTs...) const
  {
    return Functor{};
  }

  template <typename Array1T, typename Array2T>
  void AllocateSourceArrays(viskores::Id newSize,
                            viskores::CopyFlag preserve,
                            viskores::cont::Token& token,
                            Array1T& a1,
                            Array2T& a2) const
  {
    VISKORES_IS_ARRAY_HANDLE(Array1T);
    VISKORES_IS_ARRAY_HANDLE(Array2T);

    // Resize differently based on preserve to verify the flag is correct.
    if (preserve == viskores::CopyFlag::Off)
    {
      // Resize each to 3*newSize:
      a1.Allocate(3 * newSize, preserve, token);
      a2.Allocate(3 * newSize, preserve, token);
    }
    else
    {
      // Resize each to 2*newSize:
      a1.Allocate(2 * newSize, viskores::CopyFlag::On);
      a2.Allocate(2 * newSize, viskores::CopyFlag::On);
    }
  }
};

// ArrayHandleDecorator that does not implement AllocateSourceArrays, thus not allowing
// it to be resized.
struct NonResizableDecorImpl
{
  // We don't actually read/write from this, so use a dummy functor:
  struct Functor
  {
    VISKORES_EXEC_CONT viskores::Id operator()(viskores::Id) const { return 0; }
  };

  template <typename... PortalTs>
  VISKORES_CONT Functor CreateFunctor(PortalTs...) const
  {
    return Functor{};
  }
};

void ResizeTest()
{
  {
    viskores::cont::ArrayHandle<viskores::Id> a1;
    viskores::cont::ArrayHandle<viskores::Id> a2;
    ResizableDecorImpl impl;

    auto decor = viskores::cont::make_ArrayHandleDecorator(5, impl, a1, a2);
    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 5);

    decor.Allocate(10); // Should allocate a1&a2 to have 30 values:
    VISKORES_TEST_ASSERT(a1.GetNumberOfValues() == 30);
    VISKORES_TEST_ASSERT(a2.GetNumberOfValues() == 30);
    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 10);
    decor.Allocate(3, viskores::CopyFlag::On); // Should resize a1&a2 to have 6 values:
    VISKORES_TEST_ASSERT(a1.GetNumberOfValues() == 6);
    VISKORES_TEST_ASSERT(a2.GetNumberOfValues() == 6);
    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 3);
  }

  {
    viskores::cont::ArrayHandle<viskores::Id> a1;
    a1.Allocate(20);
    viskores::cont::ArrayHandle<viskores::Id> a2;
    a2.Allocate(20);
    NonResizableDecorImpl impl;

    auto decor = viskores::cont::make_ArrayHandleDecorator(5, impl, a1, a2);
    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 5);

    // Allocate and Shrink should throw an ErrorBadType:
    bool threw = false;
    try
    {
      decor.Allocate(10);
    }
    catch (viskores::cont::ErrorBadType& e)
    {
      std::cerr << "Caught expected exception: " << e.what() << "\n";
      threw = true;
    }
    VISKORES_TEST_ASSERT(threw, "Allocate did not throw as expected.");
    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 5);

    threw = false;
    try
    {
      decor.Allocate(3, viskores::CopyFlag::On);
    }
    catch (viskores::cont::ErrorBadType& e)
    {
      std::cerr << "Caught expected exception: " << e.what() << "\n";
      threw = true;
    }
    VISKORES_TEST_ASSERT(threw, "Allocate did not throw as expected.");
    VISKORES_TEST_ASSERT(decor.GetNumberOfValues() == 5);
  }
}

void TestArrayHandleDecorator()
{
  viskores::testing::Testing::TryTypes(DecoratorTests{}, viskores::TypeListScalarAll{});

  ResizeTest();
}

} // namespace ut_decorator

int UnitTestArrayHandleDecorator(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(ut_decorator::TestArrayHandleDecorator, argc, argv);
}
