/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastIsosurfaceFunction.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkVolumeRayCastIsosurfaceFunction - An isosurface ray caster for volumes
//
// .SECTION Description
// vtkVolumeRayCastIsosurfaceFunction is a volume ray cast function that
// intersects a ray with an analytic isosurface in a scalar field. The color
// and shading parameters are defined in the vtkVolumeProperty of the 
// vtkVolume, as well as the interpolation type to use when locating the
// surface (either a nearest neighbor approach or a trilinear interpolation
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

// Description:
// Construct a new vtkVolumeRayCastIsosurfaceFunction with a default ramp.
// This ramp is best suited for unsigned char data and should
// probably be modified before rendering any other data type.
// The ParcBuildValue is set to LinearRampRange[0] + 1, ensuring
// that the Parc structure will be built during the first render.
  vtkVolumeRayCastIsosurfaceFunction();


// Description:
// Destruct the vtkVolumeRayCastIsosurfaceFunction
  ~vtkVolumeRayCastIsosurfaceFunction();

  static vtkVolumeRayCastIsosurfaceFunction *New() {return new vtkVolumeRayCastIsosurfaceFunction;};
  const char *GetClassName() {return "vtkVolumeRayCastIsosurfaceFunction";};

// Description:
// Print method for vtkVolumeRayCastIsosurfaceFunction
  void PrintSelf( ostream& os, vtkIndent index );


  // Description:
  // Give a ray type (0 = unsigned char, 1 = unsigned short,
  // 2 = short) cast a ray through the scalar data starting
  // at ray_position and taking num_steps of ray_increment size.
  // Return the final compositing value in pixel_value where
  // pixel_value[0] = red, pixel_value[1] = green, 
  // pixel_value[2] = blue, pixel_value[3] = alpha
  // pixel_value[4] = depth, and pixel_value[5] = number of steps
  void CastARay( int ray_type, void *data_ptr,
		 float ray_position[3], float ray_increment[3],
		 int num_steps, float pixel_value[6] );

  float GetZeroOpacityThreshold( vtkVolume *vol );

  // Description:
  // Set/Get the value of IsoValue.
  vtkSetMacro( IsoValue, float );
  vtkGetMacro( IsoValue, float );

  float IsoValue;

  // Description:
  // These variables are filled in by SpecificFunctionInitialize
  float       Color[3];

protected:

  void SpecificFunctionInitialize( vtkRenderer *ren,
				   vtkVolume   *vol,
				   vtkVolumeRayCastMapper *mapper );
};
#endif
