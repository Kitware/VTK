/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightKit.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Michael Halle, Brigham and Women's Hospital


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
/* vtkLightKit
*/

#include <math.h>
#include "vtkLightKit.h"
#include "vtkLight.h"
#include "vtkPiecewiseFunction.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"

//------------------------------------------------------------------------------
vtkLightKit* vtkLightKit::New() {
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLightKit");
  if(ret)
    {
    return (vtkLightKit*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLightKit;
}

vtkLightKit::vtkLightKit() {
  // create members
  this->KeyLight = vtkLight::New();
  this->FillLight = vtkLight::New();
  this->Headlight = vtkLight::New();

  for(int i = 0; i < 4; i++) {
    this->WarmthFunction[i] = vtkPiecewiseFunction::New();
  }
  this->InitializeWarmthFunctions();

  // initialize values
  this->KeyLight->SetLightTypeToCameraLight();
  this->FillLight->SetLightTypeToCameraLight();
  this->Headlight->SetLightTypeToHeadlight();

  this->SetKeyLightAngle(50.0, 10.0);
  this->SetFillLightAngle(-75.0, -10.0);

  this->KeyLightWarmth  = 0.6;
  this->FillLightWarmth = 0.4;
  this->HeadlightWarmth = 0.5;

  this->KeyLightIntensity = 1.0;
  this->KeyToFillRatio = 5.0;
  this->KeyToHeadRatio = 7.0;

  this->MaintainLuminance = 0;

  // push initial values out
  this->Modified();
}  

vtkLightKit::~vtkLightKit() {

  if(this->KeyLight != NULL) {
    this->KeyLight->UnRegister(this);
    this->KeyLight = NULL;
  }

  if(this->FillLight != NULL) {
    this->FillLight->UnRegister(this);
    this->FillLight = NULL;
  }

  if(this->Headlight != NULL) {
    this->Headlight->UnRegister(this);
    this->Headlight = NULL;
  }

  for(int i = 0; i < 4; i++) {
    this->WarmthFunction[i]->Delete();
  }
}

void vtkLightKit::SetKeyLightAngle(float elevation, float azimuth) {
  this->KeyLightAngle[0] = elevation;
  this->KeyLightAngle[1] = azimuth;

  this->KeyLight->SetDirectionAngle(elevation, azimuth);
}

void vtkLightKit::SetFillLightAngle(float elevation, float azimuth) {
  this->FillLightAngle[0] = elevation;
  this->FillLightAngle[1] = azimuth;

  this->FillLight->SetDirectionAngle(elevation, azimuth);
}

void vtkLightKit::WarmthToRGB(float w, float rgb[3]) {
  rgb[0] = this->WarmthFunction[0]->GetValue(w);
  rgb[1] = this->WarmthFunction[1]->GetValue(w);
  rgb[2] = this->WarmthFunction[2]->GetValue(w);
}

float vtkLightKit::WarmthToIntensity(float w) {
  return this->WarmthFunction[3]->GetValue(w);
}

void vtkLightKit::WarmthToRGBI(float w, float rgb[3], float& i) {
  rgb[0] = this->WarmthFunction[0]->GetValue(w);
  rgb[1] = this->WarmthFunction[1]->GetValue(w);
  rgb[2] = this->WarmthFunction[2]->GetValue(w);
  i = this->WarmthFunction[3]->GetValue(w);
}

void vtkLightKit::AddLightsToRenderer(vtkRenderer *renderer) {
  if(renderer != NULL) {
    renderer->AddLight(this->Headlight);
    renderer->AddLight(this->KeyLight);
    renderer->AddLight(this->FillLight);
  }
}

void vtkLightKit::RemoveLightsFromRenderer(vtkRenderer *renderer) {
  if(renderer != NULL) {
    renderer->RemoveLight(this->Headlight);
    renderer->RemoveLight(this->KeyLight);
    renderer->RemoveLight(this->FillLight);
  }
}

void vtkLightKit::Modified() {
  this->Update();
  this->MTime.Modified();
}

void vtkLightKit::Update() {

  float *fillLightColor = this->FillLightColor;
  float fillLightPI;

  float *keyLightColor = this->KeyLightColor;
  float keyLightPI;

  float *headlightColor = this->HeadlightColor;
  float headlightPI;

  float fillLightIntensity, keyLightIntensity, headlightIntensity;

  this->WarmthToRGBI(this->KeyLightWarmth,  keyLightColor,  keyLightPI);
  this->WarmthToRGBI(this->FillLightWarmth, fillLightColor, fillLightPI);
  this->WarmthToRGBI(this->HeadlightWarmth, headlightColor, headlightPI);

  keyLightIntensity = this->KeyLightIntensity;

  fillLightIntensity = keyLightIntensity / this->KeyToFillRatio;
  headlightIntensity = keyLightIntensity / this->KeyToHeadRatio;

  // This is sort of interesting: the fill light intensity is weighted
  // by the perceptual brightness of the color of each light.  Since
  // the fill light will often be a cooler color than the key light,
  // the bluer color would otherwise seem less bright than the neutral
  // and this bias the key-to-fill ratio.  This factor compensates for
  // this problem.  Note we always do this correction, no matter what
  // the MaintainLuminance flag says.  That flag's for controlling
  // the intensity of the entire scene, not just the fill light.

  if(this->MaintainLuminance) {
    fillLightIntensity /= fillLightPI;
    headlightIntensity /= headlightPI;
    keyLightIntensity  /= keyLightPI;
  } 
  this->KeyLight->SetColor(keyLightColor);
  this->KeyLight->SetIntensity(keyLightIntensity);
  
  this->FillLight->SetColor(fillLightColor);
  this->FillLight->SetIntensity(fillLightIntensity);

  this->Headlight->SetColor(headlightColor);
  this->Headlight->SetIntensity(headlightIntensity);
}

void vtkLightKit::PrintSelf(ostream& os, vtkIndent indent) {
  vtkObject::PrintSelf(os,indent);

  os << indent << "KeyLightIntensity: " << this->KeyLightIntensity << "\n";
  os << indent << "KeyToFillRatio: " << this->KeyToFillRatio << "\n";
  os << indent << "KeyToHeadRatio: " << this->KeyToHeadRatio << "\n";

  os << indent << "KeyLightWarmth: " << this->KeyLightWarmth << "\n";
  os << indent << "KeyLightAngle: (" 
     << this->KeyLightAngle[0] << ", "
     << this->KeyLightAngle[1] << ")\n";

  os << indent << "FillLightWarmth: " << this->FillLightWarmth << "\n";
  os << indent << "FillLightAngle: (" 
     << this->FillLightAngle[0] << ", "
     << this->FillLightAngle[1] << ")\n";

  os << indent << "HeadlightWarmth: " << this->HeadlightWarmth << "\n";

  os << indent << "MaintainLuminance: " << 
    (this->MaintainLuminance ? "On" : "Off") << "\n";

  // here, but commented out to satisfy validation tests....
  // os << indent << "KeyLightColor: (" 
  //    << this->KeyLightColor[0] << ", " 
  //    << this->KeyLightColor[1] << ", " 
  //    << this->KeyLightColor[2] << ")\n";

  // os << indent << "FillLightColor: (" 
  //    << this->FillLightColor[0] << ", " 
  //    << this->FillLightColor[1] << ", " 
  //    << this->FillLightColor[2] << ")\n";

  // os << indent << "HeadlightColor: (" 
  //    << this->HeadlightColor[0] << ", " 
  //    << this->HeadlightColor[1] << ", " 
  //    << this->HeadlightColor[2] << ")\n";
}

void vtkLightKit::DeepCopy( vtkLightKit *k ) {
 this->KeyLightIntensity = k->KeyLightIntensity;
 this->KeyToFillRatio = k->KeyToFillRatio;
 this->KeyToHeadRatio = k->KeyToHeadRatio;

 this->KeyLightWarmth = k->KeyLightWarmth;
 this->FillLightWarmth = k->FillLightWarmth;
 this->HeadlightWarmth = k->HeadlightWarmth;

 this->KeyLightAngle[0] = k->KeyLightAngle[0];
 this->KeyLightAngle[1] = k->KeyLightAngle[1];

 this->FillLightAngle[0] = k->FillLightAngle[0];
 this->FillLightAngle[1] = k->FillLightAngle[1];

 this->MaintainLuminance = k->MaintainLuminance;

 this->KeyLight->DeepCopy(k->KeyLight);
 this->FillLight->DeepCopy(k->FillLight);
 this->Headlight->DeepCopy(k->Headlight);
}  

// r, g, b, sqrt(color length)
static float warmthTable[] = {
  0.1674, 0.3065, 1.0000, 0.5865,
  0.1798, 0.3204, 1.0000, 0.5965,
  0.1935, 0.3352, 1.0000, 0.6071,
  0.2083, 0.3511, 1.0000, 0.6184,
  0.2245, 0.3679, 1.0000, 0.6302,
  0.2422, 0.3859, 1.0000, 0.6426,
  0.2614, 0.4050, 1.0000, 0.6556,
  0.2822, 0.4252, 1.0000, 0.6693,
  0.3049, 0.4467, 1.0000, 0.6837,
  0.3293, 0.4695, 1.0000, 0.6986,
  0.3557, 0.4935, 1.0000, 0.7142,
  0.3841, 0.5188, 1.0000, 0.7303,
  0.4144, 0.5454, 1.0000, 0.7470,
  0.4468, 0.5731, 1.0000, 0.7642,
  0.4811, 0.6020, 1.0000, 0.7818,
  0.5173, 0.6320, 1.0000, 0.7998,
  0.5551, 0.6628, 1.0000, 0.8179,
  0.5943, 0.6942, 1.0000, 0.8362,
  0.6346, 0.7261, 1.0000, 0.8544,
  0.6756, 0.7581, 1.0000, 0.8724,
  0.7168, 0.7898, 1.0000, 0.8899,
  0.7575, 0.8209, 1.0000, 0.9068,
  0.7972, 0.8508, 1.0000, 0.9229,
  0.8351, 0.8791, 1.0000, 0.9379,
  0.8705, 0.9054, 1.0000, 0.9517,
  0.9026, 0.9290, 1.0000, 0.9640,
  0.9308, 0.9497, 1.0000, 0.9746,
  0.9546, 0.9671, 1.0000, 0.9834,
  0.9734, 0.9808, 1.0000, 0.9903,
  0.9872, 0.9907, 1.0000, 0.9954,
  0.9958, 0.9970, 1.0000, 0.9985,
  0.9996, 0.9997, 1.0000, 0.9999,
  1.0000, 0.9999, 0.9996, 0.9999,
  1.0000, 0.9988, 0.9958, 0.9994,
  1.0000, 0.9964, 0.9871, 0.9982,
  1.0000, 0.9925, 0.9730, 0.9962,
  1.0000, 0.9869, 0.9532, 0.9935,
  1.0000, 0.9796, 0.9275, 0.9898,
  1.0000, 0.9705, 0.8959, 0.9853,
  1.0000, 0.9595, 0.8584, 0.9798,
  1.0000, 0.9466, 0.8150, 0.9734,
  1.0000, 0.9317, 0.7660, 0.9660,
  1.0000, 0.9147, 0.7116, 0.9576,
  1.0000, 0.8956, 0.6522, 0.9482,
  1.0000, 0.8742, 0.5881, 0.9377,
  1.0000, 0.8506, 0.5199, 0.9261,
  1.0000, 0.8247, 0.4483, 0.9134,
  1.0000, 0.7964, 0.3739, 0.8995,
  1.0000, 0.7656, 0.2975, 0.8845,
  1.0000, 0.7324, 0.2201, 0.8683,
  1.0000, 0.6965, 0.1426, 0.8509,
  1.0000, 0.6580, 0.0662, 0.8323,
  1.0000, 0.6179, 0.0000, 0.8134,
  1.0000, 0.5832, 0.0000, 0.8008,
  1.0000, 0.5453, 0.0000, 0.7868,
  1.0000, 0.5042, 0.0000, 0.7713,
  1.0000, 0.4595, 0.0000, 0.7541,
  1.0000, 0.4111, 0.0000, 0.7350,
  1.0000, 0.3588, 0.0000, 0.7139,
  1.0000, 0.3025, 0.0000, 0.6904,
  1.0000, 0.2423, 0.0000, 0.6643,
  1.0000, 0.1782, 0.0000, 0.6353,
  1.0000, 0.1104, 0.0000, 0.6032,
  1.0000, 0.0396, 0.0000, 0.5677,
};

void vtkLightKit::InitializeWarmthFunctions() {

  const int len = sizeof(warmthTable) / sizeof(float) / 4;

  for(int i = 0; i < 4; i++) {
    this->WarmthFunction[i]->BuildFunctionFromTable(0.0, 1.0, len,
						   &warmthTable[i], 4);
  }
}


