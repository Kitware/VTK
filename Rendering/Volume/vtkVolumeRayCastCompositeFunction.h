/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastCompositeFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeRayCastCompositeFunction
 * @brief   a ray function for compositing
 *
 *
 * vtkVolumeRayCastCompositeFunction is a ray function that can be used
 * within a vtkVolumeRayCastMapper. This function performs compositing along
 * the ray according to the properties stored in the vtkVolumeProperty for
 * the volume.
 *
 * @sa
 * vtkVolumeRayCastMapper vtkVolumeProperty vtkVolume
 * @deprecated
*/

#ifndef vtkVolumeRayCastCompositeFunction_h
#define vtkVolumeRayCastCompositeFunction_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeRayCastFunction.h"

#if !defined(VTK_LEGACY_REMOVE)
#define VTK_COMPOSITE_CLASSIFY_FIRST 0
#define VTK_COMPOSITE_INTERPOLATE_FIRST 1

class VTKRENDERINGVOLUME_EXPORT vtkVolumeRayCastCompositeFunction : public vtkVolumeRayCastFunction
{
public:
  static vtkVolumeRayCastCompositeFunction *New();
  vtkTypeMacro(vtkVolumeRayCastCompositeFunction,vtkVolumeRayCastFunction);
  void PrintSelf( ostream& os, vtkIndent indent );

  //@{
  /**
   * Set the CompositeMethod to either Classify First or Interpolate First
   */
  vtkSetClampMacro( CompositeMethod, int,
        VTK_COMPOSITE_CLASSIFY_FIRST, VTK_COMPOSITE_INTERPOLATE_FIRST );
  vtkGetMacro(CompositeMethod,int);
  void SetCompositeMethodToInterpolateFirst()
    {this->SetCompositeMethod(VTK_COMPOSITE_INTERPOLATE_FIRST);}
  void SetCompositeMethodToClassifyFirst()
    {this->SetCompositeMethod(VTK_COMPOSITE_CLASSIFY_FIRST);}
  const char *GetCompositeMethodAsString(void);
  //@}

  void CastRay( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                vtkVolumeRayCastStaticInfo *staticInfo);

  float GetZeroOpacityThreshold( vtkVolume *vol );

protected:
  vtkVolumeRayCastCompositeFunction();
  ~vtkVolumeRayCastCompositeFunction();

  void SpecificFunctionInitialize( vtkRenderer *ren,
                                   vtkVolume   *vol,
                                   vtkVolumeRayCastStaticInfo *staticInfo,
                                   vtkVolumeRayCastMapper *mapper );

  int           CompositeMethod;
private:
  vtkVolumeRayCastCompositeFunction(const vtkVolumeRayCastCompositeFunction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVolumeRayCastCompositeFunction&) VTK_DELETE_FUNCTION;
};


#endif // VTK_LEGACY_REMOVE
#endif
