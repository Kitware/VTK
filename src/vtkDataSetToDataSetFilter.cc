/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetToDataSetFilter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkDataSetToDataSetFilter.hh"

// Description:
// Specify the input data or filter.
void vtkDataSetToDataSetFilter::SetInput(vtkDataSet *input)
{
  if ( this->Input != input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)input);
    this->Input = input;
    this->Modified();

    if ( this->Input == NULL ) return;

    if ( ! this->Output )
      {
      this->Output = this->Input->MakeObject();
      this->Output->SetSource(this);
      return;
      }

    // since the input has changed we might need to create a new output
    if (!strcmp(this->Output->GetClassName(),this->Input->GetClassName()))
      {
      this->Output->Delete();
      this->Output = this->Input->MakeObject();
      this->Output->SetSource(this);
      vtkWarningMacro(<<" a new output had to be created since the input type changed.");
      }
    }
}

// Description:
// Update input to this filter and the filter itself. Note that we are 
// overloading this method because the output is an abstract dataset type.
// This requires special treatment.
void vtkDataSetToDataSetFilter::Update()
{
  // make sure output has been created
  if ( !this->Output )
    {
    vtkErrorMacro(<< "No output has been created...need to set input");
    return;
    }

  // make sure input is available
  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input...can't execute!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if ( this->Input->GetMTime() > this->ExecuteTime ||
  this->GetMTime() > this->ExecuteTime )
    {
    if ( this->Input->GetDataReleased() )
      {
      this->Input->ForceUpdate();
      }

    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    // copy topological/geometric structure from input
    this->Output->CopyStructure(this->Input);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}

  
// Description:
// Get the output of this filter. If output is NULL then input hasn't been set
// which is necessary for abstract objects.

vtkDataSet *vtkDataSetToDataSetFilter::GetOutput()
{
  if ( this->Output == NULL )
    {
    vtkErrorMacro(<<"Abstract filters require input to be set before output can be retrieved");
    }
  return this->Output;
}

