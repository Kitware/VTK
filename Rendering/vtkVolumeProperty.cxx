/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProperty.cxx
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
#include "vtkVolumeProperty.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkVolumeProperty, "1.29");
vtkStandardNewMacro(vtkVolumeProperty);

// Construct a new vtkVolumeProperty with default values
vtkVolumeProperty::vtkVolumeProperty()
{
  this->InterpolationType               = VTK_NEAREST_INTERPOLATION;

  this->ColorChannels                   = 1;

  this->GrayTransferFunction            = NULL;
  this->RGBTransferFunction             = NULL;
  this->ScalarOpacity                   = NULL;
  this->GradientOpacity                 = NULL;

  this->Shade                           = 0;  
  this->Ambient                         = 0.1;
  this->Diffuse                         = 0.7;
  this->Specular                        = 0.2;
  this->SpecularPower                   = 10.0;
  this->RGBTextureCoefficient           = 0.0;
}

// Destruct a vtkVolumeProperty
vtkVolumeProperty::~vtkVolumeProperty()
{
  if (this->GrayTransferFunction != NULL)
    {
    this->GrayTransferFunction->UnRegister(this);
    }

  if (this->RGBTransferFunction != NULL)
    {
    this->RGBTransferFunction->UnRegister(this);
    }

  if (this->ScalarOpacity != NULL)
    {
    this->ScalarOpacity->UnRegister(this);
    }

  if (this->GradientOpacity != NULL)
    {
    this->GradientOpacity->UnRegister(this);
    }
}

void vtkVolumeProperty::UpdateMTimes() 
{
  this->Modified();
  this->GrayTransferFunctionMTime.Modified();
  this->RGBTransferFunctionMTime.Modified();
  this->ScalarOpacityMTime.Modified();
  this->GradientOpacityMTime.Modified();
}

unsigned long int vtkVolumeProperty::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;

  // Color MTimes
  if (this->ColorChannels == 1)
    {
    if (this->GrayTransferFunction)
      {
      // time that Gray transfer function pointer was set
      time = this->GrayTransferFunctionMTime;
      mTime = (mTime > time ? mTime : time);

      // time that Gray transfer function was last modified
      time = this->GrayTransferFunction->GetMTime();
      mTime = (mTime > time ? mTime : time);
      }
    }
  else if (this->ColorChannels == 3)
    {
    if (this->RGBTransferFunction)
      {
      // time that RGB transfer function pointer was set
      time = this->RGBTransferFunctionMTime;
      mTime = (mTime > time ? mTime : time);

      // time that RGB transfer function was last modified
      time = this->RGBTransferFunction->GetMTime();
      mTime = (mTime > time ? mTime : time);
      }
    }

  // Opacity MTimes
  if (this->ScalarOpacity)
    {
    // time that Scalar opacity transfer function pointer was set
    time = this->ScalarOpacityMTime;
    mTime = (mTime > time ? mTime : time);

    // time that Scalar opacity transfer function was last modified
    time = this->ScalarOpacity->GetMTime();
    mTime = (mTime > time ? mTime : time);
    }

  if (this->GradientOpacity)
    {
    // time that Gradient opacity transfer function pointer was set
    time = this->GradientOpacityMTime;
    mTime = (mTime > time ? mTime : time);

    // time that Gradient opacity transfer function was last modified
    time = this->GradientOpacity->GetMTime();
    mTime = (mTime > time ? mTime : time);
    }
      
  return mTime;
}


// Set the color of a volume to a gray transfer function
void vtkVolumeProperty::SetColor( vtkPiecewiseFunction *function )
{
  if (this->GrayTransferFunction != function )
    {
    if (this->GrayTransferFunction != NULL) 
      {
      this->GrayTransferFunction->UnRegister(this);
      }
    this->GrayTransferFunction  = function;
    if (this->GrayTransferFunction != NULL) 
      {
      this->GrayTransferFunction->Register(this);
      }

    this->GrayTransferFunctionMTime.Modified();
    this->Modified();
    }

  if (this->ColorChannels != 1 )
    {
    this->ColorChannels         = 1;
    this->Modified();
    }
}

// Get the currently set gray transfer function. Create one if none set.
vtkPiecewiseFunction *vtkVolumeProperty::GetGrayTransferFunction()
{
  if (this->GrayTransferFunction == NULL )
    {
    this->GrayTransferFunction = vtkPiecewiseFunction::New();
    this->GrayTransferFunction->Register(this);
    this->GrayTransferFunction->Delete();
    this->GrayTransferFunction->AddPoint(    0, 0.0 );
    this->GrayTransferFunction->AddPoint( 1024, 1.0 );
    }

  return this->GrayTransferFunction;
}

// Set the color of a volume to an RGB transfer function
void vtkVolumeProperty::SetColor( vtkColorTransferFunction *function )
{
  if (this->RGBTransferFunction != function )
    {
    if (this->RGBTransferFunction != NULL) 
      {
      this->RGBTransferFunction->UnRegister(this);
      }
    this->RGBTransferFunction   = function;
    if (this->RGBTransferFunction != NULL) 
      {
      this->RGBTransferFunction->Register(this);
      }
    this->RGBTransferFunctionMTime.Modified();
    this->Modified();
    }

  if (this->ColorChannels != 3 )
    {
    this->ColorChannels         = 3;
    this->Modified();
    }
}

// Get the currently set RGB transfer function. Create one if none set.
vtkColorTransferFunction *vtkVolumeProperty::GetRGBTransferFunction()
{
  if (this->RGBTransferFunction == NULL )
    {
    this->RGBTransferFunction = vtkColorTransferFunction::New();
    this->RGBTransferFunction->Register(this);
    this->RGBTransferFunction->Delete();
    this->RGBTransferFunction->AddRGBPoint(      0, 0.0, 0.0, 0.0 );
    this->RGBTransferFunction->AddRGBPoint(   1024, 1.0, 1.0, 1.0 );
    }

  return this->RGBTransferFunction;
}

// Set the scalar opacity of a volume to a transfer function
void vtkVolumeProperty::SetScalarOpacity( vtkPiecewiseFunction *function )
{
  if ( this->ScalarOpacity != function )
    {
    if (this->ScalarOpacity != NULL) 
      {
      this->ScalarOpacity->UnRegister(this);
      }
    this->ScalarOpacity = function;
    if (this->ScalarOpacity != NULL) 
      {
      this->ScalarOpacity->Register(this);
      }

    this->ScalarOpacityMTime.Modified();
    this->Modified();
    }
}

// Get the scalar opacity transfer function. Create one if none set.
vtkPiecewiseFunction *vtkVolumeProperty::GetScalarOpacity()
{
  if( this->ScalarOpacity == NULL )
    {
    this->ScalarOpacity = vtkPiecewiseFunction::New();
    this->ScalarOpacity->Register(this);
    this->ScalarOpacity->Delete();
    this->ScalarOpacity->AddPoint(    0, 1.0 );
    this->ScalarOpacity->AddPoint( 1024, 1.0 );
    }

  return this->ScalarOpacity;
}

// Set the gradient opacity transfer function 
void vtkVolumeProperty::SetGradientOpacity( vtkPiecewiseFunction *function )
{
  if ( this->GradientOpacity != function )
    {
    if (this->GradientOpacity != NULL) 
      {
      this->GradientOpacity->UnRegister(this);
      }
    this->GradientOpacity       = function;
    if (this->GradientOpacity != NULL) 
      {
      this->GradientOpacity->Register(this);
      }
    
    this->GradientOpacityMTime.Modified();
    this->Modified();
    }
}

// Get the gradient opacity transfer function. Create one if none set.
vtkPiecewiseFunction *vtkVolumeProperty::GetGradientOpacity()
{
  if ( this->GradientOpacity == NULL )
    {
    this->GradientOpacity = vtkPiecewiseFunction::New();
    this->GradientOpacity->Register(this);
    this->GradientOpacity->Delete();
    this->GradientOpacity->AddPoint(   0, 1.0 );
    this->GradientOpacity->AddPoint( 255, 1.0 );
    }

  return this->GradientOpacity;
}

// Print the state of the volume property.
void vtkVolumeProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Interpolation Type: "
     << this->GetInterpolationTypeAsString() << "\n";

  os << indent << "Color Channels: " << this->ColorChannels << "\n";

  if( this->ColorChannels == 1 )
    {
      os << indent << "Gray Color Transfer Function: "
         << this->GrayTransferFunction << "\n";
    }
  else if( this->ColorChannels == 3 )
    {
      os << indent << "RGB Color Transfer Function: "
         << this->RGBTransferFunction << "\n";
    }

  os << indent << "Scalar Opacity Transfer Function: "
     << this->ScalarOpacity << "\n";

  os << indent << "Gradient Opacity Transfer Function: "
     << this->GradientOpacity << "\n";

  os << indent << "RGB Texture Coefficient: " 
     << this->RGBTextureCoefficient << endl;

  os << indent << "Shade: " << this->Shade << "\n";
  os << indent << indent << "Ambient: " << this->Ambient << "\n";
  os << indent << indent << "Diffuse: " << this->Diffuse << "\n";
  os << indent << indent << "Specular: " << this->Specular << "\n";
  os << indent << indent << "SpecularPower: " << this->SpecularPower << "\n";

  // These variables should not be printed to the user:
  // this->GradientOpacityMTime
  // this->GrayTransferFunctionMTime
  // this->RGBTransferFunctionMTime
  // this->ScalarOpacityMTime

}

