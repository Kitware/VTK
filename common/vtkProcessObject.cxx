/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkProcessObject.cxx
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
#include "vtkProcessObject.h"

// Definition:
// Instantiate object with no start, end, or progress methods.
vtkProcessObject::vtkProcessObject()
{
  this->StartMethod = NULL;
  this->StartMethodArgDelete = NULL;
  this->StartMethodArg = NULL;
  this->ProgressMethod = NULL;
  this->ProgressMethodArgDelete = NULL;
  this->ProgressMethodArg = NULL;
  this->EndMethod = NULL;
  this->EndMethodArgDelete = NULL;
  this->EndMethodArg = NULL;
  this->AbortExecute = 0;
  this->Progress = 0.0;
}

// Description:
// Update the progress of the process object. If a ProgressMethod exists, executes it. 
// Then set the Progress ivar to amount. The parameter amount should range between (0,1).
void vtkProcessObject::UpdateProgress(float amount)
{
  this->Progress = amount;
  if ( this->ProgressMethod )
    {
    (*this->ProgressMethod)(this->ProgressMethodArg);
    }
}

// Description:
// Specify function to be called before object executes.
void vtkProcessObject::SetStartMethod(void (*f)(void *), void *arg)
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
// Specify function to be called to show progress of filter
void vtkProcessObject::SetProgressMethod(void (*f)(void *), void *arg)
{
  if ( f != this->ProgressMethod || arg != this->ProgressMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ProgressMethodArg)&&(this->ProgressMethodArgDelete))
      {
      (*this->ProgressMethodArgDelete)(this->ProgressMethodArg);
      }
    this->ProgressMethod = f;
    this->ProgressMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Specify function to be called after object executes.
void vtkProcessObject::SetEndMethod(void (*f)(void *), void *arg)
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
void vtkProcessObject::SetStartMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartMethodArgDelete)
    {
    this->StartMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkProcessObject::SetProgressMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ProgressMethodArgDelete)
    {
    this->ProgressMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkProcessObject::SetEndMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndMethodArgDelete)
    {
    this->EndMethodArgDelete = f;
    this->Modified();
    }
}

void vtkProcessObject::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->StartMethod ) os << indent << "Start Method defined\n";
  else os << indent <<"No Start Method\n";

  if ( this->ProgressMethod ) os << indent << "Progress Method defined\n";
  else os << indent << "No Progress Method\n";

  if ( this->EndMethod ) os << indent << "End Method defined\n";
  else os << indent << "No End Method\n";

  os << indent << "AbortExecute: " << (this->AbortExecute ? "On\n" : "Off\n");
  os << indent << "Progress: " << this->Progress << "\n";
}



