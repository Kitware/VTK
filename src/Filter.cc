/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Filter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "Filter.hh"

// Description:
// Construct new filter without start or end methods.
vtkFilter::vtkFilter()
{
  this->Input = NULL;

  this->StartMethod = NULL;
  this->StartMethodArgDelete = NULL;
  this->StartMethodArg = NULL;
  this->EndMethod = NULL;
  this->EndMethodArgDelete = NULL;
  this->EndMethodArg = NULL;

  this->Updating = 0;
}

int vtkFilter::GetDataReleased()
{
  vtk_ErrorMacro(<<"Method should be implemented by subclass!");
  return 1;
}

void vtkFilter::SetDataReleased(int flag)
{
  vtk_ErrorMacro(<<"Method should be implemented by subclass!");
}

// Description:
// Update input to this filter and the filter itself.
void vtkFilter::UpdateFilter()
{
  // make sure input is available
  if ( !this->Input )
    {
    vtk_ErrorMacro(<< "No input!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->_GetMTime() || 
  this->_GetMTime() > this->ExecuteTime ||
  this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  if ( this->Input->ShouldIReleaseData() ) this->Input->ReleaseData();
}

// Description:
// Set the filter start method. The start method is invoked before the 
// filter executes.
void vtkFilter::SetStartMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartMethod || arg != this->StartMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->StartMethodArg)&&(this->StartMethodArgDelete))
      {
      (*this->StartMethodArgDelete)(this->StartMethodArg);
      }
    this->StartMethod = f;
    this->StartMethodArg = arg;
    this->_Modified();
    }
}

// Description:
// Set the filter end method. The end method is invoked after the 
// filter executes.
void vtkFilter::SetEndMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndMethod || arg != this->EndMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EndMethodArg)&&(this->EndMethodArgDelete))
      {
      (*this->EndMethodArgDelete)(this->EndMethodArg);
      }
    this->EndMethod = f;
    this->EndMethodArg = arg;
    this->_Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkFilter::SetStartMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartMethodArgDelete)
    {
    this->StartMethodArgDelete = f;
    this->_Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkFilter::SetEndMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndMethodArgDelete)
    {
    this->EndMethodArgDelete = f;
    this->_Modified();
    }
}

void vtkFilter::Execute()
{
  vtk_ErrorMacro(<< "Execution of filter should be in derived class");
}

void vtkFilter::_PrintSelf(ostream& os, vtkIndent indent)
{
  vtkLWObject::_PrintSelf(os,indent);

  if ( this->StartMethod )
    {
    os << indent << "Start Method: (" << (void *)this->StartMethod << ")\n";
    }
  else
    {
    os << indent << "Start Method: (none)\n";
    }

  if ( this->EndMethod )
    {
    os << indent << "End Method: (" << (void *)this->EndMethod << ")\n";
    }
  else
    {
    os << indent << "End Method: (none)\n";
    }

  os << indent << "Execute Time: " <<this->ExecuteTime.GetMTime() << "\n";

  if ( this->Input )
    {
    os << indent << "Input: (" << (void *)this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
}

