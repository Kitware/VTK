/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRIBProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRIBProperty.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkRIBProperty);

vtkRIBProperty::vtkRIBProperty ()
{
  this->Declarations = NULL;
  this->SurfaceShaderParameters = NULL;
  this->DisplacementShaderParameters = NULL;
  this->SurfaceShader = new char[strlen("plastic") + 1];
  strcpy (this->SurfaceShader, "plastic");
  this->DisplacementShader = NULL;
  this->SurfaceShaderUsesDefaultParameters = true;

  // create a vtkProperty that can be rendered
  this->Property = vtkProperty::New ();
}

vtkRIBProperty::~vtkRIBProperty()
{
  delete [] this->SurfaceShader;
  delete [] this->DisplacementShader;
  delete [] this->Declarations;

  if (this->Property)
  {
    this->Property->Delete ();
  }

  delete [] this->SurfaceShaderParameters;
  delete [] this->DisplacementShaderParameters;
}

void vtkRIBProperty::Render(vtkActor *anActor, vtkRenderer *ren)
{
  int ref;

  // Copy this property's ivars into the property to be rendered
  ref = this->Property->GetReferenceCount();
  this->Property->DeepCopy(this);
  this->Property->SetReferenceCount(ref);

  // Render the property
  this->Property->Render (anActor, ren);
}

void vtkRIBProperty::SetVariable (const char *variable, const char *value)
{
  delete [] this->Declarations;

  // format of line is: Declare "variable" "type"\n
  this->Declarations = new char [strlen ("Declare ") +
                                 strlen (variable) +
                                 strlen (value) +
                                 8];

  sprintf (this->Declarations, "Declare \"%s\" \"%s\"\n", variable, value);
  this->Modified ();
}

void vtkRIBProperty::AddVariable (const char *variable, const char *value)
{
  if (this->Declarations == NULL)
  {
    this->SetVariable (variable, value);
  }
  else
  {
    char *newVariable = new char [strlen ("Declare ") +
                                  strlen (variable) +
                                  strlen (value) +
                                  8];

    sprintf (newVariable, "Declare \"%s\" \"%s\"\n", variable, value);
    char *oldDeclarations = this->Declarations;

    this->Declarations = new char [strlen (oldDeclarations) + strlen (newVariable) + 1];
    strcpy (this->Declarations, oldDeclarations);
    strcat (this->Declarations, newVariable);
    delete [] oldDeclarations;
    delete [] newVariable;
    this->Modified ();
  }
}

void vtkRIBProperty::SetParameter (const char *parameter, const char *value)
{
  vtkWarningMacro("vtkRIBProperty::SetParameter is deprecated. Using SetSurfaceShaderParameter instead.");
  this->SetSurfaceShaderParameter(parameter, value);
}

void vtkRIBProperty::SetSurfaceShaderParameter (const char *parameter, const char *value)
{
  delete [] this->SurfaceShaderParameters;

  // format of line is: "parameter" "value"
  this->SurfaceShaderParameters = new char [strlen (parameter) +
                                                  strlen (value) +
                                                  7];

  sprintf (this->SurfaceShaderParameters, " \"%s\" [%s]", parameter, value);
  this->Modified ();
}

void vtkRIBProperty::SetDisplacementShaderParameter (const char *parameter, const char *value)
{
  delete [] this->DisplacementShaderParameters;

  // format of line is: "parameter" "value"
  this->DisplacementShaderParameters = new char [strlen (parameter) +
                                                  strlen (value) +
                                                  7];

  sprintf (this->DisplacementShaderParameters, " \"%s\" [%s]", parameter, value);
  this->Modified ();
}

void vtkRIBProperty::AddParameter (const char *parameter, const char *value)
{
  vtkWarningMacro("vtkRIBProperty::AddParameter is deprecated. Using AddSurfaceShaderParameter instead.");
  this->AddSurfaceShaderParameter(parameter, value);
}

void vtkRIBProperty::AddSurfaceShaderParameter (const char *SurfaceShaderParameter, const char *value)
{
  if (this->SurfaceShaderParameters == NULL)
  {
    this->SetSurfaceShaderParameter (SurfaceShaderParameter, value);
  }
  else
  {
    char *newSurfaceShaderParameter = new char [strlen (SurfaceShaderParameter) +
                                  strlen (value) +
                                  7];

    sprintf (newSurfaceShaderParameter, " \"%s\" [%s]", SurfaceShaderParameter, value);
    char *oldSurfaceShaderParameters = this->SurfaceShaderParameters;

    this->SurfaceShaderParameters = new char [strlen (oldSurfaceShaderParameters) + strlen (newSurfaceShaderParameter) + 1];
    strcpy (this->SurfaceShaderParameters, oldSurfaceShaderParameters);
    strcat (this->SurfaceShaderParameters, newSurfaceShaderParameter);
    delete [] oldSurfaceShaderParameters;
    delete [] newSurfaceShaderParameter;
    this->Modified ();
  }
}

void vtkRIBProperty::AddDisplacementShaderParameter (const char *DisplacementShaderParameter, const char *value)
{
  if (this->DisplacementShaderParameters == NULL)
  {
    this->SetDisplacementShaderParameter (DisplacementShaderParameter, value);
  }
  else
  {
    char *newDisplacementShaderParameter = new char [strlen (DisplacementShaderParameter) +
                                  strlen (value) +
                                  7];

    sprintf (newDisplacementShaderParameter, " \"%s\" [%s]", DisplacementShaderParameter, value);
    char *oldDisplacementShaderParameters = this->DisplacementShaderParameters;

    this->DisplacementShaderParameters = new char [strlen (oldDisplacementShaderParameters) + strlen (newDisplacementShaderParameter) + 1];
    strcpy (this->DisplacementShaderParameters, oldDisplacementShaderParameters);
    strcat (this->DisplacementShaderParameters, newDisplacementShaderParameter);
    delete [] oldDisplacementShaderParameters;
    delete [] newDisplacementShaderParameter;
    this->Modified ();
  }
}

char *vtkRIBProperty::GetParameters ()
{
  vtkWarningMacro("vtkRIBProperty::GetParameters is deprecated. Using GetSurfaceShaderParameter instead.");
  return this->GetSurfaceShaderParameters();
}

char *vtkRIBProperty::GetSurfaceShaderParameters ()
{
  return this->SurfaceShaderParameters;
}

char *vtkRIBProperty::GetDisplacementShaderParameters ()
{
  return this->DisplacementShaderParameters;
}

char *vtkRIBProperty::GetDeclarations ()
{
  return this->Declarations;
}

void vtkRIBProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->SurfaceShader)
  {
    os << indent << "SurfaceShader: " << this->SurfaceShader << "\n";
  }
  else
  {
    os << indent << "SurfaceShader: (none)\n";
  }
  if (this->DisplacementShader)
  {
    os << indent << "DisplacementShader: " << this->DisplacementShader << "\n";
  }
  else
  {
    os << indent << "DisplacementShader: (none)\n";
  }
  if (this->Declarations)
  {
    os << indent << "Declarations: " << this->Declarations;
  }
  else
  {
    os << indent << "Declarations: (none)\n";
  }
  if (this->SurfaceShaderParameters)
  {
    os << indent << "SurfaceShaderParameters: " << this->SurfaceShaderParameters;
  }
  else
  {
    os << indent << "SurfaceShaderParameters: (none)\n";
  }
  if (this->DisplacementShaderParameters)
  {
    os << indent << "DisplacementShaderParameters: " << this->DisplacementShaderParameters;
  }
  else
  {
    os << indent << "DisplacementShaderParameters: (none)\n";
  }
  os << indent << "SurfaceShaderUsesDefaultParameters: "
     << this->GetSurfaceShaderUsesDefaultParameters() << std::endl;
}

