/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProperty.h
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

/****
// Constants for ShadeType
#define	VTK_NO_SHADE			0
#define	VTK_FAST_SHADE			1 // Fastest shading algorithm avail.
#define	VTK_BEST_SHADE			2 // Best shading algorithm available
*****/

class VTK_EXPORT vtkVolumeProperty : public vtkObject
{
public:

// Description:
// Construct a new vtkVolumeProperty with default values
  vtkVolumeProperty();


// Description:
// Destruct a vtkVolumeProperty
  ~vtkVolumeProperty();

  static vtkVolumeProperty *New() {return new vtkVolumeProperty;};
  const char *GetClassName() {return "vtkVolumeProperty";};

// Description:
// Print the state of the volume property.
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Set the interpolation type for sampling a volume.
  vtkSetClampMacro( InterpolationType, int,
	VTK_NEAREST_INTERPOLATION, VTK_LINEAR_INTERPOLATION);
  vtkGetMacro(InterpolationType,int);
  void SetInterpolationTypeToNearest() 
	{this->SetInterpolationType(VTK_NEAREST_INTERPOLATION);};
  void SetInterpolationTypeToLinear() 
	{this->SetInterpolationType(VTK_LINEAR_INTERPOLATION);};
  char *GetInterpolationTypeAsString(void);

  // Description:
  // Get the modified time for this object (or the properties registered
  // with this object).
  unsigned long int GetMTime();
  
  // Description:
  // Set the color of a volume to a gray level transfer function. This 
  // will also set the ColorChannels to 1.
  void SetColor( vtkPiecewiseFunction *function );

  // Description:
  // Get the number of color channels in the transfer function
  vtkGetMacro(ColorChannels,int);

  // Description:
  // Get the gray transfer function.
  vtkPiecewiseFunction *GetGrayTransferFunction();

  // Description:
  // Get the time that the GrayTransferFunction was set
  vtkGetMacro(GrayTransferFunctionMTime, vtkTimeStamp);

  // Description:
  // Set the color of a volume to an RGB transfer function. This 
  // will also set the ColorChannels to 3.
  void SetColor( vtkColorTransferFunction *function );

  // Description:
  // Get the RGB transfer function.
  vtkColorTransferFunction *GetRGBTransferFunction();

  // Description:
  // Get the time that the RGBTransferFunction was set
  vtkGetMacro(RGBTransferFunctionMTime, vtkTimeStamp);

  // Description:
  // Set the opacity of a volume to an opacity transfer function based
  // on scalar value. 
  void SetScalarOpacity( vtkPiecewiseFunction *function );

  // Description:
  // Get the scalar opacity transfer function.
  vtkPiecewiseFunction *GetScalarOpacity();

  // Description:
  // Get the time that the scalar opacity transfer function was set.
  vtkGetMacro(ScalarOpacityMTime, vtkTimeStamp);

  // Description:
  // Set the opacity of a volume to an opacity transfer function based
  // on gradient magnitude. 
  void SetGradientOpacity( vtkPiecewiseFunction *function );

  // Description:
  // Get the gradient magnitude opacity transfer function.
  vtkPiecewiseFunction *GetGradientOpacity();

  // Description:
  // Get the time that the gradient opacity transfer function was set
  vtkGetMacro(GradientOpacityMTime, vtkTimeStamp);

  // Description:
  // Set/Get the shading of a volume.
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
  // Set/Get the gradient magnitude opacity scale.
  vtkSetMacro(GradientOpacityScale,float);
  vtkGetMacro(GradientOpacityScale,float);

  // Description:
  // Set/Get the gradient magnitude opacity bias.
  vtkSetMacro(GradientOpacityBias,float);
  vtkGetMacro(GradientOpacityBias,float);

protected:

  int				InterpolationType;

  int				ColorChannels;

  vtkPiecewiseFunction		*GrayTransferFunction;
  vtkTimeStamp			GrayTransferFunctionMTime;
  int				SelfCreatedGTFun;

  vtkColorTransferFunction	*RGBTransferFunction;
  vtkTimeStamp			RGBTransferFunctionMTime;
  int				SelfCreatedRGBTFun;

  vtkPiecewiseFunction		*ScalarOpacity;
  vtkTimeStamp			ScalarOpacityMTime;
  int				SelfCreatedSOTFun;

  vtkPiecewiseFunction		*GradientOpacity;
  vtkTimeStamp			GradientOpacityMTime;
  int				SelfCreatedGOTFun;

  int				Shade;
  float				Ambient;
  float 			Diffuse;
  float				Specular;
  float				SpecularPower;

  // Constants for remapping the gradient magnitude values
  float				GradientOpacityScale;
  float				GradientOpacityBias;
};

// Description:
// Return the interpolation type as a descriptive character string.
inline char *vtkVolumeProperty::GetInterpolationTypeAsString(void)
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
    return "Unknown";
}

#endif
