/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableFilter.cxx
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
#include "vtkProgrammableFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkProgrammableFilter* vtkProgrammableFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProgrammableFilter");
  if(ret)
    {
    return (vtkProgrammableFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProgrammableFilter;
}




// Construct programmable filter with empty execute method.
vtkProgrammableFilter::vtkProgrammableFilter()
{
  this->ExecuteMethod = NULL;
  this->ExecuteMethodArg = NULL;
  this->ExecuteMethodArgDelete = NULL;
}

vtkProgrammableFilter::~vtkProgrammableFilter()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg)&&(this->ExecuteMethodArgDelete))
    {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
}


// Get the input as a concrete type. This method is typically used by the
// writer of the filter function to get the input as a particular type (i.e.,
// it essentially does type casting). It is the users responsibility to know
// the correct type of the input data.
vtkPolyData *vtkProgrammableFilter::GetPolyDataInput()
{
  return (vtkPolyData *)this->GetInput();
}

// Get the input as a concrete type.
vtkStructuredPoints *vtkProgrammableFilter::GetStructuredPointsInput()
{
  return (vtkStructuredPoints *)this->GetInput();
}

// Get the input as a concrete type.
vtkStructuredGrid *vtkProgrammableFilter::GetStructuredGridInput()
{
  return (vtkStructuredGrid *)this->GetInput();
}

// Get the input as a concrete type.
vtkUnstructuredGrid *vtkProgrammableFilter::GetUnstructuredGridInput()
{
  return (vtkUnstructuredGrid *)this->GetInput();
}

// Get the input as a concrete type.
vtkRectilinearGrid *vtkProgrammableFilter::GetRectilinearGridInput()
{
  return (vtkRectilinearGrid *)this->GetInput();
}

// Specify the function to use to operate on the point attribute data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableFilter::SetExecuteMethod(void (*f)(void *), void *arg)
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
void vtkProgrammableFilter::SetExecuteMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ExecuteMethodArgDelete)
    {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
    }
}


void vtkProgrammableFilter::Execute()
{
  vtkDebugMacro(<<"Executing programmable filter");

  // First, copy the input to the output as a starting point
  this->GetOutput()->CopyStructure( this->GetInput() );

  // Now invoke the procedure, if specified.
  if ( this->ExecuteMethod != NULL )
    {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
    }
}

