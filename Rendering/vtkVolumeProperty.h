/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProperty.h
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

// .NAME vtkVolumeProperty - represents the common properties for rendering a volume.
//
// .SECTION Description
// vtkVolumeProperty is used to represent common properties associated 
// with volume rendering. This includes properties for determining the type
// of interpolation to use when sampling a volume, the color of a volume, 
// the scalar opacity of a volume, the gradient opacity of a volume, and the 
// shading parameters of a volume.
//
// When the scalar opacity or the gradient opacity of a volume is not set,
// then the function is defined to be a constant value of 1.0. When both a
// scalar and gradient opacity are both set simultaneously, then the opacity
// is defined to be the product of the scalar opacity and gradient opacity 
// transfer functions.

// .SECTION see also
// vtkPiecewiseFunction vtkColorTransferFunction


#ifndef __vtkVolumeProperty_h
#define __vtkVolumeProperty_h

#include "vtkObject.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkTimeStamp.h"

// Constants for InterpolationType
#define	VTK_NEAREST_INTERPOLATION	0
#define	VTK_LINEAR_INTERPOLATION	1


class VTK_EXPORT vtkVolumeProperty : public vtkObject
{
public:
  static vtkVolumeProperty *New();
  vtkTypeMacro(vtkVolumeProperty,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the modified time for this object (or the properties registered
  // with this object).
  unsigned long GetMTime();
  

  // Description:
  // Set the interpolation type for sampling a volume.
  vtkSetClampMacro( InterpolationType, int,
	VTK_NEAREST_INTERPOLATION, VTK_LINEAR_INTERPOLATION);
  vtkGetMacro(InterpolationType,int);
  void SetInterpolationTypeToNearest() 
	{this->SetInterpolationType(VTK_NEAREST_INTERPOLATION);};
  void SetInterpolationTypeToLinear() 
	{this->SetInterpolationType(VTK_LINEAR_INTERPOLATION);};
  const char *GetInterpolationTypeAsString(void);

  // Description:
  // Set the color of a volume to a gray level transfer function. This 
  // will also set the ColorChannels to 1.
  void SetColor( vtkPiecewiseFunction *function );

  // Description:
  // Set the color of a volume to an RGB transfer function. This 
  // will also set the ColorChannels to 3.
  void SetColor( vtkColorTransferFunction *function );

  // Description:
  // Get the number of color channels in the transfer function
  vtkGetMacro(ColorChannels,int);

  // Description:
  // Get the gray transfer function.
  vtkPiecewiseFunction *GetGrayTransferFunction();

  // Description:
  // Get the RGB transfer function.
  vtkColorTransferFunction *GetRGBTransferFunction();

  // Description:
  // Set the opacity of a volume to an opacity transfer function based
  // on scalar value. 
  void SetScalarOpacity( vtkPiecewiseFunction *function );

  // Description:
  // Get the scalar opacity transfer function.
  vtkPiecewiseFunction *GetScalarOpacity();

  // Description:
  // Set the opacity of a volume to an opacity transfer function based
  // on gradient magnitude. 
  void SetGradientOpacity( vtkPiecewiseFunction *function );

  // Description:
  // Get the gradient magnitude opacity transfer function.
  vtkPiecewiseFunction *GetGradientOpacity();

  // Description:
  // Set/Get the shading of a volume. If shading is turned off, then
  // the mapper for the volume will not perform shading calculations.
  // If shading is turned on, the mapper may perform shading 
  // calculations - in some cases shading does not apply (for example,
  // in a maximum intensity projection) and therefore shading will
  // not be performed even if this flag is on. For a compositing type
  // of mapper, turning shading off is generally the same as setting
  // ambient=1, diffuse=0, specular=0.
  vtkSetMacro(Shade,int);
  vtkGetMacro(Shade,int);
  vtkBooleanMacro(Shade,int);

  // Description:
  // Set/Get the ambient lighting coefficient.
  vtkSetClampMacro(Ambient,float,0.0,1.0);
  vtkGetMacro(Ambient,float);

  // Description:
  // Set/Get the diffuse lighting coefficient.
  vtkSetClampMacro(Diffuse,float,0.0,1.0);
  vtkGetMacro(Diffuse,float);

  // Description:
  // Set/Get the specular lighting coefficient.
  vtkSetClampMacro(Specular,float,0.0,1.0);
  vtkGetMacro(Specular,float);

  // Description:
  // Set/Get the specular power.
  vtkSetClampMacro(SpecularPower,float,0.0,100.0);
  vtkGetMacro(SpecularPower,float);

  // Description:
  // Set/Get the texture coefficient. This controls how much of the
  // color comes from the color transfer function and how much comes
  // from the RGB texture, with RGBTextureCoefficient used for the texture
  // and (1 - RGBTextureCoefficient) used for the transfer function.
  vtkSetClampMacro(RGBTextureCoefficient,float,0.0,1.0);
  vtkGetMacro(RGBTextureCoefficient,float);

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // UpdateMTimes performs a Modified() on all TimeStamps.
  // This is used by vtkVolume when the property is set, so
  // that any other object that might have been caching
  // information for the property will rebuild.
  void UpdateMTimes(); 

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Get the time that the gradient opacity transfer function was set
  vtkGetMacro(GradientOpacityMTime, vtkTimeStamp);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Get the time that the scalar opacity transfer function was set.
  vtkGetMacro(ScalarOpacityMTime, vtkTimeStamp);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Get the time that the RGBTransferFunction was set
  vtkGetMacro(RGBTransferFunctionMTime, vtkTimeStamp);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Get the time that the GrayTransferFunction was set
  vtkGetMacro(GrayTransferFunctionMTime, vtkTimeStamp);
//ETX


protected:
  vtkVolumeProperty();
  ~vtkVolumeProperty();
  vtkVolumeProperty(const vtkVolumeProperty&);
  void operator=(const vtkVolumeProperty&);

  int				InterpolationType;

  int				ColorChannels;

  vtkPiecewiseFunction		*GrayTransferFunction;
  vtkTimeStamp			GrayTransferFunctionMTime;

  vtkColorTransferFunction	*RGBTransferFunction;
  vtkTimeStamp			RGBTransferFunctionMTime;

  vtkPiecewiseFunction		*ScalarOpacity;
  vtkTimeStamp			ScalarOpacityMTime;

  vtkPiecewiseFunction		*GradientOpacity;
  vtkTimeStamp			GradientOpacityMTime;

  int				Shade;
  float				Ambient;
  float 			Diffuse;
  float				Specular;
  float				SpecularPower;
  float                         RGBTextureCoefficient;
};

// Description:
// Return the interpolation type as a descriptive character string.
inline const char *vtkVolumeProperty::GetInterpolationTypeAsString(void)
{
  if( this->InterpolationType == VTK_NEAREST_INTERPOLATION )
    {
    return "Nearest Neighbor";
    }
  else if( this->InterpolationType == VTK_LINEAR_INTERPOLATION )
    {
    return "Linear";
    }
  else
    {
    return "Unknown";
    }
}

#endif
