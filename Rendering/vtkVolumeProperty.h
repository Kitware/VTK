/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProperty.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

class vtkPiecewiseFunction;
class vtkTimeStamp;
class vtkColorTransferFunction;

class VTK_RENDERING_EXPORT vtkVolumeProperty : public vtkObject
{
public:
  static vtkVolumeProperty *New();
  vtkTypeMacro(vtkVolumeProperty,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  void DeepCopy(vtkVolumeProperty *p);

  // Description:
  // Get the modified time for this object (or the properties registered
  // with this object).
  unsigned long GetMTime();
 
  // Description:
  // Does the data have independent components, or do some define color 
  // only? If IndependentComponents is On (the default) then each component 
  // will be independently passed through a lookup table to determine RGBA, 
  // shaded. Some volume Mappers can handle 1 to 4 component 
  // unsigned char or unsigned short data (see each mapper header file to
  // determine functionality). If IndependentComponents is Off, then you 
  // must have either 2 or 4 component data. For 2 component data, the 
  // first is passed through the first color transfer function and the 
  // second component is passed through the first opacity transfer function. 
  // Normals will be generated off of the second component. For 4 component 
  // data, the first three will directly represent RGB (no lookup table). 
  // The fourth component will be passed through the first scalar opacity 
  // transfer function for opacity. Normals will be generated from the fourth 
  // component.
  vtkSetClampMacro( IndependentComponents, int, 0, 1 );
  vtkGetMacro( IndependentComponents, int );
  vtkBooleanMacro( IndependentComponents, int );

  // Description:
  // Set the interpolation type for sampling a volume. Initial value is
  // VTK_NEAREST_INTERPOLATION.
  vtkSetClampMacro( InterpolationType, int,
        VTK_NEAREST_INTERPOLATION, VTK_LINEAR_INTERPOLATION);
  vtkGetMacro(InterpolationType,int);
  void SetInterpolationTypeToNearest() 
        {this->SetInterpolationType(VTK_NEAREST_INTERPOLATION);};
  void SetInterpolationTypeToLinear() 
        {this->SetInterpolationType(VTK_LINEAR_INTERPOLATION);};
  const char *GetInterpolationTypeAsString(void);

  // Description:
  // Set/Get the scalar component weights
  virtual void SetComponentWeight(int index, double value);
  virtual double GetComponentWeight(int index);

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
  // If no transfer function has been set for this component, a default one
  // is created and returned.
  vtkPiecewiseFunction *GetGrayTransferFunction( int index );
  vtkPiecewiseFunction *GetGrayTransferFunction()
    {return this->GetGrayTransferFunction(0);};
  
  // Description:
  // Get the RGB transfer function for the given component.
  // If no transfer function has been set for this component, a default one
  // is created and returned.
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
  // If no transfer function has been set for this component, a default one
  // is created and returned.
  vtkPiecewiseFunction *GetScalarOpacity( int index );
  vtkPiecewiseFunction *GetScalarOpacity()
    {return this->GetScalarOpacity(0);};
      
  // Description:
  // Set/Get the unit distance on which the scalar opacity transfer function
  // is defined. By default this is 1.0, meaning that over a distance of
  // 1.0 units, a given opacity (from the transfer function) is accumulated.
  // This is adjusted for the actual sampling distance during rendering.
  void SetScalarOpacityUnitDistance( int index, double distance );
  void SetScalarOpacityUnitDistance( double distance )
    {this->SetScalarOpacityUnitDistance( 0, distance );}
  double GetScalarOpacityUnitDistance( int index );
  double GetScalarOpacityUnitDistance()
    {return this->GetScalarOpacityUnitDistance(0);}
  
  
  // Description:
  // Set the opacity of a volume to an opacity transfer function based
  // on gradient magnitude for the given component.
  void SetGradientOpacity( int index, vtkPiecewiseFunction *function );
  void SetGradientOpacity( vtkPiecewiseFunction *function )
    {this->SetGradientOpacity(0,function);}
  
  // Description:
  // Get the gradient magnitude opacity transfer function for
  // the given component.
  // If no transfer function has been set for this component, a default one
  // is created and returned.
  // This default function is always returned if DisableGradientOpacity is On
  // for that component.
  vtkPiecewiseFunction *GetGradientOpacity( int index );
  vtkPiecewiseFunction *GetGradientOpacity()
    {return this->GetGradientOpacity( 0 );}

  // Description:
  // Enable/Disable the gradient opacity function for the given component. 
  // If set to true, any call to GetGradientOpacity() will return a default
  // function for this component. Note that the gradient opacity function is 
  // still stored, it is not set or reset and can be retrieved using 
  // GetStoredGradientOpacity().
  virtual void SetDisableGradientOpacity( int index, int value );
  virtual void SetDisableGradientOpacity( int value )
    { this->SetDisableGradientOpacity(0, value); }
  virtual void DisableGradientOpacityOn( int index )
    { this->SetDisableGradientOpacity(index, 1); }
  virtual void DisableGradientOpacityOn()
    { this->DisableGradientOpacityOn(0); }
  virtual void DisableGradientOpacityOff( int index )
    { this->SetDisableGradientOpacity(index, 0); }
  virtual void DisableGradientOpacityOff()
    { this->DisableGradientOpacityOff(0); }
  virtual int GetDisableGradientOpacity( int index );
  virtual int GetDisableGradientOpacity()
    { return this->GetDisableGradientOpacity(0); }
  vtkPiecewiseFunction *GetStoredGradientOpacity( int index );
  vtkPiecewiseFunction *GetStoredGradientOpacity()
    {return this->GetStoredGradientOpacity( 0 );}

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
  void SetAmbient( int index, double value );
  void SetAmbient( double value ) {this->SetAmbient( 0, value );}
  double GetAmbient( int index );
  double GetAmbient() {return this->GetAmbient(0);}

  // Description:
  // Set/Get the diffuse lighting coefficient.
  void SetDiffuse( int index, double value );
  void SetDiffuse( double value ) {this->SetDiffuse( 0, value );}
  double GetDiffuse( int index );
  double GetDiffuse() {return this->GetDiffuse(0);}

  // Description:
  // Set/Get the specular lighting coefficient.
  void SetSpecular( int index, double value );
  void SetSpecular( double value ) {this->SetSpecular( 0, value );}
  double GetSpecular( int index );
  double GetSpecular() {return this->GetSpecular(0);}

  // Description:
  // Set/Get the specular power.
  void SetSpecularPower( int index, double value );
  void SetSpecularPower( double value ) {this->SetSpecularPower( 0, value );}
  double GetSpecularPower( int index );
  double GetSpecularPower() {return this->GetSpecularPower(0);}

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

  int                           IndependentComponents;
  double                        ComponentWeight[VTK_MAX_VRCOMP];

  int                           InterpolationType;

  int                           ColorChannels[VTK_MAX_VRCOMP];

  vtkPiecewiseFunction          *GrayTransferFunction[VTK_MAX_VRCOMP];
  vtkTimeStamp                  GrayTransferFunctionMTime[VTK_MAX_VRCOMP];

  vtkColorTransferFunction      *RGBTransferFunction[VTK_MAX_VRCOMP];
  vtkTimeStamp                  RGBTransferFunctionMTime[VTK_MAX_VRCOMP];

  vtkPiecewiseFunction          *ScalarOpacity[VTK_MAX_VRCOMP];
  vtkTimeStamp                  ScalarOpacityMTime[VTK_MAX_VRCOMP];
  double                        ScalarOpacityUnitDistance[VTK_MAX_VRCOMP];
  
  vtkPiecewiseFunction          *GradientOpacity[VTK_MAX_VRCOMP];
  vtkTimeStamp                  GradientOpacityMTime[VTK_MAX_VRCOMP];
  vtkPiecewiseFunction          *DefaultGradientOpacity[VTK_MAX_VRCOMP];
  int                           DisableGradientOpacity[VTK_MAX_VRCOMP];

  int                           Shade[VTK_MAX_VRCOMP];
  double                        Ambient[VTK_MAX_VRCOMP];
  double                        Diffuse[VTK_MAX_VRCOMP];
  double                        Specular[VTK_MAX_VRCOMP];
  double                        SpecularPower[VTK_MAX_VRCOMP];

  virtual void                  CreateDefaultGradientOpacity(int index);

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
