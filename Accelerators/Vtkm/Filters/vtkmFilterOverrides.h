// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkmFilterOverrides_h
#define vtkmFilterOverrides_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation

/// A class with static methods to turn viskores filter overrides on/off at runtime
///
VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmFilterOverrides
{
public:
  ///@{
  /**
   * Runtime enable/disable for Viskores filter overrides using the object factory.
   * This only has effect if the cmake option `VTK_ENABLE_VISKORES_OVERRIDES` is set.
   * This also only affects filters that have a Viskores override.
   * Disabled by default.
   */
  static void SetEnabled(bool value);
  static bool GetEnabled();
  static void EnabledOn() { vtkmFilterOverrides::SetEnabled(true); }
  static void EnabledOff() { vtkmFilterOverrides::SetEnabled(false); }
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif // vtkmFilterOverrides_h
// VTK-HeaderTest-Exclude: vtkmFilterOverrides.h
