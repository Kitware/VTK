// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkGeoMath
 * @brief   Useful geographic calculations
 *
 *
 * vtkGeoMath provides some useful geographic calculations.
 */

#ifndef vtkGeoMath_h
#define vtkGeoMath_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISLAYOUT_EXPORT vtkGeoMath : public vtkObject
{
public:
  static vtkGeoMath* New();
  vtkTypeMacro(vtkGeoMath, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns the average radius of the earth in meters.
   */
  static double EarthRadiusMeters() { return 6356750.0; }

  /**
   * Returns the squared distance between two points.
   */
  static double DistanceSquared(double pt0[3], double pt1[3]);

  /**
   * Converts a (longitude, latitude, altitude) triple to
   * world coordinates where the center of the earth is at the origin.
   * Units are in meters.
   * Note that having altitude relative to sea level causes issues.
   */
  static void LongLatAltToRect(double longLatAlt[3], double rect[3]);

protected:
  vtkGeoMath();
  ~vtkGeoMath() override;

private:
  vtkGeoMath(const vtkGeoMath&) = delete;
  void operator=(const vtkGeoMath&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
