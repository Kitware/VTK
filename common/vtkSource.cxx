/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkSource.cxx
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
#include "vtkSource.h"

#ifndef NULL
#define NULL 0
#endif

vtkSource::vtkSource()
{
  this->Output = NULL;
  this->StartMethod = NULL;
  this->StartMethodArgDelete = NULL;
  this->StartMethodArg = NULL;
  this->EndMethod = NULL;
  this->EndMethodArgDelete = NULL;
  this->EndMethodArg = NULL;
}

int vtkSource::GetDataReleased()
{
  if (this->Output)
    {
    return this->Output->GetDataReleased();
    }
  vtkWarningMacro(<<"Output doesn't exist!");
  return 1;
}

void vtkSource::SetDataReleased(int i)
{
  if (this->Output)
    {
    this->Output->SetDataReleased(i);
    }
  else
    {
    vtkWarningMacro(<<"Output doesn't exist!");
    }
}

int vtkSource::GetReleaseDataFlag()
{
  if (this->Output)
    {
    return this->Output->GetReleaseDataFlag();
    }
  vtkWarningMacro(<<"Output doesn't exist!");
  return 1;
}

void vtkSource::SetReleaseDataFlag(int i)
{
  if (this->Output)
    {
    this->Output->SetReleaseDataFlag(i);
    }
  else
    {
    vtkWarningMacro(<<"Output doesn't exist!");
    }
}

void vtkSource::Update()
{
  // Make sure virtual getMTime method is called since subclasses will overload
  if ( this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    if (this->Output) this->Output->Initialize(); //clear output
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
}

// Description:
// Specify function to be called before object executes.
void vtkSource::SetStartMethod(void (*f)(void *), void *arg)
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
    this->Modified();
    }
}

// Description:
// Specify function to be called after object executes.
void vtkSource::SetEndMethod(void (*f)(void *), void *arg)
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
    this->Modified();
    }
}


// Description:
// Set the arg delete method. This is used to free user memory.
void vtkSource::SetStartMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartMethodArgDelete)
    {
    this->StartMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkSource::SetEndMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndMethodArgDelete)
    {
    this->EndMethodArgDelete = f;
    this->Modified();
    }
}

void vtkSource::Execute()
{
  vtkErrorMacro(<< "Execution of source should be in derived class");
}


void vtkSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->Output )
    {
    os << indent << "Output:\n";
    this->Output->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "No output generated for this filter\n";
    }
}

