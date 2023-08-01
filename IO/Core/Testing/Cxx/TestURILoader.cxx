// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkURILoader.h"

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

bool Resolve(vtkURILoader* loader, const std::string& input, const std::string& expected)
{
  auto parsed = vtkURI::Parse(input);
  Check(parsed, "Failed to parse uri \"" + input + "\"");
  auto resolved = loader->Resolve(parsed);
  Check(resolved, "Failed to resolve \"" + input + "\"");

  const auto resolvedStr = resolved->ToString();
  Check(resolvedStr == expected,
    "Resolved URI (" + resolvedStr + ", resolved from \"" + input +
      "\") does not match expected URI(" + expected + ")");

  return true;
}

bool TestResolution()
{
  vtkNew<vtkURILoader> loader;

  // Basic tests: https://datatracker.ietf.org/doc/html/rfc3986#section-5.4.1
  loader->SetBaseURI("http://a/b/c/d;p?q");
  Check(Resolve(loader, "g:h", "g:h"), "Resolution failed");
  Check(Resolve(loader, "g", "http://a/b/c/g"), "Resolution failed");
  Check(Resolve(loader, "./g", "http://a/b/c/g"), "Resolution failed");
  Check(Resolve(loader, "g/", "http://a/b/c/g/"), "Resolution failed");
  Check(Resolve(loader, "/g", "http://a/g"), "Resolution failed");
  Check(Resolve(loader, "//g", "http://g"), "Resolution failed");
  Check(Resolve(loader, "?y", "http://a/b/c/d;p?y"), "Resolution failed");
  Check(Resolve(loader, "g?y", "http://a/b/c/g?y"), "Resolution failed");
  Check(Resolve(loader, "#s", "http://a/b/c/d;p?q#s"), "Resolution failed");
  Check(Resolve(loader, "g#s", "http://a/b/c/g#s"), "Resolution failed");
  Check(Resolve(loader, "g?y#s", "http://a/b/c/g?y#s"), "Resolution failed");
  Check(Resolve(loader, ";x", "http://a/b/c/;x"), "Resolution failed");
  Check(Resolve(loader, "g;x", "http://a/b/c/g;x"), "Resolution failed");
  Check(Resolve(loader, "g;x?y#s", "http://a/b/c/g;x?y#s"), "Resolution failed");
  Check(Resolve(loader, "", "http://a/b/c/d;p?q"), "Resolution failed");
  Check(Resolve(loader, ".", "http://a/b/c/"), "Resolution failed");
  Check(Resolve(loader, "./", "http://a/b/c/"), "Resolution failed");
  Check(Resolve(loader, "..", "http://a/b/"), "Resolution failed");
  Check(Resolve(loader, "../", "http://a/b/"), "Resolution failed");
  Check(Resolve(loader, "../g", "http://a/b/g"), "Resolution failed");
  Check(Resolve(loader, "../..", "http://a/"), "Resolution failed");
  Check(Resolve(loader, "../../", "http://a/"), "Resolution failed");
  Check(Resolve(loader, "../../g", "http://a/g"), "Resolution failed");

  // "Abnormal" tests: https://datatracker.ietf.org/doc/html/rfc3986#section-5.4.2
  Check(Resolve(loader, "/./g", "http://a/g"), "Resolution failed");
  Check(Resolve(loader, "/../g", "http://a/g"), "Resolution failed");
  Check(Resolve(loader, "g.", "http://a/b/c/g."), "Resolution failed");
  Check(Resolve(loader, ".g", "http://a/b/c/.g"), "Resolution failed");
  Check(Resolve(loader, "g..", "http://a/b/c/g.."), "Resolution failed");
  Check(Resolve(loader, "..g", "http://a/b/c/..g"), "Resolution failed");
  Check(Resolve(loader, "./../g", "http://a/b/g"), "Resolution failed");
  Check(Resolve(loader, "./g/.", "http://a/b/c/g/"), "Resolution failed");
  Check(Resolve(loader, "g/./h", "http://a/b/c/g/h"), "Resolution failed");
  Check(Resolve(loader, "g/../h", "http://a/b/c/h"), "Resolution failed");
  Check(Resolve(loader, "g;x=1/./y", "http://a/b/c/g;x=1/y"), "Resolution failed");
  Check(Resolve(loader, "g;x=1/../y", "http://a/b/c/y"), "Resolution failed");
  Check(Resolve(loader, "g?y/./x", "http://a/b/c/g?y/./x"), "Resolution failed");
  Check(Resolve(loader, "g?y/../x", "http://a/b/c/g?y/../x"), "Resolution failed");
  Check(Resolve(loader, "g#s/./x", "http://a/b/c/g#s/./x"), "Resolution failed");
  Check(Resolve(loader, "g#s/../x", "http://a/b/c/g#s/../x"), "Resolution failed");
  // "http:g" can be resolved to "http:g" or "http://a/b/c/g"
  // "http:g" is what a strict parser should output.
  Check(Resolve(loader, "http:g", "http:g"), "Resolution failed");

  return true;
}

bool TestFileLoading(const std::string& tmpDir)
{
  std::ofstream{ tmpDir + "/URI Loader Tmp File.txt" } << "Hello world!";

  {
    vtkNew<vtkURILoader> loader;
    Check(loader->SetBaseDirectory(tmpDir), "Can not set base directory");

    // Percent-encoded to have a valid URI
    auto stream = loader->Load("URI%20Loader%20Tmp%20File.txt");
    Check(stream, "Could not load file URI");

    std::string text;
    text.resize(12);
    Check(stream->Read(&text[0], text.size()) == text.size(), "Truncated stream");
    Check(text == "Hello world!", "Wrong data");
  }

  // same as previous but with SetBaseFileName
  {
    std::ofstream{ tmpDir + "/URI Loader Tmp Ref File.txt" }; // create file

    vtkNew<vtkURILoader> loader;
    Check(loader->SetBaseFileName(tmpDir + "/URI Loader Tmp Ref File.txt"),
      "Can not set base directory");

    // Percent-encoded to have a valid URI
    auto stream = loader->Load("URI%20Loader%20Tmp%20File.txt");
    Check(stream, "Could not load file URI");

    std::string text;
    text.resize(12);
    Check(stream->Read(&text[0], text.size()) == text.size(), "Truncated stream");
    Check(text == "Hello world!", "Wrong data");
  }

  return true;
}

bool TestBase64DataLoading()
{
  vtkNew<vtkURILoader> loader;
  auto stream = loader->Load("data:;base64,SGVsbG8gd29ybGQh");
  Check(stream, "Could not load data URI");

  std::string text;
  text.resize(12);
  Check(stream->Read(&text[0], text.size()) == text.size(), "Truncated stream");
  Check(text == "Hello world!", "Wrong data");

  return true;
}

bool TestRawDataLoading()
{
  vtkNew<vtkURILoader> loader;
  auto stream = loader->Load("data:,%00%40%12hello");
  Check(stream, "Could not load data URI");

  std::vector<char> data;
  data.resize(8);
  Check(stream->Read(data.data(), data.size()) == data.size(), "Truncated stream");
  Check((data == std::vector<char>{ 0x00, 0x40, 0x12, 'h', 'e', 'l', 'l', 'o' }), "Wrong data");

  return true;
}

}

int TestURILoader(int argc, char* argv[])
{
  if (!TestResolution())
  {
    return EXIT_FAILURE;
  }

  char* tempDir =
    vtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "VTK_TEMP_DIR", "Testing/Temporary");
  if (!TestFileLoading(tempDir))
  {
    return EXIT_FAILURE;
  }
  delete[] tempDir;

  if (!TestBase64DataLoading())
  {
    return EXIT_FAILURE;
  }

  if (!TestRawDataLoading())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
