/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLight.cxx
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
#include "vtkMath.h"
#include "vtkLight.h"
#include "vtkGraphicsFactory.h"

// Create a light with the focal point at the origin and its position
// set to (0,0,1). The lights color is white, intensity=1, and the light 
// is turned on. 
vtkLight::vtkLight()
{
  this->FocalPoint[0] = 0.0;
  this->FocalPoint[1] = 0.0;
  this->FocalPoint[2] = 0.0;

  this->Position[0] = 0.0;
  this->Position[1] = 0.0;
  this->Position[2] = 1.0;

  this->Color[0] = 1.0;
  this->Color[1] = 1.0;
  this->Color[2] = 1.0;

  this->Switch = 1;

  this->Intensity = 1.0;
  this->Positional = 0;
  this->ConeAngle = 30;
  this->AttenuationValues[0] = 1;
  this->AttenuationValues[1] = 0;
  this->AttenuationValues[2] = 0;
  this->Exponent = 1;

  this->LightType = VTK_LIGHT_TYPE_SCENE_LIGHT;

  this->TransformMatrix = (vtkMatrix4x4 *)NULL;
}

vtkLight::~vtkLight()
{
  if(this->TransformMatrix != NULL)
    {
      this->TransformMatrix->UnRegister(this);
      this->TransformMatrix = NULL;
    }
}

void vtkLight::SetDirectionAngle(float elevation, float azimuth)
{
  elevation *= vtkMath::DegreesToRadians();
  azimuth   *= vtkMath::DegreesToRadians();

  this->SetPosition(cos(elevation)*sin(azimuth), 
		    sin(elevation), 
		    cos(elevation)*cos(azimuth));

  this->SetFocalPoint(0.0, 0.0, 0.0);
  this->SetPositional(0);
}


// return the correct type of light 
vtkLight *vtkLight::New()
{ 
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkLight");
  return (vtkLight*)ret;
}

void vtkLight::SetLightTypeToHeadlight() 
{
  this->LightType = VTK_LIGHT_TYPE_HEADLIGHT;
}

void vtkLight::SetLightTypeToCameraLight() 
{
  this->LightType = VTK_LIGHT_TYPE_CAMERA_LIGHT;
}

void vtkLight::SetLightTypeToSceneLight() 
{
  this->LightType = VTK_LIGHT_TYPE_SCENE_LIGHT;
}

int vtkLight::LightTypeIsHeadlight()
{
  return this->LightType == VTK_LIGHT_TYPE_HEADLIGHT;
}

int vtkLight::LightTypeIsCameraLight()
{
  return this->LightType == VTK_LIGHT_TYPE_CAMERA_LIGHT;
}

int vtkLight::LightTypeIsSceneLight()
{
  return this->LightType == VTK_LIGHT_TYPE_SCENE_LIGHT;
}

void vtkLight::GetTransformedPosition(float a[3]) 
{
  if(this->TransformMatrix)
  {
    float f[4];
    f[0] = this->Position[0];
    f[1] = this->Position[1];
    f[2] = this->Position[2];
    f[3] = 1.0;

    this->TransformMatrix->MultiplyPoint(f, f);

    a[0] = f[0];
    a[1] = f[1];
    a[2] = f[2];
  }
  else
  {
    a[0] = this->Position[0];
    a[1] = this->Position[1];
    a[2] = this->Position[2];
  }
}

void vtkLight::GetTransformedPosition(float &x, float &y, float &z) 
{
  float a[3];

  this->GetTransformedPosition(a);
  x = a[0];
  y = a[1];
  z = a[2];
}

float *vtkLight::GetTransformedPosition() 
{
    this->GetTransformedPosition(this->TransformedPositionReturn);
    return this->TransformedPositionReturn;
}

void vtkLight::GetTransformedFocalPoint(float a[3]) 
{
  if(this->TransformMatrix)
  {
    float f[4];
    f[0] = this->FocalPoint[0];
    f[1] = this->FocalPoint[1];
    f[2] = this->FocalPoint[2];
    f[3] = 1.0;

    this->TransformMatrix->MultiplyPoint(f, f);

    a[0] = f[0];
    a[1] = f[1];
    a[2] = f[2];
  }
  else
  {
    a[0] = this->FocalPoint[0];
    a[1] = this->FocalPoint[1];
    a[2] = this->FocalPoint[2];
  }
}

void vtkLight::GetTransformedFocalPoint(float &x, float &y, float &z)
{
  float a[3];

  this->GetTransformedFocalPoint(a);
  x = a[0];
  y = a[1];
  z = a[2];
}

float *vtkLight::GetTransformedFocalPoint() 
{
  this->GetTransformedFocalPoint(this->TransformedFocalPointReturn);
  return this->TransformedFocalPointReturn;
}

void vtkLight::DeepCopy(vtkLight *light)
{
  this->SetFocalPoint(light->GetFocalPoint());
  this->SetPosition(light->GetPosition());
  this->SetIntensity(light->GetIntensity());
  this->SetColor(light->GetColor());
  this->SetSwitch(light->GetSwitch());
  this->SetPositional(light->GetPositional());
  this->SetExponent(light->GetExponent());
  this->SetConeAngle(light->GetConeAngle());
  this->SetAttenuationValues(light->GetAttenuationValues());
}

void vtkLight::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "AttenuationValues: (" << this->AttenuationValues[0] << ", " 
    << this->AttenuationValues[1] << ", " << this->AttenuationValues[2] << ")\n";
  os << indent << "Color: (" << this->Color[0] << ", " 
    << this->Color[1] << ", " << this->Color[2] << ")\n";
  os << indent << "Cone Angle: " << this->ConeAngle << "\n";
  os << indent << "Exponent: " << this->Exponent << "\n";
  os << indent << "Focal Point: (" << this->FocalPoint[0] << ", " 
    << this->FocalPoint[1] << ", " << this->FocalPoint[2] << ")\n";
  os << indent << "Intensity: " << this->Intensity << "\n";
  os << indent << "Position: (" << this->Position[0] << ", " 
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Positional: " << (this->Positional ? "On\n" : "Off\n");
  os << indent << "Switch: " << (this->Switch ? "On\n" : "Off\n");

  os << indent << "LightType: ";
  if (this->LightType == VTK_LIGHT_TYPE_HEADLIGHT) 
    {
      os << "Headlight\n";
    }
  else if (this->LightType == VTK_LIGHT_TYPE_CAMERA_LIGHT)
    {
      os << "CameraLight\n";
    }
  else if (this->LightType == VTK_LIGHT_TYPE_SCENE_LIGHT)
    {
      os << "SceneLight\n";
    }
  else
    {
      os << "(unknown light type)\n";
    }

  os << indent << "TransformMatrix: ";
  if(this->TransformMatrix != NULL)
    {
      os << this->TransformMatrix << "\n";
    }
  else
    {
      os << "(none)\n";
    }
}

void vtkLight::WriteSelf(ostream& os)
{
  os << this->FocalPoint[0] << " " << this->FocalPoint[1] << " "
     << this->FocalPoint[2] << " ";
  os << this->Position[0] << " " << this->Position[1] << " "
     << this->Position[2] << " ";
  os << this->Intensity << " ";
  os << this->Color[0] << " " << this->Color[1] << " "
     << this->Color[2] << " ";
  os << this->Switch << " ";
  os << this->Positional << " ";
  os << this->Exponent << " ";
  os << this->ConeAngle << " ";
  os << this->AttenuationValues[0] << " " << this->AttenuationValues[1] << " "
     << this->AttenuationValues[2] << " ";
  // XXX - LightType, TransformMatrix ???
}

void vtkLight::ReadSelf(istream& is)
{
  is >> this->FocalPoint[0] >> this->FocalPoint[1] >> this->FocalPoint[2] ;
  is >> this->Position[0] >> this->Position[1] >> this->Position[2];
  is >> this->Intensity;
  is >> this->Color[0] >> this->Color[1] >> this->Color[2];
  is >> this->Switch;
  is >> this->Positional;
  is >> this->Exponent;
  is >> this->ConeAngle;
  is >> this->AttenuationValues[0] >> this->AttenuationValues[1] 
     >> this->AttenuationValues[2];
  // XXX - LightType, TransformMatrix ???
}



