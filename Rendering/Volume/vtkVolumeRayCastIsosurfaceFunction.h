/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastIsosurfaceFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeRayCastIsosurfaceFunction
 * @brief   An isosurface ray caster for volumes
 *
 *
 * vtkVolumeRayCastIsosurfaceFunction is a volume ray cast function that
 * intersects a ray with an analytic isosurface in a scalar field. The color
 * and shading parameters are defined in the vtkVolumeProperty of the
 * vtkVolume, as well as the interpolation type to use when locating the
 * surface (either a nearest neighbor approach or a tri-linear interpolation
 * approach)
 *
 * @sa
 * vtkVolumeRayCastFunction vtkVolumeRayCastMapper vtkVolumeProperty
 * vtkVolumeRayCastCompositeFunction vtkVolumeRayCastMIPFunction
 * vtkVolume vtkVolumeProperty
 * @deprecated
*/

#ifndef vtkVolumeRayCastIsosurfaceFunction_h
#define vtkVolumeRayCastIsosurfaceFunction_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeRayCastFunction.h"

#if !defined(VTK_LEGACY_REMOVE)
class VTKRENDERINGVOLUME_EXPORT vtkVolumeRayCastIsosurfaceFunction : public vtkVolumeRayCastFunction
{
public:
  vtkTypeMacro(vtkVolumeRayCastIsosurfaceFunction,vtkVolumeRayCastFunction);
  void PrintSelf( ostream& os, vtkIndent indent );

  /**
   * Construct a new vtkVolumeRayCastIsosurfaceFunction
   */
  static vtkVolumeRayCastIsosurfaceFunction *New();

  /**
   * Get the scalar value below which all scalar values have 0 opacity
   */
  float GetZeroOpacityThreshold( vtkVolume *vol );

  //@{
  /**
   * Set/Get the value of IsoValue.
   */
  vtkSetMacro( IsoValue, double );
  vtkGetMacro( IsoValue, double );
  //@}


  /**
   * This is the isovalue at which to view a surface
   */
  double IsoValue;

  /**
   * These variables are filled in by SpecificFunctionInitialize
   */
  float       Color[3];

  void CastRay( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                vtkVolumeRayCastStaticInfo *staticInfo);

protected:
  vtkVolumeRayCastIsosurfaceFunction();
  ~vtkVolumeRayCastIsosurfaceFunction();

  void SpecificFunctionInitialize( vtkRenderer *ren,
                                   vtkVolume   *vol,
                                   vtkVolumeRayCastStaticInfo *staticInfo,
                                   vtkVolumeRayCastMapper *mapper );

private:
  vtkVolumeRayCastIsosurfaceFunction(const vtkVolumeRayCastIsosurfaceFunction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeRayCastIsosurfaceFunction&) VTK_DELETE_FUNCTION;
};
#endif // VTK_LEGACY_REMOVE
#endif
