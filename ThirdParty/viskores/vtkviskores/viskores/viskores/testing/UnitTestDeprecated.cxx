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

#include <viskores/Deprecated.h>

#include <viskores/testing/Testing.h>

namespace
{

struct NewClass
{
  VISKORES_CONT
  void ImportantMethod(double x, double tolerance)
  {
    std::cout << "Using " << x << " with tolerance " << tolerance << std::endl;
  }

  VISKORES_CONT
  VISKORES_DEPRECATED(1.7, "You must now specify a tolerance.") void ImportantMethod(double x)
  {
    this->ImportantMethod(x, 1e-6);
  }

  VISKORES_CONT
  VISKORES_DEPRECATED(1.6, "You must now specify both a value and tolerance.")
  void ImportantMethod()
  {
    // It can be the case that to implement a deprecated method you need to use other
    // deprecated features. To do that, just temporarily suppress those warnings.
    VISKORES_DEPRECATED_SUPPRESS_BEGIN
    this->ImportantMethod(0.0);
    VISKORES_DEPRECATED_SUPPRESS_END
  }
};

struct VISKORES_DEPRECATED(1.6, "OldClass replaced with NewClass.") OldClass
{
};

using OldAlias VISKORES_DEPRECATED(1.6, "Use NewClass instead.") = NewClass;

// Should be OK for one deprecated alias to use another deprecated thing, but most compilers
// do not think so. So, when implementing deprecated things, you might need to suppress
// warnings for that part of the code.
VISKORES_DEPRECATED_SUPPRESS_BEGIN
using OlderAlias VISKORES_DEPRECATED(1.6, "Update your code to NewClass.") = OldAlias;
VISKORES_DEPRECATED_SUPPRESS_END

enum struct VISKORES_DEPRECATED(1.7, "Use NewEnum instead.") OldEnum
{
  OLD_VALUE
};

enum struct NewEnum
{
  OLD_VALUE1 VISKORES_DEPRECATED(1.7, "Use NEW_VALUE instead."),
  NEW_VALUE,
  OLD_VALUE2 VISKORES_DEPRECATED(1.7) = 42
};

template <typename T>
void DoSomethingWithObject(T)
{
  std::cout << "Looking at " << typeid(T).name() << std::endl;
}

static void DoTest()
{
  std::cout << "C++14 [[deprecated]] supported: "
#ifdef VISKORES_DEPRECATED_ATTRIBUTE_SUPPORTED
            << "yes"
#else
            << "no"
#endif
            << std::endl;
  std::cout << "Deprecated warnings can be suppressed: "
#ifdef VISKORES_DEPRECATED_SUPPRESS_SUPPORTED
            << "yes"
#else
            << "no"
#endif
            << std::endl;
  std::cout << "Deprecation is: " << VISKORES_STRINGIFY_FIRST(VISKORES_DEPRECATED(X.Y, "Message."))
            << std::endl;

  VISKORES_TEST_ASSERT(VISKORES_DEPRECATED_MAKE_MESSAGE(X.Y) ==
                       std::string(" Deprecated in version X.Y."));
  VISKORES_TEST_ASSERT(VISKORES_DEPRECATED_MAKE_MESSAGE(X.Y.Z, "Use feature foo instead.") ==
                       std::string("Use feature foo instead. Deprecated in version X.Y.Z."));

  // Using valid classes with unused deprecated parts should be fine.
  NewClass useIt;
  DoSomethingWithObject(useIt);
  useIt.ImportantMethod(1.1, 1e-8);
  DoSomethingWithObject(NewEnum::NEW_VALUE);

  // These should each give compiler warnings without the suppressions.
  VISKORES_DEPRECATED_SUPPRESS_BEGIN
  OldClass useOldClass;
  DoSomethingWithObject(useOldClass);
  OldAlias useOldAlias;
  DoSomethingWithObject(useOldAlias);
  OlderAlias useOlderAlias;
  DoSomethingWithObject(useOlderAlias);
  useIt.ImportantMethod(1.1);
  useIt.ImportantMethod();
  DoSomethingWithObject(OldEnum::OLD_VALUE);
  DoSomethingWithObject(NewEnum::OLD_VALUE1);
  DoSomethingWithObject(NewEnum::OLD_VALUE2);
  VISKORES_DEPRECATED_SUPPRESS_END
}

} // anonymous namespace

int UnitTestDeprecated(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(DoTest, argc, argv);
}
