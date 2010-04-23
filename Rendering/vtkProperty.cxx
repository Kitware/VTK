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
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkTexture.h"
#include "vtkTIFFReader.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLMaterial.h"
#include "vtkXMLMaterialParser.h"
#include "vtkXMLShader.h"

#include <stdlib.h>
#include <vtksys/ios/sstream>

#include <vtkstd/map>
#include <vtksys/SystemTools.hxx>

class vtkPropertyInternals
{
public:
  // key==texture unit, value==texture
  typedef vtkstd::map<int, vtkSmartPointer<vtkTexture> > MapOfTextures;
  MapOfTextures Textures;

  // key==texture name, value==texture-unit.
  typedef vtkstd::map<vtkStdString, int> MapOfTextureNames;
  MapOfTextureNames TextureNames;
};

vtkCxxSetObjectMacro(vtkProperty, ShaderProgram, vtkShaderProgram);
//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkProperty);
//----------------------------------------------------------------------------

// Construct object with object color, ambient color, diffuse color,

enum IVarEnum {
  IVarNone = 0,
  IVarColor,
  IVarAmbientColor,
  IVarDiffuseColor,
  IVarSpecularColor,
  IVarEdgeColor,
  IVarAmbient,
  IVarDiffuse,
  IVarSpecular,
  IVarSpecularPower,
  IVarOpacity,

  IVarPointSize,
  IVarLineWidth,

  IVarLineStipplePattern,
  IVarLineStippleRepeatFactor,
  IVarInterpolation,
  IVarRepresentation,
  IVarEdgeVisibility,
  IVarBackfaceCulling,
  IVarFrontfaceCulling
};

static IVarEnum XMLMemberToIvar( const char* name )
{
  if ( (strcmp(name,"Color") == 0) )
    {
    return IVarColor;
    }
  else if (strcmp(name,"AmbientColor") == 0)
    {
    return IVarAmbientColor;
    }
  else if (strcmp(name,"DiffuseColor") == 0)
    {
    return IVarDiffuseColor;
    }
  else if (strcmp(name,"SpecularColor") == 0)
    {
    return IVarSpecularColor;
    }
  else if (strcmp(name,"EdgeColor") == 0)
    {
    return IVarEdgeColor;
    }
  else if (strcmp(name,"Ambient") == 0)
    {
    return IVarAmbient;
    }
  else if (strcmp(name,"Diffuse") == 0)
    {
    return IVarDiffuse;
    }
  else if (strcmp(name,"Specular") == 0)
    {
    return IVarSpecular;
    }
  else if (strcmp(name,"SpecularPower") == 0)
    {
    return IVarSpecularPower;
    }
  else if (strcmp(name,"Opacity") == 0)
    {
    return IVarOpacity;
    }
  else if (strcmp(name,"PointSize") == 0)
    {
    return IVarPointSize;
    }
  else if (strcmp(name,"LineWidth") == 0)
    {
    return IVarLineWidth;
    }
  else if (strcmp(name,"LineStipplePattern") == 0)
    {
    return IVarLineStipplePattern;
    }
  else if (strcmp(name,"LineStippleRepeatFactor") == 0)
    {
    return IVarLineStippleRepeatFactor;
    }
  else if (strcmp(name,"Interpolation") == 0)
    {
    return IVarInterpolation;
    }
  else if (strcmp(name,"Representation") == 0)
    {
    return IVarRepresentation;
    }
  else if (strcmp(name,"EdgeVisibility") == 0) 
    {
    return IVarEdgeVisibility;
    }
  else if (strcmp(name,"BackfaceCulling") == 0)
    {
    return IVarBackfaceCulling;
    }
  else if (strcmp(name,"FrontfaceCulling") == 0)
    {
    return IVarFrontfaceCulling;
    }
  else
    {
    return IVarNone;
    }


};


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
  this->Lighting=true;

  this->Shading = 0;
  this->ShaderProgram = 0;
  this->Material = 0;
  this->MaterialName = 0;
  this->Internals = new vtkPropertyInternals;
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
  delete this->Internals;
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
   
    this->RemoveAllTextures();
    vtkPropertyInternals::MapOfTextures::iterator iter =
      p->Internals->Textures.begin();
    for ( ;iter != p->Internals->Textures.end(); ++iter)
      {
      this->Internals->Textures[iter->first] = iter->second;
      }

    // TODO: need to pass shader variables.
    }
}

//----------------------------------------------------------------------------
// return the correct type of Property 
vtkProperty *vtkProperty::New()
{ 
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkProperty");
  return static_cast<vtkProperty *>(ret);
}

//----------------------------------------------------------------------------
void vtkProperty::SetColor(double R,double G,double B)
{
  // Really should set the placeholder Color[3] variable
  this->Color[0] = R;
  this->Color[1] = G;
  this->Color[2] = B;

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
  for (int id=0; iter != this->Internals->Textures.end(); ++iter, ++id)
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
  for (int id=0; iter != this->Internals->Textures.end(); ++iter, ++id)
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
void vtkProperty::LoadMaterial(const char* name)
{
  this->SetMaterialName(0);
  if( !name || strlen(name) == 0)
    {
    this->LoadMaterial(static_cast<vtkXMLMaterial*>(0));
    return;
    }

  // vtkXMLMaterial::CreateInstance using library/absolute path/repository
  // in that order.
  vtkXMLMaterial* material = vtkXMLMaterial::CreateInstance(name);
  if (material)
    {
    this->LoadMaterial(material);
    material->Delete();
    return;
    }
  else
    {
    vtkErrorMacro("Failed to create Material : " << name);
    }
}

//----------------------------------------------------------------------------
void vtkProperty::LoadMaterialFromString(const char* materialxml)
{
  this->SetMaterialName(0);
  if (!materialxml)
    {
    this->LoadMaterial(static_cast<vtkXMLMaterial*>(0));
    return;
    }
  vtkXMLMaterialParser* parser = vtkXMLMaterialParser::New();
  vtkXMLMaterial* material = vtkXMLMaterial::New();
  parser->SetMaterial(material);
  parser->Parse(materialxml);
  parser->Delete();
  this->LoadMaterial(material);
  material->Delete();
}

// ----------------------------------------------------------------------------
// Description:
// Read this->Material from new style shaders.
// Default implementation is empty.
void vtkProperty::ReadFrameworkMaterial()
{
  // empty. See vtkOpenGLProperty.
}

//----------------------------------------------------------------------------
void vtkProperty::LoadMaterial(vtkXMLMaterial* material)
{
  this->SetMaterialName(0);
  vtkSetObjectBodyMacro(Material, vtkXMLMaterial, material);
  if (this->Material)
    {
    this->SetMaterialName(this->Material->GetRootElement()->GetAttribute("name"));
    this->LoadProperty();
    this->LoadTextures();
    int lang = this->Material->GetShaderLanguage();
    int style=this->Material->GetShaderStyle();
    
    if(style==2)
      {
      if(lang==vtkXMLShader::LANGUAGE_GLSL)
        {
        // ready-for-multipass
        this->ReadFrameworkMaterial();
//        vtkShader2Collection *shaders=vtkShader2Collection::New();
//        this->SetShaderCollection(shaders);
//        shaders->Delete();
        }
      else
        {
        vtkErrorMacro(<<"style 2 is only supported with GLSL. Failed to setup the shader.");
        this->SetShaderProgram(0); // failed to create shaders.
        }
      }
    else
      {
      vtkShaderProgram* shader = vtkShaderProgram::CreateShaderProgram(lang);
      if (shader)
        {
        this->SetShaderProgram(shader);
        shader->Delete();
        this->ShaderProgram->SetMaterial(this->Material);
        this->ShaderProgram->ReadMaterial();
        }
      // Some materials may have no shaders and only set ivars for vtkProperty.
      else if( (material->GetNumberOfVertexShaders() != 0) ||
               (material->GetNumberOfFragmentShaders() != 0) )
        {
        vtkErrorMacro("Failed to setup the shader.");
        this->SetShaderProgram(0); // failed to create shaders.
        }
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

    if (strcmp(tagname, "Member") == 0)
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
void vtkProperty::LoadTextures()
{
  int numTextures = this->Material->GetNumberOfTextures();
  for (int i=0; i < numTextures; i++)
    {
    this->LoadTexture(this->Material->GetTexture(i));
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

  if (!elem->GetAttribute("value"))
    {
    vtkErrorMacro("Element with name=" << name << " missing required attribute "
      "'value'");
    return;
    }
  int number_of_elements;

  int* pint = 0;
  double* pdouble = 0;
  float* pfloat = 0;
  int success = 0;

  IVarEnum member = XMLMemberToIvar( name );

  // Sort to find the correct number of ivar values
  if ( member == IVarColor ||
       member == IVarAmbientColor||
       member == IVarDiffuseColor||
       member == IVarSpecularColor||
       member == IVarEdgeColor )
    {
    number_of_elements = 3;
    }
  else if ( member == IVarAmbient ||
            member == IVarDiffuse ||
            member == IVarSpecular ||
            member == IVarSpecularPower ||
            member == IVarSpecularColor||
            member == IVarOpacity ||
            member == IVarPointSize ||
            member == IVarLineWidth ||
            member == IVarLineStipplePattern ||
            member == IVarLineStippleRepeatFactor ||
            member == IVarInterpolation ||
            member == IVarRepresentation ||
            member == IVarEdgeVisibility || 
            member == IVarBackfaceCulling ||
            member == IVarFrontfaceCulling )
    {
    number_of_elements = 1;
    }
  else
    {
    vtkErrorMacro("Invalid name='" << name);
    return;
    }



  if ( (member == IVarColor) ||
       (member == IVarAmbientColor) ||
       (member == IVarDiffuseColor) ||
       (member == IVarSpecularColor) ||
       (member == IVarEdgeColor) ||
       (member == IVarAmbient) ||
       (member == IVarDiffuse) ||
       (member == IVarSpecular) ||
       (member == IVarSpecularPower) ||
       (member == IVarOpacity) )
    {
    pdouble = new double[number_of_elements];
    success = elem->GetVectorAttribute("value", number_of_elements, pdouble);
    }
  else if( (member == IVarPointSize) ||
           (member == IVarLineWidth) )
    {
    pfloat = new float[number_of_elements];
    success = elem->GetVectorAttribute("value", number_of_elements, pfloat);
    }
  else if ( (member == IVarLineStipplePattern) ||
            (member == IVarLineStippleRepeatFactor) ||
            (member == IVarInterpolation) ||
            (member == IVarRepresentation) ||
            (member == IVarEdgeVisibility) ||
            (member == IVarBackfaceCulling) ||
            (member == IVarFrontfaceCulling) )
    {
    pint = new int[number_of_elements];
    success = elem->GetVectorAttribute( "value", number_of_elements, pint);
    }
  else
    {
    vtkErrorMacro("Invalid name='" << name);
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
    if (member == IVarColor)
      {
      this->SetColor(pdouble);
      }
    else if (member == IVarAmbientColor)
      {
      this->SetAmbientColor(pdouble);
      }
    else if (member == IVarDiffuseColor)
      {
      this->SetDiffuseColor(pdouble);
      }
    else if (member == IVarSpecularColor)
      {
      this->SetSpecularColor(pdouble);
      }
    else if (member == IVarEdgeColor)
      {
      this->SetEdgeColor(pdouble);
      }
    else if (member == IVarAmbient)
      {
      this->SetAmbient(*pdouble);
      }
    else if (member == IVarDiffuse)
      {
      this->SetDiffuse(*pdouble);
      }
    else if (member == IVarSpecular)
      {
      this->SetSpecular(*pdouble);
      }
    else if (member == IVarSpecularPower)
      {
      this->SetSpecularPower(*pdouble);
      }
    else if (member == IVarOpacity)
      {
      this->SetOpacity(*pdouble);
      }
    }
  else if (pfloat)
    {
    if (member == IVarPointSize)
      {
      this->SetPointSize(*pfloat);
      }
    else if (member == IVarLineWidth)
      {
      this->SetLineWidth(*pfloat);
      }
    }
  else if (pint)
    {
    if (member == IVarLineStipplePattern)
      {
      this->SetLineStipplePattern(*pint);
      }
    else if (member == IVarLineStippleRepeatFactor)
      {
      this->SetLineStippleRepeatFactor(*pint);
      }
    else if (member == IVarInterpolation)
      {
      this->SetInterpolation(*pint);
      }
    else if (member == IVarRepresentation)
      {
      this->SetRepresentation(*pint);
      }
    else if (member == IVarEdgeVisibility)
      {
      this->SetEdgeVisibility(*pint);
      }
    else if (member == IVarBackfaceCulling)
      {
      this->SetBackfaceCulling(*pint);
      }
    else if (member == IVarFrontfaceCulling)
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
  vtkStdString string_format;

  if (!format)
    {
    // determine format from file extension.
    string_format = vtksys::SystemTools::GetFilenameLastExtension(location);
    format = string_format.c_str();
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
    this->SetTexture(name, t);
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
 
  if (renderer->GetSelector())
    {
    // nothing to do when rendering for hardware selection.
    return;
    }

  if (this->ShaderProgram && this->GetShading())
    {
    vtkDebugMacro("Attempting to use Shaders");

    this->ShaderProgram->Render(actor, renderer);
    }
}

//----------------------------------------------------------------------------
void vtkProperty::PostRender(vtkActor* actor, vtkRenderer* renderer)
{
  if (renderer->GetSelector())
    {
    // nothing to do when rendering for hardware selection.
    return;
    }

  if (this->ShaderProgram && this->Shading)
    {
    this->ShaderProgram->PostRender(actor, renderer);
    }
}

//----------------------------------------------------------------------------
void vtkProperty::AddShaderVariable(const char* name, int numVars, int* x)
{
  if( !this->ShaderProgram )
    {
    return;
    }
  this->ShaderProgram->AddShaderVariable( name, numVars, x );
}

//----------------------------------------------------------------------------
void vtkProperty::AddShaderVariable(const char* name, int numVars, float* x)
{
  if( !this->ShaderProgram )
    {
    return;
    }
  this->ShaderProgram->AddShaderVariable( name, numVars, x );
}

//----------------------------------------------------------------------------
void vtkProperty::AddShaderVariable(const char* name, int numVars, double* x)
{
  if( !this->ShaderProgram )
    {
    return;
    }
  this->ShaderProgram->AddShaderVariable( name, numVars, x );
}

//-----------------------------------------------------------------------------
void vtkProperty::ReleaseGraphicsResources(vtkWindow *win)
{
  if (this->ShaderProgram)
    {
    this->ShaderProgram->ReleaseGraphicsResources(win);
    }

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

  os << indent << "Shading: " 
    << (this->Shading? "On" : "Off") << endl;
  
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
 os << indent << "MaterialName: " << 
   (this->MaterialName? this->MaterialName:"(none)") << endl;

  os << indent << "ShaderProgram: ";
  if (this->ShaderProgram)
    {
    os << endl;
    this->ShaderProgram->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}
