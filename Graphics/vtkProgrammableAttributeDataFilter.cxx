/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableAttributeDataFilter.cxx
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
#include "vtkProgrammableAttributeDataFilter.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------------
vtkProgrammableAttributeDataFilter* vtkProgrammableAttributeDataFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProgrammableAttributeDataFilter");
  if(ret)
    {
    return (vtkProgrammableAttributeDataFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProgrammableAttributeDataFilter;
}




vtkProgrammableAttributeDataFilter::vtkProgrammableAttributeDataFilter()
{
  this->ExecuteMethod = NULL;
  this->ExecuteMethodArg = NULL;
  this->ExecuteMethodArgDelete = NULL;
  this->InputList = vtkDataSetCollection::New();
}

vtkProgrammableAttributeDataFilter::~vtkProgrammableAttributeDataFilter()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg)&&(this->ExecuteMethodArgDelete))
    {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
  this->InputList->Delete();
  this->InputList = NULL;
}

// Add a dataset to the list of data to process.
void vtkProgrammableAttributeDataFilter::AddInput(vtkDataSet *ds)
{
  if ( ! this->InputList->IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList->AddItem(ds);
    }
}

// Remove a dataset from the list of data to process.
void vtkProgrammableAttributeDataFilter::RemoveInput(vtkDataSet *ds)
{
  if ( this->InputList->IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList->RemoveItem(ds);
    }
}

// Specify the function to use to operate on the point attribute data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableAttributeDataFilter::SetExecuteMethod(void (*f)(void *), void *arg)
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
void vtkProgrammableAttributeDataFilter::SetExecuteMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ExecuteMethodArgDelete)
    {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
    }
}

void vtkProgrammableAttributeDataFilter::Execute()
{
  vtkDataSet *input = this->GetInput();
  vtkDataSet *output = this->GetOutput();

  vtkDebugMacro(<<"Executing programmable point data filter");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  // Output data is the same as input data by default.
  output->GetCellData()->PassData(input->GetCellData());
  output->GetPointData()->PassData(input->GetPointData());
  
  // Now invoke the procedure, if specified.
  if ( this->ExecuteMethod != NULL )
    {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
    }
}

void vtkProgrammableAttributeDataFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToDataSetFilter::PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList->PrintSelf(os,indent.GetNextIndent());
  
  if ( this->ExecuteMethod )
    {
    os << indent << "An ExecuteMethod has been defined\n";
    }
  else
    {
    os << indent << "An ExecuteMethod has NOT been defined\n";
    }
}



