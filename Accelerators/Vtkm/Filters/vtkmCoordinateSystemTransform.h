// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmCoordinateSystemTransform
 * @brief   transform a coordinate system between Cartesian&Cylindrical and
 *          Cartesian&Spherical
 *
 * vtkmCoordinateSystemTransform is a filter that transforms a coordinate system
 * between Cartesian&Cylindrical and Cartesian&Spherical.
 */

#ifndef vtkmCoordinateSystemTransform_h
#define vtkmCoordinateSystemTransform_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // required for correct export
#include "vtkPointSetAlgorithm.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmCoordinateSystemTransform : public vtkPointSetAlgorithm
{
  enum struct TransformTypes
  {
    None,
    CarToCyl,
    CylToCar,
    CarToSph,
    SphToCar
  };

public:
  vtkTypeMacro(vtkmCoordinateSystemTransform, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmCoordinateSystemTransform* New();

  void SetCartesianToCylindrical();
  void SetCylindricalToCartesian();

  void SetCartesianToSpherical();
  void SetSphericalToCartesian();

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkmCoordinateSystemTransform();
  ~vtkmCoordinateSystemTransform() override;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmCoordinateSystemTransform(const vtkmCoordinateSystemTransform&) = delete;
  void operator=(const vtkmCoordinateSystemTransform&) = delete;

  TransformTypes TransformType;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmCoordinateSystemTransform_h
