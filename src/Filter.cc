/*=========================================================================

  Program:   Visualization Library
  Module:    Filter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Filter.hh"

// Description:
// Construct new filter without start or end methods.
vlFilter::vlFilter()
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

int vlFilter::GetDataReleased()
{
  vl_ErrorMacro(<<"Method should be implemented by subclass!");
  return 1;
}

void vlFilter::SetDataReleased(int flag)
{
  vl_ErrorMacro(<<"Method should be implemented by subclass!");
}

// Description:
// Update input to this filter and the filter itself.
void vlFilter::UpdateFilter()
{
  // make sure input is available
  if ( !this->Input )
    {
    vl_ErrorMacro(<< "No input!");
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
void vlFilter::SetStartMethod(void (*f)(void *), void *arg)
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
void vlFilter::SetEndMethod(void (*f)(void *), void *arg)
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
void vlFilter::SetStartMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartMethodArgDelete)
    {
    this->StartMethodArgDelete = f;
    this->_Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vlFilter::SetEndMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndMethodArgDelete)
    {
    this->EndMethodArgDelete = f;
    this->_Modified();
    }
}

void vlFilter::Execute()
{
  vl_ErrorMacro(<< "Execution of filter should be in derived class");
}

void vlFilter::_PrintSelf(ostream& os, vlIndent indent)
{
  vlLWObject::_PrintSelf(os,indent);

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

