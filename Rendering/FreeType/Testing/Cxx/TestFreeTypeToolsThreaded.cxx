// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Regression test for the per-thread FreeType instances in vtkFreeTypeTools
// (see issue #15376). Before that change, the FT_Library and the FreeType
// cache objects were shared between threads, so rendering text concurrently
// raced inside FreeType and crashed or produced corrupt glyphs.

#include "vtkFreeTypeTools.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkTextProperty.h"

#include <atomic>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{

constexpr int NumThreads = 8;
constexpr int NumIterations = 25;
constexpr int DPI = 96;

const std::vector<std::string>& TestStrings()
{
  static const std::vector<std::string> strings = { "Thread safety", "VTK FreeType", "0123456789",
    "The quick brown fox", "AVWA kerning", "Multi\nline\ntext" };
  return strings;
}

// Configure a property deterministically from a seed so that every thread
// exercises a different set of faces, sizes and orientations.
void ConfigureProperty(vtkTextProperty* tprop, int seed)
{
  static const char* const families[] = { "Arial", "Courier", "Times" };
  tprop->SetFontFamilyAsString(families[seed % 3]);
  tprop->SetFontSize(10 + (seed % 17));
  tprop->SetBold(seed % 2 == 0);
  tprop->SetItalic(seed % 5 == 0);
  tprop->SetOrientation((seed % 4) * 15.0);
}

struct Result
{
  int bbox[4] = { 0, 0, 0, 0 };
  int dims[3] = { 0, 0, 0 };
  // Sum of the rendered alpha values; a cheap way to notice corrupt glyphs.
  vtkTypeUInt64 checksum = 0;
};

bool Render(int seed, const std::string& str, Result& result)
{
  vtkFreeTypeTools* tools = vtkFreeTypeTools::GetInstance();

  vtkNew<vtkTextProperty> tprop;
  ConfigureProperty(tprop, seed);

  if (!tools->GetBoundingBox(tprop, str, DPI, result.bbox))
  {
    std::cerr << "GetBoundingBox failed for \"" << str << "\"\n";
    return false;
  }

  vtkNew<vtkImageData> image;
  if (!tools->RenderString(tprop, str, DPI, image))
  {
    std::cerr << "RenderString failed for \"" << str << "\"\n";
    return false;
  }

  image->GetDimensions(result.dims);
  const vtkIdType numTuples = image->GetNumberOfPoints();
  auto* scalars = static_cast<unsigned char*>(image->GetScalarPointer());
  if (!scalars)
  {
    std::cerr << "RenderString produced no scalars for \"" << str << "\"\n";
    return false;
  }
  const int numComponents = image->GetNumberOfScalarComponents();
  for (vtkIdType i = 0; i < numTuples; ++i)
  {
    // The alpha channel is the last component.
    result.checksum += scalars[i * numComponents + (numComponents - 1)];
  }
  return true;
}

// Body of each worker thread: repeat the baseline renders and compare.
void RunThread(int seed, const std::vector<Result>& baseline, std::atomic<int>& failures);

bool Matches(const Result& expected, const Result& actual, int seed, const std::string& str)
{
  for (int i = 0; i < 4; ++i)
  {
    if (expected.bbox[i] != actual.bbox[i])
    {
      std::cerr << "Bounding box mismatch (seed " << seed << ", \"" << str << "\"): expected ["
                << expected.bbox[0] << ", " << expected.bbox[1] << ", " << expected.bbox[2] << ", "
                << expected.bbox[3] << "] got [" << actual.bbox[0] << ", " << actual.bbox[1] << ", "
                << actual.bbox[2] << ", " << actual.bbox[3] << "]\n";
      return false;
    }
  }
  for (int i = 0; i < 3; ++i)
  {
    if (expected.dims[i] != actual.dims[i])
    {
      std::cerr << "Image dimension mismatch (seed " << seed << ", \"" << str << "\")\n";
      return false;
    }
  }
  if (expected.checksum != actual.checksum)
  {
    std::cerr << "Rendered image mismatch (seed " << seed << ", \"" << str
              << "\"): expected checksum " << expected.checksum << " got " << actual.checksum
              << '\n';
    return false;
  }
  return true;
}

void RunThread(int seed, const std::vector<Result>& baseline, std::atomic<int>& failures)
{
  const std::vector<std::string>& strings = TestStrings();
  for (int iteration = 0; iteration < NumIterations; ++iteration)
  {
    for (size_t s = 0; s < strings.size(); ++s)
    {
      Result actual;
      if (!Render(seed, strings[s], actual) || !Matches(baseline[s], actual, seed, strings[s]))
      {
        ++failures;
        return;
      }
    }
  }
}

}

//------------------------------------------------------------------------------
int TestFreeTypeToolsThreaded(int, char*[])
{
  const std::vector<std::string>& strings = TestStrings();

  // Single-threaded baseline. Computing it first also means the singleton and
  // the main thread's FreeType instances already exist when the threads start,
  // which is the configuration that used to race.
  std::vector<std::vector<Result>> baseline(NumThreads);
  for (int t = 0; t < NumThreads; ++t)
  {
    baseline[t].resize(strings.size());
    for (size_t s = 0; s < strings.size(); ++s)
    {
      if (!Render(t, strings[s], baseline[t][s]))
      {
        std::cerr << "Failed to establish the single-threaded baseline.\n";
        return EXIT_FAILURE;
      }
    }
  }

  // Now hammer the same calls from several threads at once.
  std::atomic<int> failures{ 0 };
  std::vector<std::thread> threads;
  threads.reserve(NumThreads);
  for (int t = 0; t < NumThreads; ++t)
  {
    threads.emplace_back(RunThread, t, std::cref(baseline[t]), std::ref(failures));
  }
  for (std::thread& thread : threads)
  {
    thread.join();
  }

  if (failures > 0)
  {
    std::cerr << failures << " thread(s) reported a failure.\n";
    return EXIT_FAILURE;
  }

  std::cout << "Rendered " << (NumThreads * NumIterations * strings.size()) << " strings across "
            << NumThreads << " threads with matching results.\n";
  return EXIT_SUCCESS;
}
