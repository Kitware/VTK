/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitBoolean.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImplicitBoolean.h"

#include "vtkImplicitFunctionCollection.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkStandardNewMacro(vtkImplicitBoolean);

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

  vtkCollectionSimpleIterator sit;
  for (this->FunctionList->InitTraversal(sit);
       (f=this->FunctionList->GetNextImplicitFunction(sit)); )
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
double vtkImplicitBoolean::EvaluateFunction(double x[3])
{
  double value = 0;
  double v;
  vtkImplicitFunction *f;

  if (this->FunctionList->GetNumberOfItems() == 0)
    {
    return value;
    }

  vtkCollectionSimpleIterator sit;
  if ( this->OperationType == VTK_UNION )
    { //take minimum value
    for (value = VTK_DOUBLE_MAX, this->FunctionList->InitTraversal(sit);
         (f=this->FunctionList->GetNextImplicitFunction(sit)); )
      {
      if ( (v=f->FunctionValue(x)) < value )
        {
        value = v;
        }
      }
    }

  else if ( this->OperationType == VTK_INTERSECTION )
    { //take maximum value
    for (value=-VTK_DOUBLE_MAX, this->FunctionList->InitTraversal(sit);
    (f=this->FunctionList->GetNextImplicitFunction(sit)); )
      {
      if ( (v=f->FunctionValue(x)) > value )
        {
        value = v;
        }
      }
    }

  else if ( this->OperationType == VTK_UNION_OF_MAGNITUDES )
    { //take minimum absolute value
    for (value = VTK_DOUBLE_MAX, this->FunctionList->InitTraversal(sit);
    (f=this->FunctionList->GetNextImplicitFunction(sit)); )
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
    this->FunctionList->InitTraversal(sit);
    if ( (firstF = this->FunctionList->GetNextImplicitFunction(sit)) != NULL )
      {
      value = firstF->FunctionValue(x);
      }

    for (this->FunctionList->InitTraversal(sit);
    (f=this->FunctionList->GetNextImplicitFunction(sit)); )
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
void vtkImplicitBoolean::EvaluateGradient(double x[3], double g[3])
{
  double value = 0;
  double v;
  vtkImplicitFunction *f;
  vtkCollectionSimpleIterator sit;

  if (this->FunctionList->GetNumberOfItems() == 0)
    {
    g[0] = 0; g[1] = 0; g[2] = 0;
    return;
    }

  if ( this->OperationType == VTK_UNION )
    { //take minimum value
    for (value = VTK_DOUBLE_MAX, this->FunctionList->InitTraversal(sit);
    (f=this->FunctionList->GetNextImplicitFunction(sit)); )
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
    for (value=-VTK_DOUBLE_MAX, this->FunctionList->InitTraversal(sit);
    (f=this->FunctionList->GetNextImplicitFunction(sit)); )
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
    for (value = VTK_DOUBLE_MAX, this->FunctionList->InitTraversal(sit);
    (f=this->FunctionList->GetNextImplicitFunction(sit)); )
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
    double gTemp[3];
    vtkImplicitFunction *firstF;
    this->FunctionList->InitTraversal(sit);
    if ( (firstF = this->FunctionList->GetNextImplicitFunction(sit)) != NULL )
      {
      value = firstF->FunctionValue(x);
      firstF->FunctionGradient(x,gTemp);
      g[0] = -1.0*gTemp[0]; g[1] = -1.0*gTemp[1]; g[2] = -1.0*gTemp[2];
      }

    for (this->FunctionList->InitTraversal(sit);
    (f=this->FunctionList->GetNextImplicitFunction(sit)); )
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
  this->Superclass::PrintSelf(os,indent);

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
