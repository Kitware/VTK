/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMIPFunction.h
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

class VTK_EXPORT vtkVolumeRayCastMIPFunction : public vtkVolumeRayCastFunction
{
public:
  vtkVolumeRayCastMIPFunction();
  ~vtkVolumeRayCastMIPFunction();
  static vtkVolumeRayCastMIPFunction *New() {
    return new vtkVolumeRayCastMIPFunction;};
  const char *GetClassName() {return "vtkVolumeRayCastMIPFunction";};
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Given a ray type, cast a ray through the scalar data starting
  // at ray_position and taking num_steps of ray_increment size.
  // Return the final compositing value in pixel_value where
  // pixel_value[0] = red, pixel_value[1] = green, 
  // pixel_value[2] = blue, pixel_value[3] = alpha
  // pixel_value[4] = depth, and pixel_value[5] = number of steps
  void CastARay( int ray_type, void *data_ptr,
		 float ray_position[3], float ray_increment[3],
		 int num_steps, float pixel_value[6] );



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
  char *GetMaximizeMethodAsString(void);


protected:
  int MaximizeMethod;

  void SpecificFunctionInitialize( vtkRenderer *ren,
				   vtkVolume   *vol,
				   vtkVolumeRayCastMapper *mapper );

};


// Description:
// Return the maximize method as a descriptive character string.
inline char *vtkVolumeRayCastMIPFunction::GetMaximizeMethodAsString(void)
{
  if( this->MaximizeMethod == VTK_MAXIMIZE_SCALAR_VALUE )
    {
    return "Maximize Scalar Value";
    }
  if( this->MaximizeMethod == VTK_MAXIMIZE_OPACITY )
    {
    return "Maximize Opacity";
    }
  else
    return "Unknown";
}

#endif
