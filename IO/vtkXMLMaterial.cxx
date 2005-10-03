/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLMaterial.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXMLMaterial.h"

#include "vtkMaterialLibrary.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLMaterialParser.h"
#include "vtkXMLShader.h"

#include <vtkstd/vector>
class vtkXMLMaterialInternals
{
public:
  typedef vtkstd::vector<vtkXMLDataElement*> VectorOfElements;
  typedef vtkstd::vector<vtkSmartPointer<vtkXMLShader> > VectorOfShaders;
  VectorOfElements Properties;
  VectorOfShaders VertexShaders;
  VectorOfShaders FragmentShaders;
  void Initialize()
    {
    this->Properties.clear();
    this->VertexShaders.clear();
    this->FragmentShaders.clear();
    }
};

vtkStandardNewMacro(vtkXMLMaterial);
vtkCxxRevisionMacro(vtkXMLMaterial, "1.1.2.4");
//-----------------------------------------------------------------------------
vtkXMLMaterial::vtkXMLMaterial()
{
  this->RootElement = 0;
  this->Internals = new vtkXMLMaterialInternals;
}

//-----------------------------------------------------------------------------
vtkXMLMaterial::~vtkXMLMaterial()
{
  this->SetRootElement(0);
  delete this->Internals;
}

//-----------------------------------------------------------------------------
vtkXMLMaterial* vtkXMLMaterial::CreateInstance(const char* name)
{
  char* xml = vtkMaterialLibrary::GetMaterial(name);
  if (xml)
    {
    vtkXMLMaterialParser* parser = vtkXMLMaterialParser::New();
    vtkXMLMaterial* material = vtkXMLMaterial::New();
    parser->SetMaterial(material);
    parser->Parse(xml);
    parser->Delete();
    return material;
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void vtkXMLMaterial::SetRootElement(vtkXMLDataElement* root)
{
  this->Internals->Initialize();
  
  vtkSetObjectBodyMacro(RootElement, vtkXMLDataElement, root);
  if (this->RootElement)
    {
    // Update the internal data structure to 
    // avoid repeated searches.
    int numElems = this->RootElement->GetNumberOfNestedElements();
    for (int i=0; i<numElems; i++)
      {
      vtkXMLDataElement* elem = this->RootElement->GetNestedElement(i);
      const char* name = elem->GetName();
      if (!name)
        {
        continue;
        }
      if (strcmp(name, "Property") == 0)
        {
        this->Internals->Properties.push_back(elem);
        }
      else if (strcmp(name, "Shader") == 0)
        {
        vtkXMLShader* shader = vtkXMLShader::New();
        shader->SetRootElement(elem);

        switch (shader->GetScope())
          {
        case vtkXMLShader::SCOPE_VERTEX:
          this->Internals->VertexShaders.push_back(shader);
          break;
        case vtkXMLShader::SCOPE_FRAGMENT:
          this->Internals->FragmentShaders.push_back(shader);
          break;
        default:
          vtkErrorMacro("Invalid scope for shader: " << shader->GetName());
          }

        shader->Delete();
        }
      }
    }
}

//-----------------------------------------------------------------------------
int vtkXMLMaterial::GetNumberOfProperties()
{
  return this->Internals->Properties.size();
}


//-----------------------------------------------------------------------------
int vtkXMLMaterial::GetNumberOfVertexShaders()
{
  return this->Internals->VertexShaders.size();
}

//-----------------------------------------------------------------------------
int vtkXMLMaterial::GetNumberOfFragmentShaders()
{
  return this->Internals->FragmentShaders.size();
}

//-----------------------------------------------------------------------------
vtkXMLDataElement* vtkXMLMaterial::GetProperty(int id)
{
  if (id < this->GetNumberOfProperties())
    {
    return this->Internals->Properties[id];
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkXMLShader* vtkXMLMaterial::GetVertexShader(int id)
{
  if (id < this->GetNumberOfVertexShaders())
    {
    return this->Internals->VertexShaders[id].GetPointer();
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkXMLShader* vtkXMLMaterial::GetFragmentShader(int id)
{
  if (id < this->GetNumberOfFragmentShaders())
    {
    return this->Internals->FragmentShaders[id].GetPointer();
    }
  return NULL;
}

//-----------------------------------------------------------------------------
int vtkXMLMaterial::GetShaderLanguage()
{
  if( this->GetVertexShader() && this->GetFragmentShader() )
    {
    int vLang = this->GetVertexShader()->GetLanguage();
    int fLang = this->GetFragmentShader()->GetLanguage();

    if( vLang == fLang )
      {
      return this->GetVertexShader()->GetLanguage();
      }
    else if( vLang != vtkXMLShader::LANGUAGE_NONE &&
      fLang == vtkXMLShader::LANGUAGE_NONE )
      {
      return this->GetVertexShader()->GetLanguage();
      }
    else if( vLang == vtkXMLShader::LANGUAGE_NONE &&
      fLang != vtkXMLShader::LANGUAGE_NONE )
      {
      return this->GetFragmentShader()->GetLanguage();
      }
    else
      {
      return vtkXMLShader::LANGUAGE_MIXED;
      }
    }
  else if( this->GetVertexShader() )
    {
    return this->GetVertexShader()->GetLanguage();
    }
  else if( this->GetFragmentShader() )
    {
    return this->GetFragmentShader()->GetLanguage();
    }
  return vtkXMLShader::LANGUAGE_NONE;
}

//-----------------------------------------------------------------------------
void vtkXMLMaterial::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of Properties: " << this->GetNumberOfProperties()
    << endl;
  os << indent << "Number of Vertex Shaders: " 
    << this->GetNumberOfVertexShaders() << endl;
  os << indent << "Number of Fragment Shaders: " 
    << this->GetNumberOfFragmentShaders() << endl;
  os << indent << "RootElement: ";
  if (this->RootElement)
    {
    os << endl;
    this->RootElement->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(null)" << endl;
    }
}
