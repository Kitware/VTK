// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef GLTFSampler_h
#define GLTFSampler_h

#include "vtkIOGeometryModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

/**
 * This struct describes a glTF sampler object.
 * Samplers specify filter and wrapping options corresponding to GL types.
 */
struct VTKIOGEOMETRY_EXPORT GLTFSampler
{
  enum FilterType : unsigned short
  {
    NEAREST = 9728,
    LINEAR = 9729,
    NEAREST_MIPMAP_NEAREST = 9984,
    LINEAR_MIPMAP_NEAREST = 9985,
    NEAREST_MIPMAP_LINEAR = 9986,
    LINEAR_MIPMAP_LINEAR = 9987
  };
  enum WrapType : unsigned short
  {
    CLAMP_TO_EDGE = 33071,
    MIRRORED_REPEAT = 33648,
    REPEAT = 10497
  };
  FilterType MagFilter;
  FilterType MinFilter;
  WrapType WrapS;
  WrapType WrapT;
};

VTK_ABI_NAMESPACE_END
#endif
