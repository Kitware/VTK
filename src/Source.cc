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

void vlSource::Execute()
{
  vlDebugMacro(<< "Executing Source\n");
}

void vlSource::Update()
{
  // Make sure virtual getMTime method is called since subclasses will overload
  if ( this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
}

// Description:
// Specify function to be called before source executes.
void vlSource::SetStartMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartMethod || arg != this->StartMethodArg )
    {
    this->StartMethod = f;
    this->StartMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Specify function to be called after source executes.
void vlSource::SetEndMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndMethod || arg != this->EndMethodArg )
    {
    this->EndMethod = f;
    this->EndMethodArg = arg;
    this->Modified();
    }
}

void vlSource::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

  os << indent << "Execute Time: " << this->ExecuteTime.GetMTime() << "\n";
}
