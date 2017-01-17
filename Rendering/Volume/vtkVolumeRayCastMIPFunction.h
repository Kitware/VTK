/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMIPFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeRayCastMIPFunction
 * @brief   A maximum intensity projection ray caster for volumes
 *
 *
 * vtkVolumeRayCastMIPFunction is a volume ray cast function that
 * computes the maximum value encountered along the ray. This is
 * either the maximum scalar value, or the maximum opacity, as
 * defined by the MaximizeMethod. The color and opacity returned
 * by this function is based on the color, scalar opacity, and
 * gradient opacity transfer functions defined in the vtkVolumeProperty
 * of the vtkVolume.
 *
 * @sa
 * vtkVolumeRayCastFunction vtkVolumeRayCastMapper vtkVolumeProperty
 * vtkVolumeRayCastCompositeFunction vtkVolumeRayCastIsosurfaceFunction
 * vtkVolume vtkVolumeProperty
 * @deprecated
*/

#ifndef vtkVolumeRayCastMIPFunction_h
#define vtkVolumeRayCastMIPFunction_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeRayCastFunction.h"

#if !defined(VTK_LEGACY_REMOVE)

#define VTK_MAXIMIZE_SCALAR_VALUE 0
#define VTK_MAXIMIZE_OPACITY      1

class VTKRENDERINGVOLUME_EXPORT vtkVolumeRayCastMIPFunction : public vtkVolumeRayCastFunction
{
public:
  static vtkVolumeRayCastMIPFunction *New();
  vtkTypeMacro(vtkVolumeRayCastMIPFunction,vtkVolumeRayCastFunction);
  void PrintSelf( ostream& os, vtkIndent indent ) VTK_OVERRIDE;


  /**
   * Get the scalar value below which all scalar values have zero opacity.
   */
  float GetZeroOpacityThreshold( vtkVolume *vol ) VTK_OVERRIDE;


  //@{
  /**
   * Set the MaximizeMethod to either ScalarValue or Opacity.
   */
  vtkSetClampMacro( MaximizeMethod, int,
        VTK_MAXIMIZE_SCALAR_VALUE, VTK_MAXIMIZE_OPACITY );
  vtkGetMacro(MaximizeMethod,int);
  void SetMaximizeMethodToScalarValue()
    {this->SetMaximizeMethod(VTK_MAXIMIZE_SCALAR_VALUE);}
  void SetMaximizeMethodToOpacity()
    {this->SetMaximizeMethod(VTK_MAXIMIZE_OPACITY);}
  const char *GetMaximizeMethodAsString(void);
  //@}

  void CastRay( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                vtkVolumeRayCastStaticInfo *staticInfo ) VTK_OVERRIDE;

protected:
  vtkVolumeRayCastMIPFunction();
  ~vtkVolumeRayCastMIPFunction() VTK_OVERRIDE;

  int MaximizeMethod;

  void SpecificFunctionInitialize( vtkRenderer *ren,
                                   vtkVolume   *vol,
                                   vtkVolumeRayCastStaticInfo *staticInfo,
                                   vtkVolumeRayCastMapper *mapper ) VTK_OVERRIDE;

private:
  vtkVolumeRayCastMIPFunction(const vtkVolumeRayCastMIPFunction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeRayCastMIPFunction&) VTK_DELETE_FUNCTION;
};


#endif // VTK_LEGACY_REMOVE
#endif
