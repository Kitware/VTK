/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastIsosurfaceFunction.h
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

class VTK_EXPORT vtkVolumeRayCastIsosurfaceFunction : public vtkVolumeRayCastFunction
{
public:
  vtkTypeMacro(vtkVolumeRayCastIsosurfaceFunction,vtkVolumeRayCastFunction);
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Construct a new vtkVolumeRayCastIsosurfaceFunction
  static vtkVolumeRayCastIsosurfaceFunction *New();

  // Description:
  // Get the scalar value below which all scalar values have 0 opacity
  float GetZeroOpacityThreshold( vtkVolume *vol );

  // Description:
  // Set/Get the value of IsoValue.
  vtkSetMacro( IsoValue, float );
  vtkGetMacro( IsoValue, float );

  
  // Description:
  // This is the isovalue at which to view a surface
  float IsoValue;

  // Description:
  // These variables are filled in by SpecificFunctionInitialize
  float       Color[3];

//BTX
  void CastRay(	VTKVRCDynamicInfo *dynamicInfo,
		VTKVRCStaticInfo *staticInfo);
//ETX

protected:
  vtkVolumeRayCastIsosurfaceFunction();
  ~vtkVolumeRayCastIsosurfaceFunction();
  vtkVolumeRayCastIsosurfaceFunction(const vtkVolumeRayCastIsosurfaceFunction&);
  void operator=(const vtkVolumeRayCastIsosurfaceFunction&);

//BTX
  void SpecificFunctionInitialize( vtkRenderer *ren,
				   vtkVolume   *vol,
				   VTKVRCStaticInfo *staticInfo,
				   vtkVolumeRayCastMapper *mapper );
//ETX
};
#endif
