/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProperty.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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
// then the function is defined to be a constant value of 1.0. When a
// scalar and gradient opacity are both set simultaneously, then the opacity
// is defined to be the product of the scalar opacity and gradient opacity 
// transfer functions.
// 
// Most properties can be set per "component" for volume mappers that
// support multiple independent components. If you are using 2 component
// data as LV or 4 component data as RGBV (as specified in the mapper) 
// only the first scalar opacity and gradient opacity transfer functions
// will be used (and all color functions will be ignored). Omitting the
// index parameter on the Set/Get methods will access index = 0.


// .SECTION see also
// vtkPiecewiseFunction vtkColorTransferFunction


#ifndef __vtkVolumeProperty_h
#define __vtkVolumeProperty_h

#include "vtkObject.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkTimeStamp.h"

// Constants for InterpolationType
#define VTK_NEAREST_INTERPOLATION       0
#define VTK_LINEAR_INTERPOLATION        1

#define VTK_MAX_VRCOMP                  4

class VTK_RENDERING_EXPORT vtkVolumeProperty : public vtkObject
{
public:
  static vtkVolumeProperty *New();
  vtkTypeRevisionMacro(vtkVolumeProperty,vtkObject);
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
  // Set the color of a volume to a gray level transfer function
  // for the component indicated by index. This will set the
  // color channels for this component to 1.
  void SetColor( int index, vtkPiecewiseFunction *function );
  void SetColor( vtkPiecewiseFunction *f ){this->SetColor(0,f);};
  
      
  // Description:
  // Set the color of a volume to an RGB transfer function
  // for the component indicated by index. This will set the
  // color channels for this component to 3.
  // This will also recompute the color channels
  void SetColor( int index, vtkColorTransferFunction *function );
  void SetColor( vtkColorTransferFunction *f ){this->SetColor(0,f);};
  
  // Description:
  // Get the number of color channels in the transfer function
  // for the given component.
  int GetColorChannels( int index );
  int GetColorChannels(){return this->GetColorChannels(0);};
  
  // Description:
  // Get the gray transfer function.
  vtkPiecewiseFunction *GetGrayTransferFunction( int index );
  vtkPiecewiseFunction *GetGrayTransferFunction()
    {return this->GetGrayTransferFunction(0);};
  
  // Description:
  // Get the RGB transfer function for the given component.
  vtkColorTransferFunction *GetRGBTransferFunction( int index );
  vtkColorTransferFunction *GetRGBTransferFunction()
    {return this->GetRGBTransferFunction(0);};
  
  // Description:
  // Set the opacity of a volume to an opacity transfer function based
  // on scalar value for the component indicated by index. 
  void SetScalarOpacity( int index, vtkPiecewiseFunction *function );
  void SetScalarOpacity( vtkPiecewiseFunction *f )
    {this->SetScalarOpacity(0,f);};
  
  // Description:
  // Get the scalar opacity transfer function for the given component.
  vtkPiecewiseFunction *GetScalarOpacity( int index );
  vtkPiecewiseFunction *GetScalarOpacity()
    {return this->GetScalarOpacity(0);};
  
      
  // Description:
  // Set the opacity of a volume to an opacity transfer function based
  // on gradient magnitude for the given component.
  void SetGradientOpacity( int index, vtkPiecewiseFunction *function );
  void SetGradientOpacity( vtkPiecewiseFunction *function )
    {this->SetGradientOpacity(0,function);}
  
  // Description:
  // Get the gradient magnitude opacity transfer function for
  // the given component.
  vtkPiecewiseFunction *GetGradientOpacity( int index );
  vtkPiecewiseFunction *GetGradientOpacity()
    {return this->GetGradientOpacity( 0 );}
  
  // Description:
  // Set/Get the shading of a volume. If shading is turned off, then
  // the mapper for the volume will not perform shading calculations.
  // If shading is turned on, the mapper may perform shading 
  // calculations - in some cases shading does not apply (for example,
  // in a maximum intensity projection) and therefore shading will
  // not be performed even if this flag is on. For a compositing type
  // of mapper, turning shading off is generally the same as setting
  // ambient=1, diffuse=0, specular=0. Shading can be independently
  // turned on/off per component.
  void SetShade( int index, int value );
  void SetShade( int value ) {this->SetShade(0,value);}
  int GetShade( int index );
  int GetShade() {return this->GetShade(0);}
  void ShadeOn( int index );
  void ShadeOn() {this->ShadeOn(0);}
  void ShadeOff( int index );
  void ShadeOff() {this->ShadeOff(0);}
  

  // Description:
  // Set/Get the ambient lighting coefficient.
  void SetAmbient( int index, float value );
  void SetAmbient( float value ) {this->SetAmbient( 0, value );}
  float GetAmbient( int index );
  float GetAmbient() {return this->GetAmbient(0);}
  

  // Description:
  // Set/Get the diffuse lighting coefficient.
  void SetDiffuse( int index, float value );
  void SetDiffuse( float value ) {this->SetDiffuse( 0, value );}
  float GetDiffuse( int index );
  float GetDiffuse() {return this->GetDiffuse(0);}

  // Description:
  // Set/Get the specular lighting coefficient.
  void SetSpecular( int index, float value );
  void SetSpecular( float value ) {this->SetSpecular( 0, value );}
  float GetSpecular( int index );
  float GetSpecular() {return this->GetSpecular(0);}

  // Description:
  // Set/Get the specular power.
  void SetSpecularPower( int index, float value );
  void SetSpecularPower( float value ) {this->SetSpecularPower( 0, value );}
  float GetSpecularPower( int index );
  float GetSpecularPower() {return this->GetSpecularPower(0);}

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
  vtkTimeStamp GetGradientOpacityMTime( int index );
  vtkTimeStamp GetGradientOpacityMTime()
    { return this->GetGradientOpacityMTime(0); }  
      
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Get the time that the scalar opacity transfer function was set.
  vtkTimeStamp GetScalarOpacityMTime( int index );
  vtkTimeStamp GetScalarOpacityMTime()
    { return this->GetScalarOpacityMTime(0); }  

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Get the time that the RGBTransferFunction was set
  vtkTimeStamp GetRGBTransferFunctionMTime( int index );
  vtkTimeStamp GetRGBTransferFunctionMTime()
    { return this->GetRGBTransferFunctionMTime(0); }  

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Get the time that the GrayTransferFunction was set
  vtkTimeStamp GetGrayTransferFunctionMTime( int index );
  vtkTimeStamp GetGrayTransferFunctionMTime()
    { return this->GetGrayTransferFunctionMTime(0); }  
//ETX


protected:
  vtkVolumeProperty();
  ~vtkVolumeProperty();

  int                           InterpolationType;

  int                           ColorChannels[VTK_MAX_VRCOMP];

  vtkPiecewiseFunction          *GrayTransferFunction[VTK_MAX_VRCOMP];
  vtkTimeStamp                  GrayTransferFunctionMTime[VTK_MAX_VRCOMP];

  vtkColorTransferFunction      *RGBTransferFunction[VTK_MAX_VRCOMP];
  vtkTimeStamp                  RGBTransferFunctionMTime[VTK_MAX_VRCOMP];

  vtkPiecewiseFunction          *ScalarOpacity[VTK_MAX_VRCOMP];
  vtkTimeStamp                  ScalarOpacityMTime[VTK_MAX_VRCOMP];

  vtkPiecewiseFunction          *GradientOpacity[VTK_MAX_VRCOMP];
  vtkTimeStamp                  GradientOpacityMTime[VTK_MAX_VRCOMP];

  int                           Shade[VTK_MAX_VRCOMP];
  float                         Ambient[VTK_MAX_VRCOMP];
  float                         Diffuse[VTK_MAX_VRCOMP];
  float                         Specular[VTK_MAX_VRCOMP];
  float                         SpecularPower[VTK_MAX_VRCOMP];

private:
  vtkVolumeProperty(const vtkVolumeProperty&);  // Not implemented.
  void operator=(const vtkVolumeProperty&);  // Not implemented.
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
