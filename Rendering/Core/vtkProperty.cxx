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
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTexture.h"

#include <cstdlib>
#include <sstream>

#include <map>
#include <vtksys/SystemTools.hxx>

class vtkPropertyInternals
{
public:
  // key==texture unit, value==texture
  typedef std::map<int, vtkSmartPointer<vtkTexture> > MapOfTextures;
  MapOfTextures Textures;

  // key==texture name, value==texture-unit.
  typedef std::map<vtkStdString, int> MapOfTextureNames;
  MapOfTextureNames TextureNames;
};

//----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkProperty)

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
  this->MaterialName = 0;
  this->Internals = new vtkPropertyInternals;
}

//----------------------------------------------------------------------------
vtkProperty::~vtkProperty()
{
  this->SetMaterialName(0);
  delete this->Internals;
}

//----------------------------------------------------------------------------
// Assign one property to another.
void vtkProperty::DeepCopy(vtkProperty *p)
{
  if (p != NULL)
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
    vtkPropertyInternals::MapOfTextures::iterator iter =
      p->Internals->Textures.begin();
    for (;iter != p->Internals->Textures.end(); ++iter)
    {
      this->Internals->Textures[iter->first] = iter->second;
    }
    // TODO: need to pass shader variables.
  }
}

//----------------------------------------------------------------------------
void vtkProperty::SetColor(double r, double g, double b)
{
  double newColor[3] = { r, g, b };

  // SetColor is shorthand for "set all colors"
  double *color[4] = {
    this->Color,
    this->AmbientColor,
    this->DiffuseColor,
    this->SpecularColor
  };

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
void vtkProperty::ComputeCompositeColor(double result[3],
  double ambient, const double ambient_color[3],
  double diffuse, const double diffuse_color[3],
  double specular, const double specular_color[3])
{
  double norm = 0.0;
  if ((ambient + diffuse + specular)>0)
  {
    norm = 1.0 / (ambient + diffuse + specular);
  }

  for (int i = 0; i < 3; i ++)
  {
    result[i] = ( ambient * ambient_color[i] +
                  diffuse * diffuse_color[i] +
                  specular * specular_color[i] ) * norm;
  }
}

//----------------------------------------------------------------------------
// Return composite color of object (ambient + diffuse + specular). Return value
// is a pointer to rgb values.
double *vtkProperty::GetColor()
{
  vtkProperty::ComputeCompositeColor(this->Color,
    this->Ambient, this->AmbientColor,
    this->Diffuse, this->DiffuseColor,
    this->Specular, this->SpecularColor);
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
void vtkProperty::GetColor(double &r, double &g, double &b)
{
  this->GetColor();
  r = this->Color[0];
  g = this->Color[1];
  b = this->Color[2];
}

//----------------------------------------------------------------------------
void vtkProperty::SetTexture(const char* name, vtkTexture* tex)
{
  vtkPropertyInternals::MapOfTextureNames::iterator iter =
    this->Internals->TextureNames.find(vtkStdString(name));
  if (iter != this->Internals->TextureNames.end())
  {
    vtkWarningMacro("Texture with name " << name
      << " exists. It will be replaced.");
  }

  // Locate a free texture unit.
  int texture_unit = -1;
  for (int cc=0; ; cc++)
  {
    if (this->Internals->Textures.find(cc) == this->Internals->Textures.end())
    {
      texture_unit = cc;
      break;
    }
  }

  this->Internals->TextureNames[name] = texture_unit;
  this->SetTexture(texture_unit, tex);
}

//----------------------------------------------------------------------------
vtkTexture* vtkProperty::GetTexture(const char* name)
{
  vtkPropertyInternals::MapOfTextureNames::iterator iter =
    this->Internals->TextureNames.find(vtkStdString(name));
  if (iter == this->Internals->TextureNames.end())
  {
    vtkErrorMacro("No texture with name " << name << " exists.");
    return NULL;
  }

  return this->GetTexture(iter->second);
}

//----------------------------------------------------------------------------
void vtkProperty::SetTexture(int unit, vtkTexture* tex)
{
  vtkPropertyInternals::MapOfTextures::iterator iter =
    this->Internals->Textures.find(unit);
  if (iter != this->Internals->Textures.end())
  {
    vtkWarningMacro("Replacing texture previously assigned to unit " << unit);
  }
  this->Internals->Textures[unit] = tex;
}

//----------------------------------------------------------------------------
vtkTexture* vtkProperty::GetTexture(int unit)
{
  vtkPropertyInternals::MapOfTextures::iterator iter =
    this->Internals->Textures.find(unit);
  if (iter != this->Internals->Textures.end())
  {
    return iter->second.GetPointer();
  }
  vtkErrorMacro("No texture assigned to texture unit " << unit << " exists.");
  return NULL;
}

//----------------------------------------------------------------------------
int vtkProperty::GetNumberOfTextures()
{
  return static_cast<int>(this->Internals->Textures.size());
}

//----------------------------------------------------------------------------
void vtkProperty::RemoveTexture(const char* name)
{
  vtkPropertyInternals::MapOfTextureNames::iterator iter =
    this->Internals->TextureNames.find(vtkStdString(name));
  if (iter != this->Internals->TextureNames.end())
  {
    this->RemoveTexture(iter->second);
    this->Internals->TextureNames.erase(iter);
  }
}

//----------------------------------------------------------------------------
void vtkProperty::RemoveTexture(int unit)
{
  vtkPropertyInternals::MapOfTextures::iterator iter =
    this->Internals->Textures.find(unit);
  if (iter != this->Internals->Textures.end())
  {
    this->Internals->Textures.erase(unit);
  }
}

//----------------------------------------------------------------------------
void vtkProperty::RemoveAllTextures()
{
  this->Internals->TextureNames.clear();
  this->Internals->Textures.clear();
}

//----------------------------------------------------------------------------
vtkTexture* vtkProperty::GetTextureAtIndex(int index)
{
  vtkPropertyInternals::MapOfTextures::iterator iter =
    this->Internals->Textures.begin();
  for (int id = 0; iter != this->Internals->Textures.end(); ++iter, ++id)
  {
    if (id == index)
    {
      return iter->second.GetPointer();
    }
  }

  vtkErrorMacro("No texture at index " << index );
  return 0;
}

//----------------------------------------------------------------------------
int vtkProperty::GetTextureUnitAtIndex(int index)
{
  vtkPropertyInternals::MapOfTextures::iterator iter =
    this->Internals->Textures.begin();
  for (int id = 0; iter != this->Internals->Textures.end(); ++iter, ++id)
  {
    if (id == index)
    {
      return iter->first;
    }
  }

  vtkErrorMacro("No texture at index " << index );
  return -1;
}

//----------------------------------------------------------------------------
int vtkProperty::GetTextureUnit(const char* name)
{
  vtkPropertyInternals::MapOfTextureNames::iterator iter =
    this->Internals->TextureNames.find(name);
  if (iter != this->Internals->TextureNames.end())
  {
    return iter->second;
  }

  vtkErrorMacro("No texture with name " << name);
  return -1;
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
void vtkProperty::AddShaderVariable(const char*, int, int*)
{
}

//----------------------------------------------------------------------------
void vtkProperty::AddShaderVariable(const char*, int, float*)
{
}

//----------------------------------------------------------------------------
void vtkProperty::AddShaderVariable(const char*, int, double*)
{
}

//-----------------------------------------------------------------------------
void vtkProperty::ReleaseGraphicsResources(vtkWindow *)
{
  // vtkOpenGLRenderer releases texture resources, so we don't need to release
  // them here.
}

//----------------------------------------------------------------------------
void vtkProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Ambient: " << this->Ambient << "\n";
  os << indent << "Ambient Color: (" << this->AmbientColor[0] << ", "
    << this->AmbientColor[1] << ", " << this->AmbientColor[2] << ")\n";
  os << indent << "Diffuse: " << this->Diffuse << "\n";
  os << indent << "Diffuse Color: (" << this->DiffuseColor[0] << ", "
    << this->DiffuseColor[1] << ", " << this->DiffuseColor[2] << ")\n";
  os << indent << "Edge Color: (" << this->EdgeColor[0] << ", "
    << this->EdgeColor[1] << ", " << this->EdgeColor[2] << ")\n";
  os << indent << "Edge Visibility: "
    << (this->EdgeVisibility ? "On\n" : "Off\n");
  os << indent << "Vertex Color: (" << this->VertexColor[0] << ", "
    << this->VertexColor[1] << ", " << this->VertexColor[2] << ")\n";
  os << indent << "Vertex Visibility: "
    << (this->VertexVisibility ? "On\n" : "Off\n");
  os << indent << "Interpolation: ";
  switch (this->Interpolation)
  {
  case VTK_FLAT: os << "VTK_FLAT\n"; break;
  case VTK_GOURAUD: os << "VTK_GOURAUD\n"; break;
  case VTK_PHONG: os << "VTK_PHONG\n"; break;
  default: os << "unknown\n";
  }
  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Representation: ";
  switch (this->Representation)
  {
  case VTK_POINTS: os << "VTK_POINTS\n"; break;
  case VTK_WIREFRAME: os << "VTK_WIREFRAME\n"; break;
  case VTK_SURFACE: os << "VTK_SURFACE\n"; break;
  default: os << "unknown\n";
  }
  os << indent << "Specular: " << this->Specular << "\n";
  os << indent << "Specular Color: (" << this->SpecularColor[0] << ", "
    << this->SpecularColor[1] << ", " << this->SpecularColor[2] << ")\n";
  os << indent << "Specular Power: " << this->SpecularPower << "\n";
  os << indent << "Backface Culling: "
    << (this->BackfaceCulling ? "On\n" : "Off\n");
  os << indent << "Frontface Culling: "
    << (this->FrontfaceCulling ? "On\n" : "Off\n");
  os << indent << "Point size: " << this->PointSize << "\n";
  os << indent << "Line width: " << this->LineWidth << "\n";
  os << indent << "Line stipple pattern: " << this->LineStipplePattern << "\n";
  os << indent << "Line stipple repeat factor: " << this->LineStippleRepeatFactor << "\n";
  os << indent << "Lighting: ";
  if(this->Lighting)
  {
    os << "On" << endl;
  }
  else
  {
    os << "Off" << endl;
  }
  os << indent << "RenderPointsAsSpheres: "
     << (this->RenderPointsAsSpheres ? "On" : "Off") << endl;
  os << indent << "RenderLinesAsTubes: "
     << (this->RenderLinesAsTubes ? "On" : "Off") << endl;

  os << indent << "Shading: "
    << (this->Shading? "On" : "Off") << endl;

 os << indent << "MaterialName: " <<
   (this->MaterialName? this->MaterialName:"(none)") << endl;
}
