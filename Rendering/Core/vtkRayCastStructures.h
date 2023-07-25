// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkRayCastStructures
 * @brief   the structure definitions for ray casting
 *
 *
 * These are the structures required for ray casting.
 *
 * @sa
 * vtkRayCaster
 */

#ifndef vtkRayCastStructures_h
#define vtkRayCastStructures_h

#include "vtkABINamespace.h"

VTK_ABI_NAMESPACE_BEGIN
struct vtkRayCastRayInfo_t
{
  // These are the input values that define the ray. Depending on
  // whether we are casting a WorldRay or a ViewRay, these are in
  // world coordinates or view coordinates.
  float Origin[3];
  float Direction[3];

  // The pixel location for the ray that is being cast can be
  // important, for example if hardware ray bounding is being used
  // and the location in the depth buffer must be matched to this
  // ray.
  int Pixel[2];

  // The world coordinate location of the camera is important for the
  // ray caster to be able to return a Z value for the intersection
  float CameraPosition[3];

  // This input value defines the size of the image
  int ImageSize[2];

  // These are input values for clipping but may be changed
  // along the way
  float NearClip;
  float FarClip;

  // These are the return values - RGBA and Z
  float Color[4];
  float Depth;

  // Some additional space that may be useful for the
  // specific implementation of the ray caster. This structure
  // is a convenient place to put it, since there is one
  // per thread so that writing to these locations is safe

  // Ray information transformed into local coordinates
  float TransformedStart[4];
  float TransformedEnd[4];
  float TransformedDirection[4];
  float TransformedIncrement[3];

  // The number of steps we want to take if this is
  // a ray caster that takes steps
  int NumberOfStepsToTake;

  // The number of steps we actually take if this is
  // a ray caster that takes steps
  int NumberOfStepsTaken;
};
using vtkRayCastRayInfo = struct vtkRayCastRayInfo_t;

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkRayCastStructures.h
