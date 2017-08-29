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
 * @class   vtkmGradient
 * @brief   A general filter for gradient estimation.
 *
 * Estimates the gradient of a field in a data set.  The gradient calculation
 * is dependent on the input dataset type.  The created gradient array
 * is of the same type as the array it is calculated from (e.g. point data
 * or cell data) as well as data type (e.g. float, double). The output array has
 * 3*number of components of the input data array.  The ordering for the
 * output tuple will be {du/dx, du/dy, du/dz, dv/dx, dv/dy, dv/dz, dw/dx,
 * dw/dy, dw/dz} for an input array {u, v, w}.
 *
 * Also options to additionally compute the divergence, vorticity and
 * Q criterion of input vector fields.
 *
*/

#ifndef vtkmGradient_h
#define vtkmGradient_h

#include "vtkAcceleratorsVTKmModule.h" //required for correct implementation
#include "vtkGradientFilter.h"

class VTKACCELERATORSVTKM_EXPORT vtkmGradient : public vtkGradientFilter
{
public:
  vtkTypeMacro(vtkmGradient, vtkGradientFilter)
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkmGradient* New();

protected:
  vtkmGradient();
  ~vtkmGradient();


  virtual int RequestData(vtkInformation*, vtkInformationVector**,
                          vtkInformationVector*) VTK_OVERRIDE;

private:
  vtkmGradient(const vtkmGradient&) VTK_DELETE_FUNCTION;
  void operator=(const vtkmGradient&) VTK_DELETE_FUNCTION;
};

#endif // vtkmGradient_h
// VTK-HeaderTest-Exclude: vtkmGradient.h
