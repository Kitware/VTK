/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableDataObjectSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkProgrammableDataObjectSource.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkProgrammableDataObjectSource* vtkProgrammableDataObjectSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProgrammableDataObjectSource");
  if(ret)
    {
    return (vtkProgrammableDataObjectSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProgrammableDataObjectSource;
}




// Construct programmable filter with empty execute method.
vtkProgrammableDataObjectSource::vtkProgrammableDataObjectSource()
{
  this->ExecuteMethod = NULL;
  this->ExecuteMethodArg = NULL;
  this->ExecuteMethodArgDelete = NULL;

  this->vtkSource::SetNthOutput(0,vtkDataObject::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

vtkProgrammableDataObjectSource::~vtkProgrammableDataObjectSource()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg)&&(this->ExecuteMethodArgDelete))
    {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
}

vtkDataObject *vtkProgrammableDataObjectSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataObject *)(this->Outputs[0]);
}

// Specify the function to use to generate the source data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableDataObjectSource::SetExecuteMethod(void (*f)(void *), void *arg)
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
void vtkProgrammableDataObjectSource::SetExecuteMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ExecuteMethodArgDelete)
    {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
    }
}


void vtkProgrammableDataObjectSource::Execute()
{
  vtkDebugMacro(<<"Executing programmable data object filter");

  // Now invoke the procedure, if specified.
  if ( this->ExecuteMethod != NULL )
    {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
    }
}

void vtkProgrammableDataObjectSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkSource::PrintSelf(os,indent);
  
  if ( this->ExecuteMethod )
    {
    os << indent << "An ExecuteMethod has been defined\n";
    }
  else
    {
    os << indent << "An ExecuteMethod has NOT been defined\n";
    }

}

