/*=========================================================================

 Program:   Visualization Library
 Module:    Source.cc
 Language:  C++
 Date:      $Date$
 Version:   $Revision$

 This file is part of the Visualization Library. No part of this file or its
 contents may be copied, reproduced or altered in any way without the express
 written consent of the authors.
 
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
#include "Source.hh"

#ifndef NULL
#define NULL 0
#endif

vlSource::vlSource()
{
  this->StartMethod = NULL;
  this->StartMethodArgDelete = NULL;
  this->StartMethodArg = NULL;
  this->EndMethod = NULL;
  this->EndMethodArgDelete = NULL;
  this->EndMethodArg = NULL;
}

void vlSource::UpdateFilter()
{
  // Make sure virtual getMTime method is called since subclasses will overload
  if ( this->_GetMTime() > this->ExecuteTime || this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
}

// Description:
// Specify function to be called before source executes.
void vlSource::SetStartMethod(void (*f)(void *), void *arg)
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
// Specify function to be called after source executes.
void vlSource::SetEndMethod(void (*f)(void *), void *arg)
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
void vlSource::SetStartMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartMethodArgDelete)
    {
    this->StartMethodArgDelete = f;
    this->_Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vlSource::SetEndMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndMethodArgDelete)
    {
    this->EndMethodArgDelete = f;
    this->_Modified();
    }
}

void vlSource::Execute()
{
  vl_ErrorMacro(<< "Execution of filter should be in derived class");
}

int vlSource::GetDataReleased()
{
  vl_ErrorMacro(<<"Method should be implemented by subclass!");
  return 1;
}

void vlSource::SetDataReleased(int flag)
{
  vl_ErrorMacro(<<"Method should be implemented by subclass!");
}

void vlSource::_PrintSelf(ostream& os, vlIndent indent)
{
  vlLWObject::_PrintSelf(os,indent);

  os << indent << "Execute Time: " << this->ExecuteTime.GetMTime() << "\n";
}
