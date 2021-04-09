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
 * @class vtkmClip
 * @brief Clip a dataset using the accelerated vtk-m Clip filter.
 *
 * Clip a dataset using either a given value or by using an vtkImplicitFunction
 * Currently the supported implicit functions are Box, Plane, and Sphere.
 *
 */

#ifndef vtkmClip_h
#define vtkmClip_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

#include <memory> // For std::unique_ptr

class vtkImplicitFunction;

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmClip : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkmClip* New();
  vtkTypeMacro(vtkmClip, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The scalar value to use when clipping the dataset. Values greater than
   * ClipValue are preserved in the output dataset. Default is 0.
   */
  double GetClipValue();
  void SetClipValue(double);

  /**
   * If true, all input point data arrays will be mapped onto the output
   * dataset. Default is true.
   */
  bool GetComputeScalars();
  void SetComputeScalars(bool);

  /**
   * Set the implicit function with which to perform the clipping. If set,
   * \c ClipValue is ignored and the clipping is performed using the implicit
   * function.
   */
  void SetClipFunction(vtkImplicitFunction*);
  vtkImplicitFunction* GetClipFunction();

  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * When this flag is off (the default), then the computation will fall back
   * to the serial VTK version if VTK-m fails to run. When the flag is on,
   * the filter will generate an error if VTK-m fails to run. This is mostly
   * useful in testing to make sure the expected algorithm is run.
   */
  vtkGetMacro(ForceVTKm, vtkTypeBool);
  vtkSetMacro(ForceVTKm, vtkTypeBool);
  vtkBooleanMacro(ForceVTKm, vtkTypeBool);
  ///@}

protected:
  vtkmClip();
  ~vtkmClip() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkTypeBool ForceVTKm = false;

  struct internals;
  std::unique_ptr<internals> Internals;

private:
  vtkmClip(const vtkmClip&) = delete;
  void operator=(const vtkmClip&) = delete;
};

#endif // vtkmClip_h
