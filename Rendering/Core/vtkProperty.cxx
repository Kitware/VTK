/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProperty.h"

#include "vtkActor.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTexture.h"

#include <cstdlib>
#include <sstream>

#include <vtksys/SystemTools.hxx>

vtkCxxSetObjectMacro(vtkProperty, Information, vtkInformation);

//----------------------------------------------------------------------------
// Return nullptr if no override is supplied.
vtkObjectFactoryNewMacro(vtkProperty);

// Construct object with object color, ambient color, diffuse color,
// specular color, and edge color white; ambient coefficient=0; diffuse
// coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
// and surface representation. Backface and frontface culling are off.
vtkProperty::vtkProperty()
{
  // Really should initialize all colors including Color[3]
  this->Color[0] = 1;
  this->Color[1] = 1;
  this->Color[2] = 1;

  this->AmbientColor[0] = 1;
  this->AmbientColor[1] = 1;
  this->AmbientColor[2] = 1;

  this->DiffuseColor[0] = 1;
  this->DiffuseColor[1] = 1;
  this->DiffuseColor[2] = 1;

  this->SpecularColor[0] = 1;
  this->SpecularColor[1] = 1;
  this->SpecularColor[2] = 1;

  this->EdgeColor[0] = 0;
  this->EdgeColor[1] = 0;
  this->EdgeColor[2] = 0;

  this->VertexColor[0] = 0.5;
  this->VertexColor[1] = 1.0;
  this->VertexColor[2] = 0.5;

  this->EmissiveFactor[0] = 1.0;
  this->EmissiveFactor[1] = 1.0;
  this->EmissiveFactor[2] = 1.0;

  this->NormalScale = 1.0;
  this->OcclusionStrength = 1.0;
  this->Metallic = 0.0;
  this->Roughness = 0.5;
  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.0;
  this->SpecularPower = 1.0;
  this->Opacity = 1.0;
  this->Interpolation = VTK_GOURAUD;
  this->Representation = VTK_SURFACE;
  this->EdgeVisibility = 0;
  this->VertexVisibility = 0;
  this->BackfaceCulling = 0;
  this->FrontfaceCulling = 0;
  this->PointSize = 1.0;
  this->LineWidth = 1.0;
  this->LineStipplePattern = 0xFFFF;
  this->LineStippleRepeatFactor = 1;
  this->Lighting = true;
  this->RenderPointsAsSpheres = false;
  this->RenderLinesAsTubes = false;

  this->Shading = 0;
  this->MaterialName = nullptr;

  this->Information = vtkInformation::New();
  this->Information->Register(this);
  this->Information->Delete();
}

//----------------------------------------------------------------------------
vtkProperty::~vtkProperty()
{
  this->RemoveAllTextures();
  this->SetMaterialName(nullptr);

  this->SetInformation(nullptr);
}

//----------------------------------------------------------------------------
// Assign one property to another.
void vtkProperty::DeepCopy(vtkProperty* p)
{
  if (p != nullptr)
  {
    this->SetColor(p->GetColor());
    this->SetAmbientColor(p->GetAmbientColor());
    this->SetDiffuseColor(p->GetDiffuseColor());
    this->SetSpecularColor(p->GetSpecularColor());
    this->SetEdgeColor(p->GetEdgeColor());
    this->SetVertexColor(p->GetVertexColor());
    this->SetAmbient(p->GetAmbient());
    this->SetDiffuse(p->GetDiffuse());
    this->SetSpecular(p->GetSpecular());
    this->SetSpecularPower(p->GetSpecularPower());
    this->SetOpacity(p->GetOpacity());
    this->SetInterpolation(p->GetInterpolation());
    this->SetRepresentation(p->GetRepresentation());
    this->SetEdgeVisibility(p->GetEdgeVisibility());
    this->SetVertexVisibility(p->GetVertexVisibility());
    this->SetBackfaceCulling(p->GetBackfaceCulling());
    this->SetFrontfaceCulling(p->GetFrontfaceCulling());
    this->SetPointSize(p->GetPointSize());
    this->SetLineWidth(p->GetLineWidth());
    this->SetLineStipplePattern(p->GetLineStipplePattern());
    this->SetLineStippleRepeatFactor(p->GetLineStippleRepeatFactor());
    this->SetLighting(p->GetLighting());
    this->SetRenderPointsAsSpheres(p->GetRenderPointsAsSpheres());
    this->SetRenderLinesAsTubes(p->GetRenderLinesAsTubes());
    this->SetShading(p->GetShading());

    this->RemoveAllTextures();
    auto iter = p->Textures.begin();
    for (; iter != p->Textures.end(); ++iter)
    {
      this->Textures[iter->first] = iter->second;
    }
    // TODO: need to pass shader variables.
  }
}

//----------------------------------------------------------------------------
void vtkProperty::SetColor(double r, double g, double b)
{
  double newColor[3] = { r, g, b };

  // SetColor is shorthand for "set all colors"
  double* color[4] = { this->Color, this->AmbientColor, this->DiffuseColor, this->SpecularColor };

  // Set colors, and check for changes
  bool modified = false;
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      if (color[i][j] != newColor[j])
      {
        modified = true;
        color[i][j] = newColor[j];
      }
    }
  }

  // Call Modified() if anything changed
  if (modified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkProperty::SetColor(double a[3])
{
  this->SetColor(a[0], a[1], a[2]);
}

//----------------------------------------------------------------------------
void vtkProperty::ComputeCompositeColor(double result[3], double ambient,
  const double ambient_color[3], double diffuse, const double diffuse_color[3], double specular,
  const double specular_color[3])
{
  double norm = 0.0;
  if ((ambient + diffuse + specular) > 0)
  {
    norm = 1.0 / (ambient + diffuse + specular);
  }

  for (int i = 0; i < 3; i++)
  {
    result[i] =
      (ambient * ambient_color[i] + diffuse * diffuse_color[i] + specular * specular_color[i]) *
      norm;
  }
}

//----------------------------------------------------------------------------
// Return composite color of object (ambient + diffuse + specular). Return value
// is a pointer to rgb values.
double* vtkProperty::GetColor()
{
  vtkProperty::ComputeCompositeColor(this->Color, this->Ambient, this->AmbientColor, this->Diffuse,
    this->DiffuseColor, this->Specular, this->SpecularColor);
  return this->Color;
}

//----------------------------------------------------------------------------
// Copy composite color of object (ambient + diffuse + specular) into array
// provided.
void vtkProperty::GetColor(double rgb[3])
{
  this->GetColor();
  rgb[0] = this->Color[0];
  rgb[1] = this->Color[1];
  rgb[2] = this->Color[2];
}

//----------------------------------------------------------------------------
void vtkProperty::GetColor(double& r, double& g, double& b)
{
  this->GetColor();
  r = this->Color[0];
  g = this->Color[1];
  b = this->Color[2];
}

//----------------------------------------------------------------------------
void vtkProperty::SetTexture(const char* name, vtkTexture* tex)
{
  if (tex == nullptr)
  {
    this->RemoveTexture(name);
    return;
  }

  if ((strcmp(name, "albedoTex") == 0 || strcmp(name, "emissiveTex") == 0) &&
    !tex->GetUseSRGBColorSpace())
  {
    vtkErrorMacro("The " << name << " texture is not in sRGB color space.");
    return;
  }
  if ((strcmp(name, "materialTex") == 0 || strcmp(name, "normalTex") == 0) &&
    tex->GetUseSRGBColorSpace())
  {
    vtkErrorMacro("The " << name << " texture is not in linear color space.");
    return;
  }

  auto iter = this->Textures.find(std::string(name));
  if (iter != this->Textures.end())
  {
    // same value?
    if (iter->second == tex)
    {
      return;
    }
    vtkWarningMacro("Texture with name " << name << " exists. It will be replaced.");
    iter->second->UnRegister(this);
  }

  tex->Register(this);
  this->Textures[name] = tex;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkTexture* vtkProperty::GetTexture(const char* name)
{
  auto iter = this->Textures.find(std::string(name));
  if (iter == this->Textures.end())
  {
    return nullptr;
  }

  return iter->second;
}

//----------------------------------------------------------------------------
int vtkProperty::GetNumberOfTextures()
{
  return static_cast<int>(this->Textures.size());
}

//----------------------------------------------------------------------------
void vtkProperty::RemoveTexture(const char* name)
{
  auto iter = this->Textures.find(std::string(name));
  if (iter != this->Textures.end())
  {
    iter->second->UnRegister(this);
    this->Textures.erase(iter);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkProperty::RemoveAllTextures()
{
  while (!this->Textures.empty())
  {
    auto iter = this->Textures.begin();
    iter->second->UnRegister(this);
    this->Textures.erase(iter);
  }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkProperty::Render(vtkActor*, vtkRenderer* renderer)
{
  // subclass would have renderer the property already.
  // this class, just handles the shading.

  if (renderer->GetSelector())
  {
    // nothing to do when rendering for hardware selection.
    return;
  }
}

//----------------------------------------------------------------------------
void vtkProperty::PostRender(vtkActor*, vtkRenderer* renderer)
{
  if (renderer->GetSelector())
  {
    // nothing to do when rendering for hardware selection.
    return;
  }
}

//----------------------------------------------------------------------------
void vtkProperty::AddShaderVariable(const char*, int, int*) {}

//----------------------------------------------------------------------------
void vtkProperty::AddShaderVariable(const char*, int, float*) {}

//----------------------------------------------------------------------------
void vtkProperty::AddShaderVariable(const char*, int, double*) {}

//-----------------------------------------------------------------------------
void vtkProperty::ReleaseGraphicsResources(vtkWindow*)
{
  // vtkOpenGLRenderer releases texture resources, so we don't need to release
  // them here.
}

//----------------------------------------------------------------------------
void vtkProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Ambient: " << this->Ambient << "\n";
  os << indent << "Ambient Color: (" << this->AmbientColor[0] << ", " << this->AmbientColor[1]
     << ", " << this->AmbientColor[2] << ")\n";
  os << indent << "Diffuse: " << this->Diffuse << "\n";
  os << indent << "Diffuse Color: (" << this->DiffuseColor[0] << ", " << this->DiffuseColor[1]
     << ", " << this->DiffuseColor[2] << ")\n";
  os << indent << "Edge Color: (" << this->EdgeColor[0] << ", " << this->EdgeColor[1] << ", "
     << this->EdgeColor[2] << ")\n";
  os << indent << "Edge Visibility: " << (this->EdgeVisibility ? "On\n" : "Off\n");
  os << indent << "Vertex Color: (" << this->VertexColor[0] << ", " << this->VertexColor[1] << ", "
     << this->VertexColor[2] << ")\n";
  os << indent << "Vertex Visibility: " << (this->VertexVisibility ? "On\n" : "Off\n");
  os << indent << "Interpolation: ";
  switch (this->Interpolation)
  {
    case VTK_FLAT:
      os << "VTK_FLAT\n";
      break;
    case VTK_GOURAUD:
      os << "VTK_GOURAUD\n";
      break;
    case VTK_PHONG:
      os << "VTK_PHONG\n";
      break;
    case VTK_PBR:
      os << "VTK_PBR\n";
      break;
    default:
      os << "unknown\n";
  }
  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Representation: ";
  switch (this->Representation)
  {
    case VTK_POINTS:
      os << "VTK_POINTS\n";
      break;
    case VTK_WIREFRAME:
      os << "VTK_WIREFRAME\n";
      break;
    case VTK_SURFACE:
      os << "VTK_SURFACE\n";
      break;
    default:
      os << "unknown\n";
  }
  os << indent << "Specular: " << this->Specular << "\n";
  os << indent << "Specular Color: (" << this->SpecularColor[0] << ", " << this->SpecularColor[1]
     << ", " << this->SpecularColor[2] << ")\n";
  os << indent << "Specular Power: " << this->SpecularPower << "\n";
  os << indent << "Backface Culling: " << (this->BackfaceCulling ? "On\n" : "Off\n");
  os << indent << "Frontface Culling: " << (this->FrontfaceCulling ? "On\n" : "Off\n");
  os << indent << "Point size: " << this->PointSize << "\n";
  os << indent << "Line width: " << this->LineWidth << "\n";
  os << indent << "Line stipple pattern: " << this->LineStipplePattern << "\n";
  os << indent << "Line stipple repeat factor: " << this->LineStippleRepeatFactor << "\n";
  os << indent << "Lighting: ";
  if (this->Lighting)
  {
    os << "On" << endl;
  }
  else
  {
    os << "Off" << endl;
  }
  os << indent << "RenderPointsAsSpheres: " << (this->RenderPointsAsSpheres ? "On" : "Off") << endl;
  os << indent << "RenderLinesAsTubes: " << (this->RenderLinesAsTubes ? "On" : "Off") << endl;

  os << indent << "Shading: " << (this->Shading ? "On" : "Off") << endl;

  os << indent << "MaterialName: " << (this->MaterialName ? this->MaterialName : "(none)") << endl;

  os << indent << "Color: (" << this->Color[0] << ", " << this->Color[1] << ", " << this->Color[2]
     << ")" << endl;
  os << indent << "EmissiveFactor: (" << this->EmissiveFactor[0] << ", " << this->EmissiveFactor[1]
     << ", " << this->EmissiveFactor[2] << ")" << endl;
  os << indent << "NormalScale: " << this->NormalScale << endl;
  os << indent << "OcclusionStrength: " << this->OcclusionStrength << endl;
  os << indent << "Metallic: " << this->Metallic << endl;
  os << indent << "Roughness: " << this->Roughness << endl;
}
