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
#ifndef viskores_testing_Testing_h
#define viskores_testing_Testing_h

#include <viskores/Bitset.h>
#include <viskores/Bounds.h>
#include <viskores/CellShape.h>
#include <viskores/List.h>
#include <viskores/Math.h>
#include <viskores/Matrix.h>
#include <viskores/Pair.h>
#include <viskores/Range.h>
#include <viskores/TypeList.h>
#include <viskores/TypeTraits.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>
#include <viskores/VecVariable.h>

#include <viskores/cont/Logging.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <math.h>

// Uncomment to turn on floating point exceptions for Mac builds
// This non-portable solution is known to fail for some platforms (such as the dashboard).
//#define VISKORES_TEST_APPLE_FPE

#if defined(VISKORES_GCC) && !defined(__APPLE__)
#include <fenv.h>
#elif defined(__APPLE__) && defined(__MACH__) && defined(VISKORES_TEST_APPLE_FPE)
#include <cfenv>
#endif

// Try to enforce using the correct testing version. (Those that include the
// control environment have more possible exceptions.) This is not guaranteed
// to work. To make it more likely, place the Testing.h include last.
#ifdef viskores_cont_Error_h
#ifndef viskores_cont_testing_Testing_h
#error Use viskores::cont::testing::Testing instead of viskores::testing::Testing.
#else
#define VISKORES_TESTING_IN_CONT
#endif
#endif

/// \def VISKORES_STRINGIFY_FIRST(...)
///
/// A utility macro that takes 1 or more arguments and converts it into the C string version
/// of the first argument.

#define VISKORES_STRINGIFY_FIRST(...) \
  VISKORES_EXPAND(VISKORES_STRINGIFY_FIRST_IMPL(__VA_ARGS__, dummy))
#define VISKORES_STRINGIFY_FIRST_IMPL(first, ...) #first

/// \def VISKORES_TEST_ASSERT(condition, messages..)
///
/// Asserts a condition for a test to pass. A passing condition is when \a
/// condition resolves to true. If \a condition is false, then the test is
/// aborted and failure is returned. If one or more message arguments are
/// given, they are printed out by concatinating them. If no messages are
/// given, a generic message is given. In any case, the condition that failed
/// is written out.

#define VISKORES_TEST_ASSERT(...)       \
  ::viskores::testing::Testing::Assert( \
    VISKORES_STRINGIFY_FIRST(__VA_ARGS__), __FILE__, __LINE__, __func__, __VA_ARGS__)

/// \def VISKORES_TEST_FAIL(messages..)
///
/// Causes a test to fail with the given \a messages. At least one argument must be given.

#define VISKORES_TEST_FAIL(...) \
  ::viskores::testing::Testing::TestFail(__FILE__, __LINE__, __func__, __VA_ARGS__)

class TestEqualResult
{
public:
  void PushMessage(const std::string& msg) { this->Messages.push_back(msg); }

  const std::vector<std::string>& GetMessages() const { return this->Messages; }

  std::string GetMergedMessage() const
  {
    std::string msg;
    std::for_each(this->Messages.rbegin(),
                  this->Messages.rend(),
                  [&](const std::string& next)
                  {
                    msg += (msg.empty() ? "" : ": ");
                    msg += next;
                  });

    return msg;
  }

  operator bool() const { return this->Messages.empty(); }

private:
  std::vector<std::string> Messages;
};

namespace viskores
{
namespace testing
{

// TODO: Move these 2 functions to the testing library.

// Note: We are explicitly not trapping FE_INEXACT and FE_UNDERFLOW. Inexact numbers are too common
// to completely remove (that is the nature of floating point, especially when converting from
// integers), and underflows are considered normal in rendering (for example, the specular
// highlight essentially goes to zero most places).

inline void FloatingPointExceptionTrapEnable()
{
  // Turn on floating point exception trapping where available
#if defined(VISKORES_GCC) && !defined(__APPLE__)
  feenableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID);
#elif defined(__APPLE__) && defined(__MACH__) && defined(VISKORES_TEST_APPLE_FPE)
  std::fenv_t fenv;
  if (std::fegetenv(&fenv) != 0)
  {
    return;
  }
  int excepts = FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID;
#if defined(__arm) || defined(__arm64) || defined(__aarch64__)
  // ARM architecture
  fenv.__fpcr |= (excepts << 8);
#else
  // Intel architecture
  // Control flags are masked exceptions, so we have to unset them.
  fenv.__control &= ~excepts;
  fenv.__mxcsr &= ~(excepts << 7);
#endif
  fesetenv(&fenv);
#endif
}

inline void FloatingPointExceptionTrapDisable()
{
  // Turn on floating point exception trapping where available
#if defined(VISKORES_GCC) && !defined(__APPLE__)
  fedisableexcept(FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID);
#elif defined(__APPLE__) && defined(__MACH__) && defined(VISKORES_TEST_APPLE_FPE)
  std::fenv_t fenv;
  if (std::fegetenv(&fenv) != 0)
  {
    return;
  }
  int excepts = FE_DIVBYZERO | FE_OVERFLOW | FE_INVALID;
#if defined(__arm) || defined(__arm64) || defined(__aarch64__)
  // ARM architecture
  fenv.__fpcr &= ~(excepts << 8);
#else
  // Control flags are masked exceptions, so we have to set them.
  fenv.__control |= excepts;
  fenv.__mxcsr |= excepts << 7;
#endif
  fesetenv(&fenv);
#endif
}

// If you get an error about this class definition being incomplete, it means
// that you tried to get the name of a type that is not specified. You can
// either not use that type, not try to get the string name, or add it to the
// list.
template <typename T>
struct TypeName;

#define VISKORES_BASIC_TYPE(type, name) \
  template <>                           \
  struct TypeName<type>                 \
  {                                     \
    static std::string Name()           \
    {                                   \
      return #name;                     \
    }                                   \
  }

VISKORES_BASIC_TYPE(viskores::Float32, F32);
VISKORES_BASIC_TYPE(viskores::Float64, F64);
VISKORES_BASIC_TYPE(viskores::Int8, I8);
VISKORES_BASIC_TYPE(viskores::UInt8, UI8);
VISKORES_BASIC_TYPE(viskores::Int16, I16);
VISKORES_BASIC_TYPE(viskores::UInt16, UI16);
VISKORES_BASIC_TYPE(viskores::Int32, I32);
VISKORES_BASIC_TYPE(viskores::UInt32, UI32);
VISKORES_BASIC_TYPE(viskores::Int64, I64);
VISKORES_BASIC_TYPE(viskores::UInt64, UI64);

// types without viskores::typedefs:
VISKORES_BASIC_TYPE(bool, bool);
VISKORES_BASIC_TYPE(char, char);
VISKORES_BASIC_TYPE(long, long);
VISKORES_BASIC_TYPE(unsigned long, unsigned long);

#define VISKORES_BASIC_TYPE_HELPER(type) VISKORES_BASIC_TYPE(viskores::type, type)

// Special containers:
VISKORES_BASIC_TYPE_HELPER(Bounds);
VISKORES_BASIC_TYPE_HELPER(Range);

// Special Vec types:
VISKORES_BASIC_TYPE_HELPER(Vec2f_32);
VISKORES_BASIC_TYPE_HELPER(Vec2f_64);
VISKORES_BASIC_TYPE_HELPER(Vec2i_8);
VISKORES_BASIC_TYPE_HELPER(Vec2i_16);
VISKORES_BASIC_TYPE_HELPER(Vec2i_32);
VISKORES_BASIC_TYPE_HELPER(Vec2i_64);
VISKORES_BASIC_TYPE_HELPER(Vec2ui_8);
VISKORES_BASIC_TYPE_HELPER(Vec2ui_16);
VISKORES_BASIC_TYPE_HELPER(Vec2ui_32);
VISKORES_BASIC_TYPE_HELPER(Vec2ui_64);
VISKORES_BASIC_TYPE_HELPER(Vec3f_32);
VISKORES_BASIC_TYPE_HELPER(Vec3f_64);
VISKORES_BASIC_TYPE_HELPER(Vec3i_8);
VISKORES_BASIC_TYPE_HELPER(Vec3i_16);
VISKORES_BASIC_TYPE_HELPER(Vec3i_32);
VISKORES_BASIC_TYPE_HELPER(Vec3i_64);
VISKORES_BASIC_TYPE_HELPER(Vec3ui_8);
VISKORES_BASIC_TYPE_HELPER(Vec3ui_16);
VISKORES_BASIC_TYPE_HELPER(Vec3ui_32);
VISKORES_BASIC_TYPE_HELPER(Vec3ui_64);
VISKORES_BASIC_TYPE_HELPER(Vec4f_32);
VISKORES_BASIC_TYPE_HELPER(Vec4f_64);
VISKORES_BASIC_TYPE_HELPER(Vec4i_8);
VISKORES_BASIC_TYPE_HELPER(Vec4i_16);
VISKORES_BASIC_TYPE_HELPER(Vec4i_32);
VISKORES_BASIC_TYPE_HELPER(Vec4i_64);
VISKORES_BASIC_TYPE_HELPER(Vec4ui_8);
VISKORES_BASIC_TYPE_HELPER(Vec4ui_16);
VISKORES_BASIC_TYPE_HELPER(Vec4ui_32);
VISKORES_BASIC_TYPE_HELPER(Vec4ui_64);

#undef VISKORES_BASIC_TYPE

template <typename T, viskores::IdComponent Size>
struct TypeName<viskores::Vec<T, Size>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Vec<" << TypeName<T>::Name() << ", " << Size << ">";
    return stream.str();
  }
};

template <typename T, viskores::IdComponent numRows, viskores::IdComponent numCols>
struct TypeName<viskores::Matrix<T, numRows, numCols>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Matrix<" << TypeName<T>::Name() << ", " << numRows << ", " << numCols << ">";
    return stream.str();
  }
};

template <typename T, typename U>
struct TypeName<viskores::Pair<T, U>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Pair<" << TypeName<T>::Name() << ", " << TypeName<U>::Name() << ">";
    return stream.str();
  }
};

template <typename T>
struct TypeName<viskores::Bitset<T>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Bitset<" << TypeName<T>::Name() << ">";
    return stream.str();
  }
};

template <typename T0, typename... Ts>
struct TypeName<viskores::List<T0, Ts...>>
{
  static std::string Name()
  {
    std::initializer_list<std::string> subtypeStrings = { TypeName<Ts>::Name()... };

    std::stringstream stream;
    stream << "List<" << TypeName<T0>::Name();
    for (auto&& subtype : subtypeStrings)
    {
      stream << ", " << subtype;
    }
    stream << ">";
    return stream.str();
  }
};
template <>
struct TypeName<viskores::ListEmpty>
{
  static std::string Name() { return "ListEmpty"; }
};
template <>
struct TypeName<viskores::ListUniversal>
{
  static std::string Name() { return "ListUniversal"; }
};

namespace detail
{

template <viskores::IdComponent cellShapeId>
struct InternalTryCellShape
{
  template <typename FunctionType>
  void operator()(const FunctionType& function) const
  {
    this->PrintAndInvoke(function, typename viskores::CellShapeIdToTag<cellShapeId>::valid());
    InternalTryCellShape<cellShapeId + 1>()(function);
  }

private:
  template <typename FunctionType>
  void PrintAndInvoke(const FunctionType& function, std::true_type) const
  {
    using CellShapeTag = typename viskores::CellShapeIdToTag<cellShapeId>::Tag;
    std::cout << "*** " << viskores::GetCellShapeName(CellShapeTag()) << " ***************"
              << std::endl;
    function(CellShapeTag());
  }

  template <typename FunctionType>
  void PrintAndInvoke(const FunctionType&, std::false_type) const
  {
    // Not a valid cell shape. Do nothing.
  }
};

template <>
struct InternalTryCellShape<viskores::NUMBER_OF_CELL_SHAPES>
{
  template <typename FunctionType>
  void operator()(const FunctionType&) const
  {
    // Done processing cell sets. Do nothing and return.
  }
};

} // namespace detail

struct Testing
{
public:
  class TestFailure
  {
  public:
    template <typename... Ts>
    VISKORES_CONT TestFailure(const std::string& file,
                              viskores::Id line,
                              const char* func,
                              Ts&&... messages)
      : File(file)
      , Line(line)
      , Func(func)
    {
      std::stringstream messageStream;
      this->AppendMessages(messageStream, std::forward<Ts>(messages)...);
      this->Message = messageStream.str();
    }

    VISKORES_CONT const std::string& GetFile() const { return this->File; }
    VISKORES_CONT viskores::Id GetLine() const { return this->Line; }
    VISKORES_CONT const char* GetFunction() const { return this->Func; }
    VISKORES_CONT const std::string& GetMessage() const { return this->Message; }

  private:
    template <typename T1>
    VISKORES_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1)
    {
      messageStream << m1;
    }
    template <typename T1, typename T2>
    VISKORES_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1, T2&& m2)
    {
      messageStream << m1 << m2;
    }
    template <typename T1, typename T2, typename T3>
    VISKORES_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1, T2&& m2, T3&& m3)
    {
      messageStream << m1 << m2 << m3;
    }
    template <typename T1, typename T2, typename T3, typename T4>
    VISKORES_CONT void AppendMessages(std::stringstream& messageStream,
                                      T1&& m1,
                                      T2&& m2,
                                      T3&& m3,
                                      T4&& m4)
    {
      messageStream << m1 << m2 << m3 << m4;
    }
    template <typename T1, typename T2, typename T3, typename T4, typename... Ts>
    VISKORES_CONT void AppendMessages(std::stringstream& messageStream,
                                      T1&& m1,
                                      T2&& m2,
                                      T3&& m3,
                                      T4&& m4,
                                      Ts&&... ms)
    {
      messageStream << m1 << m2 << m3 << m4;
      this->AppendMessages(messageStream, std::forward<Ts>(ms)...);
    }

    std::string File;
    viskores::Id Line;
    const char* Func;
    std::string Message;
  };

  template <typename... Ts>
  static VISKORES_CONT void Assert(const std::string& conditionString,
                                   const std::string& file,
                                   viskores::Id line,
                                   const char* func,
                                   bool condition,
                                   Ts&&... messages)
  {
    if (condition)
    {
      // Do nothing.
    }
    else
    {
      throw TestFailure(
        file, line, func, std::forward<Ts>(messages)..., " (", conditionString, ")");
    }
  }

  static VISKORES_CONT void Assert(const std::string& conditionString,
                                   const std::string& file,
                                   viskores::Id line,
                                   const char* func,
                                   bool condition)
  {
    Assert(conditionString, file, line, func, condition, "Test assertion failed");
  }

  static VISKORES_CONT void Assert(const std::string& conditionString,
                                   const std::string& file,
                                   viskores::Id line,
                                   const char* func,
                                   const TestEqualResult& result)
  {
    Assert(conditionString, file, line, func, static_cast<bool>(result), result.GetMergedMessage());
  }

  template <typename... Ts>
  static VISKORES_CONT void TestFail(const std::string& file,
                                     viskores::Id line,
                                     const char* func,
                                     Ts&&... messages)
  {
    throw TestFailure(file, line, func, std::forward<Ts>(messages)...);
  }

#ifndef VISKORES_TESTING_IN_CONT
  /// Calls the test function \a function with no arguments. Catches any errors
  /// generated by VISKORES_TEST_ASSERT or VISKORES_TEST_FAIL, reports the error, and
  /// returns "1" (a failure status for a program's main). Returns "0" (a
  /// success status for a program's main).
  ///
  /// The intention is to implement a test's main function with this. For
  /// example, the implementation of UnitTestFoo might look something like
  /// this.
  ///
  /// \code
  /// #include <viskores/testing/Testing.h>
  ///
  /// namespace {
  ///
  /// void TestFoo()
  /// {
  ///    // Do actual test, which checks in VISKORES_TEST_ASSERT or VISKORES_TEST_FAIL.
  /// }
  ///
  /// } // anonymous namespace
  ///
  /// int UnitTestFoo(int, char *[])
  /// {
  ///   return viskores::testing::Testing::Run(TestFoo);
  /// }
  /// \endcode
  ///
  template <class Func>
  static VISKORES_CONT int Run(Func function, int& argc, char* argv[])
  {
    if (argc == 0 || argv == nullptr)
    {
      viskores::cont::InitLogging();
    }
    else
    {
      viskores::cont::InitLogging(argc, argv);
    }

    // Some simulations trap floating point exceptions, and we want to be able to run in them
    viskores::testing::FloatingPointExceptionTrapEnable();

    try
    {
      function();
    }
    catch (TestFailure const& error)
    {
      std::cerr << "***** Test failed @ " << error.GetFile() << ":" << error.GetLine() << ":"
                << error.GetFunction() << "\n"
                << error.GetMessage() << "\n";
      return 1;
    }
    catch (std::exception const& error)
    {
      std::cerr << "***** STL exception throw.\n" << error.what() << "\n";
      return 1;
    }
    catch (...)
    {
      std::cerr << "***** Unidentified exception thrown.\n";
      return 1;
    }
    return 0;
  }
#endif

  template <typename FunctionType>
  struct InternalPrintTypeAndInvoke
  {
    InternalPrintTypeAndInvoke(FunctionType function)
      : Function(function)
    {
    }

    template <typename T>
    void operator()(T t) const
    {
      std::cout << "*** " << viskores::testing::TypeName<T>::Name() << " ***************"
                << std::endl;
      this->Function(t);
    }

  private:
    FunctionType Function;
  };

  /// Runs template \p function on all the types in the given list. If no type
  /// list is given, then an exemplar list of types is used.
  ///
  template <typename FunctionType, typename TypeList>
  static void TryTypes(const FunctionType& function, TypeList)
  {
    viskores::ListForEach(InternalPrintTypeAndInvoke<FunctionType>(function), TypeList());
  }

  using TypeListExemplarTypes =
    viskores::List<viskores::UInt8, viskores::Id, viskores::FloatDefault, viskores::Vec3f_64>;

  template <typename FunctionType>
  static void TryTypes(const FunctionType& function)
  {
    TryTypes(function, TypeListExemplarTypes());
  }

  // Disabled: This very long list results is very long compile times.
  //  /// Runs templated \p function on all the basic types defined in Viskores. This
  //  /// is helpful to test templated functions that should work on all types. If
  //  /// the function is supposed to work on some subset of types, then use
  //  /// \c TryTypes to restrict the call to some other list of types.
  //  ///
  //  template<typename FunctionType>
  //  static void TryAllTypes(const FunctionType &function)
  //  {
  //    TryTypes(function, viskores::TypeListAll());
  //  }

  /// Runs templated \p function on all cell shapes defined in Viskores. This is
  /// helpful to test templated functions that should work on all cell types.
  ///
  template <typename FunctionType>
  static void TryAllCellShapes(const FunctionType& function)
  {
    detail::InternalTryCellShape<0>()(function);
  }
};
}
} // namespace viskores::internal

namespace detail
{

// Forward declaration
template <typename T1, typename T2>
struct TestEqualImpl;

} // namespace detail

/// Helper function to test two quanitites for equality accounting for slight
/// variance due to floating point numerical inaccuracies.
///
template <typename T1, typename T2>
static inline VISKORES_EXEC_CONT bool test_equal(T1 value1,
                                                 T2 value2,
                                                 viskores::Float64 tolerance = 0.00001)
{
  return detail::TestEqualImpl<T1, T2>()(value1, value2, tolerance);
}

namespace detail
{

template <typename T1, typename T2>
struct TestEqualImpl
{
  template <typename IsBase1, typename IsBase2>
  VISKORES_EXEC_CONT bool DoIt(T1 vector1,
                               T2 vector2,
                               viskores::Float64 tolerance,
                               IsBase1,
                               IsBase2) const
  {
    using Traits1 = viskores::VecTraits<T1>;
    using Traits2 = viskores::VecTraits<T2>;

    // If vectors have different number of components, then they cannot be equal.
    if (Traits1::GetNumberOfComponents(vector1) != Traits2::GetNumberOfComponents(vector2))
    {
      return false;
    }

    for (viskores::IdComponent component = 0; component < Traits1::GetNumberOfComponents(vector1);
         ++component)
    {
      bool componentEqual = test_equal(Traits1::GetComponent(vector1, component),
                                       Traits2::GetComponent(vector2, component),
                                       tolerance);
      if (!componentEqual)
      {
        return false;
      }
    }

    return true;
  }

  VISKORES_EXEC_CONT bool DoIt(T1 scalar1,
                               T2 scalar2,
                               viskores::Float64 tolerance,
                               std::true_type,
                               std::true_type) const
  {
    // Do all comparisons using 64-bit floats.
    return test_equal(
      static_cast<viskores::Float64>(scalar1), static_cast<viskores::Float64>(scalar2), tolerance);
  }

  VISKORES_EXEC_CONT bool operator()(T1 value1, T2 value2, viskores::Float64 tolerance) const
  {
    using Base1 = typename viskores::VecTraits<T1>::BaseComponentType;
    using Base2 = typename viskores::VecTraits<T2>::BaseComponentType;
    return this->DoIt(value1,
                      value2,
                      tolerance,
                      typename std::is_same<T1, Base1>::type{},
                      typename std::is_same<T2, Base2>::type{});
  }
};

template <>
struct TestEqualImpl<viskores::Float64, viskores::Float64>
{
  VISKORES_EXEC_CONT bool operator()(viskores::Float64 value1,
                                     viskores::Float64 value2,
                                     viskores::Float64 tolerance) const
  {
    // Handle non-finites. Normally, non-finites are never "equal" to each other (for valid
    // mathematical reasons), but for testing purposes if the two values are the same type of
    // non-finite, then they are the same in the sense that they gave the same result.
    if (viskores::IsNan(value1) && viskores::IsNan(value2))
    {
      return true;
    }
    if (viskores::IsInf(value1) && viskores::IsInf(value2) &&
        (viskores::IsNegative(value1) == viskores::IsNegative(value2)))
    {
      return true;
    }

    if (viskores::Abs(value1 - value2) <= tolerance)
    {
      return true;
    }

    // We are using a ratio to compare the relative tolerance of two numbers.
    // Using an ULP based comparison (comparing the bits as integers) might be
    // a better way to go, but this has been working pretty well so far.
    viskores::Float64 ratio;
    if ((viskores::Abs(value2) > tolerance) && (value2 != 0))
    {
      ratio = value1 / value2;
    }
    else
    {
      // If we are here, it means that value2 is close to 0 but value1 is not.
      // These cannot be within tolerance, so just return false.
      return false;
    }
    if ((ratio > viskores::Float64(1.0) - tolerance) &&
        (ratio < viskores::Float64(1.0) + tolerance))
    {
      // This component is OK. The condition is checked in this way to
      // correctly handle non-finites that fail all comparisons. Thus, if a
      // non-finite is encountered, this condition will fail and false will be
      // returned.
      return true;
    }
    else
    {
      return false;
    }
  }
};

/// Special implementation of test_equal for strings, which don't fit a model
/// of fixed length vectors of numbers.
///
template <>
struct TestEqualImpl<std::string, std::string>
{
  VISKORES_CONT bool operator()(const std::string& string1,
                                const std::string& string2,
                                viskores::Float64 viskoresNotUsed(tolerance)) const
  {
    return string1 == string2;
  }
};
template <typename T>
struct TestEqualImpl<const char*, T>
{
  VISKORES_CONT bool operator()(const char* string1, T value2, viskores::Float64 tolerance) const
  {
    return TestEqualImpl<std::string, T>()(string1, value2, tolerance);
  }
};
template <typename T>
struct TestEqualImpl<T, const char*>
{
  VISKORES_CONT bool operator()(T value1, const char* string2, viskores::Float64 tolerance) const
  {
    return TestEqualImpl<T, std::string>()(value1, string2, tolerance);
  }
};
template <>
struct TestEqualImpl<const char*, const char*>
{
  VISKORES_CONT bool operator()(const char* string1,
                                const char* string2,
                                viskores::Float64 tolerance) const
  {
    return TestEqualImpl<std::string, std::string>()(string1, string2, tolerance);
  }
};

/// Special implementation of test_equal for Pairs, which are a bit different
/// than a vector of numbers of the same type.
///
template <typename T1, typename T2, typename T3, typename T4>
struct TestEqualImpl<viskores::Pair<T1, T2>, viskores::Pair<T3, T4>>
{
  VISKORES_EXEC_CONT bool operator()(const viskores::Pair<T1, T2>& pair1,
                                     const viskores::Pair<T3, T4>& pair2,
                                     viskores::Float64 tolerance) const
  {
    return test_equal(pair1.first, pair2.first, tolerance) &&
      test_equal(pair1.second, pair2.second, tolerance);
  }
};

/// Special implementation of test_equal for Ranges.
///
template <>
struct TestEqualImpl<viskores::Range, viskores::Range>
{
  VISKORES_EXEC_CONT bool operator()(const viskores::Range& range1,
                                     const viskores::Range& range2,
                                     viskores::Float64 tolerance) const
  {
    return (test_equal(range1.Min, range2.Min, tolerance) &&
            test_equal(range1.Max, range2.Max, tolerance));
  }
};

/// Special implementation of test_equal for Bounds.
///
template <>
struct TestEqualImpl<viskores::Bounds, viskores::Bounds>
{
  VISKORES_EXEC_CONT bool operator()(const viskores::Bounds& bounds1,
                                     const viskores::Bounds& bounds2,
                                     viskores::Float64 tolerance) const
  {
    return (test_equal(bounds1.X, bounds2.X, tolerance) &&
            test_equal(bounds1.Y, bounds2.Y, tolerance) &&
            test_equal(bounds1.Z, bounds2.Z, tolerance));
  }
};

/// Special implementation of test_equal for booleans.
///
template <>
struct TestEqualImpl<bool, bool>
{
  VISKORES_EXEC_CONT bool operator()(bool bool1,
                                     bool bool2,
                                     viskores::Float64 viskoresNotUsed(tolerance))
  {
    return bool1 == bool2;
  }
};

} // namespace detail

namespace detail
{

template <typename T>
struct TestValueImpl;

} // namespace detail

/// Many tests involve getting and setting values in some index-based structure
/// (like an array). These tests also often involve trying many types. The
/// overloaded TestValue function returns some unique value for an index for a
/// given type. Different types might give different values.
///
template <typename T>
static inline VISKORES_EXEC_CONT T TestValue(viskores::Id index, T)
{
  return detail::TestValueImpl<T>()(index);
}

namespace detail
{

template <typename T>
struct TestValueImpl
{
  VISKORES_EXEC_CONT T DoIt(viskores::Id index, viskores::TypeTraitsIntegerTag) const
  {
    constexpr bool larger_than_2bytes = sizeof(T) > 2;
    if (larger_than_2bytes)
    {
      return T(index * 100);
    }
    else
    {
      return T(index + 100);
    }
  }

  VISKORES_EXEC_CONT T DoIt(viskores::Id index, viskores::TypeTraitsRealTag) const
  {
    return T(0.01f * static_cast<float>(index) + 1.001f);
  }

  VISKORES_EXEC_CONT T operator()(viskores::Id index) const
  {
    return this->DoIt(index, typename viskores::TypeTraits<T>::NumericTag());
  }
};

template <typename T, viskores::IdComponent N>
struct TestValueImpl<viskores::Vec<T, N>>
{
  VISKORES_EXEC_CONT viskores::Vec<T, N> operator()(viskores::Id index) const
  {
    viskores::Vec<T, N> value;
    for (viskores::IdComponent i = 0; i < N; i++)
    {
      value[i] = TestValue(index * N + i, T());
    }
    return value;
  }
};

template <typename U, typename V>
struct TestValueImpl<viskores::Pair<U, V>>
{
  VISKORES_EXEC_CONT viskores::Pair<U, V> operator()(viskores::Id index) const
  {
    return viskores::Pair<U, V>(TestValue(2 * index, U()), TestValue(2 * index + 1, V()));
  }
};

template <typename T, viskores::IdComponent NumRow, viskores::IdComponent NumCol>
struct TestValueImpl<viskores::Matrix<T, NumRow, NumCol>>
{
  VISKORES_EXEC_CONT viskores::Matrix<T, NumRow, NumCol> operator()(viskores::Id index) const
  {
    viskores::Matrix<T, NumRow, NumCol> value;
    viskores::Id runningIndex = index * NumRow * NumCol;
    for (viskores::IdComponent row = 0; row < NumRow; ++row)
    {
      for (viskores::IdComponent col = 0; col < NumCol; ++col)
      {
        value(row, col) = TestValue(runningIndex, T());
        ++runningIndex;
      }
    }
    return value;
  }
};

template <>
struct TestValueImpl<std::string>
{
  VISKORES_CONT std::string operator()(viskores::Id index) const
  {
    std::stringstream stream;
    stream << index;
    return stream.str();
  }
};

} // namespace detail

/// Verifies that the contents of the given array portal match the values
/// returned by viskores::testing::TestValue.
///
template <typename PortalType>
static inline VISKORES_CONT void CheckPortal(
  const PortalType& portal,
  typename PortalType::ValueType offset = typename PortalType::ValueType(0))
{
  using ValueType = typename PortalType::ValueType;

  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    ValueType expectedValue = TestValue(index, ValueType()) + offset;
    ValueType foundValue = portal.Get(index);
    if (!test_equal(expectedValue, foundValue))
    {
      std::stringstream message;
      message << "Got unexpected value in array. Expected: " << expectedValue
              << ", Found: " << foundValue << "\n";
      VISKORES_TEST_FAIL(message.str().c_str());
    }
  }
}

/// Sets all the values in a given array portal to be the values returned
/// by viskores::testing::TestValue. The ArrayPortal must be allocated first.
///
template <typename PortalType>
static inline VISKORES_CONT void SetPortal(const PortalType& portal)
{
  using ValueType = typename PortalType::ValueType;

  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    portal.Set(index, TestValue(index, ValueType()));
  }
}

/// Verifies that the contents of the two portals are the same.
///
template <typename PortalType1, typename PortalType2>
static inline VISKORES_CONT bool test_equal_portals(const PortalType1& portal1,
                                                    const PortalType2& portal2)
{
  if (portal1.GetNumberOfValues() != portal2.GetNumberOfValues())
  {
    return false;
  }

  for (viskores::Id index = 0; index < portal1.GetNumberOfValues(); index++)
  {
    if (!test_equal(portal1.Get(index), portal2.Get(index)))
    {
      return false;
    }
  }

  return true;
}

#endif //viskores_testing_Testing_h
