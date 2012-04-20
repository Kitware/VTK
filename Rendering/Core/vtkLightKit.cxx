/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightKit.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLightKit.h"

#include "vtkObjectFactory.h"
#include "vtkLight.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkLightKit);

static const char *vtkLightKitTypeStrings[] = {
  "KeyLight",
  "FillLight",
  "BackLight",
  "HeadLight",
  NULL
};

static const char *vtkLightKitSubTypeStrings[] = {
  "Warmth",
  "Intensity",
  "Elevation",
  "Azimuth",
  "K:F Ratio",
  "K:B Ratio",
  "K:H Ratio",
  NULL
};

// These are the same as vtkLightKitSubTypeStrings but shorter
// useful for a GUI with minimum space
static const char *vtkLightKitSubTypeShortStrings[] = {
  "War.",
  "Int. ",
  "Ele.",
  "Azi.",
  "K:F",
  "K:B",
  "K:H",
  NULL
};


//----------------------------------------------------------------------------
vtkLightKit::vtkLightKit()
{
  // create members
  this->KeyLight = vtkLight::New();
  this->FillLight = vtkLight::New();
  this->HeadLight = vtkLight::New();
  this->BackLight0 = vtkLight::New();
  this->BackLight1 = vtkLight::New();

  for(int i = 0; i < 4; i++)
    {
    this->WarmthFunction[i] = vtkPiecewiseFunction::New();
    }
  this->InitializeWarmthFunctions();

  // initialize values
  this->KeyLight->SetLightTypeToCameraLight();
  this->FillLight->SetLightTypeToCameraLight();
  this->BackLight0->SetLightTypeToCameraLight();
  this->BackLight1->SetLightTypeToCameraLight();

  this->HeadLight->SetLightTypeToHeadlight();

  this->SetKeyLightAngle(50.0, 10.0);
  this->SetFillLightAngle(-75.0, -10.0);
  this->SetBackLightAngle(0.0, 110.0);

  this->KeyLightWarmth  = 0.6;
  this->FillLightWarmth = 0.4;
  this->HeadLightWarmth = 0.5;
  this->BackLightWarmth = 0.5;

  this->KeyLightIntensity = 0.75;
  this->KeyToFillRatio = 3.0;
  this->KeyToBackRatio = 3.5;
  this->KeyToHeadRatio = 3.0;

  this->MaintainLuminance = 0;
  this->Update();
}

//----------------------------------------------------------------------------
vtkLightKit::~vtkLightKit()
{
  this->KeyLight->Delete();
  this->FillLight->Delete();
  this->HeadLight->Delete();
  this->BackLight0->Delete();
  this->BackLight1->Delete();

  for(int i = 0; i < 4; i++)
    {
    this->WarmthFunction[i]->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkLightKit::SetKeyLightAngle(double elevation, double azimuth)
{
  this->KeyLightAngle[0] = elevation;
  this->KeyLightAngle[1] = azimuth;

  this->KeyLight->SetDirectionAngle(elevation, azimuth);
}

//----------------------------------------------------------------------------
void vtkLightKit::SetFillLightAngle(double elevation, double azimuth)
{
  this->FillLightAngle[0] = elevation;
  this->FillLightAngle[1] = azimuth;

  this->FillLight->SetDirectionAngle(elevation, azimuth);
}

//----------------------------------------------------------------------------
void vtkLightKit::SetBackLightAngle(double elevation, double azimuth)
{
  this->BackLightAngle[0] = elevation;
  this->BackLightAngle[1] = azimuth;

  this->BackLight0->SetDirectionAngle(elevation, azimuth);
  this->BackLight1->SetDirectionAngle(elevation, -azimuth);
}

//----------------------------------------------------------------------------
void vtkLightKit::WarmthToRGB(double w, double rgb[3])
{
  rgb[0] = this->WarmthFunction[0]->GetValue(w);
  rgb[1] = this->WarmthFunction[1]->GetValue(w);
  rgb[2] = this->WarmthFunction[2]->GetValue(w);
}

//----------------------------------------------------------------------------
double vtkLightKit::WarmthToIntensity(double w)
{
  return this->WarmthFunction[3]->GetValue(w);
}

//----------------------------------------------------------------------------
void vtkLightKit::WarmthToRGBI(double w, double rgb[3], double& i)
{
  rgb[0] = this->WarmthFunction[0]->GetValue(w);
  rgb[1] = this->WarmthFunction[1]->GetValue(w);
  rgb[2] = this->WarmthFunction[2]->GetValue(w);
  i = this->WarmthFunction[3]->GetValue(w);
}

//----------------------------------------------------------------------------
void vtkLightKit::AddLightsToRenderer(vtkRenderer *renderer)
{
  if(renderer != NULL)
    {
    renderer->AddLight(this->HeadLight);
    renderer->AddLight(this->KeyLight);
    renderer->AddLight(this->FillLight);
    renderer->AddLight(this->BackLight0);
    renderer->AddLight(this->BackLight1);
    }
}

//----------------------------------------------------------------------------
void vtkLightKit::RemoveLightsFromRenderer(vtkRenderer *renderer)
{
  if(renderer != NULL)
    {
    renderer->RemoveLight(this->HeadLight);
    renderer->RemoveLight(this->KeyLight);
    renderer->RemoveLight(this->FillLight);
    renderer->RemoveLight(this->BackLight0);
    renderer->RemoveLight(this->BackLight1);
    }
}

//----------------------------------------------------------------------------
void vtkLightKit::Modified()
{
  this->Update();
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
void vtkLightKit::Update()
{
  double *fillLightColor = this->FillLightColor;
  double fillLightPI;

  double *keyLightColor = this->KeyLightColor;
  double keyLightPI;

  double *headlightColor = this->HeadLightColor;
  double headlightPI;

  double *backLightColor = this->BackLightColor;
  double backLightPI;

  double fillLightIntensity, keyLightIntensity, headlightIntensity;
  double backLightIntensity;

  this->WarmthToRGBI(this->KeyLightWarmth,  keyLightColor,  keyLightPI);
  this->WarmthToRGBI(this->FillLightWarmth, fillLightColor, fillLightPI);
  this->WarmthToRGBI(this->HeadLightWarmth, headlightColor, headlightPI);
  this->WarmthToRGBI(this->BackLightWarmth, backLightColor, backLightPI);

  keyLightIntensity = this->KeyLightIntensity;

  fillLightIntensity = keyLightIntensity / this->KeyToFillRatio;
  headlightIntensity = keyLightIntensity / this->KeyToHeadRatio;
  backLightIntensity = keyLightIntensity / this->KeyToBackRatio;

  // This is sort of interesting: the fill light intensity is weighted
  // by the perceptual brightness of the color of each light.  Since
  // the fill light will often be a cooler color than the key light,
  // the bluer color would otherwise seem less bright than the neutral
  // and this bias the key-to-fill ratio.  This factor compensates for
  // this problem.  Note we always do this correction, no matter what
  // the MaintainLuminance flag says.  That flag's for controlling
  // the intensity of the entire scene, not just the fill light.

  if(this->MaintainLuminance)
    {
    fillLightIntensity /= fillLightPI;
    headlightIntensity /= headlightPI;
    keyLightIntensity  /= keyLightPI;
    backLightIntensity /= backLightPI;
    }
  this->KeyLight->SetColor(keyLightColor);
  this->KeyLight->SetIntensity(keyLightIntensity);

  this->FillLight->SetColor(fillLightColor);
  this->FillLight->SetIntensity(fillLightIntensity);

  this->HeadLight->SetColor(headlightColor);
  this->HeadLight->SetIntensity(headlightIntensity);

  this->BackLight0->SetColor(backLightColor);
  this->BackLight0->SetIntensity(backLightIntensity);

  this->BackLight1->SetColor(backLightColor);
  this->BackLight1->SetIntensity(backLightIntensity);
}

//----------------------------------------------------------------------------
void vtkLightKit::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "KeyLightIntensity: " << this->KeyLightIntensity << "\n";
  os << indent << "KeyToFillRatio: " << this->KeyToFillRatio << "\n";
  os << indent << "KeyToHeadRatio: " << this->KeyToHeadRatio << "\n";
  os << indent << "KeyToBackRatio: " << this->KeyToBackRatio << "\n";

  os << indent << "KeyLightWarmth: " << this->KeyLightWarmth << "\n";
  os << indent << "KeyLightAngle: ("
     << this->KeyLightAngle[0] << ", "
     << this->KeyLightAngle[1] << ")\n";

  os << indent << "FillLightWarmth: " << this->FillLightWarmth << "\n";
  os << indent << "FillLightAngle: ("
     << this->FillLightAngle[0] << ", "
     << this->FillLightAngle[1] << ")\n";

  os << indent << "BackLightWarmth: " << this->BackLightWarmth << "\n";
  os << indent << "BackLightAngle: ("
     << this->BackLightAngle[0] << ", "
     << this->BackLightAngle[1] << ")\n";

  os << indent << "HeadLightWarmth: " << this->HeadLightWarmth << "\n";

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

  // os << indent << "HeadLightColor: ("
  //    << this->HeadLightColor[0] << ", "
  //    << this->HeadLightColor[1] << ", "
  //    << this->HeadLightColor[2] << ")\n";

  // os << indent << "BackLightColor: ("
  //    << this->BackLightColor[0] << ", "
  //    << this->BackLightColor[1] << ", "
  //    << this->BackLightColor[2] << ")\n";
}

//----------------------------------------------------------------------------
void vtkLightKit::DeepCopy( vtkLightKit *k )
{
 this->KeyLightIntensity = k->KeyLightIntensity;
 this->KeyToFillRatio = k->KeyToFillRatio;
 this->KeyToHeadRatio = k->KeyToHeadRatio;
 this->KeyToBackRatio = k->KeyToBackRatio;

 this->KeyLightWarmth = k->KeyLightWarmth;
 this->FillLightWarmth = k->FillLightWarmth;
 this->HeadLightWarmth = k->HeadLightWarmth;
 this->BackLightWarmth = k->BackLightWarmth;

 this->KeyLightAngle[0] = k->KeyLightAngle[0];
 this->KeyLightAngle[1] = k->KeyLightAngle[1];

 this->FillLightAngle[0] = k->FillLightAngle[0];
 this->FillLightAngle[1] = k->FillLightAngle[1];

 this->BackLightAngle[0] = k->BackLightAngle[0];
 this->BackLightAngle[1] = k->BackLightAngle[1];

 this->MaintainLuminance = k->MaintainLuminance;

 this->KeyLight->DeepCopy(k->KeyLight);
 this->FillLight->DeepCopy(k->FillLight);
 this->HeadLight->DeepCopy(k->HeadLight);
 this->BackLight0->DeepCopy(k->BackLight0);
 this->BackLight1->DeepCopy(k->BackLight1);
}

//----------------------------------------------------------------------------
// r, g, b, sqrt(color length)
static double warmthTable[] = {
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

//----------------------------------------------------------------------------
void vtkLightKit::InitializeWarmthFunctions()
{
  const int len = sizeof(warmthTable) / sizeof(double) / 4;

  for(int i = 0; i < 4; i++)
    {
    this->WarmthFunction[i]->BuildFunctionFromTable(0.0, 1.0, len,
      &warmthTable[i], 4);
    }
}

//----------------------------------------------------------------------------
const char *vtkLightKit::GetStringFromType(int type)
{
  static const int n = sizeof(vtkLightKitTypeStrings)/sizeof(char*);
  if( type < n )
    {
    return vtkLightKitTypeStrings[type];
    }
  else
    {
    return NULL;
    }
}

//----------------------------------------------------------------------------
const char *vtkLightKit::GetStringFromSubType(int subtype)
{
  static const int n = sizeof(vtkLightKitSubTypeStrings)/sizeof(char*);
  if( subtype < n )
    {
    return vtkLightKitSubTypeStrings[subtype];
    }
  else
    {
    return NULL;
    }
}

//----------------------------------------------------------------------------
const char *vtkLightKit::GetShortStringFromSubType(int subtype)
{
  static const int n = sizeof(vtkLightKitSubTypeShortStrings)/sizeof(char*);
  if( subtype < n )
    {
    return vtkLightKitSubTypeShortStrings[subtype];
    }
  else
    {
    return NULL;
    }
}
//----------------------------------------------------------------------------
vtkLightKit::LightKitSubType vtkLightKit::GetSubType(vtkLightKit::LightKitType type, int i)
{
  //return subtype
  const LightKitSubType KeyLightSubType[4]  = { Warmth, Intensity, Elevation, Azimuth };
  const LightKitSubType FillLightSubType[4] = { Warmth, KFRatio, Elevation, Azimuth };
  const LightKitSubType BackLightSubType[4] = { Warmth, KBRatio, Elevation, Azimuth };
  const LightKitSubType HeadLightSubType[2] = { Warmth, KHRatio };

  LightKitSubType subtype = Warmth; // please VS6
  switch(type)
    {
    case TKeyLight:
      subtype = KeyLightSubType[i];
      break;
    case TFillLight:
      subtype = FillLightSubType[i];
      break;
    case TBackLight:
      subtype = BackLightSubType[i];
      break;
    case THeadLight:
      subtype = HeadLightSubType[i];
      break;
    }

  return subtype;
}
