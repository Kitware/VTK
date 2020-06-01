/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphericalHarmonics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSphericalHarmonics
 * @brief   compute spherical harmonics of an equirectangular projection image
 *
 * vtkSphericalHarmonics is a filter that computes spherical harmonics of an
 * equirectangular projection image representing a 360 degree image.
 * Its output is a vtkTable containing the third degree spherical harmonics coefficients.
 * This filter expects the image data to be a RGB image.
 * 8-bits images are expected to be sRGB encoded and other formats are expected to be in
 * linear color space.
 */

#ifndef vtkSphericalHarmonics_h
#define vtkSphericalHarmonics_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkSphericalHarmonics : public vtkImageAlgorithm
{
public:
  static vtkSphericalHarmonics* New();
  vtkTypeMacro(vtkSphericalHarmonics, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSphericalHarmonics() = default;
  ~vtkSphericalHarmonics() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int, vtkInformation*) override;

private:
  vtkSphericalHarmonics(const vtkSphericalHarmonics&) = delete;
  void operator=(const vtkSphericalHarmonics&) = delete;
};

#endif
