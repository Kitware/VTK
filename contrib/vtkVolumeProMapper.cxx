/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProMapper.cxx
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

#include "vtkVolumeProMapper.h"
#include "vtkRenderer.h"
#include "vtkRayCaster.h"

#ifdef VTK_USE_VLI
#include "vtkVolumeProVG500Mapper.h"
#endif

#include "vtkObjectFactory.h"

// Create the mapper. No context has been created, no volume has
// been created yet.
vtkVolumeProMapper::vtkVolumeProMapper()
{
  int               i;

  this->Context = NULL;
  this->Volume = NULL;
  this->VolumeInput = NULL;
  this->VolumeBuildTime = vtkTimeStamp::New();
  this->Lights  = NULL;
  this->NumberOfLights = 0;
  this->BlendMode = VTK_BLEND_MODE_COMPOSITE;
  
  // Disable the subvolume
  for ( i = 0; i < 6; i++ )
    {
    this->SubVolume[i] = -1;
    }

  this->GradientOpacityModulation  = 0;
  this->GradientDiffuseModulation  = 0;
  this->GradientSpecularModulation = 0;

  this->Cursor            = 0;
  this->CursorType        = VTK_CURSOR_TYPE_CROSSHAIR;
  this->CursorPosition[0] = 0.0;
  this->CursorPosition[1] = 0.0;
  this->CursorPosition[2] = 0.0;

  this->CutPlane                   = 0;
  this->CutPlaneEquation[0]        = 1.0;
  this->CutPlaneEquation[1]        = 0.0;
  this->CutPlaneEquation[2]        = 0.0;
  this->CutPlaneEquation[3]        = 0.0;
  this->CutPlaneThickness          = 0.0;
  this->CutPlaneFallOffDistance    = 0;

  this->SuperSampling = 0;
  this->SuperSamplingFactor[0] = 1.0;
  this->SuperSamplingFactor[1] = 1.0;
  this->SuperSamplingFactor[2] = 1.0;

  this->NumberOfBoards      = 0;
  this->MajorBoardVersion   = 0;
  this->MinorBoardVersion   = 0;

  this->NoHardware        = 0;
  this->WrongVLIVersion   = 0;
  this->DisplayedMessage  = 0;

  this->Cut = NULL;
  

}

// Destroy the mapper. Delete the context, volume build time, and the
// volume if necessary
vtkVolumeProMapper::~vtkVolumeProMapper()
{
 
}

// Simplified version - just assume the mapper type
vtkVolumeProMapper *vtkVolumeProMapper::New()
{ 
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVolumeProMapper");
  if(ret)
    {
    return (vtkVolumeProMapper*)ret;
    }
  // if VTK_USE_VLI is defined, then create the vtkVolumeProVG500Mapper
#ifdef VTK_USE_VLI
  // If the factory was unable to create the object, then create it here.
  return vtkVolumeProVG500Mapper::New();
#else
  // if not using vli, then return the stub class, which will render
  // nothing....
  return new vtkVolumeProMapper;
#endif  
}

int vtkVolumeProMapper::StatusOK()
{
  if ( this->NoHardware )
    {
    if ( !this->DisplayedMessage )
      {
      vtkErrorMacro( << "No Hardware Found!" );
      this->DisplayedMessage = 1;
      }
    return 0;
    }

  if ( this->WrongVLIVersion )
    {
    if ( !this->DisplayedMessage )
      {
      vtkErrorMacro( << "Wrong VLI Version found!" );
      this->DisplayedMessage = 1;
      }
    return 0;
    }

  if ( this->Context == NULL )
    {
    return 0;
    }

  if ( this->LookupTable == NULL )
    {
    return 0;
    }

  if ( this->Cut == NULL )
    {
    return 0;
    }

  return 1;
}

void vtkVolumeProMapper::SetSuperSamplingFactor( double x, double y, double z )
{
  if ( x < 0.0 || x > 1.0 ||
       y < 0.0 || y > 1.0 ||
       z < 0.0 || z > 1.0  )
    {
    vtkErrorMacro( << "Invalid supersampling factor" << endl <<
      "Each component must be between 0 and 1" );
    return;
    }

  this->SuperSamplingFactor[0] = x;
  this->SuperSamplingFactor[1] = y;
  this->SuperSamplingFactor[2] = z;

  this->Modified();
}

void vtkVolumeProMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkVolumeMapper::PrintSelf(os,indent);

  // don't print this->SubVolume 
  os << indent << "Number Of Boards: " << this->NumberOfBoards << endl;

  os << indent << "Major Board Version: " << this->MajorBoardVersion << endl;

  os << indent << "Minor Board Version: " << this->MinorBoardVersion << endl;

  os << indent << "Hardware Available: " << 
    (this->NoHardware ? "No\n" : "Yes\n");

  os << indent << "Correct vli Version: " << 
    (this->WrongVLIVersion ? "No\n" : "Yes\n");

  os << indent << "Super Sampling: " <<  
    (this->SuperSampling ? "On\n" : "Off\n");

  os << indent << "Super Sampling Factor: " << 
    this->SuperSamplingFactor[0] << " by " << 
    this->SuperSamplingFactor[1] << " by " << 
    this->SuperSamplingFactor[2] << endl;

  os << indent << "Cursor: " <<  (this->Cursor ? "On\n" : "Off\n");

  os << indent << "Cursor Position: (" << 
    this->CursorPosition[0] << ", " <<
    this->CursorPosition[1] << ", " <<
    this->CursorPosition[0] << ")\n";

  os << indent << "Cursor Type: " << this->GetCursorTypeAsString() << endl;

  os << indent << "Blend Mode: " << this->GetBlendModeAsString() << endl;

  os << indent << "Cut Plane: " << (this->CutPlane ? "On\n" : "Off\n");

  os << indent << "Cut Plane Equation: \n" << indent << "  (" <<
    this->CutPlaneEquation[0] << ")X + (" <<
    this->CutPlaneEquation[1] << ")Y + (" <<
    this->CutPlaneEquation[2] << ")Z + (" <<
    this->CutPlaneEquation[3] << ") = 0\n";

  os << indent << "Cut Plane Thickness " << this->CutPlaneThickness << endl;

  os << indent << "Cut Plane FallOff Distance " << 
    this->CutPlaneFallOffDistance << endl;

  os << indent << "Gradient Opacity Modulation: " <<
    (this->GradientOpacityModulation ? "On\n" : "Off\n");

  os << indent << "Gradient Specular Modulation: " <<
    (this->GradientSpecularModulation ? "On\n" : "Off\n");

  os << indent << "Gradient Diffuse Modulation: " <<
    (this->GradientDiffuseModulation ? "On\n" : "Off\n");
}
