//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
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

#include "vtkAcceleratorsVTKmModule.h" // required for correct export
#include "vtkPointSetAlgorithm.h"

class VTKACCELERATORSVTKM_EXPORT vtkmCoordinateSystemTransform : public vtkPointSetAlgorithm
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
};

#endif // vtkmCoordinateSystemTransform_h

// VTK-HeaderTest-Exclude: vtkmCoordinateSystemTransform.h
