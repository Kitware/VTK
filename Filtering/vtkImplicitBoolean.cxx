/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitBoolean.cxx
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
#include <math.h>
#include "vtkImplicitBoolean.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImplicitBoolean* vtkImplicitBoolean::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImplicitBoolean");
  if(ret)
    {
    return (vtkImplicitBoolean*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImplicitBoolean;
}




// Construct with union operation.
vtkImplicitBoolean::vtkImplicitBoolean()
{
  this->OperationType = VTK_UNION;
  this->FunctionList = vtkImplicitFunctionCollection::New();
}

vtkImplicitBoolean::~vtkImplicitBoolean()
{
  this->FunctionList->Delete();
}

unsigned long int vtkImplicitBoolean::GetMTime()
{
  unsigned long int fMtime;
  unsigned long int mtime = this->vtkImplicitFunction::GetMTime();
  vtkImplicitFunction *f;

  for (this->FunctionList->InitTraversal(); 
       (f=this->FunctionList->GetNextItem()); )
    {
    fMtime = f->GetMTime();
    if ( fMtime > mtime )
      {
      mtime = fMtime;
      }
    }
  return mtime;
}

// Add another implicit function to the list of functions.
void vtkImplicitBoolean::AddFunction(vtkImplicitFunction *f)
{
  if ( ! this->FunctionList->IsItemPresent(f) )
    {
    this->Modified();
    this->FunctionList->AddItem(f);
    }
}

// Remove a function from the list of implicit functions to boolean.
void vtkImplicitBoolean::RemoveFunction(vtkImplicitFunction *f)
{
  if ( this->FunctionList->IsItemPresent(f) )
    {
    this->Modified();
    this->FunctionList->RemoveItem(f);
    }
}

// Evaluate boolean combinations of implicit function using current operator.
float vtkImplicitBoolean::EvaluateFunction(float x[3])
{
  float value = 0;
  float v;
  vtkImplicitFunction *f;

  if ( this->OperationType == VTK_UNION )
    { //take minimum value
    for (value = VTK_LARGE_FLOAT, this->FunctionList->InitTraversal(); 
    (f=this->FunctionList->GetNextItem()); )
      {
      if ( (v=f->FunctionValue(x)) < value )
	{
	value = v;
	}
      }
    }

  else if ( this->OperationType == VTK_INTERSECTION )
    { //take maximum value
    for (value=-VTK_LARGE_FLOAT, this->FunctionList->InitTraversal(); 
    (f=this->FunctionList->GetNextItem()); )
      {
      if ( (v=f->FunctionValue(x)) > value )
	{
	value = v;
	}
      }
    }

  else if ( this->OperationType == VTK_UNION_OF_MAGNITUDES )
    { //take minimum absolute value
    for (value = VTK_LARGE_FLOAT, this->FunctionList->InitTraversal(); 
    (f=this->FunctionList->GetNextItem()); )
      {
      if ( (v=fabs(f->FunctionValue(x))) < value )
	{
	value = v;
	}
      }
    }

  else //difference
    {
    vtkImplicitFunction *firstF;
    this->FunctionList->InitTraversal();
    if ( (firstF = this->FunctionList->GetNextItem()) != NULL )
      {
      value = firstF->FunctionValue(x);
      }

    for (this->FunctionList->InitTraversal(); 
    (f=this->FunctionList->GetNextItem()); )
      {
      if ( f != firstF )
        {
        if ( (v=(-1.0)*f->FunctionValue(x)) > value )
	  {
	  value = v;
	  }
        }
      }
    }//else

  return value;
}

// Evaluate gradient of boolean combination.
void vtkImplicitBoolean::EvaluateGradient(float x[3], float g[3])
{
  float value = 0;
  float v;
  vtkImplicitFunction *f;

  if ( this->OperationType == VTK_UNION )
    { //take minimum value
    for (value = VTK_LARGE_FLOAT, this->FunctionList->InitTraversal(); 
    (f=this->FunctionList->GetNextItem()); )
      {
      if ( (v=f->FunctionValue(x)) < value )
        {
        value = v;
        f->FunctionGradient(x,g);
        }
      }
    }

  else if ( this->OperationType == VTK_INTERSECTION )
    { //take maximum value
    for (value=-VTK_LARGE_FLOAT, this->FunctionList->InitTraversal(); 
    (f=this->FunctionList->GetNextItem()); )
      {
      if ( (v=f->FunctionValue(x)) > value ) 
        {
        value = v;
        f->FunctionGradient(x,g);
        }
      }
    }

  if ( this->OperationType == VTK_UNION_OF_MAGNITUDES )
    { //take minimum value
    for (value = VTK_LARGE_FLOAT, this->FunctionList->InitTraversal(); 
    (f=this->FunctionList->GetNextItem()); )
      {
      if ( (v=fabs(f->FunctionValue(x))) < value )
        {
        value = v;
        f->FunctionGradient(x,g);
        }
      }
    }

  else //difference
    {
    float gTemp[3];
    vtkImplicitFunction *firstF;
    this->FunctionList->InitTraversal();
    if ( (firstF = this->FunctionList->GetNextItem()) != NULL )
      {
      value = firstF->FunctionValue(x);
      firstF->FunctionGradient(x,gTemp); 
      g[0] = -1.0*gTemp[0]; g[1] = -1.0*gTemp[1]; g[2] = -1.0*gTemp[2];
      }

    for (this->FunctionList->InitTraversal(); 
    (f=this->FunctionList->GetNextItem()); )
      {
      if ( f != firstF )
        {
        if ( (v=(-1.0)*f->FunctionValue(x)) > value )
          {
          value = v;
          f->FunctionGradient(x,gTemp);
          g[0] = -1.0*gTemp[0]; g[1] = -1.0*gTemp[1]; g[2] = -1.0*gTemp[2];
          }
        }
      }
    }//else
}

void vtkImplicitBoolean::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImplicitFunction::PrintSelf(os,indent);

  os << indent << "Function List:\n";
  this->FunctionList->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Operator Type: ";
  if ( this->OperationType == VTK_INTERSECTION )
    {
    os << "VTK_INTERSECTION\n";
    }
  else if ( this->OperationType == VTK_UNION )
    {
    os << "VTK_UNION\n";
    }
  else
    {
    os << "VTK_INTERSECTION\n";
    }
}
