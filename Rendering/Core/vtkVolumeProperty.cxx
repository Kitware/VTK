/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"

vtkStandardNewMacro(vtkVolumeProperty);

// Construct a new vtkVolumeProperty with default values
vtkVolumeProperty::vtkVolumeProperty()
{
  this->IndependentComponents = 1;

  this->InterpolationType               = VTK_NEAREST_INTERPOLATION;

  for ( int i = 0; i < VTK_MAX_VRCOMP; i++ )
  {
    this->ColorChannels[i]                   = 1;

    this->GrayTransferFunction[i]            = NULL;
    this->RGBTransferFunction[i]             = NULL;
    this->ScalarOpacity[i]                   = NULL;
    this->ScalarOpacityUnitDistance[i]       = 1.0;
    this->GradientOpacity[i]                 = NULL;
    this->DefaultGradientOpacity[i]          = NULL;
    this->DisableGradientOpacity[i]          = 0;

    this->ComponentWeight[i]                 = 1.0;

    this->Shade[i]                           = 0;
    this->Ambient[i]                         = 0.1;
    this->Diffuse[i]                         = 0.7;
    this->Specular[i]                        = 0.2;
    this->SpecularPower[i]                   = 10.0;
  }
}

// Destruct a vtkVolumeProperty
vtkVolumeProperty::~vtkVolumeProperty()
{
  for ( int i = 0; i < VTK_MAX_VRCOMP; i++ )
  {
    if (this->GrayTransferFunction[i] != NULL)
    {
      this->GrayTransferFunction[i]->UnRegister(this);
    }

    if (this->RGBTransferFunction[i] != NULL)
    {
      this->RGBTransferFunction[i]->UnRegister(this);
    }

    if (this->ScalarOpacity[i] != NULL)
    {
      this->ScalarOpacity[i]->UnRegister(this);
    }

    if (this->GradientOpacity[i] != NULL)
    {
      this->GradientOpacity[i]->UnRegister(this);
    }

    if (this->DefaultGradientOpacity[i] != NULL)
    {
      this->DefaultGradientOpacity[i]->UnRegister(this);
    }
  }
}

void vtkVolumeProperty::DeepCopy(vtkVolumeProperty *p)
{
  if (!p)
  {
    return;
  }

  this->SetIndependentComponents(p->GetIndependentComponents());

  this->SetInterpolationType(p->GetInterpolationType());

  for (int i = 0; i < VTK_MAX_VRCOMP; i++)
  {
    this->SetComponentWeight(i, p->GetComponentWeight(i));

    // Force ColorChannels to the right value and/or create a default tfunc
    // then DeepCopy all the points

    if (p->GetColorChannels(i) > 1)
    {
      this->SetColor(i, this->GetRGBTransferFunction(i));
      this->GetRGBTransferFunction(i)->DeepCopy(
        p->GetRGBTransferFunction(i));
    }
    else
    {
      this->SetColor(i, this->GetGrayTransferFunction(i));
      this->GetGrayTransferFunction(i)->DeepCopy(
        p->GetGrayTransferFunction(i));
    }

    this->GetScalarOpacity(i)->DeepCopy(p->GetScalarOpacity(i));

    this->SetScalarOpacityUnitDistance(i, p->GetScalarOpacityUnitDistance(i));

    this->GetGradientOpacity(i)->DeepCopy(p->GetGradientOpacity(i));

    this->SetDisableGradientOpacity(i, p->GetDisableGradientOpacity(i));

    this->SetShade(i, p->GetShade(i));
    this->SetAmbient(i, p->GetAmbient(i));
    this->SetDiffuse(i, p->GetDiffuse(i));
    this->SetSpecular(i, p->GetSpecular(i));
    this->SetSpecularPower(i, p->GetSpecularPower(i));
  }

  this->Modified();
}

void vtkVolumeProperty::UpdateMTimes()
{
  this->Modified();

  for ( int i = 0; i < VTK_MAX_VRCOMP; i++ )
  {
    this->GrayTransferFunctionMTime[i].Modified();
    this->RGBTransferFunctionMTime[i].Modified();
    this->ScalarOpacityMTime[i].Modified();
    this->GradientOpacityMTime[i].Modified();
  }
}

vtkMTimeType vtkVolumeProperty::GetMTime()
{
  vtkMTimeType mTime=this->vtkObject::GetMTime();
  vtkMTimeType time;

  for ( int i = 0; i < VTK_MAX_VRCOMP; i++ )
  {
    // Color MTimes
    if (this->ColorChannels[i] == 1)
    {
      if (this->GrayTransferFunction[i])
      {
        // time that Gray transfer function pointer was set
        time = this->GrayTransferFunctionMTime[i];
        mTime = (mTime > time ? mTime : time);

        // time that Gray transfer function was last modified
        time = this->GrayTransferFunction[i]->GetMTime();
        mTime = (mTime > time ? mTime : time);
      }
    }
    else if (this->ColorChannels[i] == 3)
    {
      if (this->RGBTransferFunction[i])
      {
        // time that RGB transfer function pointer was set
        time = this->RGBTransferFunctionMTime[i];
        mTime = (mTime > time ? mTime : time);

        // time that RGB transfer function was last modified
        time = this->RGBTransferFunction[i]->GetMTime();
        mTime = (mTime > time ? mTime : time);
      }
    }

    // Opacity MTimes
    if (this->ScalarOpacity[i])
    {
      // time that Scalar opacity transfer function pointer was set
      time = this->ScalarOpacityMTime[i];
      mTime = (mTime > time ? mTime : time);

      // time that Scalar opacity transfer function was last modified
      time = this->ScalarOpacity[i]->GetMTime();
      mTime = (mTime > time ? mTime : time);
    }

    if (this->GradientOpacity[i])
    {
      // time that Gradient opacity transfer function pointer was set
      time = this->GradientOpacityMTime[i];
      mTime = (mTime > time ? mTime : time);

      if (!this->DisableGradientOpacity[i])
      {
        // time that Gradient opacity transfer function was last modified
        time = this->GradientOpacity[i]->GetMTime();
        mTime = (mTime > time ? mTime : time);
      }
    }
  }

  return mTime;
}

int vtkVolumeProperty::GetColorChannels( int index )
{
  if ( index < 0 || index > 3 )
  {
    vtkErrorMacro("Bad index - must be between 0 and 3");
    return 0;
  }

  return this->ColorChannels[index];
}


// Set the color of a volume to a gray transfer function
void vtkVolumeProperty::SetColor( int index, vtkPiecewiseFunction *function )
{
  if (this->GrayTransferFunction[index] != function )
  {
    if (this->GrayTransferFunction[index] != NULL)
    {
      this->GrayTransferFunction[index]->UnRegister(this);
    }
    this->GrayTransferFunction[index]  = function;
    if (this->GrayTransferFunction[index] != NULL)
    {
      this->GrayTransferFunction[index]->Register(this);
    }

    this->GrayTransferFunctionMTime[index].Modified();
    this->Modified();
  }

  if (this->ColorChannels[index] != 1)
  {
    this->ColorChannels[index] = 1;
    this->Modified();
  }
}

// Get the currently set gray transfer function. Create one if none set.
vtkPiecewiseFunction *vtkVolumeProperty::GetGrayTransferFunction( int index )
{
  if (this->GrayTransferFunction[index] == NULL )
  {
    this->GrayTransferFunction[index] = vtkPiecewiseFunction::New();
    this->GrayTransferFunction[index]->Register(this);
    this->GrayTransferFunction[index]->Delete();
    this->GrayTransferFunction[index]->AddPoint(    0, 0.0 );
    this->GrayTransferFunction[index]->AddPoint( 1024, 1.0 );
    if (this->ColorChannels[index] != 1)
    {
      this->ColorChannels[index] = 1;
    }
    this->Modified();
  }

  return this->GrayTransferFunction[index];
}

// Set the color of a volume to an RGB transfer function
void vtkVolumeProperty::SetColor( int index, vtkColorTransferFunction *function )
{
  if (this->RGBTransferFunction[index] != function )
  {
    if (this->RGBTransferFunction[index] != NULL)
    {
      this->RGBTransferFunction[index]->UnRegister(this);
    }
    this->RGBTransferFunction[index]   = function;
    if (this->RGBTransferFunction[index] != NULL)
    {
      this->RGBTransferFunction[index]->Register(this);
    }
    this->RGBTransferFunctionMTime[index].Modified();
    this->Modified();
  }

  if (this->ColorChannels[index] != 3)
  {
    this->ColorChannels[index] = 3;
    this->Modified();
  }
}

// Get the currently set RGB transfer function. Create one if none set.
vtkColorTransferFunction *vtkVolumeProperty::GetRGBTransferFunction( int index )
{
  if (this->RGBTransferFunction[index] == NULL )
  {
    this->RGBTransferFunction[index] = vtkColorTransferFunction::New();
    this->RGBTransferFunction[index]->Register(this);
    this->RGBTransferFunction[index]->Delete();
    this->RGBTransferFunction[index]->AddRGBPoint(      0, 0.0, 0.0, 0.0 );
    this->RGBTransferFunction[index]->AddRGBPoint(   1024, 1.0, 1.0, 1.0 );
    if (this->ColorChannels[index] != 3)
    {
      this->ColorChannels[index] = 3;
    }
    this->Modified();
  }

  return this->RGBTransferFunction[index];
}

// Set the scalar opacity of a volume to a transfer function
void vtkVolumeProperty::SetScalarOpacity( int index, vtkPiecewiseFunction *function )
{
  if ( this->ScalarOpacity[index] != function )
  {
    if (this->ScalarOpacity[index] != NULL)
    {
      this->ScalarOpacity[index]->UnRegister(this);
    }
    this->ScalarOpacity[index] = function;
    if (this->ScalarOpacity[index] != NULL)
    {
      this->ScalarOpacity[index]->Register(this);
    }

    this->ScalarOpacityMTime[index].Modified();
    this->Modified();
  }
}

// Get the scalar opacity transfer function. Create one if none set.
vtkPiecewiseFunction *vtkVolumeProperty::GetScalarOpacity( int index )
{
  if( this->ScalarOpacity[index] == NULL )
  {
    this->ScalarOpacity[index] = vtkPiecewiseFunction::New();
    this->ScalarOpacity[index]->Register(this);
    this->ScalarOpacity[index]->Delete();
    this->ScalarOpacity[index]->AddPoint(    0, 1.0 );
    this->ScalarOpacity[index]->AddPoint( 1024, 1.0 );
  }

  return this->ScalarOpacity[index];
}

void vtkVolumeProperty::SetScalarOpacityUnitDistance( int index, double distance )
{
  if ( index < 0 || index > 3 )
  {
    vtkErrorMacro("Bad index - must be between 0 and 3");
    return;
  }

  if ( this->ScalarOpacityUnitDistance[index] != distance )
  {
    this->ScalarOpacityUnitDistance[index] = distance;
    this->Modified();
  }
}

double vtkVolumeProperty::GetScalarOpacityUnitDistance( int index )
{
  if ( index < 0 || index > 3 )
  {
    vtkErrorMacro("Bad index - must be between 0 and 3");
    return 0;
  }

  return  this->ScalarOpacityUnitDistance[index];
}


// Set the gradient opacity transfer function
void vtkVolumeProperty::SetGradientOpacity( int index, vtkPiecewiseFunction *function )
{
  if ( this->GradientOpacity[index] != function )
  {
    if (this->GradientOpacity[index] != NULL)
    {
      this->GradientOpacity[index]->UnRegister(this);
    }
    this->GradientOpacity[index]       = function;
    if (this->GradientOpacity[index] != NULL)
    {
      this->GradientOpacity[index]->Register(this);
    }

    this->GradientOpacityMTime[index].Modified();
    this->Modified();
  }
}

void vtkVolumeProperty::CreateDefaultGradientOpacity( int index )
{
  if ( this->DefaultGradientOpacity[index] == NULL )
  {
    this->DefaultGradientOpacity[index] = vtkPiecewiseFunction::New();
    this->DefaultGradientOpacity[index]->Register(this);
    this->DefaultGradientOpacity[index]->Delete();
  }

  this->DefaultGradientOpacity[index]->RemoveAllPoints();
  this->DefaultGradientOpacity[index]->AddPoint(   0, 1.0 );
  this->DefaultGradientOpacity[index]->AddPoint( 255, 1.0 );
}

vtkPiecewiseFunction *vtkVolumeProperty::GetGradientOpacity( int index )
{
  if (this->DisableGradientOpacity[index])
  {
    if ( this->DefaultGradientOpacity[index] == NULL )
    {
      this->CreateDefaultGradientOpacity(index);
    }
    return this->DefaultGradientOpacity[index];
  }

  return this->GetStoredGradientOpacity(index);
}

// Get the gradient opacity transfer function. Create one if none set.
vtkPiecewiseFunction *vtkVolumeProperty::GetStoredGradientOpacity( int index )
{
  if ( this->GradientOpacity[index] == NULL )
  {
    this->GradientOpacity[index] = vtkPiecewiseFunction::New();
    this->GradientOpacity[index]->Register(this);
    this->GradientOpacity[index]->Delete();
    this->GradientOpacity[index]->AddPoint(   0, 1.0 );
    this->GradientOpacity[index]->AddPoint( 255, 1.0 );
  }

  return this->GradientOpacity[index];
}

void vtkVolumeProperty::SetDisableGradientOpacity( int index, int value )
{
  if (this->DisableGradientOpacity[index] == value)
  {
    return;
  }

  this->DisableGradientOpacity[index] = value;

  // Make sure the default function is up-to-date (since the user
  // could have modified the default function)

  if (value)
  {
    this->CreateDefaultGradientOpacity(index);
  }

  // Since this Ivar basically "sets" the gradient opacity function to be
  // either a default one or the user-specified one, update the MTime
  // accordingly

  this->GradientOpacityMTime[index].Modified();

  this->Modified();
}

int vtkVolumeProperty::GetDisableGradientOpacity( int index )
{
  return this->DisableGradientOpacity[index];
}

void vtkVolumeProperty::SetComponentWeight(int index, double value)
{
  if (index < 0 || index >= VTK_MAX_VRCOMP)
  {
    vtkErrorMacro("Invalid index");
    return;
  }

  double val = value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
  if (this->ComponentWeight[index] != val)
  {
    this->ComponentWeight[index] = val;
    this->Modified();
  }
}

double vtkVolumeProperty::GetComponentWeight(int index)
{
  if (index < 0 || index >= VTK_MAX_VRCOMP)
  {
    vtkErrorMacro("Invalid index");
    return 0.0;
  }

  return this->ComponentWeight[index];
}

void vtkVolumeProperty::SetShade( int index, int value )
{
  if ( value != 0 && value != 1 )
  {
    vtkErrorMacro("SetShade accepts values 0 or 1");
    return;
  }

  if ( this->Shade[index] != value )
  {
    this->Shade[index] = value;
    this->Modified();
  }
}

void vtkVolumeProperty::ShadeOn( int index )
{
  this->SetShade( index, 1 );
}


void vtkVolumeProperty::ShadeOff( int index )
{
  this->SetShade( index, 0 );
}


int vtkVolumeProperty::GetShade( int index )
{
  return this->Shade[index];
}

void vtkVolumeProperty::SetAmbient( int index, double value )
{
  if ( this->Ambient[index] != value )
  {
    this->Ambient[index] = value;
    this->Modified();
  }
}

double vtkVolumeProperty::GetAmbient( int index )
{
  return this->Ambient[index];
}

void vtkVolumeProperty::SetDiffuse( int index, double value )
{
  if ( this->Diffuse[index] != value )
  {
    this->Diffuse[index] = value;
    this->Modified();
  }
}

double vtkVolumeProperty::GetDiffuse( int index )
{
  return this->Diffuse[index];
}

void vtkVolumeProperty::SetSpecular( int index, double value )
{
  if ( this->Specular[index] != value )
  {
    this->Specular[index] = value;
    this->Modified();
  }
}

double vtkVolumeProperty::GetSpecular( int index )
{
  return this->Specular[index];
}

void vtkVolumeProperty::SetSpecularPower( int index, double value )
{
  if ( this->SpecularPower[index] != value )
  {
    this->SpecularPower[index] = value;
    this->Modified();
  }
}

double vtkVolumeProperty::GetSpecularPower( int index )
{
  return this->SpecularPower[index];
}

vtkTimeStamp vtkVolumeProperty::GetScalarOpacityMTime( int index )
{
  return this->ScalarOpacityMTime[index];
}

vtkTimeStamp vtkVolumeProperty::GetGradientOpacityMTime( int index )
{
  return this->GradientOpacityMTime[index];
}

vtkTimeStamp vtkVolumeProperty::GetRGBTransferFunctionMTime( int index )
{
  return this->RGBTransferFunctionMTime[index];
}

vtkTimeStamp vtkVolumeProperty::GetGrayTransferFunctionMTime( int index )
{
  return this->GrayTransferFunctionMTime[index];
}

// Print the state of the volume property.
void vtkVolumeProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Independent Components: " <<
    (this->IndependentComponents ? "On\n" : "Off\n");

  os << indent << "Interpolation Type: "
     << this->GetInterpolationTypeAsString() << "\n";

  for ( int i = 0; i < VTK_MAX_VRCOMP; i++ )
  {
    os << indent << "Properties for material " << i << endl;

    os << indent << "Color Channels: " << this->ColorChannels[i] << "\n";

    if( this->ColorChannels[i] == 1 )
    {
      os << indent << "Gray Color Transfer Function: "
         << this->GrayTransferFunction[i] << "\n";
    }
    else if( this->ColorChannels[i] == 3 )
    {
      os << indent << "RGB Color Transfer Function: "
         << this->RGBTransferFunction[i] << "\n";
    }

    os << indent << "Scalar Opacity Transfer Function: "
       << this->ScalarOpacity[i] << "\n";

    os << indent << "Gradient Opacity Transfer Function: "
       << this->GradientOpacity[i] << "\n";

    os << indent << "DisableGradientOpacity: "
       << (this->DisableGradientOpacity[i] ? "On" : "Off") << "\n";


    os << indent << "ComponentWeight: "
       << this->ComponentWeight[i] << "\n";

    os << indent << "Shade: " << this->Shade[i] << "\n";
    os << indent << indent << "Ambient: " << this->Ambient[i] << "\n";
    os << indent << indent << "Diffuse: " << this->Diffuse[i] << "\n";
    os << indent << indent << "Specular: " << this->Specular[i] << "\n";
    os << indent << indent << "SpecularPower: " << this->SpecularPower[i] << "\n";
  }

  // These variables should not be printed to the user:
  // this->GradientOpacityMTime
  // this->GrayTransferFunctionMTime
  // this->RGBTransferFunctionMTime
  // this->ScalarOpacityMTime

}

