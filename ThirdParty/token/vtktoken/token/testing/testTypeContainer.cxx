//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "token/Compiler.h" // for expectedKeys
#include "token/TypeContainer.h"

#include "token/testing/helpers.h"

#include <cmath>

namespace
{
const double float_epsilon = 1.e-6;

struct Foo
{
  Foo(int i)
    : value(i)
  {
  }
  int value;
};

struct Bar
{
  Bar(int i)
    : value(i)
  {
  }
  Bar(const Bar& other) = default;
  Bar(Bar&& other) = default;

  int value;
};
} // namespace

int testTypeContainer(int /*unused*/, char** const /*unused*/)
{
  token_NAMESPACE::TypeContainer typeContainer;

  test(typeContainer.empty(), "New instance shoud contain no values.");
  test(!typeContainer.contains<int>(), "New instance should contain no values of integer type.");

  typeContainer.get<int>() = 3;

  test(typeContainer.size() == 1, "Assigned value should increment the container size.");
  test(typeContainer.get<int>() == 3, "Assigned value should be retrievable.");

  typeContainer.clear();

  test(typeContainer.empty(), "Cleared instance shoud contain no values.");
  test(
    !typeContainer.contains<int>(), "Cleared instance should contain no values of integer type.");

  typeContainer.insert<float>(2.3f);

  test(
    fabs(typeContainer.get<float>() - 2.3f) < float_epsilon,
    "Assigned value should be retrievable.");

  try
  {
    typeContainer.get<Foo>() = Foo(3);
    test(false, "Access to a type with no default constructor should throw an error.");
  }
  catch (const std::out_of_range&)
  {
  }

  typeContainer.emplace<Foo>(3);
  test(typeContainer.get<Foo>().value == 3, "Assigned value should be retrievable.");

  typeContainer.insert<Bar>(Bar(2));
  test(typeContainer.get<Bar>().value == 2, "Assigned value should be retrievable.");

  token_NAMESPACE::TypeContainer typeContainer2(typeContainer);
  test(typeContainer2.get<Foo>().value == 3, "Copied container should behave like the original.");
  test(typeContainer2.get<Bar>().value == 2, "Copied container should behave like the original.");

  token_NAMESPACE::TypeContainer typeContainer3(typeContainer2.get<Foo>(), typeContainer2.get<Bar>());
  test(
    typeContainer3.get<Foo>().value == 3,
    "Variadic constructed container should behave like the original.");
  test(
    typeContainer3.get<Bar>().value == 2,
    "Variadic constructed container should behave like the original.");

  {
    using namespace token_NAMESPACE::literals;
    std::cout << "Type container now holds:\n";
    for (const auto& token : typeContainer.keys())
    {
      std::cout << "  " << token.data() << " (" << token.getId() << ")\n";
    }
    std::unordered_set<token_NAMESPACE::Token> expectedKeys{
      { "float"_token, "(anonymous namespace)::Foo"_token, "(anonymous namespace)::Bar"_token }
    };
    test(typeContainer.keys() == expectedKeys, "Container keys were improperly reported.");
  }

  return 0;
}
