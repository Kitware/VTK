// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkColorTransferFunction.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <iostream>

bool TestSymlogScale()
{
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->SetColorSpaceToRGB();
  ctf->AddRGBPoint(-100.0, 0.0, 0.0, 1.0);
  ctf->AddRGBPoint(100.0, 1.0, 0.0, 0.0);

  ctf->SetScaleToSymlog();
  ctf->SetSymlogLinearThreshold(10.0);

  if (ctf->UsingLogScale() != 0)
  {
    std::cerr << "ERROR: UsingLogScale() should return 0 when Scale is Symlog!" << std::endl;
    return false;
  }

  double table[3 * 3];
  ctf->GetTable(-10.0, 10.0, 3, table);

  for (int i = 0; i < 9; ++i)
  {
    if (vtkMath::IsNan(table[i]))
    {
      std::cerr << "ERROR: GetTable() with symlog scale produced NaN values!" << std::endl;
      return false;
    }
  }

  const double expectedPosS = (std::log10(2.0) + std::log10(11.0)) / (2.0 * std::log10(11.0));
  const double expectedNegS = 1.0 - expectedPosS;
  if (std::abs(table[3] - 0.5) > 1e-6 || std::abs(table[5] - 0.5) > 1e-6 ||
    std::abs(table[0] - expectedNegS) > 1e-6 || std::abs(table[2] - expectedPosS) > 1e-6 ||
    std::abs(table[6] - expectedPosS) > 1e-6 || std::abs(table[8] - expectedNegS) > 1e-6)
  {
    std::cerr << "ERROR: GetTable() symlog interpolation values do not match expected values!"
              << std::endl;
    return false;
  }

  // High-dynamic-range test with multiple control points across decades (1e-7, 1e-5, 1e6)
  ctf->RemoveAllPoints();
  const double linthresh = 1e-7;
  ctf->SetSymlogLinearThreshold(linthresh);
  ctf->AddRGBPoint(-1e6, 0.0, 0.0, 1.0);
  ctf->AddRGBPoint(-1e-5, 0.0, 1.0, 1.0);
  ctf->AddRGBPoint(-1e-7, 0.0, 1.0, 0.5);
  ctf->AddRGBPoint(0.0, 0.0, 1.0, 0.0);
  ctf->AddRGBPoint(1e-7, 0.5, 1.0, 0.0);
  ctf->AddRGBPoint(1e-5, 1.0, 0.5, 0.0);
  ctf->AddRGBPoint(1e6, 1.0, 0.0, 0.0);

  // Verify colors at control points
  double rgb[3];
  ctf->GetColor(0.0, rgb);
  if (std::abs(rgb[0] - 0.0) > 1e-5 || std::abs(rgb[1] - 1.0) > 1e-5 ||
    std::abs(rgb[2] - 0.0) > 1e-5)
  {
    std::cerr << "ERROR: Symlog CTF at control point 0.0 failed!" << std::endl;
    return false;
  }

  ctf->GetColor(1e-7, rgb);
  if (std::abs(rgb[0] - 0.5) > 1e-5 || std::abs(rgb[1] - 1.0) > 1e-5 ||
    std::abs(rgb[2] - 0.0) > 1e-5)
  {
    std::cerr << "ERROR: Symlog CTF at control point 1e-7 failed!" << std::endl;
    return false;
  }

  ctf->GetColor(1e-5, rgb);
  if (std::abs(rgb[0] - 1.0) > 1e-5 || std::abs(rgb[1] - 0.5) > 1e-5 ||
    std::abs(rgb[2] - 0.0) > 1e-5)
  {
    std::cerr << "ERROR: Symlog CTF at control point 1e-5 failed!" << std::endl;
    return false;
  }

  // Verify interpolated color at intermediate decades:
  // At 1e-6 (between 1e-7 and 1e-5 in symlog space)
  const double s1e7 = std::log10(2.0);
  const double s1e6 = std::log10(11.0);
  const double s1e5 = std::log10(101.0);
  const double tPos = (s1e6 - s1e7) / (s1e5 - s1e7);
  const double expectedPosR = 0.5 + tPos * 0.5;
  const double expectedPosG = 1.0 - tPos * 0.5;

  ctf->GetColor(1e-6, rgb);
  if (std::abs(rgb[0] - expectedPosR) > 1e-5 || std::abs(rgb[1] - expectedPosG) > 1e-5 ||
    std::abs(rgb[2] - 0.0) > 1e-5)
  {
    std::cerr << "ERROR: Symlog CTF interpolation at 1e-6 failed! Expected (" << expectedPosR
              << ", " << expectedPosG << ", 0.0) got (" << rgb[0] << ", " << rgb[1] << ", "
              << rgb[2] << ")" << std::endl;
    return false;
  }

  // At -1e-6 (between -1e-7 and -1e-5 in symlog space)
  const double expectedNegB = 0.5 + tPos * 0.5;
  ctf->GetColor(-1e-6, rgb);
  if (std::abs(rgb[0] - 0.0) > 1e-5 || std::abs(rgb[1] - 1.0) > 1e-5 ||
    std::abs(rgb[2] - expectedNegB) > 1e-5)
  {
    std::cerr << "ERROR: Symlog CTF interpolation at -1e-6 failed! Expected (0.0, 1.0, "
              << expectedNegB << ") got (" << rgb[0] << ", " << rgb[1] << ", " << rgb[2] << ")"
              << std::endl;
    return false;
  }

  // At 5e-8 (inside linear region between 0.0 and 1e-7)
  const double s5e8 = std::log10(1.5);
  const double tLinear = s5e8 / s1e7;
  const double expectedLinR = 0.5 * tLinear;

  ctf->GetColor(5e-8, rgb);
  if (std::abs(rgb[0] - expectedLinR) > 1e-5 || std::abs(rgb[1] - 1.0) > 1e-5 ||
    std::abs(rgb[2] - 0.0) > 1e-5)
  {
    std::cerr << "ERROR: Symlog CTF linear region interpolation at 5e-8 failed! Expected ("
              << expectedLinR << ", 1.0, 0.0) got (" << rgb[0] << ", " << rgb[1] << ", " << rgb[2]
              << ")" << std::endl;
    return false;
  }

  // Verify threshold clamping to VTK_DBL_MIN for non-positive values
  ctf->SetSymlogLinearThreshold(1e-16);
  if (ctf->GetSymlogLinearThreshold() != 1e-16)
  {
    std::cerr << "ERROR: SymlogLinearThreshold failed to accept 1e-16!" << std::endl;
    return false;
  }

  ctf->SetSymlogLinearThreshold(0.0);
  if (ctf->GetSymlogLinearThreshold() != VTK_DBL_MIN)
  {
    std::cerr << "ERROR: SymlogLinearThreshold failed to clamp 0.0 to VTK_DBL_MIN!" << std::endl;
    return false;
  }

  ctf->SetSymlogLinearThreshold(-1.0);
  if (ctf->GetSymlogLinearThreshold() != VTK_DBL_MIN)
  {
    std::cerr << "ERROR: SymlogLinearThreshold failed to clamp -1.0 to VTK_DBL_MIN!" << std::endl;
    return false;
  }

  return true;
}

bool TestLogScaleNaNPropagation()
{
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->SetColorSpaceToRGB();
  ctf->AddRGBPoint(0.05, 0.0, 0.0, 1.0);
  ctf->AddRGBPoint(500.0, 1.0, 0.0, 0.0);

  ctf->SetScaleToLog10();

  // Test GetTable with a negative range. Ensure that evaluating negative ranges in log scale yields
  // valid colors rather than NaN.
  double table[3 * 10];
  ctf->GetTable(-500.0, -100.0, 10, table);

  for (int i = 0; i < 30; ++i)
  {
    if (vtkMath::IsNan(table[i]))
    {
      std::cerr << "ERROR: GetTable() with negative range in Log10 scale produced NaN values!"
                << std::endl;
      return false;
    }
  }

  return true;
}

bool TestLogScale()
{
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->SetColorSpaceToRGB();
  ctf->AddRGBPoint(0.05, 0.0, 0.0, 1.0);
  ctf->AddRGBPoint(500.0, 1.0, 0.0, 0.0);

  ctf->SetScaleToLog10();
  ctf->SetUseBelowRangeColor(true);
  ctf->SetBelowRangeColor(1.0, 1.0, 0.0);

  if (ctf->UsingLogScale() != 1)
  {
    std::cerr << "ERROR: UsingLogScale() should return 1 when Scale is Log10 and Range[0] > 0.0!"
              << std::endl;
    return false;
  }

  double color[3];
  ctf->GetColor(-505.0, color);

  if (color[0] != 1.0 || color[1] != 1.0 || color[2] != 0.0)
  {
    std::cerr << "ERROR: Log scale below range mapping failed! Expected (1, 1, 0) got (" << color[0]
              << ", " << color[1] << ", " << color[2] << ")" << std::endl;
    return false;
  }

  return true;
}

bool TestColorSpace()
{
  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  ctf->AddRGBPoint(1.0, 0.0, 0.0, 1.0);
  const unsigned char* rgba = ctf->MapValue(0.5);
  if (rgba[0] != 128 || rgba[1] != 0 || rgba[2] != 128)
  {
    std::cerr << "ERROR: ColorSpace == VTK_CTF_RGB failed!" << std::endl;
    return false;
  }

  ctf->SetColorSpaceToLabCIEDE2000();
  rgba = ctf->MapValue(0.5);
  if (rgba[0] != 196 || rgba[1] != 16 || rgba[2] != 123)
  {
    std::cerr << "ERROR: ColorSpace == VTK_CTF_LAB_CIEDE2000 failed!" << std::endl;
    return false;
  }

  ctf->SetColorSpaceToStep();
  rgba = ctf->MapValue(0.5);
  if (rgba[0] != 0 || rgba[1] != 0 || rgba[2] != 255)
  {
    std::cerr << "ERROR: ColorSpace == VTK_CTF_STEP failed!" << std::endl;
    return false;
  }

  ctf->SetColorSpaceToProlab();
  rgba = ctf->MapValue(0.5);
  if (rgba[0] != 199 || rgba[1] != 0 || rgba[2] != 175)
  {
    std::cerr << "ERROR: ColorSpace == VTK_CTF_LAB_Prolab failed!" << std::endl;
    return false;
  }

  return true;
}

int TestColorTransferFunction(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkColorTransferFunction> ctf;

  // Test for crash when getting table with no points
  ctf->RemoveAllPoints();

  // Range should be [0, 0]. Honest to goodness 0, not just very close to 0.
  double range[2];
  ctf->GetRange(range);

  if (range[0] != 0.0 || range[1] != 0.0)
  {
    std::cerr << "After RemoveAllPoints() is called, range should be [0, 0]. "
              << "It was [" << range[0] << ", " << range[1] << "].\n";
    return EXIT_FAILURE;
  }

  double table[256 * 3];
  ctf->GetTable(0.0, 1.0, 256, table);

  // Table should be all black.
  for (int i = 0; i < 3 * 256; ++i)
  {
    if (table[i] != 0.0)
    {
      std::cerr << "Table should have all zeros.\n";
      return EXIT_FAILURE;
    }
  }

  ctf->SetNanColorRGBA(1., 1., 1., 0.5);
  ctf->GetTable(vtkMath::Nan(), 1., 256, table);
  // Table should be all white (Nan color).
  for (int i = 0; i < 3 * 256; ++i)
  {
    if (table[i] != 1.0)
    {
      std::cerr << "Table should have all ones.\n";
      return EXIT_FAILURE;
    }
  }

  ctf->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
  ctf->AddRGBPoint(1.0, 1.0, 1.0, 1.0);
  double color[4] = { -1., -1., -1., -1. };
  ctf->GetIndexedColor(0, color);
  for (int i = 0; i < 3; ++i)
  {
    if (color[i] != 0.0)
    {
      std::cerr << "Color should have all zeros.\n";
      return EXIT_FAILURE;
    }
  }
  if (color[3] != 1.0)
  {
    std::cerr << "Opacity should be 1.\n";
    return EXIT_FAILURE;
  }

  ctf->GetIndexedColor(-1, color);
  for (int i = 0; i < 3; ++i)
  {
    if (color[i] != 1.0)
    {
      std::cerr << "Nan Color should have all ones.\n";
      return EXIT_FAILURE;
    }
  }
  if (color[3] != 0.5)
  {
    std::cerr << "Nan Color opacity should be 0.5.\n";
    return EXIT_FAILURE;
  }

  if (!TestSymlogScale())
  {
    return EXIT_FAILURE;
  }

  if (!TestLogScaleNaNPropagation())
  {
    return EXIT_FAILURE;
  }

  if (!TestLogScale())
  {
    return EXIT_FAILURE;
  }

  if (!TestColorSpace())
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
