/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProperty.cxx
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

#include "vtkVolumeProperty.h"

// Description:
// Construct a new vtkVolumeProperty with default values
vtkVolumeProperty::vtkVolumeProperty()
{
  this->InterpolationType		= VTK_NEAREST_INTERPOLATION;

  this->ColorChannels			= 1;

  this->GrayTransferFunction		= NULL;
  this->RGBTransferFunction		= NULL;
  this->OpacityTransferFunction		= NULL;

  this->SelfCreatedGTFun		= 0;
  this->SelfCreatedRGBTFun		= 0;
  this->SelfCreatedOTFun		= 0;

  this->Shade				= 0;  // False
  this->Ambient				= 0.1;
  this->Diffuse				= 0.7;
  this->Specular			= 0.2;
  this->SpecularPower			= 10.0;
}

// Description:
// Destruct a vtkVolumeProperty
vtkVolumeProperty::~vtkVolumeProperty()
{
  if( this->SelfCreatedGTFun && this->GrayTransferFunction )
    this->GrayTransferFunction->Delete();

  if( this->SelfCreatedRGBTFun && this->RGBTransferFunction )
    this->RGBTransferFunction->Delete();

  if( this->SelfCreatedOTFun && this->OpacityTransferFunction )
    this->OpacityTransferFunction->Delete();
}

// Description:
// Set the color of a volume to a gray transfer function
void vtkVolumeProperty::SetColor( vtkPiecewiseFunction *function )
{
  if( this->GrayTransferFunction != function )
    {
    if( this->SelfCreatedGTFun )
      this->GrayTransferFunction->Delete();

    this->SelfCreatedGTFun      = 0;
    this->GrayTransferFunction	= function;
    this->GrayTransferFunctionMTime.Modified();
    this->Modified();
    }

  if ( this->ColorChannels != 1 )
    {
    this->ColorChannels		= 1;
    this->Modified();
    }
}

// Description:
// Get the currently set gray transfer function. Create one if none set.
vtkPiecewiseFunction *vtkVolumeProperty::GetGrayTransferFunction()
{
  if( this->GrayTransferFunction == NULL )
    {
    this->GrayTransferFunction = vtkPiecewiseFunction::New();
    this->GrayTransferFunction->AddPoint(    0, 0.0 );
    this->GrayTransferFunction->AddPoint( 1024, 1.0 );
    this->SelfCreatedGTFun = 1;
    }

  return this->GrayTransferFunction;
}

// Description:
// Set the color of a volume to an RGB transfer function
void vtkVolumeProperty::SetColor( vtkColorTransferFunction *function )
{
  if( this->RGBTransferFunction != function )
    {
    if( this->SelfCreatedRGBTFun )
      this->RGBTransferFunction->Delete();

    this->SelfCreatedRGBTFun    = 0;
    this->RGBTransferFunction	= function;
    this->RGBTransferFunctionMTime.Modified();
    this->Modified();
    }

  if ( this->ColorChannels != 3 )
    {
    this->ColorChannels		= 3;
    this->Modified();
    }
}

// Description:
// Get the currently set RGB transfer function. Create one if none set.
vtkColorTransferFunction *vtkVolumeProperty::GetRGBTransferFunction()
{
  if( this->RGBTransferFunction == NULL )
    {
    this->RGBTransferFunction = vtkColorTransferFunction::New();
    this->RGBTransferFunction->AddRedPoint(      0, 0.0 );
    this->RGBTransferFunction->AddRedPoint(   1024, 1.0 );
    this->RGBTransferFunction->AddGreenPoint(    0, 0.0 );
    this->RGBTransferFunction->AddGreenPoint( 1024, 1.0 );
    this->RGBTransferFunction->AddBluePoint(     0, 0.0 );
    this->RGBTransferFunction->AddBluePoint(  1024, 1.0 );
    this->SelfCreatedRGBTFun = 1;
    }

  return this->RGBTransferFunction;
}

// Description:
// Set the opacity of a volume to a transfer function
void vtkVolumeProperty::SetOpacity( vtkPiecewiseFunction *function )
{
  if( this->OpacityTransferFunction != function )
    {
    if( this->SelfCreatedOTFun )
      this->OpacityTransferFunction->Delete();

    this->SelfCreatedOTFun		= 0;
    this->OpacityTransferFunction	= function;

    this->OpacityTransferFunctionMTime.Modified();
    this->Modified();
    }
}

// Description:
// Get the currently set opacity transfer function. Create one if none set.
vtkPiecewiseFunction *vtkVolumeProperty::GetOpacityTransferFunction()
{
  if( this->OpacityTransferFunction == NULL )
    {
    this->OpacityTransferFunction = vtkPiecewiseFunction::New();
    this->OpacityTransferFunction->AddPoint(    0, 0.0 );
    this->OpacityTransferFunction->AddPoint( 1024, 1.0 );
    this->SelfCreatedOTFun = 1;
    }

  return this->OpacityTransferFunction;
}

// Description:
// Print the state of the volume property.
void vtkVolumeProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Interpolation Type: "
     << this->GetInterpolationTypeAsString() << "\n";

  os << indent << "Color Channels: " << this->ColorChannels << "\n";

  if( this->ColorChannels == 1 )
    {
      os << indent << "Gray Color Transfer Function: " \
	 << this->GrayTransferFunction << "\n";
    }
  else if( this->ColorChannels == 3 )
    {
      os << indent << "RGB Color Transfer Function: " \
	 << this->RGBTransferFunction << "\n";
    }

  os << indent << "Opacity Transfer Function: " \
     << this->OpacityTransferFunction << "\n";

  os << indent << "Shade: " << this->Shade << "\n";

  if( this->Shade )
    {
    os << indent << indent << "Ambient: " << this->Ambient << "\n";
    os << indent << indent << "Diffuse: " << this->Diffuse << "\n";
    os << indent << indent << "Specular: " << this->Specular << "\n";
    os << indent << indent << "SpecularPower: " << this->SpecularPower << "\n";
    }
}

