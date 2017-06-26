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

#include "vtkAcceleratorsVTKmModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkmlib/ImplicitFunctionConverter.h" // For ImplicitFunctionConverter

#include <memory> // For std::shared_ptr

class vtkImplicitFunction;

class VTKACCELERATORSVTKM_EXPORT vtkmClip : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkmClip* New();
  vtkTypeMacro(vtkmClip, vtkUnstructuredGridAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * The scalar value to use when clipping the dataset. Values greater than
   * ClipValue are preserved in the output dataset. Default is 0.
   */
  vtkGetMacro(ClipValue, double)
  vtkSetMacro(ClipValue, double)

  /**
   * If true, all input point data arrays will be mapped onto the output
   * dataset. Default is true.
   */
  vtkGetMacro(ComputeScalars, bool)
  vtkSetMacro(ComputeScalars, bool)

  /**
   * Set the implicit function with which to perform the clipping. If set,
   * \c ClipValue is ignored and the clipping is performed using the implicit
   * function.
   */
  void SetClipFunction(vtkImplicitFunction *);
  vtkGetObjectMacro(ClipFunction, vtkImplicitFunction);

  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkmClip();
  ~vtkmClip();

  int RequestData(vtkInformation*, vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  double ClipValue;
  bool ComputeScalars;

  vtkImplicitFunction *ClipFunction;
  tovtkm::ImplicitFunctionConverter ClipFunctionConverter;

private:
  vtkmClip(const vtkmClip&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmClip&) VTK_DELETE_FUNCTION;
};

#endif // vtkmClip_h
// VTK-HeaderTest-Exclude: vtkmClip.h
