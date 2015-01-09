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
  this->Parameters = NULL;
  this->SurfaceShader = new char[strlen("plastic") + 1];
  strcpy (this->SurfaceShader, "plastic");
  this->DisplacementShader = NULL;
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

  delete [] this->Parameters;
}

void vtkRIBProperty::Render(vtkActor *anActor, vtkRenderer *ren)
{
  int ref;

  // Copy this property's ivars into the property to be rendered
  ref = this->Property->GetReferenceCount();
  this->Property->DeepCopy(this);
  //this->Property->SetDeleteMethod(NULL);
  this->Property->SetReferenceCount(ref);

  // Render the property
  this->Property->Render (anActor, ren);
}

void vtkRIBProperty::SetVariable (char *variable, char *value)
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

void vtkRIBProperty::AddVariable (char *variable, char *value)
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

void vtkRIBProperty::SetParameter (char *parameter, char *value)
{
  delete [] this->Parameters;

  // format of line is: "parameter" "value"
  this->Parameters = new char [strlen (parameter) +
                              strlen (value) +
                              7];

  sprintf (this->Parameters, " \"%s\" [%s]", parameter, value);
  this->Modified ();
}

void vtkRIBProperty::AddParameter (char *Parameter, char *value)
{
  if (this->Parameters == NULL)
    {
    this->SetParameter (Parameter, value);
    }
  else
    {
    char *newParameter = new char [strlen (Parameter) +
                                  strlen (value) +
                                  7];

    sprintf (newParameter, " \"%s\" [%s]", Parameter, value);
    char *oldParameters = this->Parameters;

    this->Parameters = new char [strlen (oldParameters) + strlen (newParameter) + 1];
    strcpy (this->Parameters, oldParameters);
    strcat (this->Parameters, newParameter);
    delete [] oldParameters;
    delete [] newParameter;
    this->Modified ();
    }
}

char *vtkRIBProperty::GetParameters ()
{
  return this->Parameters;
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
  if (this->Parameters)
    {
    os << indent << "Parameters: " << this->Parameters;
    }
  else
    {
    os << indent << "Parameters: (none)\n";
    }

}

