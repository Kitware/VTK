/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableDataObjectSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgrammableDataObjectSource.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkProgrammableDataObjectSource);

// Construct programmable filter with empty execute method.
vtkProgrammableDataObjectSource::vtkProgrammableDataObjectSource()
{
  this->ExecuteMethod = NULL;
  this->ExecuteMethodArg = NULL;
  this->ExecuteMethodArgDelete = NULL;

  vtkDataObject *output = vtkDataObject::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  output->ReleaseData();
  output->Delete();

  this->SetNumberOfInputPorts(0);
}

vtkProgrammableDataObjectSource::~vtkProgrammableDataObjectSource()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg)&&(this->ExecuteMethodArgDelete))
    {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
}

// Specify the function to use to generate the source data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableDataObjectSource::SetExecuteMethod(
  void (*f)(void *), void *arg)
{
  if ( f != this->ExecuteMethod || arg != this->ExecuteMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ExecuteMethodArg)&&(this->ExecuteMethodArgDelete))
      {
      (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
      }
    this->ExecuteMethod = f;
    this->ExecuteMethodArg = arg;
    this->Modified();
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkProgrammableDataObjectSource::SetExecuteMethodArgDelete(
  void (*f)(void *))
{
  if ( f != this->ExecuteMethodArgDelete)
    {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
    }
}

int vtkProgrammableDataObjectSource::RequestData(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *)
{
  vtkDebugMacro(<<"Executing programmable data object filter");

  // Now invoke the procedure, if specified.
  if ( this->ExecuteMethod != NULL )
    {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
    }

  return 1;
}

void vtkProgrammableDataObjectSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  if ( this->ExecuteMethod )
    {
    os << indent << "An ExecuteMethod has been defined\n";
    }
  else
    {
    os << indent << "An ExecuteMethod has NOT been defined\n";
    }
}
