/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitBoolean.cc
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
#include <math.h>
#include "vtkImplicitBoolean.hh"

// Description:
// 
vtkImplicitBoolean::vtkImplicitBoolean()
{
  this->OperationType = VTK_UNION;
}

vtkImplicitBoolean::~vtkImplicitBoolean()
{
}

unsigned long int vtkImplicitBoolean::GetMTime()
{
  unsigned long int fMtime;
  unsigned long int mtime = this->vtkImplicitFunction::GetMTime();
  vtkImplicitFunction *f;

  for (this->FunctionList.InitTraversal(); 
       (f=this->FunctionList.GetNextItem()); )
    {
    fMtime = f->GetMTime();
    if ( fMtime > mtime ) mtime = fMtime;
    }
  return mtime;
}

// Description:
// Add another implicit function to the list of functions.
void vtkImplicitBoolean::AddFunction(vtkImplicitFunction *f)
{
  if ( ! this->FunctionList.IsItemPresent(f) )
    {
    this->Modified();
    this->FunctionList.AddItem(f);
    }
}

// Description:
// Remove a function from the list of implicit functions to boolean.
void vtkImplicitBoolean::RemoveFunction(vtkImplicitFunction *f)
{
  if ( this->FunctionList.IsItemPresent(f) )
    {
    this->Modified();
    this->FunctionList.RemoveItem(f);
    }
}

// Description
// Evaluate boolean combinations of implicit function using current operator.
float vtkImplicitBoolean::EvaluateFunction(float x[3])
{
  float value = 0;
  float v;
  vtkImplicitFunction *f;

  if ( this->OperationType == VTK_UNION )
    { //take minimum value
    for (value = VTK_LARGE_FLOAT, this->FunctionList.InitTraversal(); 
	 (f=this->FunctionList.GetNextItem()); )
      {
      if ( (v=f->FunctionValue(x)) < value ) value = v;
      }
    }

  else if ( this->OperationType == VTK_INTERSECTION )
    { //take maximum value
    for (value=-VTK_LARGE_FLOAT, this->FunctionList.InitTraversal(); 
	 (f=this->FunctionList.GetNextItem()); )
      {
      if ( (v=f->FunctionValue(x)) > value ) value = v;
      }
    }

  else if ( this->OperationType == VTK_UNION_OF_MAGNITUDES )
    { //take minimum absolute value
    for (value = VTK_LARGE_FLOAT, this->FunctionList.InitTraversal(); 
	 (f=this->FunctionList.GetNextItem()); )
      {
      if ( (v=fabs(f->FunctionValue(x))) < value ) value = v;
      }
    }

  else //difference
    {
    vtkImplicitFunction *firstF;
    this->FunctionList.InitTraversal();
    if ( (firstF = this->FunctionList.GetNextItem()) != NULL )
      value = firstF->FunctionValue(x);

    for (this->FunctionList.InitTraversal(); 
	 (f=this->FunctionList.GetNextItem()); )
      {
      if ( f != firstF )
        {
        if ( (v=(-1.0)*f->FunctionValue(x)) > value ) value = v;
        }
      }
    }//else

  return value;
}

// Description
// Evaluate gradient of boolean combination.
void vtkImplicitBoolean::EvaluateGradient(float x[3], float g[3])
{
  float value = 0;
  float v;
  vtkImplicitFunction *f;

  if ( this->OperationType == VTK_UNION )
    { //take minimum value
    for (value = VTK_LARGE_FLOAT, this->FunctionList.InitTraversal(); 
	 (f=this->FunctionList.GetNextItem()); )
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
    for (value=-VTK_LARGE_FLOAT, this->FunctionList.InitTraversal(); 
	 (f=this->FunctionList.GetNextItem()); )
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
    for (value = VTK_LARGE_FLOAT, this->FunctionList.InitTraversal(); 
	 (f=this->FunctionList.GetNextItem()); )
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
    this->FunctionList.InitTraversal();
    if ( (firstF = this->FunctionList.GetNextItem()) != NULL )
      {
      value = firstF->FunctionValue(x);
      firstF->FunctionGradient(x,gTemp); 
      g[0] = -1.0*gTemp[0]; g[1] = -1.0*gTemp[1]; g[2] = -1.0*gTemp[2];
      }

    for (this->FunctionList.InitTraversal(); 
	 (f=this->FunctionList.GetNextItem()); )
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
  this->FunctionList.PrintSelf(os,indent.GetNextIndent());

  os << indent << "Operator Type: ";
  if ( this->OperationType == VTK_INTERSECTION ) os << "VTK_INTERSECTION\n";
  else if ( this->OperationType == VTK_UNION ) os << "VTK_UNION\n";
  else os << "VTK_INTERSECTION\n";
}
