/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSurfaceLIC.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkTestSurfaceLICDriver_h
#define __vtkTestSurfaceLICDriver_h

#include "vtkSystemIncludes.h" // include it first
#include <vector> // for vector
#include <string> // for string
class vtkDataObject;

// Description:
// Internal test driver. Used by serial and parallel
// ctests.
int vtkSurfaceLICTestDriver(
      int argc,
      char **argv,
      vtkDataObject *dataObj,
      int num_steps,
      double step_size,
      int enhanced_lic,
      int normalize_vectors,
      int camera_config,
      int generate_noise_texture,
      int noise_type,
      int noise_texture_size,
      int noise_grain_size,
      double min_noise_value,
      double max_noise_value,
      int number_of_noise_levels,
      double impulse_noise_prob,
      double impulse_noise_bg_value,
      int noise_gen_seed,
      int enhance_contrast,
      double low_lic_contrast_enhancement_factor,
      double high_lic_contrast_enhancement_factor,
      double low_color_contrast_enhancement_factor,
      double high_color_contrast_enhancement_factor,
      int anti_alias,
      int color_mode,
      double lic_intensity,
      double map_mode_bias,
      int color_by_mag,
      int mask_on_surface,
      double mask_threshold,
      double mask_intensity,
      std::vector<double> &mask_color_rgb,
      std::string &vectors);

#endif
