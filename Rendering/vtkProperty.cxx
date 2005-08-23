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
#include "vtkCollection.h"
#include "vtkBMPReader.h"
#include "vtkGraphicsFactory.h"
#include "vtkImageData.h"
#include "vtkImageReader2.h"
#include "vtkJPEGReader.h"
#include "vtkPNGReader.h"
#include "vtkPNMReader.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkShaderProgram.h"
#include "vtkTexture.h"
#include "vtkTIFFReader.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLMaterial.h"
#include "vtkXMLMaterialReader.h"
#include "vtkXMLShader.h"

#include <stdlib.h>

vtkCxxRevisionMacro(vtkProperty, "1.55.24.2");
vtkCxxSetObjectMacro(vtkProperty, ShaderProgram, vtkShaderProgram);
//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkProperty);
//----------------------------------------------------------------------------

// Construct object with object color, ambient color, diffuse color,
// specular color, and edge color white; ambient coefficient=0; diffuse 
// coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
// and surface representation. Backface and frontface culling are off.
vtkProperty::vtkProperty()
{
  this->AmbientColor[0] = 1;
  this->AmbientColor[1] = 1;
  this->AmbientColor[2] = 1;

  this->DiffuseColor[0] = 1;
  this->DiffuseColor[1] = 1;
  this->DiffuseColor[2] = 1;

  this->SpecularColor[0] = 1;
  this->SpecularColor[1] = 1;
  this->SpecularColor[2] = 1;

  this->EdgeColor[0] = 1;
  this->EdgeColor[1] = 1;
  this->EdgeColor[2] = 1;

  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.0;
  this->SpecularPower = 1.0;
  this->Opacity = 1.0;
  this->Interpolation = VTK_GOURAUD;
  this->Representation = VTK_SURFACE;
  this->EdgeVisibility = 0;
  this->BackfaceCulling = 0;
  this->FrontfaceCulling = 0;
  this->PointSize = 1.0;
  this->LineWidth = 1.0;
  this->LineStipplePattern = 0xFFFF;
  this->LineStippleRepeatFactor = 1;

  this->Shading = 0;
  this->ShaderProgram = 0;
  this->Material = 0;
  this->MaterialName = 0;
  this->TextureCollection = vtkCollection::New();
}

//----------------------------------------------------------------------------
vtkProperty::~vtkProperty()
{
  if (this->Material)
    {
    this->Material->UnRegister(this);
    }
  this->SetShaderProgram(0); 
  this->SetMaterialName(0);
  this->TextureCollection->Delete();
}


//----------------------------------------------------------------------------
// Assign one property to another. 
void vtkProperty::DeepCopy(vtkProperty *p)
{
  if ( p != NULL )
    {
    this->SetColor(p->GetColor());
    this->SetAmbientColor(p->GetAmbientColor());
    this->SetDiffuseColor(p->GetDiffuseColor());
    this->SetSpecularColor(p->GetSpecularColor());
    this->SetEdgeColor(p->GetEdgeColor());
    this->SetAmbient(p->GetAmbient());
    this->SetDiffuse(p->GetDiffuse());
    this->SetSpecular(p->GetSpecular());
    this->SetSpecularPower(p->GetSpecularPower());
    this->SetOpacity(p->GetOpacity());
    this->SetInterpolation(p->GetInterpolation());
    this->SetRepresentation(p->GetRepresentation());
    this->SetEdgeVisibility(p->GetEdgeVisibility());
    this->SetBackfaceCulling(p->GetBackfaceCulling());
    this->SetFrontfaceCulling(p->GetFrontfaceCulling());
    this->SetPointSize(p->GetPointSize());
    this->SetLineWidth(p->GetLineWidth());
    this->SetLineStipplePattern(p->GetLineStipplePattern());
    this->SetLineStippleRepeatFactor(p->GetLineStippleRepeatFactor());
    this->SetShading(p->GetShading());
    this->LoadMaterial(p->GetMaterial());
    }
}

//----------------------------------------------------------------------------
// return the correct type of Property 
vtkProperty *vtkProperty::New()
{ 
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkProperty");
  return (vtkProperty*)ret;
}

//----------------------------------------------------------------------------
void vtkProperty::SetColor(double R,double G,double B)
{
  // Use Set macros to insure proper modified time behavior
  this->SetAmbientColor(R,G,B);
  this->SetDiffuseColor(R,G,B);
  this->SetSpecularColor(R,G,B);
}

//----------------------------------------------------------------------------
// Return composite color of object (ambient + diffuse + specular). Return value
// is a pointer to rgb values.
double *vtkProperty::GetColor()
{
  double norm;
  int i;
  
  if ((this->Ambient + this->Diffuse + this->Specular)>0)
    {
    norm = 1.0 / (this->Ambient + this->Diffuse + this->Specular);
    }
  else
    {
    norm = 0.0;
    }
  
  for (i = 0; i < 3; i ++)
    {
    this->Color[i] = this->AmbientColor[i]*this->Ambient*norm;
    this->Color[i] = this->Color[i] + this->DiffuseColor[i]*this->Diffuse*norm;
    this->Color[i] = this->Color[i] + this->SpecularColor[i]*this->Specular*norm;
    }
  
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
void vtkProperty::SetTexture(vtkTexture* tex)
{
  if (this->GetNumberOfTextures() == 0)
    {
    this->AddTexture(tex);
    }
  else
    {
    this->ReplaceTexture(0, tex);
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkProperty::AddTexture(vtkTexture* tex)
{
  this->TextureCollection->AddItem(tex);
  return (this->TextureCollection->GetNumberOfItems()-1);
}

//----------------------------------------------------------------------------
void vtkProperty::ReplaceTexture(vtkIdType index, vtkTexture* newTex)
{
  if (this->GetNumberOfTextures() <= index)
    {
    vtkErrorMacro("Invalid texture index " << index);
    return;
    }
  this->TextureCollection->ReplaceItem(index, newTex);
}

//----------------------------------------------------------------------------
vtkTexture* vtkProperty::GetTexture(vtkIdType index)
{
  if (this->GetNumberOfTextures() <= index)
    {
    return 0;
    }
  return vtkTexture::SafeDownCast(
    this->TextureCollection->GetItemAsObject(index));
}

//----------------------------------------------------------------------------
int vtkProperty::GetNumberOfTextures()
{
  return this->TextureCollection->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void vtkProperty::LoadMaterial(const char* name)
{
  this->SetMaterialName(name);
  //TODO: Here we must check to see if the material is built-in.

  char* filename = vtkXMLShader::LocateFile(name);
  if (filename)
    {
    vtkXMLMaterialReader* reader = vtkXMLMaterialReader::New();
    reader->SetFileName(filename);
    reader->ReadMaterial();
    this->LoadMaterial(reader->GetMaterial());
    reader->Delete();
    delete [] filename;
    }
  else
    {
    vtkErrorMacro("Failed to locate Material file: " << name);
    }
}

//----------------------------------------------------------------------------
void vtkProperty::LoadMaterial(vtkXMLMaterial* material)
{
  vtkSetObjectBodyMacro(Material, vtkXMLMaterial, material);
  if (this->Material)
    {
    this->LoadProperty();
    int lang = this->Material->GetShaderLanguage();
    vtkShaderProgram* shader = vtkShaderProgram::CreateShaderProgram(lang);
    if (shader)
      {
      this->SetShaderProgram(shader);
      shader->Delete();
      this->ShaderProgram->SetMaterial(this->Material);
      this->ShaderProgram->ReadMaterial();
      }
    else
      {
      vtkErrorMacro("Failed to setup the shader.");
      this->SetShaderProgram(0); // failed to read the material.
      // dump the shader.
      }
    }
  else
    {
    this->SetShaderProgram(0);
    }
}

//----------------------------------------------------------------------------
void vtkProperty::LoadProperty()
{
  vtkXMLDataElement* elem = this->Material->GetProperty();
  if( elem == NULL )
    {
    return;
    }

  int iElem = 0;
  int numNested = elem->GetNumberOfNestedElements();
  
  // Each element is a child node of <Property />
  for( iElem=0; iElem<numNested; iElem++ )
    {
    vtkXMLDataElement* currElement = elem->GetNestedElement(iElem);
    const char* tagname = currElement->GetName();

    if (strcmp(tagname, "Texture") == 0)
      {
      // If texture, load it.
      this->LoadTexture(currElement);
      }
    else if (strcmp(tagname, "PerlinNoise") == 0)
      {
      this->LoadPerlineNoise(currElement);
      }
    else if (strcmp(tagname, "Member") == 0)
      {
      this->LoadMember(currElement);
      }
    else
      {
      vtkErrorMacro("Unknown tag name '" << tagname << "'");
      }
    }
}

//----------------------------------------------------------------------------
void vtkProperty::LoadMember(vtkXMLDataElement* elem)
{
  const char* name = elem->GetAttribute("name");
  if (!name)
    {
    vtkErrorMacro("Element missing required attribute 'name'");
    return;
    }

  const char* type = elem->GetAttribute("type");
  if (!type)
    {
    vtkErrorMacro("Element with name=" << name 
      << " missing required attribute 'type'.");
    return;
    }

  int number_of_elements;
  if (!elem->GetScalarAttribute("number_of_elements", number_of_elements))
    {
    vtkErrorMacro("Element with name=" << name << " missing required attribute "
      "'number_of_elements'");
    return;
    }
  if (!elem->GetAttribute("value"))
    {
    vtkErrorMacro("Element with name=" << name << " missing required attribute "
      "'value'");
    return;
    }

  int* pint = 0;
  double* pdouble = 0;
  float* pfloat = 0;
  int success = 0;

  if (strcmp(type,"Double") == 0)
    {
    pdouble = new double[number_of_elements];
    success = elem->GetVectorAttribute("value", number_of_elements, pdouble);
    }
  else if (strcmp(type, "Float") == 0)
    {
    pfloat = new float[number_of_elements];
    success = elem->GetVectorAttribute("value", number_of_elements, pfloat);
    }
  else if (strcmp(type,"Int") == 0)
    {
    pint = new int[number_of_elements];
    success = elem->GetVectorAttribute( "value", number_of_elements, pint);
    }
  else
    {
    vtkErrorMacro("Invalid type='" << type << "' for name=" << name);
    return;
    }

  if (!success)
    {
    vtkErrorMacro("Error reading 'value' for name=" << name);
    delete []pdouble;
    delete []pfloat;
    delete []pint;
    return;
    }

  if (pdouble)
    {
    if (strcmp(name,"Color") == 0)
      {
      this->SetColor(pdouble);
      }
    else if (strcmp(name, "AmbientColor") == 0)
      {
      this->SetAmbientColor(pdouble);
      }
    else if (strcmp(name, "DiffuseColor") == 0)
      {
      this->SetDiffuseColor(pdouble);
      }
    else if (strcmp(name, "SpecularColor") == 0)
      {
      this->SetSpecularColor(pdouble);
      }
    else if (strcmp(name, "EdgeColor") == 0)
      {
      this->SetEdgeColor(pdouble);
      }
    else if (strcmp(name, "Ambient") == 0)
      {
      this->SetAmbient(*pdouble);
      }
    else if (strcmp(name, "Diffuse") == 0)
      {
      this->SetDiffuse(*pdouble);
      }
    else if (strcmp(name, "Specular") == 0)
      {
      this->SetSpecular(*pdouble);
      }
    else if (strcmp(name, "SpecularPower") == 0)
      {
      this->SetSpecularPower(*pdouble);
      }
    else if (strcmp(name,"Opacity") == 0)
      {
      this->SetOpacity(*pdouble);
      }
    }
  else if (pfloat)
    {
    if (strcmp(name, "PointSize") == 0)
      {
      this->SetPointSize(*pfloat);
      }
    else if (strcmp(name, "LineWidth") == 0)
      {
      this->SetLineWidth(*pfloat);
      }
    }
  else if (pint)
    {
    if (strcmp(name, "LineStipplePattern") == 0)
      {
      this->SetLineStipplePattern(*pint);
      }
    else if (strcmp(name, "LineStippleRepeatFactor") == 0)
      {
      this->SetLineStippleRepeatFactor(*pint);
      }
    else if (strcmp(name, "Interpolation") == 0)
      {
      this->SetInterpolation(*pint);
      }
    else if (strcmp(name, "Representation") == 0)
      {
      this->SetRepresentation(*pint);
      }
    else if (strcmp(name, "EdgeVisibility") == 0)
      {
      this->SetEdgeVisibility(*pint);
      }
    else if (strcmp(name, "BackfaceCulling") == 0)
      {
      this->SetBackfaceCulling(*pint);
      }
    else if (strcmp(name, "FrontfaceCulling") == 0)
      {
      this->SetFrontfaceCulling(*pint);
      }
    }

  delete []pdouble;
  delete []pfloat;
  delete []pint;
}

//----------------------------------------------------------------------------
void vtkProperty::LoadTexture(vtkXMLDataElement* elem )
{
  const char* name = elem->GetAttribute("name");
  if (!name)
    {
    vtkErrorMacro("Missing required attribute 'name'");
    return;
    }

  const char* type = elem->GetAttribute("type");
  if (!type)
    {
    vtkErrorMacro("Missing required attribute 'type' "
      "for element with name=" << name);
    return;
    }
  
  const char* location = elem->GetAttribute("location");
  if (!location)
    {
    vtkErrorMacro("Missing required attribute 'location'"
      "for element with name=" << name);
    return;
    }

  const char* format = elem->GetAttribute("format");
  if (!format)
    {
    vtkErrorMacro("Missing required attribute 'format'"
      "for element with name=" << name);
    return;
    }
  
  vtkImageReader2* reader;
  if (strcmp(format, "bmp") == 0)
    {
    reader = vtkBMPReader::New();
    }
  else if (strcmp(format, "jpg") == 0 || strcmp(format, "jpeg") == 0)
    {
    reader = vtkJPEGReader::New();
    }
  else if (strcmp(format, "png") == 0)
    {
    reader = vtkPNGReader::New();
    }
  else if (strcmp(format, "tiff") == 0 || strcmp(format, "tif") == 0)
    {
    reader = vtkTIFFReader::New();
    }
  else if (strcmp(format, "ppm") == 0)
    {
    reader = vtkPNMReader::New();
    }
  else
    {
    vtkErrorMacro("Invalid format='" << format << "' for element with name="
      << name);
    return;
    }
 
  char* filename = vtkXMLShader::LocateFile(location);
  if (filename)
    {
    reader->SetFileName(filename);
    vtkTexture* t = vtkTexture::New();
    t->SetInput(reader->GetOutput());
    t->InterpolateOn();
    this->AddTexture(t);
    // Eventually, we may want to assign names to the textures.
    // but for now, the shaders will use them by their index.
    t->Delete();
    }
  else
    {
    vtkErrorMacro("Failed to locate texture file " << location);
    }
 
  reader->Delete();
  delete []filename;
}

//----------------------------------------------------------------------------
void vtkProperty::LoadPerlineNoise(vtkXMLDataElement* )
{
  vtkWarningMacro("Perlin Noise support not complete yet!");
}

//----------------------------------------------------------------------------
void vtkProperty::Render(vtkActor* actor, vtkRenderer* renderer)
{
  // subclass would have renderer the property already.
  // this class, just handles the shading.
  if (this->ShaderProgram && this->GetShading())
    {
    vtkDebugMacro("Attempting to use Shaders");
    this->ShaderProgram->Render(actor, renderer);
    }
}

//----------------------------------------------------------------------------
void vtkProperty::PostRender(vtkActor* actor, vtkRenderer* renderer)
{
  if (this->ShaderProgram && this->Shading)
    {
    this->ShaderProgram->PostRender(actor, renderer);
    }
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
  os << indent << "Material: " ;
  if (this->Material)
    {
    os << endl;
    this->Material->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
