/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkProgrammableSource.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkProgrammableSource* vtkProgrammableSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkProgrammableSource");
  if(ret)
    {
    return (vtkProgrammableSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkProgrammableSource;
}




// Construct programmable filter with empty execute method.
vtkProgrammableSource::vtkProgrammableSource()
{
  this->ExecuteMethod = NULL;
  this->ExecuteMethodArg = NULL;
  this->ExecuteMethodArgDelete = NULL;

  this->vtkSource::SetNthOutput(0,vtkPolyData::New());
  this->Outputs[0]->Delete();

  this->vtkSource::SetNthOutput(1,vtkStructuredPoints::New());
  this->Outputs[1]->Delete();

  this->vtkSource::SetNthOutput(2,vtkStructuredGrid::New());
  this->Outputs[2]->Delete();

  this->vtkSource::SetNthOutput(3,vtkUnstructuredGrid::New());
  this->Outputs[3]->Delete();

  this->vtkSource::SetNthOutput(4,vtkRectilinearGrid::New());
  this->Outputs[4]->Delete();
}

vtkProgrammableSource::~vtkProgrammableSource()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg)&&(this->ExecuteMethodArgDelete))
    {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
}

// Specify the function to use to generate the source data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableSource::SetExecuteMethod(void (*f)(void *), void *arg)
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
void vtkProgrammableSource::SetExecuteMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ExecuteMethodArgDelete)
    {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
    }
}


// Get the output as a concrete type. This method is typically used by the
// writer of the source function to get the output as a particular type (i.e.,
// it essentially does type casting). It is the users responsibility to know
// the correct type of the output data.
vtkPolyData *vtkProgrammableSource::GetPolyDataOutput()
{
  if (this->NumberOfOutputs < 5)
    {
    return NULL;
    }
  
  return (vtkPolyData *)(this->Outputs[0]);
}

// Get the output as a concrete type.
vtkStructuredPoints *vtkProgrammableSource::GetStructuredPointsOutput()
{
  if (this->NumberOfOutputs < 5)
    {
    return NULL;
    }
  
  return (vtkStructuredPoints *)(this->Outputs[1]);
}

// Get the output as a concrete type.
vtkStructuredGrid *vtkProgrammableSource::GetStructuredGridOutput()
{
  if (this->NumberOfOutputs < 5)
    {
    return NULL;
    }
  
  return (vtkStructuredGrid *)(this->Outputs[2]);
}

// Get the output as a concrete type.
vtkUnstructuredGrid *vtkProgrammableSource::GetUnstructuredGridOutput()
{
  if (this->NumberOfOutputs < 5)
    {
    return NULL;
    }
  
  return (vtkUnstructuredGrid *)(this->Outputs[3]);
}

// Get the output as a concrete type.
vtkRectilinearGrid *vtkProgrammableSource::GetRectilinearGridOutput()
{
  if (this->NumberOfOutputs < 5)
    {
    return NULL;
    }
  
  return (vtkRectilinearGrid *)(this->Outputs[4]);
}

// Override in order to execute. Otherwise, we won't know what the
// whole update extent is.
void vtkProgrammableSource::UpdateInformation()
{
  int idx;

  if ( this->GetMTime() > this->ExecuteTime.GetMTime() )
    {
  // Initialize all the outputs
  for (idx = 0; idx < this->NumberOfOutputs; idx++)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->Initialize(); 
      }
    }
  // If there is a start method, call it
  if ( this->StartMethod )
    {
    (*this->StartMethod)(this->StartMethodArg);
    }

  // Execute this object - we have not aborted yet, and our progress
  // before we start to execute is 0.0.
  this->AbortExecute = 0;
  this->Progress = 0.0;
  this->Execute();

  // If we ended due to aborting, push the progress up to 1.0 (since
  // it probably didn't end there)
  if ( !this->AbortExecute )
    {
    this->UpdateProgress(1.0);
    }

  // Call the end method, if there is one
  if ( this->EndMethod )
    {
    (*this->EndMethod)(this->EndMethodArg);
    }
    
  // Now we have to mark the data as up to data.
  for (idx = 0; idx < this->NumberOfOutputs; ++idx)
    {
    if (this->Outputs[idx])
      {
      this->Outputs[idx]->DataHasBeenGenerated();
      }
    }    

  // Information gets invalidated as soon as Update is called,
  // so validate it again here.
  this->InformationTime.Modified();

  this->ExecuteTime.Modified();
    }

  this->vtkSource::UpdateInformation();

}

void vtkProgrammableSource::UpdateData(vtkDataObject *output)
{
}

void vtkProgrammableSource::Execute()
{
  vtkDebugMacro(<<"Executing programmable filter");

  // Now invoke the procedure, if specified.
  if ( this->ExecuteMethod != NULL )
    {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
    }
}




