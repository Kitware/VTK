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

#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

//-----------------------------------------------------------------------------
// Helper method.
static inline int vtkShaderGetType(const char* type)
{
  if (!type)
    {
    return 0;
    }
  if (strcmp(type,"double")==0 ||
      strcmp(type,"double1")==0 ||
      strcmp(type,"double2")==0 ||
      strcmp(type,"double3")==0 ||
      strcmp(type,"double4")==0 )
    {
    return VTK_DOUBLE;
    }

  // XML attributes should reflect native shader types
  if (strcmp(type, "float")==0  ||
      strcmp(type, "float1")==0 ||
      strcmp(type, "float2")==0 ||
      strcmp(type, "float3")==0 ||
      strcmp(type, "float4")==0 ||

      strcmp(type, "vec1")==0 ||
      strcmp(type, "vec2")==0 ||
      strcmp(type, "vec3")==0 ||
      strcmp(type, "vec4")==0 ||

      strcmp(type, "mat2")==0 ||
      strcmp(type, "mat3")==0 ||
      strcmp(type, "mat4")==0  )
    {
    return VTK_FLOAT;
    }
  if (strcmp(type, "int")==0 ||
      strcmp(type, "ivec2")==0 ||
      strcmp(type, "ivec3")==0 ||
      strcmp(type, "ivec4")==0 )
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
    : Name(),
      NumberOfValues(0),
      Type(0),
      IntValues(NULL),
      FloatValues(NULL),
      DoubleValues(NULL)
    {
    }

  vtkShaderUniformVariable(const char* name, int num, const int* val)
    : Name(),
      NumberOfValues(0),
      Type(0),
      IntValues(NULL),
      FloatValues(NULL),
      DoubleValues(NULL)
    {
    this->SetName(name);
    this->NumberOfValues = num;
    this->Type = VTK_INT;
    this->IntValues = new int[num];
    for (int i=0; i < num; i++)
      {
      this->IntValues[i] = val[i];
      }
    }

  vtkShaderUniformVariable(const char* name, int num, const double* val)
    : Name(),
      NumberOfValues(0),
      Type(0),
      IntValues(NULL),
      FloatValues(NULL),
      DoubleValues(NULL)
    {
    this->SetName(name);
    this->NumberOfValues = num;
    this->Type = VTK_DOUBLE;
    this->DoubleValues = new double[num];
    for (int i=0; i < num; i++)
      {
      this->DoubleValues[i] = val[i];
      }
    }

  vtkShaderUniformVariable(const char* name, int num, const float* val)
    : Name(),
      NumberOfValues(0),
      Type(0),
      IntValues(NULL),
      FloatValues(NULL),
      DoubleValues(NULL)
    {
    this->SetName(name);
    this->NumberOfValues = num;
    this->Type = VTK_FLOAT;
    this->FloatValues = new float[num];
    for (int i=0; i < num; i++)
      {
      this->FloatValues[i] = val[i];
      }
    }

  // A copy constructor is required to use a class as
  // a map value.
  vtkShaderUniformVariable(const vtkShaderUniformVariable& x)
    {
    this->SetName( x.GetName() );
    this->NumberOfValues = x.GetNumberOfValues();
    this->Type = x.GetType();
    this->IntValues = NULL;
    this->FloatValues = NULL;
    this->DoubleValues = NULL;
    if ( (this->Type == VTK_INT) && (this->NumberOfValues > 0) )
      {
      this->IntValues = new int[this->NumberOfValues];
      x.GetValue( this->IntValues );
      }
    else if ( (this->Type == VTK_FLOAT) && (this->NumberOfValues > 0) )
      {
      this->FloatValues = new float[this->NumberOfValues];
      x.GetValue( this->FloatValues );
      }
    else if ( (this->Type == VTK_DOUBLE) && (this->NumberOfValues > 0) )
      {
      this->DoubleValues = new double[this->NumberOfValues];
      x.GetValue( this->DoubleValues );
      }
    }

  // Don't allow the default assignment operator to copy pointers
  // that might be made invalid later if the original objects move,
  // for instance, in a map operation.
  void operator=(const vtkShaderUniformVariable& x)
    {
    this->SetName( x.GetName() );
    this->NumberOfValues = x.GetNumberOfValues();
    this->Type = x.GetType();

    if (this->IntValues)
      {
      delete [] this->IntValues;
      this->IntValues = NULL;
      }
    if (this->FloatValues)
      {
      delete [] this->FloatValues;
      this->FloatValues = NULL;
      }
    if (this->DoubleValues)
      {
      delete [] this->DoubleValues;
      this->DoubleValues = NULL;
      }

    if ( (this->Type == VTK_INT) && (this->NumberOfValues > 0) )
      {
      this->IntValues = new int[this->NumberOfValues];
      x.GetValue( this->IntValues );
      }
    else if ( (this->Type == VTK_FLOAT) && (this->NumberOfValues > 0) )
      {
      this->FloatValues = new float[this->NumberOfValues];
      x.GetValue( this->FloatValues );
      }
    else if ( (this->Type == VTK_DOUBLE) && (this->NumberOfValues > 0) )
      {
      this->DoubleValues = new double[this->NumberOfValues];
      x.GetValue( this->DoubleValues );
      }
    }



  int GetType() const { return this->Type; }
  int GetNumberOfValues() const { return this->NumberOfValues; }

  int GetValue(int *a) const
    {
    if( (this->Type == VTK_INT) && this->IntValues )
      {
      for (int i=0; i < this->NumberOfValues; i++)
        {
        a[i] = this->IntValues[i];
        }
      return 1;
      }
    return 0;
    }

  int GetValue(float *a) const
    {
    if( (this->Type == VTK_FLOAT) && this->FloatValues )
      {
      for (int i=0; i < this->NumberOfValues; i++)
        {
        a[i] = this->FloatValues[i];
        }
      return 1;
      }
    return 0;
    }

  int GetValue(double *a) const
    {
    if( (this->Type == VTK_DOUBLE) && this->DoubleValues )
      {
      for (int i=0; i < this->NumberOfValues; i++)
        {
        a[i] = this->DoubleValues[i];
        }
      return 1;
      }
    return 0;
    }



  ~vtkShaderUniformVariable()
    {
    if (this->IntValues)
      {
      delete [] this->IntValues;
      this->IntValues = NULL;
      }
    if (this->FloatValues)
      {
      delete [] this->FloatValues;
      this->FloatValues = NULL;
      }
    if (this->DoubleValues)
      {
      delete [] this->DoubleValues;
      this->DoubleValues = NULL;
      }
    }

  void Print(ostream& os, vtkIndent indent)
    {
    int i;
    os << indent << "Name: " << ((this->GetName())? this->GetName() : "(none)") << endl;
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

  const char* GetName() const
    {
    return this->Name.c_str();
    }
  void SetName(const char* name)
    {
    if (name)
      {
      this->Name = name;
      }
    }
private:
  std::string Name;
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
  std::map<std::string, vtkShaderUniformVariable> UniformVariables;
};

//-----------------------------------------------------------------------------
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
  if( !this->XMLShader )
    {
    return;
    }

  if( !this->XMLShader->GetRootElement() )
    {
    return;
    }

  this->SetShaderParameters(actor, renderer, this->XMLShader->GetRootElement());
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
    vtkWarningMacro("Need more info to build a Shader Variables!");
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
      vtkErrorMacro("Uniform parameter missing required attribute 'name' " << *elem);
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
    else if (strcmp(tagname, "ApplicationUniform") == 0)
      {
      this->SetApplicationParameter(elem);
      }
    else
      {
      vtkErrorMacro("Invalid tag: " << tagname);
      }
    }
}


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

    int number_of_elements = 0;
    if (!elem->GetScalarAttribute("number_of_elements", number_of_elements))
      {
      vtkErrorMacro("Missing return attribute 'number_of_elements' " << name );
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
  if (this->GetMTime() < this->PassShaderVariablesTime &&
    camera->GetMTime() < this->PassShaderVariablesTime)
    {
    return; // no need to update.
    }
  const char* name = elem->GetAttribute("name");
  const char* value = elem->GetAttribute("value");

  if (!name)
    {
    vtkErrorMacro("Missing required attribute 'name' on name=");
    return;
    }

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
  if (!name)
    {
    vtkErrorMacro("Missing required attribute 'name'");
    return;
    }

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

  // If number of lights is requested then we don;t need to locate the light
  if (strcmp(value, "NumberOfLights") == 0)
    {
    int v = lights->GetNumberOfItems();
    this->SetUniformParameter(name, 1, &v);
    return;
    }

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
// FIXME: Cg allows non-square matrices to be set and program parameters, that
// should be reflected here as well, but I'm not sure just how.
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
  // Matrices CAN be set as shader variables, specifically, they can
  // be used as uniform variables to both fragment and vertex programs.
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

  // FIXME : 'State' is only meaningful in a Cg context, so it should be in
  // vtkCgShader and not in vtkShader
  if (strcmp(type, "State") == 0)
    {
    std::vector<std::string> args;
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

    if ( (strcmp(type, "float")==0) ||
         (strcmp(type,"mat2")==0) ||
         (strcmp(type,"mat3")==0) ||
         (strcmp(type,"mat4")==0) )
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
    else if (strcmp(type, "double") == 0)
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


  vtkTexture* texture = act->GetProperty()->GetTexture(value);

  if (!texture)
    {
    vtkErrorMacro("Property does have texture with name=" << value);
    return;
    }

  int texture_unit = act->GetProperty()->GetTextureUnit(value);
  this->SetSamplerParameter(name, texture, texture_unit);
}

//-----------------------------------------------------------------------------
void vtkShader::SetApplicationParameter(vtkXMLDataElement* elem)
{
  // 'name' is the variable name in the hardware shader program.
  const char* name = elem->GetAttribute("name");
  if (!name)
    {
    vtkErrorMacro("Missing required attribute 'name' on element.");
    return;
    }

  // 'value' is the variable name in the application.
  const char* value = elem->GetAttribute("value");
  if (!value)
    {
    value = name;
    }

  // check to see if the application has set a variable named 'value'
  // If it exists, set it as a uniform parameter
  if( this->HasShaderVariable(value) )
    {
    vtkShaderUniformVariable var = this->Internals->UniformVariables.find(value)->second;
    if( var.GetType() == VTK_INT )
      {
      std::vector<int> x(4,0);
      if( var.GetValue(&x[0])==1 )
        {
        this->SetUniformParameter( name,
                                   var.GetNumberOfValues(),
                                   &x[0]);
        }
      }
    else if( var.GetType() == VTK_FLOAT )
      {
      std::vector<float> x(4,0.0);
      if( var.GetValue(&x[0])==1 )
        {
        this->SetUniformParameter( name,
                                   var.GetNumberOfValues(),
                                   &x[0]);
        }
      }
    else if( var.GetType() == VTK_DOUBLE )
      {
      std::vector<double> x(4,0.0);
      if( var.GetValue(&x[0])==1 )
        {
        this->SetUniformParameter( name,
                                   var.GetNumberOfValues(),
                                   &x[0]);
        }
      }
    }
  else
    {
    vtkErrorMacro("Shader requires application variable " << name
      << " which is missing.");
    }
}

//-----------------------------------------------------------------------------
int vtkShader::GetScope()
{
  return (this->XMLShader? this->XMLShader->GetScope() : vtkXMLShader::SCOPE_NONE);
}

//-----------------------------------------------------------------------------
void vtkShader::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Shader Variables: "
    << this->Internals->UniformVariables.size() << endl;

  std::map<std::string, vtkShaderUniformVariable>::iterator iter;
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
