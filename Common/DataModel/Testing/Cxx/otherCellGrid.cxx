// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests vtkCellGrid

#include "vtkCellGrid.h"
#include "vtkLogger.h"
#include "vtkNew.h"

int otherCellGrid(int, char*[])
{
#if !defined(VTK_COMPILER_GCC) || VTK_COMPILER_GCC_VERSION > 40805
  std::string space;
  double dim;
  int hs = -42;
  // clang-format off
  std::array<std::tuple<std::string, bool, std::string, double, int, bool>, 12> tests{{
    // Space  Should Base   Exp   Half-  Test
    //         Pass               space  Encode
    // Tests that are expected to succeed:
    { "ℚ¹²⁸¹", true, "ℚ", 1281.0,  0,    true },
    { "ℝ³⁻",   true, "ℝ",    3.0, -1,    true },
    { "ℝ²⁺",   true, "ℝ",    2.0, +1,    true },
    { "ℂ⁺",    true, "ℂ",    1.0, +1,    true },
    { "B⁻",    true, "B",    1.0, -1,    true },
    { "SO⁰",   true, "SO",   0.0,  0,    true },
    { "SO⁸⁰",  true, "SO",  80.0,  0,    true },
    { "SO⁰⁸",  true, "SO",   8.0,  0,   false },
    // Tests that are expected to fail:
    { "¹²⁸¹", false, "",     0.0,  0,   false },
    { "ℚ⁺¹x", false, "",     0.0,  0,   false },
    { "ℚ⁻x",  false, "",     0.0,  0,   false },
    { "ℚ⁻¹",  false, "",     0.0,  0,   false }
  }};
  // clang-format on
  bool ok = true;
  for (const auto& test : tests)
  {
    bool shouldPass = std::get<1>(test);
    if (shouldPass)
    {
      if (!vtkCellAttribute::DecodeSpace(std::get<0>(test), space, dim, hs))
      {
        ok = false;
        vtkLog(ERROR, "Failed to parse '" << std::get<0>(test) << "'.");
      }
      std::cout << "Space <" << space << ">, dimension <" << dim << ">, restriction "
                << (hs == 0 ? "(none)" : (hs < 0 ? "negative" : "positive")) << "\n";
      if (space != std::get<2>(test))
      {
        ok = false;
        vtkLog(ERROR, "Space '" << space << "' does not match '" << std::get<2>(test) << "'.");
      }
      if (dim != std::get<3>(test))
      {
        ok = false;
        vtkLog(ERROR, "Dimension '" << dim << "' does not match '" << std::get<3>(test) << "'.");
      }
      if (hs != std::get<4>(test))
      {
        ok = false;
        vtkLog(ERROR, "Halfspace '" << hs << "' does not match '" << std::get<4>(test) << "'.");
      }

      // Also test that encoding works.
      if (std::get<5>(test))
      {
        space = vtkCellAttribute::EncodeSpace(
          std::get<2>(test), static_cast<unsigned int>(std::get<3>(test)), std::get<4>(test));
        if (space != std::get<0>(test))
        {
          ok = false;
          vtkLog(ERROR, "Encoding produced '" << space << "', not '" << std::get<0>(test) << "'.");
        }
      }
    }
    else
    {
      if (vtkCellAttribute::DecodeSpace(std::get<0>(test), space, dim, hs, true))
      {
        ok = false;
        vtkLog(ERROR, "Expected '" << std::get<0>(test) << "' to fail but it succeeded.");
      }
    }
  }
  if (!ok)
  {
    vtkLog(ERROR, "Unexpected results parsing vtkCellAttribute space.");
    return EXIT_FAILURE;
  }
#endif

  vtkNew<vtkCellGrid> cg;
  if (cg->SupportsGhostArray(vtkDataObject::POINT) || !cg->SupportsGhostArray(vtkDataObject::CELL))
  {
    vtkLog(ERROR, "Unexpected results on SupportsGhostArray");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
