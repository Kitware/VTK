/*=========================================================================

  Program:   Visualization Library
  Module:    ImpBool.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "ImpBool.hh"

// Description:
// 
vlImplicitBoolean::vlImplicitBoolean()
{
  this->OperationType = UNION;
}

vlImplicitBoolean::~vlImplicitBoolean()
{
}

unsigned long int vlImplicitBoolean::GetMTime()
{
  unsigned long int fMtime;
  unsigned long int mtime = this->vlImplicitFunction::GetMTime();
  vlImplicitFunction *f;

  for (this->FunctionList.InitTraversal(); f=this->FunctionList.GetNextItem(); )
    {
    fMtime = f->GetMTime();
    if ( fMtime > mtime ) mtime = fMtime;
    }
  return mtime;
}

// Description:
// Add another implicit function to the list of functions.
void vlImplicitBoolean::AddFunction(vlImplicitFunction *f)
{
  if ( ! this->FunctionList.IsItemPresent(f) )
    {
    this->Modified();
    this->FunctionList.AddItem(f);
    }
}

// Description:
// Remove a function from the list of implicit functions to boolean.
void vlImplicitBoolean::RemoveFunction(vlImplicitFunction *f)
{
  if ( this->FunctionList.IsItemPresent(f) )
    {
    this->Modified();
    this->FunctionList.RemoveItem(f);
    }
}

// Description
// Evaluate boolean combinations of implicit function using current operator.
float vlImplicitBoolean::Evaluate(float x, float y, float z)
{
  float value, v;
  vlImplicitFunction *f;

  if ( this->OperationType == UNION )
    { //take minimum value
    for (value = LARGE_FLOAT, this->FunctionList.InitTraversal(); 
    f=this->FunctionList.GetNextItem(); )
      {
      if ( (v=f->Evaluate(x,y,z)) < value ) value = v;
      }
    }

  else if ( this->OperationType == INTERSECTION )
    { //take maximum value
    for (value=-LARGE_FLOAT, this->FunctionList.InitTraversal(); 
    f=this->FunctionList.GetNextItem(); )
      {
      if ( (v=f->Evaluate(x,y,z)) > value ) value = v;
      }
    }

  else //difference
    {
    vlImplicitFunction *firstF;
    this->FunctionList.InitTraversal();
    if ( (firstF = this->FunctionList.GetNextItem()) != NULL )
      value = firstF->Evaluate(x,y,z);

    for (this->FunctionList.InitTraversal(); 
    f=this->FunctionList.GetNextItem(); )
      {
      if ( f != firstF )
        {
        if ( (v=(-1.0)*f->Evaluate(x,y,z)) > value ) value = v;
        }
      }
    }//else

  return value;
}

// Description
// Evaluate sphere normal.
void vlImplicitBoolean::EvaluateNormal(float x, float y, float z, float n[3])
{
  float value, v;
  vlImplicitFunction *f;

  if ( this->OperationType == UNION )
    { //take minimum value
    for (value = LARGE_FLOAT, this->FunctionList.InitTraversal(); 
    f=this->FunctionList.GetNextItem(); )
      {
      if ( (v=f->Evaluate(x,y,z)) < value )
        {
        value = v;
        f->EvaluateNormal(x,y,z,n);
        }
      }
    }

  else if ( this->OperationType == INTERSECTION )
    { //take maximum value
    for (value=-LARGE_FLOAT, this->FunctionList.InitTraversal(); 
    f=this->FunctionList.GetNextItem(); )
      {
      if ( (v=f->Evaluate(x,y,z)) > value ) 
        {
        value = v;
        f->EvaluateNormal(x,y,z,n);
        }
      }
    }

  else //difference
    {
    float nTemp[3];
    vlImplicitFunction *firstF;
    this->FunctionList.InitTraversal();
    if ( (firstF = this->FunctionList.GetNextItem()) != NULL )
      {
      value = firstF->Evaluate(x,y,z);
      firstF->EvaluateNormal(x,y,z,nTemp); 
      n[0] = -1.0*nTemp[0]; n[1] = -1.0*nTemp[1]; n[2] = -1.0*nTemp[2];
      }

    for (this->FunctionList.InitTraversal(); 
    f=this->FunctionList.GetNextItem(); )
      {
      if ( f != firstF )
        {
        if ( (v=(-1.0)*f->Evaluate(x,y,z)) > value )
          {
          value = v;
          f->EvaluateNormal(x,y,z,nTemp);
          n[0] = -1.0*nTemp[0]; n[1] = -1.0*nTemp[1]; n[2] = -1.0*nTemp[2];
          }
        }
      }
    }//else
}

void vlImplicitBoolean::PrintSelf(ostream& os, vlIndent indent)
{
  vlImplicitFunction::PrintSelf(os,indent);

  os << indent << "Function List:\n";
  this->FunctionList.PrintSelf(os,indent.GetNextIndent());

  os << indent << "Operator Type: ";
  if ( this->OperationType == INTERSECTION ) os << "INTERSECTION\n";
  else if ( this->OperationType == UNION ) os << "UNION\n";
  else os << "INTERSECTION\n";
}
