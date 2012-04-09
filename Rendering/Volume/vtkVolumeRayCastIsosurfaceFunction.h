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
// .NAME vtkVolumeRayCastIsosurfaceFunction - An isosurface ray caster for volumes
//
// .SECTION Description
// vtkVolumeRayCastIsosurfaceFunction is a volume ray cast function that
// intersects a ray with an analytic isosurface in a scalar field. The color
// and shading parameters are defined in the vtkVolumeProperty of the 
// vtkVolume, as well as the interpolation type to use when locating the
// surface (either a nearest neighbor approach or a tri-linear interpolation
// approach)
//
// .SECTION See Also
// vtkVolumeRayCastFunction vtkVolumeRayCastMapper vtkVolumeProperty
// vtkVolumeRayCastCompositeFunction vtkVolumeRayCastMIPFunction
// vtkVolume vtkVolumeProperty

#ifndef __vtkVolumeRayCastIsosurfaceFunction_h
#define __vtkVolumeRayCastIsosurfaceFunction_h

#include "vtkVolumeRayCastFunction.h"

class VTK_VOLUMERENDERING_EXPORT vtkVolumeRayCastIsosurfaceFunction : public vtkVolumeRayCastFunction
{
public:
  vtkTypeMacro(vtkVolumeRayCastIsosurfaceFunction,vtkVolumeRayCastFunction);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Construct a new vtkVolumeRayCastIsosurfaceFunction
  static vtkVolumeRayCastIsosurfaceFunction *New();

  // Description:
  // Get the scalar value below which all scalar values have 0 opacity
  float GetZeroOpacityThreshold( vtkVolume *vol );

  // Description:
  // Set/Get the value of IsoValue.
  vtkSetMacro( IsoValue, double );
  vtkGetMacro( IsoValue, double );

  
  // Description:
  // This is the isovalue at which to view a surface
  double IsoValue;

  // Description:
  // These variables are filled in by SpecificFunctionInitialize
  float       Color[3];

//BTX
  void CastRay( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                vtkVolumeRayCastStaticInfo *staticInfo);
//ETX

protected:
  vtkVolumeRayCastIsosurfaceFunction();
  ~vtkVolumeRayCastIsosurfaceFunction();

//BTX
  void SpecificFunctionInitialize( vtkRenderer *ren,
                                   vtkVolume   *vol,
                                   vtkVolumeRayCastStaticInfo *staticInfo,
                                   vtkVolumeRayCastMapper *mapper );
//ETX
private:
  vtkVolumeRayCastIsosurfaceFunction(const vtkVolumeRayCastIsosurfaceFunction&);  // Not implemented.
  void operator=(const vtkVolumeRayCastIsosurfaceFunction&);  // Not implemented.
};
#endif
