
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

}

// Description:
// Set the color of a volume to a gray transfer function
void vtkVolumeProperty::SetColor( vtkPiecewiseFunction *function )
{
  if( this->GrayTransferFunction != function )
    {
    this->GrayTransferFunction	= function;
    this->ColorChannels		= 1;

    this->GrayTransferFunctionMTime.Modified();
    this->Modified();
    }
}

// Description:
// Set the color of a volume to an RGB transfer function
void vtkVolumeProperty::SetColor( vtkColorTransferFunction *function )
{
  if( this->RGBTransferFunction != function )
    {
    this->RGBTransferFunction	= function;
    this->ColorChannels		= 3;

    this->RGBTransferFunctionMTime.Modified();
    this->Modified();
    }
}

// Description:
// Set the opacity of a volume to a transfer function
void vtkVolumeProperty::SetOpacity( vtkPiecewiseFunction *function )
{
  if( this->OpacityTransferFunction != function )
    {
    this->OpacityTransferFunction	= function;

    this->OpacityTransferFunctionMTime.Modified();
    this->Modified();
    }
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

