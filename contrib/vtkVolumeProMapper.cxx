/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProMapper.cxx
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
  
  this->CursorXAxisColor[0] = 1.0;
  this->CursorXAxisColor[1] = 0.0;
  this->CursorXAxisColor[2] = 0.0;
  
  this->CursorYAxisColor[0] = 0.0;
  this->CursorYAxisColor[1] = 1.0;
  this->CursorYAxisColor[2] = 0.0;
  
  this->CursorZAxisColor[0] = 0.0;
  this->CursorZAxisColor[1] = 0.0;
  this->CursorZAxisColor[2] = 1.0;
  
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
  this->VolumeBuildTime->Delete();
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

  os << indent << "Cursor X Axis Color: (" << 
    this->CursorXAxisColor[0] << ", " <<
    this->CursorXAxisColor[1] << ", " <<
    this->CursorXAxisColor[0] << ")\n";

  os << indent << "Cursor Y Axis Color: (" << 
    this->CursorYAxisColor[0] << ", " <<
    this->CursorYAxisColor[1] << ", " <<
    this->CursorYAxisColor[0] << ")\n";

  os << indent << "Cursor Z Axis Color: (" << 
    this->CursorZAxisColor[0] << ", " <<
    this->CursorZAxisColor[1] << ", " <<
    this->CursorZAxisColor[0] << ")\n";

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
