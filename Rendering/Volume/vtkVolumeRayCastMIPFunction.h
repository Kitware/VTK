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
// .NAME vtkVolumeRayCastMIPFunction - A maximum intensity projection ray caster for volumes
//
// .SECTION Description
// vtkVolumeRayCastMIPFunction is a volume ray cast function that
// computes the maximum value encountered along the ray. This is
// either the maximum scalar value, or the maximum opacity, as
// defined by the MaximizeMethod. The color and opacity returned
// by this function is based on the color, scalar opacity, and
// gradient opacity transfer functions defined in the vtkVolumeProperty
// of the vtkVolume.
//
// .SECTION See Also
// vtkVolumeRayCastFunction vtkVolumeRayCastMapper vtkVolumeProperty
// vtkVolumeRayCastCompositeFunction vtkVolumeRayCastIsosurfaceFunction
// vtkVolume vtkVolumeProperty

#ifndef __vtkVolumeRayCastMIPFunction_h
#define __vtkVolumeRayCastMIPFunction_h

#include "vtkVolumeRayCastFunction.h"

#define VTK_MAXIMIZE_SCALAR_VALUE 0
#define VTK_MAXIMIZE_OPACITY      1

class VTK_VOLUMERENDERING_EXPORT vtkVolumeRayCastMIPFunction : public vtkVolumeRayCastFunction
{
public:
  static vtkVolumeRayCastMIPFunction *New();
  vtkTypeMacro(vtkVolumeRayCastMIPFunction,vtkVolumeRayCastFunction);
  void PrintSelf( ostream& os, vtkIndent indent );


  // Description:
  // Get the scalar value below which all scalar values have zero opacity.
  float GetZeroOpacityThreshold( vtkVolume *vol );


  // Description:
  // Set the MaximizeMethod to either ScalarValue or Opacity.
  vtkSetClampMacro( MaximizeMethod, int,
        VTK_MAXIMIZE_SCALAR_VALUE, VTK_MAXIMIZE_OPACITY );
  vtkGetMacro(MaximizeMethod,int);
  void SetMaximizeMethodToScalarValue() 
    {this->SetMaximizeMethod(VTK_MAXIMIZE_SCALAR_VALUE);}
  void SetMaximizeMethodToOpacity() 
    {this->SetMaximizeMethod(VTK_MAXIMIZE_OPACITY);}
  const char *GetMaximizeMethodAsString(void);

//BTX
  void CastRay( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                vtkVolumeRayCastStaticInfo *staticInfo );
//ETX


protected:
  vtkVolumeRayCastMIPFunction();
  ~vtkVolumeRayCastMIPFunction();

  int MaximizeMethod;

//BTX
  void SpecificFunctionInitialize( vtkRenderer *ren,
                                   vtkVolume   *vol,
                                   vtkVolumeRayCastStaticInfo *staticInfo,
                                   vtkVolumeRayCastMapper *mapper );

//ETX
private:
  vtkVolumeRayCastMIPFunction(const vtkVolumeRayCastMIPFunction&);  // Not implemented.
  void operator=(const vtkVolumeRayCastMIPFunction&);  // Not implemented.
};



#endif
