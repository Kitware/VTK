/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableAttributeDataFilter.cxx
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
#include "vtkProgrammableAttributeDataFilter.h"

vtkProgrammableAttributeDataFilter::vtkProgrammableAttributeDataFilter()
{
  this->ExecuteMethod = NULL;
  this->ExecuteMethodArg = NULL;
}

// Description:
// Add a dataset to the list of data to process.
void vtkProgrammableAttributeDataFilter::AddInput(vtkDataSet *ds)
{
  if ( ! this->InputList.IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList.AddItem(ds);
    }
}

// Description:
// Remove a dataset from the list of data to process.
void vtkProgrammableAttributeDataFilter::RemoveInput(vtkDataSet *ds)
{
  if ( this->InputList.IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList.RemoveItem(ds);
    }
}

// Description:
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

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkProgrammableAttributeDataFilter::SetExecuteMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ExecuteMethodArgDelete)
    {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
    }
}

void vtkProgrammableAttributeDataFilter::Update()
{
  unsigned long int mtime, dsMtime;
  vtkDataSet *ds;

  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  // Update the inputs
  this->Updating = 1;

  mtime = this->GetMTime();
  this->Input->Update();
  if (this->Input->GetMTime() > mtime ) mtime = this->Input->GetMTime();
  
  for (this->InputList.InitTraversal(); (ds = this->InputList.GetNextItem()); )
    {
    ds->Update();
    dsMtime = ds->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  this->Updating = 0;

  // see whether we need to execute
  if ( mtime > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() )
      {
      this->Input->ForceUpdate();
      }

    for ( this->InputList.InitTraversal(); (ds=this->InputList.GetNextItem()); )
      {
      if ( ds->GetDataReleased() ) ds->ForceUpdate();
      }

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    ((vtkDataSet *)this->Output)->CopyStructure((vtkDataSet *)this->Input);
    // reset AbortExecute flag and Progress
    this->AbortExecute = 0;
    this->Progress = 0.0;
    this->Execute();
    this->ExecuteTime.Modified();
    if ( !this->AbortExecute ) this->UpdateProgress(1.0);
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) 
    {
    this->Input->ReleaseData();
    }
  
  for (this->InputList.InitTraversal(); (ds = this->InputList.GetNextItem()); )
    if ( ds->ShouldIReleaseData() ) ds->ReleaseData();
}

void vtkProgrammableAttributeDataFilter::Execute()
{
  vtkDataSet *input=(vtkDataSet *)this->Input;
  vtkDataSet *output=(vtkDataSet *)this->Output;

  vtkDebugMacro(<<"Executing programmable point data filter");

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
  vtkFilter::PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList.PrintSelf(os,indent.GetNextIndent());
  
  if ( this->ExecuteMethod )
    {
    os << indent << "An ExecuteMethod has been defined\n";
    }
  else
    {
    os << indent << "An ExecuteMethod has NOT been defined\n";
    }
}



