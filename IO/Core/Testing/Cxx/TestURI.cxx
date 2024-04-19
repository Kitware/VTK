// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkURI.h"

#include <iostream>

#define Check(expr, message)                                                                       \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      vtkErrorWithObjectMacro(nullptr, "Test failed: \n" << message);                              \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

namespace
{

bool Parse(const std::string& uri)
{
  auto parsed = vtkURI::Parse(uri);
  Check(parsed, "Failed to parse URI \"" + uri + "\"");
  const auto parsedStr = parsed->ToString();
  Check(parsedStr == uri, "Parsed URI (" + parsedStr + ") does not match input (" + uri + ")");

  return true;
}

bool TestParsing()
{
  // full
  Check(Parse("scheme://auth/p/a/t/h?query#frag"), "Parsing failed");

  // parts
  Check(Parse("scheme:"), "Parsing failed");
  Check(Parse("//auth"), "Parsing failed");
  Check(Parse("/p/a/t/h"), "Parsing failed");
  Check(Parse("p/a/t/h"), "Parsing failed");
  Check(Parse("?query"), "Parsing failed");
  Check(Parse("#frag"), "Parsing failed");

  // combinations
  Check(Parse("scheme:/p/a/t/h#frag"), "Parsing failed");
  Check(Parse("scheme://auth#frag"), "Parsing failed");
  Check(Parse("scheme:#frag"), "Parsing failed");
  Check(Parse("scheme:?query#frag"), "Parsing failed");
  Check(Parse("//auth/p/a/t/h?query#frag"), "Parsing failed");
  Check(Parse("p/a/t/h?query#frag"), "Parsing failed");

  return true;
}

bool TestEmptyButDefinedComponents()
{
  auto uri = vtkURI::Parse("s://?#");
  Check(uri->GetScheme(), "Scheme must be defined");
  Check(uri->GetScheme().GetValue() == "s",
    "Scheme must be \"s\", got \"" + uri->GetScheme().GetValue() + "\"");

  Check(uri->GetAuthority(), "Authority must be defined");
  Check(uri->GetAuthority().GetValue().empty(), "Authority must be empty");
  Check(uri->GetPath(), "Path must be defined");
  Check(uri->GetPath().GetValue().empty(), "Path must be empty");
  Check(uri->GetQuery(), "Query must be defined");
  Check(uri->GetQuery().GetValue().empty(), "Query must be empty");
  Check(uri->GetFragment(), "Fragment must be defined");
  Check(uri->GetFragment().GetValue().empty(), "Fragment must be empty");

  const auto uriStr = uri->ToString();
  Check(
    uriStr == "s://?#", "Invalid string recomposition, expected \"s://?#\" got \"" + uriStr + "\"");

  return true;
}

bool TestTypes()
{
  Check(vtkURI::Parse("s:")->IsAbsolute(), "\"s:\" must be absolute");
  Check(!vtkURI::Parse("s:#f")->IsAbsolute(), "\"s:#f\" must not be absolute");

  Check(vtkURI::Parse("//")->IsRelative(), "\"//\" must be relative");
  Check(vtkURI::Parse("p")->IsRelative(), "\"p\" must be relative");
  Check(vtkURI::Parse("?")->IsRelative(), "\"?\" must be relative");
  Check(vtkURI::Parse("#")->IsRelative(), "\"#\" must be relative");
  Check(vtkURI::Parse("///?#")->IsRelative(), "\"///?#\" must be relative");
  Check(!vtkURI::Parse("s:")->IsRelative(), "\"s:\" must not be relative");

  Check(vtkURI::Parse("s:///")->IsFull(), "\"s:///\" must be full");
  Check(vtkURI::Parse("s:///?#")->IsFull(), "\"s:///?#\" must be full");
  Check(!vtkURI::Parse("///?#")->IsFull(), "\"///?#\" must not be full");

  Check(vtkURI::Parse("#")->IsSameDocRef(), "\"s:///\" must be same document reference");
  Check(!vtkURI::Parse("s:///?#")->IsSameDocRef(), "\"s:///?#\" must be full");

  Check(vtkURI::Parse("")->IsEmpty(), "\"\" must be empty");
  Check(!vtkURI::Parse("x")->IsEmpty(), "\"x\" must not be empty");

  return true;
}

bool TestPercentEncoding()
{
  const std::string str{ "Th1s; is/ \\@ #string \xF2\t \n !" };
  Check(vtkURI::PercentDecode(vtkURI::PercentEncode(str)) == str,
    "Failed to encode/decode percent-encoded string");

  return true;
}

}

int TestURI(int, char*[])
{
  if (!TestParsing())
  {
    return EXIT_FAILURE;
  }

  if (!TestEmptyButDefinedComponents())
  {
    return EXIT_FAILURE;
  }

  if (!TestTypes())
  {
    return EXIT_FAILURE;
  }

  if (!TestPercentEncoding())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
