/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */


#include "vtkShader.h"
#include <vtkObjectFactory.h>

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkTimeStamp.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLShader.h"

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
// Helper method.
static inline int vtkShaderGetType(const char* type)
{
  if (!type)
    {
    return 0;
    }
  if (strcmp(type,"Double")==0)
    {
    return VTK_DOUBLE;
    }
  if (strcmp(type, "Float")==0)
    {
    return VTK_FLOAT;
    }
  if (strcmp(type, "Int") == 0)
    {
    return VTK_INT;
    }
  return 0;
}

//-----------------------------------------------------------------------------
class vtkShaderUniformVariable
{
public:
  vtkShaderUniformVariable()
    {
    this->Initialize();    
    }

  vtkShaderUniformVariable(const char* name, int num, const int* val)
    {
    this->Initialize();
    this->SetName(name);
    this->Type = VTK_INT;
    this->NumberOfValues = num;
    this->IntValues = new int[num];
    for (int i=0; i < num; i++)
      {
      this->IntValues[i] = val[i];
      }
    }

  vtkShaderUniformVariable(const char* name, int num, const double* val)
    {
    this->Initialize();
    this->SetName(name);
    this->Type = VTK_DOUBLE;
    this->NumberOfValues = num;
    this->DoubleValues = new double[num];
    for (int i=0; i < num; i++)
      {
      this->DoubleValues[i] = val[i];
      }
    }

  vtkShaderUniformVariable(const char* name, int num, const float* val)
    {
    this->Initialize();
    this->SetName(name);
    this->Type = VTK_FLOAT;
    this->NumberOfValues = num;
    this->FloatValues = new float[num];
    for (int i=0; i < num; i++)
      {
      this->FloatValues[i] = val[i];
      }
    }

  int GetType() { return this->Type; }
  int GetNumberOfValues() { return this->NumberOfValues; }

  int GetValue(int *a)
    {
    if (this->Type != VTK_INT)
      {
      return 0;
      }
    for (int i=0; i < this->NumberOfValues; i++)
      {
      a[i] = this->IntValues[i];
      }
    return 1;
    }
  
  int GetValue(float *a)
    {
    if (this->Type != VTK_FLOAT)
      {
      return 0;
      }
    for (int i=0; i < this->NumberOfValues; i++)
      {
      a[i] = this->FloatValues[i];
      }
    return 1;
    }
  
  int GetValue(double *a)
    {
    if (this->Type != VTK_DOUBLE)
      {
      return 0;
      }
    for (int i=0; i < this->NumberOfValues; i++)
      {
      a[i] = this->DoubleValues[i];
      }
    return 1;
    }

  ~vtkShaderUniformVariable()
    {
    this->SetName(0);
    if (this->IntValues)
      {
      delete [] this->IntValues;
      }
    if (this->FloatValues)
      {
      delete [] this->FloatValues;
      }
    if (this->DoubleValues)
      {
      delete [] this->DoubleValues;
      }
    }

  void Print(ostream& os, vtkIndent indent)
    {
    int i;
    os << indent << "Name: " << ((this->Name)? this->Name : "(none)") << endl;
    os << indent << "NumberOfValues: " << this->NumberOfValues;
    switch (this->Type)
      {
    case VTK_INT:
      os << indent << "Type: int" << endl;
      os << indent << "Values: " ;
      for (i=0; i < this->NumberOfValues; i++)
        {
        os << this->IntValues[i] << " ";
        }
      os << endl;
      break;
    case VTK_DOUBLE:
      os << indent << "Type: double" << endl;
      os << indent << "Values: " ;
      for (i=0; i < this->NumberOfValues; i++)
        {
        os << this->DoubleValues[i] << " ";
        }
      os << endl;
      break;
    case VTK_FLOAT:
      os << indent << "Type: float" << endl;
      os << indent << "Values: " ;
      for (i=0; i < this->NumberOfValues; i++)
        {
        os << this->FloatValues[i] << " ";
        }
      os << endl;
      break;
      }
    }

private:
  void Initialize()
    {
    this->Name = 0;
    this->Type = 0;
    this->NumberOfValues = 0;
    this->IntValues = 0;
    this->FloatValues = 0;
    this->DoubleValues = 0;
    }
  void SetName(const char* name)
    {
    if (this->Name)
      {
      delete [] this->Name;
      this->Name = 0;
      }
    if (name)
      {
      this->Name = vtksys::SystemTools::DuplicateString(name);
      }
    }
  char* Name;
  int NumberOfValues;
  int Type;
  int* IntValues;
  float* FloatValues;
  double* DoubleValues;
};


//-----------------------------------------------------------------------------
class vtkShaderInternals
{
public:
  vtkstd::map<vtkstd::string, vtkShaderUniformVariable> UniformVariables;
};

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkShader, "1.3")
vtkCxxSetObjectMacro(vtkShader, XMLShader, vtkXMLShader);
//-----------------------------------------------------------------------------
vtkShader::vtkShader()
{
  this->XMLShader = 0;
  this->Internals = new vtkShaderInternals;
}

//-----------------------------------------------------------------------------
vtkShader::~vtkShader()
{
  this->SetXMLShader(0);
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkShader::PassShaderVariables(vtkActor* actor, vtkRenderer* renderer)
{
  this->SetShaderParameters(actor, renderer, 
    this->XMLShader->GetRootElement());
  this->PassShaderVariablesTime.Modified();
}

//-----------------------------------------------------------------------------
int vtkShader::HasShaderVariable(const char* name)
{
  if (!name)
    {
    return 0;
    }
  if (this->Internals->UniformVariables.find(name) !=
    this->Internals->UniformVariables.end())
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkShader::AddShaderVariable(const char* name, int num_of_elements,
  const int * values)
{
  if (!name || num_of_elements <= 0 || !values)
    {
    return;
    }
  if (this->HasShaderVariable(name))
    {
    vtkWarningMacro("Variable with name '" << name 
      <<"' already exists. Ignoring.");
    return;
    }
  this->Internals->UniformVariables[name] = vtkShaderUniformVariable(
    name, num_of_elements, values);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkShader::AddShaderVariable(const char* name, int num_of_elements,
  const float* values)
{
  if (!name || num_of_elements <= 0 || !values)
    {
    return;
    }
  if (this->HasShaderVariable(name))
    {
    vtkWarningMacro("Variable with name '" << name 
      <<"' already exists. Ignoring.");
    return;
    }
  this->Internals->UniformVariables[name] = vtkShaderUniformVariable(
    name, num_of_elements, values);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkShader::AddShaderVariable(const char* name, int num_of_elements,
  const double* values)
{
  if (!name || num_of_elements <= 0 || !values)
    {
    return;
    }
  if (this->HasShaderVariable(name))
    {
    vtkWarningMacro("Variable with name '" << name 
      <<"' already exists. Ignoring.");
    return;
    }
  this->Internals->UniformVariables[name] = vtkShaderUniformVariable(
    name, num_of_elements, values);
  this->Modified();
}


//-----------------------------------------------------------------------------
int vtkShader::GetShaderVariableSize(const char* name)
{
  if (!this->HasShaderVariable(name))
    {
    return 0;
    }
  return this->Internals->UniformVariables[name].GetNumberOfValues();
}


//-----------------------------------------------------------------------------
int vtkShader::GetShaderVariableType(const char* name)
{
  if (!this->HasShaderVariable(name))
    {
    return 0;
    }
  return this->Internals->UniformVariables[name].GetType();
}


//-----------------------------------------------------------------------------
int vtkShader::GetShaderVariable(const char* name, int *values)
{
  if (!this->HasShaderVariable(name))
    {
    return 0;
    }
  return this->Internals->UniformVariables[name].GetValue(values);
}

//-----------------------------------------------------------------------------
int vtkShader::GetShaderVariable(const char* name, float *values)
{
  if (!this->HasShaderVariable(name))
    {
    return 0;
    }
  return this->Internals->UniformVariables[name].GetValue(values);
}

//-----------------------------------------------------------------------------
int vtkShader::GetShaderVariable(const char* name, double *values)
{
  if (!this->HasShaderVariable(name))
    {
    return 0;
    }
  return this->Internals->UniformVariables[name].GetValue(values);
}

//-----------------------------------------------------------------------------
// Set all children elements of start elements
void vtkShader::SetShaderParameters(vtkActor* actor, vtkRenderer* renderer,
  vtkXMLDataElement* root)
{
  if(root==NULL)
    {
    return;
    }

  int max = root->GetNumberOfNestedElements();
  for (int i=0; i < max; i++)
    {
    vtkXMLDataElement* elem = root->GetNestedElement(i);
    // Decide what to do with the elem element.
    const char* name = elem->GetAttribute("name");
    if (!name)
      {
      vtkErrorMacro("Uniform parameter missing required attribute 'name'");
      continue;
      }

    const char* tagname = elem->GetName();
    if (!tagname)
      {
      vtkErrorMacro("Unexpected error. XML element has no tag name!");
      continue;
      }

    if (strcmp(tagname, "Uniform") == 0)
      {
      this->SetUniformParameter(actor, renderer, elem);
      }
    else if (strcmp(tagname, "CameraUniform") == 0)
      {
      this->SetCameraParameter(actor, renderer, elem);
      } 
    else if (strcmp(tagname, "LightUniform") == 0)
      {
      this->SetLightParameter(actor, renderer, elem);
      }
    else if (strcmp(tagname, "MatrixUniform") == 0)
      {
      this->SetMatrixParameter(actor, renderer, elem);
      }
    else if (strcmp(tagname, "PropertyUniform") == 0)
      {
      this->SetPropertyParameter(actor, renderer, elem);
      }
    else if (strcmp(tagname, "SamplerUniform") == 0)
      {
      this->SetSamplerParameter(actor, renderer, elem);
      }
    else
      {
      vtkErrorMacro("Invalid tag: " << tagname);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkShader::SetUniformParameter(vtkActor* , vtkRenderer* , 
  vtkXMLDataElement* elem)
{
  if (this->GetMTime() < this->PassShaderVariablesTime)
    {
    return; // no need to update.
    }
  const char* name = elem->GetAttribute("name");
  const char* ctype = elem->GetAttribute("type");
  const char* cvalue = elem->GetAttribute("value");

  if (!ctype)
    {
    vtkErrorMacro("Missing required attribute 'type' on name=" << name);
    return;
    }

  int number_of_elements;

  if (!elem->GetScalarAttribute("number_of_elements", number_of_elements))
    {
    vtkErrorMacro("Missing return attribute 'number_of_elements'");
    return;
    }

  if (number_of_elements <= 0)
    {
    vtkErrorMacro("'number_of_elements' cannot be " << number_of_elements);
    return;
    }

  if (!cvalue && !this->HasShaderVariable(name))
    {
    vtkErrorMacro("Variable '" << name << "' doesn't have a value specified in the XML"
      << " nor as a Shader Variable.");
    return;
    }

  int type = vtkShaderGetType(ctype);
  if (!cvalue && type != this->GetShaderVariableType(name))
    {
    vtkErrorMacro("Parameter type mismatch: " << name);
    return;
    }

  if (!cvalue && number_of_elements != this->GetShaderVariableSize(name))
    {
    vtkErrorMacro("Parameter size mismatch: " << name);
    return;
    }

  switch (type)
    {
  case VTK_INT:
      {
      int *v = new int [number_of_elements];
      if  ((cvalue && elem->GetVectorAttribute("value", number_of_elements, v))
        ||(!cvalue && this->GetShaderVariable(name, v)))
        {
        this->SetUniformParameter(name, number_of_elements, v);
        }
      else
        {
        vtkErrorMacro("Failed to set unform variable : " << name);
        }
      delete []v;
      }
    break;

  case VTK_FLOAT:
      {
      float *v = new float [number_of_elements];
      if  ((cvalue && elem->GetVectorAttribute("value", number_of_elements, v))
        ||(!cvalue && this->GetShaderVariable(name, v)))
        {
        this->SetUniformParameter(name, number_of_elements, v);
        }
      else
        {
        vtkErrorMacro("Failed to set unform variable : " << name);
        }     
      delete []v;
      }
    break;

  case VTK_DOUBLE:
      {
      double *v = new double[number_of_elements];
      if  ((cvalue && elem->GetVectorAttribute("value", number_of_elements, v))
        ||(!cvalue && this->GetShaderVariable(name, v)))
        {
        this->SetUniformParameter(name, number_of_elements, v);
        }
      else
        {
        vtkErrorMacro("Failed to set unform variable : " << name);
        }
      delete []v;
      }
    break;

  default:
    vtkErrorMacro("Invalid type: " << ctype);
    }
}

//-----------------------------------------------------------------------------
void vtkShader::SetCameraParameter(vtkActor* , vtkRenderer* ren, 
  vtkXMLDataElement* elem)
{
  vtkCamera* camera = ren->GetActiveCamera();

  if (camera->GetMTime() < this->PassShaderVariablesTime)
    {
    // no need to update.
    return;
    }

  const char* name = elem->GetAttribute("name");
  const char* value = elem->GetAttribute("value");
  
  if (!value)
    {
    vtkErrorMacro("Missing required attribute 'value' on name=" << name);
    return;
    }
  
  double *x = 0;
  if (strcmp(value, "FocalPoint")==0)
    {
    x = camera->GetFocalPoint();
    this->SetUniformParameter(name, 3, x);
    }
  else if (strcmp(value, "Position")==0)
    {
    x = camera->GetPosition();
    this->SetUniformParameter(name, 3, x);
    }
  else if (strcmp(value, "ViewUp")==0)
    {
    x = camera->GetViewUp();
    this->SetUniformParameter(name, 3, x);
    }
  else if (strcmp(value, "DirectionOfProjection")==0)
    {
    x = camera->GetDirectionOfProjection();
    this->SetUniformParameter(name, 3, x);
    }
  else if (strcmp(value, "ViewPlaneNormal") == 0)
    {
    x = camera->GetViewPlaneNormal();
    this->SetUniformParameter(name, 3, x);
    }
  else if (strcmp(value, "ViewShear") == 0)
    {
    x = camera->GetViewShear();
    this->SetUniformParameter(name, 3, x);
    }
  else if (strcmp(value, "WindowCenter") == 0)
    {
    x = camera->GetWindowCenter();
    this->SetUniformParameter(name, 2, x);
    }
  else if (strcmp(value, "ClippingRange") == 0)
    {
    x = camera->GetClippingRange();
    this->SetUniformParameter(name, 2, x);
    }
  else if (strcmp(value, "ViewAngle") == 0)
    {
    double c = camera->GetViewAngle();
    this->SetUniformParameter(name, 1, &c);
    }
  else if (strcmp(value, "EyeAngle") == 0)
    {
    double c = camera->GetEyeAngle();
    this->SetUniformParameter(name, 1, &c);
    }
  else if (strcmp(value, "ParallelScale") == 0)
    {
    double c = camera->GetParallelScale();
    this->SetUniformParameter(name, 1, &c);
    }
  else if (strcmp(value, "Thickness") == 0)
    {
    double c = camera->GetThickness();
    this->SetUniformParameter(name, 1, &c);
    }
  else if (strcmp(value, "Distance") == 0)
    {
    double c = camera->GetDistance();
    this->SetUniformParameter(name, 1, &c);
    }
  else if (strcmp(value, "FocalDisk") == 0)
    {
    double c = camera->GetFocalDisk();
    this->SetUniformParameter(name, 1, &c);
    }
  else if (strcmp(value, "ParallelProjection") == 0)
    {
    double c = camera->GetParallelProjection();
    this->SetUniformParameter(name, 1, &c);
    }
  else if (strcmp(value, "UseHorizontalViewAngle") == 0)
    {
    double c = camera->GetUseHorizontalViewAngle();
    this->SetUniformParameter(name, 1, &c);
    }
  else 
    {
    vtkErrorMacro("Invalid camera property " << value);
    }
}


//-----------------------------------------------------------------------------
void vtkShader::SetPropertyParameter(vtkActor* actor, vtkRenderer* , 
  vtkXMLDataElement* elem)
{
  vtkProperty* property = actor->GetProperty();
  if (property->GetMTime() < this->PassShaderVariablesTime)
    {
    // no need to update.
    return;
    }
  const char* name = elem->GetAttribute("name");
  const char* value = elem->GetAttribute("value");
  if (!value)
    {
    vtkErrorMacro("Missing required attribute 'value' on name=" << name);
    return;
    }
  
  if( strcmp(value,"Color")==0 )
    {
    this->SetUniformParameter(name, 3, property->GetColor());
    }
  else if( strcmp(value,"AmbientColor")==0 )
    {
    this->SetUniformParameter(name, 3, property->GetAmbientColor());
    }
  else if( strcmp(value,"DiffuseColor")==0 )
    {
    this->SetUniformParameter(name, 3, property->GetDiffuseColor());
    }
  else if( strcmp(value,"SpecularColor")==0 )
    {
    this->SetUniformParameter(name, 3, property->GetSpecularColor());
    }
  else if( strcmp(value,"EdgeColor")==0 )
    {
    this->SetUniformParameter(name, 3, property->GetEdgeColor());
    }
  else if( strcmp(value,"Ambient")==0 )
    {
    double v = property->GetAmbient();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"Diffuse")==0 )
    {
    double v = property->GetDiffuse();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"Specular")==0 )
    {
    double v = property->GetSpecular();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"SpecularPower")==0 )
    {
    double v = property->GetSpecularPower();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"Opacity")==0 )
    {
    double v = property->GetOpacity();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"PointSize")==0 )
    {
    double v = property->GetPointSize();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"LineWidth")==0 )
    {
    double v = property->GetLineWidth();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"LineStipplePattern")==0 )
    {
    int v = property->GetLineStipplePattern();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"LineStippleRepeatFactor")==0 )
    {
    int v = property->GetLineStippleRepeatFactor();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"Interpolation")==0 )
    {
    int v = property->GetInterpolation();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"Representation")==0 )
    {
    int v = property->GetRepresentation();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"EdgeVisibility")==0 )
    {
    int v = property->GetEdgeVisibility();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"BackfaceCulling")==0 )
    {
    int v = property->GetBackfaceCulling();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"FrontfaceCulling")==0 )
    {
    int v = property->GetFrontfaceCulling();
    this->SetUniformParameter(name, 1, &v);
    }
  else if( strcmp(value,"MTime")==0 )
    {
    double mtime = static_cast<double>(property->GetMTime());
    this->SetUniformParameter(name, 1, &mtime);
    }
  else
    {
    vtkErrorMacro("Invalid property name for vtkProperty " << value);
    }
}

//-----------------------------------------------------------------------------
void vtkShader::SetLightParameter(vtkActor* , vtkRenderer* renderer, 
  vtkXMLDataElement* elem)
{
  const char* name = elem->GetAttribute("name");
  const char* value = elem->GetAttribute("value");
  if (!value)
    {
    vtkErrorMacro("Missing required attribute 'value'.");
    return;
    }
  int lightid;
  if (!elem->GetScalarAttribute("light_id",lightid))
    {
    lightid = 0;
    }

  vtkLightCollection* lights = renderer->GetLights();
  vtkLight* light = 0;

  vtkCollectionIterator *iter = lights->NewIterator();
  int id = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem(), id++)
    {
    if (id == lightid)
      {
      light = vtkLight::SafeDownCast(iter->GetCurrentObject());
      break;
      }
    }
  iter->Delete();

  if (!light)
    {
    vtkErrorMacro("Failed to locate light with id " << lightid);
    return;
    }

  if (lights->GetMTime() < this->PassShaderVariablesTime &&
    light->GetMTime() < this->PassShaderVariablesTime)
    {
    // no need to update.
    return;
    }
  
  if (strcmp(value, "Position") == 0)
    {
    this->SetUniformParameter(name, 3, light->GetPosition());
    }
  else if (strcmp(value, "FocalPoint") == 0)
    {
    this->SetUniformParameter(name, 3, light->GetFocalPoint());
    }
  else if (strcmp(value, "AmbientColor") == 0)
    {
    this->SetUniformParameter(name, 3, light->GetAmbientColor());
    }
  else if (strcmp(value, "DiffuseColor") == 0)
    {
    this->SetUniformParameter(name, 3, light->GetDiffuseColor());
    }
  else if (strcmp(value, "SpecularColor") == 0)
    {
    this->SetUniformParameter(name, 3, light->GetSpecularColor());
    }
  else if (strcmp(value, "AttenuationValues") == 0)
    {
    this->SetUniformParameter(name, 3, light->GetAttenuationValues());
    }
  else if (strcmp(value, "Intensity") == 0)
    {
    double v = light->GetIntensity();
    this->SetUniformParameter(name, 1, &v);
    }
  else if (strcmp(value, "Exponent") == 0)
    {
    double v = light->GetExponent();
    this->SetUniformParameter(name, 1, &v);
    }
  else if (strcmp(value, "ConeAngle") == 0)
    {
    double v = light->GetConeAngle();
    this->SetUniformParameter(name, 1, &v);
    }
  else if (strcmp(value, "Switch") == 0)
    {
    int v = light->GetSwitch();
    this->SetUniformParameter(name, 1, &v);
    }
  else if (strcmp(value, "Positional") == 0)
    {
    int v = light->GetPositional();
    this->SetUniformParameter(name, 1, &v);
    }
  else if (strcmp(value, "LightType") == 0)
    {
    int v = light->GetLightType();
    this->SetUniformParameter(name, 1, &v);
    }
  else
    {
    vtkErrorMacro("Invalid light property: " << value);
    }

}


//-----------------------------------------------------------------------------
void vtkShader::SetMatrixParameter(vtkActor* , vtkRenderer* , 
  vtkXMLDataElement* elem)
{
  const char* name = elem->GetAttribute("name");
  const char* type = elem->GetAttribute("type");
  if (!type)
    {
    vtkErrorMacro("Missing required attribute 'type' for name=" << name);
    return;
    }
  
  // TODO: for starters, matrices can't be set as Shader Variables.
  const char* cvalue = elem->GetAttribute("value");
  if (!cvalue)
    {
    vtkErrorMacro("Missing required attribute 'value' for name=" << name);
    return;
    }
  int number_of_elements;
  if (!elem->GetScalarAttribute("number_of_elements", number_of_elements) ||
    number_of_elements <= 0)
    {
    vtkErrorMacro("Invalid number_of_elements on name=" << name);
    return;
    }
    
  int order = vtkShader::RowMajor;
  const char* corder = elem->GetAttribute("order");
  if (corder && strcmp(corder, "ColumnMajor") == 0)
    {
    order = vtkShader::ColumnMajor;
    }
    
  if (strcmp(type, "State") == 0)
    {
    vtkstd::vector<vtkstd::string> args;
    vtksys::SystemTools::Split(cvalue, args, ' ');
    if (args.size() != static_cast<unsigned int>(number_of_elements))
      {
      vtkErrorMacro("Mismatch in number_of_elements and actual values!");
      return;
      }
    
    const char* state_matix_type = args[0].c_str();
    const char* transform_type = (number_of_elements > 1)?
      args[1].c_str() : 0;
    this->SetMatrixParameter(name, state_matix_type, transform_type);
    }
  else
    {

    if (strcmp(type, "Float") == 0)
      {
      float *v = new float[number_of_elements];
      if (elem->GetVectorAttribute("value",number_of_elements, v))
        {
        this->SetMatrixParameter(name, number_of_elements, order, v);
        }
      else
        {
        vtkErrorMacro("Failed to obtain value for name=" << name);
        }
      delete [] v;
      }
    else if (strcmp(type, "Double") == 0)
      {
      double *v = new double[number_of_elements];
      if (elem->GetVectorAttribute("value",number_of_elements, v))
        {
        this->SetMatrixParameter(name, number_of_elements, order, v);
        }
      else
        {
        vtkErrorMacro("Failed to obtain value for name=" << name);
        }
      delete [] v;
      }
    else
      {
      vtkErrorMacro("Invalid 'type'='" << type << "' for name=" << name);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkShader::SetSamplerParameter(vtkActor* act, vtkRenderer*, 
  vtkXMLDataElement* elem)
{
  const char* name = elem->GetAttribute("name");
  const char* value = elem->GetAttribute("value");
  if (!value)
    {
    vtkErrorMacro("Missing required attribute 'value' on element "
      "with name=" << name);
    return;
    }

  int texture_id;
  if (!elem->GetScalarAttribute("value", texture_id))
    {
    vtkErrorMacro("Expected interger 'value' for element "
      "with name=" << name);
    return;
    }

  vtkTexture* texture = act->GetProperty()->GetTexture(texture_id);
  
  if (!texture)
    {
    vtkErrorMacro("Property does have texture at index="<<texture_id);
    return;
    }
  
  this->SetSamplerParameter(name, texture);
}

//-----------------------------------------------------------------------------
void vtkShader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Shader Variables: " 
    << this->Internals->UniformVariables.size() << endl;
 
  vtkstd::map<vtkstd::string, vtkShaderUniformVariable>::iterator iter;
  for (iter = this->Internals->UniformVariables.begin();
    iter != this->Internals->UniformVariables.end(); ++iter)
    {
    os << indent << "ShaderVariable: " << endl;
    iter->second.Print(os, indent.GetNextIndent());
    } 
  
  os << indent << "XMLShader: ";
  if (this->XMLShader)
    {
    os << endl;
    this->XMLShader->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
 
  
}
