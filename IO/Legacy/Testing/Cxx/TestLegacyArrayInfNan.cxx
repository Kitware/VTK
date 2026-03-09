// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Test that reading infinity and NaN values works for floating-point arrays.

#include "vtkDataReader.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkTestErrorObserver.h"
#include "vtkTypeName.h"

#include <iostream>
#include <sstream>

namespace
{

template <typename ArrayType>
bool TestInfNanForStorage()
{
  bool ok = true;
  vtkNew<ArrayType> array;
  vtkNew<vtkDataReader> reader;
  reader->ReadFromInputStringOn();
  auto dataTypeStr = vtk::TypeName<typename ArrayType::ValueType>();
  // clang-format off
  std::cerr
    << "Testing data type " << dataTypeStr
    << ", array type " << array->GetClassName() << "\n";
  // clang-format on

  const auto inf = std::numeric_limits<typename ArrayType::ValueType>::infinity();
  const auto nan = std::numeric_limits<typename ArrayType::ValueType>::quiet_NaN();

  // Success cases (strings that should produce a valid array)
  std::vector<std::pair<std::string, std::array<typename ArrayType::ValueType, 2>>> successCases = {
    { "inf NaN\n", { inf, nan } },
    { "-inf -nan", { -inf, -nan } },
    { "5\nnan", { 5.0, nan } },
    { "nan 2.50", { nan, 2.5 } },
    { "-infinity 0", { -inf, 0.0 } },
    { "1 infinity", { 1.0, inf } },
    { "Infinity NaN", { inf, nan } },
  };
  // Failure cases (strings that should halt parsing and produce no array)
  std::vector<std::string> failureCases = {
    "\ninfnan 0", // Whitespace after inf/nan tokens required
    "infini nan", // Tokens must be full matches (inf/infinity/nan), not partial.
    "nan5",       // Whitespace after inf/nan tokens required
    "infinity1",  // Whitespace after inf/nan tokens required
    "-infinity0", // Whitespace after inf/nan tokens required
    "x 0",        // Must fail on non-numeric, non-token strings
    "nan x",      // Must fail on non-numeric, non-token strings
    "inf x",      // Must fail on non-numeric, non-token strings
    "y -inf",     // Must fail on non-numeric, non-token strings
  };
  int testNum = 0;
  std::cout << "  Testing cases expecting success\n";
  for (auto [test, expected] : successCases)
  {
    reader->SetInputString(test.c_str());
    reader->OpenVTKFile(""); // This creates the string stream to read from.
    auto arr = reader->ReadArray(dataTypeStr.c_str(), /*numTuples*/ 2, /*numComp*/ 1);
    if (!arr)
    {
      ok = false;
      std::cerr << "ERROR: Failed to parse <<" << test << ">>\n";
    }
    else
    {
      auto* data = ArrayType::SafeDownCast(arr);
      if (!data)
      {
        std::cerr << "ERROR: Invalid array type " << arr->GetClassName() << "\n";
        ok = false;
      }
      else
      {
        for (vtkIdType ii = 0; ii < static_cast<vtkIdType>(expected.size()); ++ii)
        {
          typename ArrayType::ValueType val = data->GetTuple1(ii);
          bool valuesMatch = std::isnan(val) ? std::isnan(expected[ii]) : (val == expected[ii]);
          if (!valuesMatch)
          {
            std::cerr << "ERROR: Value " << ii << " (" << data->GetTuple1(ii)
                      << ") "
                         "should be "
                      << expected[ii] << "\n";
            ok = false;
          }
        }
      }
      std::cout << "    Test " << testNum++ << ": " << (ok ? "passed" : "failed") << "\n";
      arr->Delete();
    }
  }

  std::cout << "  Testing cases expecting failure\n";
  testNum = 0;
  vtkNew<vtkTest::ErrorObserver> errorObserver;
  reader->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  for (const auto& test : failureCases)
  {
    reader->SetInputString(test.c_str());
    reader->OpenVTKFile(""); // This creates the string stream to read from.
    auto arr = reader->ReadArray(dataTypeStr.c_str(), /*numTuples*/ 2, /*numComp*/ 1);
    if (errorObserver->CheckErrorMessage(
          "Error reading ascii data. Possible mismatch of datasize with declaration."))
    {
      ok = false;
    }
    if (arr)
    {
      arr->Delete();
      ok = false;
      std::cerr << "ERROR: Parsed <<" << test << ">> to produce array.\n";
    }
    else
    {
      std::cout << "    Test " << testNum++ << ": passed\n";
    }
  }
  return ok;
}

} // anonymous namespace

int TestLegacyArrayInfNan(int, char*[])
{
  bool ok = true;
  ok &= TestInfNanForStorage<vtkDoubleArray>();
  ok &= TestInfNanForStorage<vtkFloatArray>();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
