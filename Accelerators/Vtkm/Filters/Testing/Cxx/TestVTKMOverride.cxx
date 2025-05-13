// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include <vtkmContour.h>
#include <vtkmFilterOverrides.h>

#include <vtkNew.h>

#include <string>

namespace
{

template <typename BaseClass>
bool TestOverride(const std::string& baseClassName, const std::string& overrideClassName)
{
  std::cout << "Using object factory to instantiate " << baseClassName << "\n";
  vtkNew<BaseClass> instance;
  std::cout << "instantiated: " << instance->GetClassName() << "\n";

  return instance->GetClassName() ==
    (vtkmFilterOverrides::GetEnabled() ? overrideClassName : baseClassName);
}

#define TEST_OVERRIDE(BASE, OVERRIDE)                                                              \
  if (!TestOverride<BASE>(#BASE, #OVERRIDE))                                                       \
  {                                                                                                \
    return false;                                                                                  \
  }

bool TestOverrides()
{
  std::cout << "Testing with vtkmFilterOverrides::GetEnabled() = "
            << vtkmFilterOverrides::GetEnabled() << "\n";

  TEST_OVERRIDE(vtkContourFilter, vtkmContour)

  return true;
}

} // namespace

int TestVTKMOverride(int, char*[])
{
  // When VTK_ENABLE_VISKORES_OVERRIDES is off, vtkmFilterOverrides::EnabledOn() should not have any
  // effects
  std::cout << "Build option VTK_ENABLE_VISKORES_OVERRIDES: " << VTK_ENABLE_VISKORES_OVERRIDES
            << "\n";

  vtkmFilterOverrides::EnabledOn();
  if (!VTK_ENABLE_VISKORES_OVERRIDES && vtkmFilterOverrides::GetEnabled())
  {
    std::cerr << "vtkmFilterOverrides::GetEnabled() should always be false when "
              << "VTK_ENABLE_VISKORES_OVERRIDES is off";
    return 1;
  }
  if (!TestOverrides())
  {
    return 1;
  }

#if VTK_ENABLE_VISKORES_OVERRIDES
  // Only makes sense when VTK_ENABLE_VISKORES_OVERRIDES is On.
  vtkmFilterOverrides::EnabledOff();
  if (!TestOverrides())
  {
    return 1;
  }
#endif

  return 0;
}
