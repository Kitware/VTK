/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMIPFunction.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_RENDERING_EXPORT vtkVolumeRayCastMIPFunction : public vtkVolumeRayCastFunction
{
public:
  static vtkVolumeRayCastMIPFunction *New();
  vtkTypeMacro(vtkVolumeRayCastMIPFunction,vtkVolumeRayCastFunction);
  void PrintSelf( ostream& os, vtkIndent index );


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
  void CastRay( VTKVRCDynamicInfo *dynamicInfo,
		VTKVRCStaticInfo *staticInfo );
//ETX


protected:
  vtkVolumeRayCastMIPFunction();
  ~vtkVolumeRayCastMIPFunction();
  vtkVolumeRayCastMIPFunction(const vtkVolumeRayCastMIPFunction&);
  void operator=(const vtkVolumeRayCastMIPFunction&);

  int MaximizeMethod;

//BTX
  void SpecificFunctionInitialize( vtkRenderer *ren,
				   vtkVolume   *vol,
				   VTKVRCStaticInfo *staticInfo,
				   vtkVolumeRayCastMapper *mapper );

//ETX
};



#endif
