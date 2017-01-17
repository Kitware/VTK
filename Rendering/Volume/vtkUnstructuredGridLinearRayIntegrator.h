/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridLinearRayIntegrator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   vtkUnstructuredGridLinearRayIntegrator
 * @brief   performs piecewise linear ray integration.
 *
 *
 *
 * vtkUnstructuredGridLinearRayIntegrator performs piecewise linear ray
 * integration.  Considering that transfer functions in VTK are piecewise
 * linear, this class should give the "correct" integration under most
 * circumstances.  However, the computations performed are fairly hefty and
 * should, for the most part, only be used as a benchmark for other, faster
 * methods.
 *
 * @sa
 * vtkUnstructuredGridPartialPreIntegration
 *
*/

#ifndef vtkUnstructuredGridLinearRayIntegrator_h
#define vtkUnstructuredGridLinearRayIntegrator_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeRayIntegrator.h"

class vtkLinearRayIntegratorTransferFunction;
class vtkVolumeProperty;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridLinearRayIntegrator : public vtkUnstructuredGridVolumeRayIntegrator
{
public:
  vtkTypeMacro(vtkUnstructuredGridLinearRayIntegrator,
                       vtkUnstructuredGridVolumeRayIntegrator);
  static vtkUnstructuredGridLinearRayIntegrator *New();
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  void Initialize(vtkVolume *volume, vtkDataArray *scalars) VTK_OVERRIDE;

  void Integrate(vtkDoubleArray *intersectionLengths,
                         vtkDataArray *nearIntersections,
                         vtkDataArray *farIntersections,
                         float color[4]) VTK_OVERRIDE;

  //@{
  /**
   * Integrates a single ray segment.  \c color is blended with the result
   * (with \c color in front).  The result is written back into \c color.
   */
  static void IntegrateRay(double length,
                           double intensity_front, double attenuation_front,
                           double intensity_back, double attenuation_back,
                           float color[4]);
  static void IntegrateRay(double length,
                           const double color_front[3],
                           double attenuation_front,
                           const double color_back[3],
                           double attenuation_back,
                           float color[4]);
  //@}

  /**
   * Computes Psi (as defined by Moreland and Angel, "A Fast High Accuracy
   * Volume Renderer for Unstructured Data").
   */
  static float Psi(float length,
                   float attenuation_front, float attenuation_back);

protected:
  vtkUnstructuredGridLinearRayIntegrator();
  ~vtkUnstructuredGridLinearRayIntegrator() VTK_OVERRIDE;

  vtkVolumeProperty *Property;

  vtkLinearRayIntegratorTransferFunction *TransferFunctions;
  vtkTimeStamp TransferFunctionsModified;
  int NumIndependentComponents;

private:
  vtkUnstructuredGridLinearRayIntegrator(const vtkUnstructuredGridLinearRayIntegrator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridLinearRayIntegrator&) VTK_DELETE_FUNCTION;
};

#endif //vtkUnstructuredGridLinearRayIntegrator_h
