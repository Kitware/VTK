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
  cerr << "Executing Source\n";
}

void vlSource::Update()
{
  // Make sure virtual getMTime method is called since subclasses will overload
  if ( this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}

void vlSource::SetStartMethod(void (*f)())
{
  if ( f != this->StartMethod )
    {
    this->StartMethod = f;
    this->Modified();
    }
}

void vlSource::SetEndMethod(void (*f)())
{
  if ( f != this->EndMethod )
    {
    this->EndMethod = f;
    this->Modified();
    }
}

void vlSource::PrintSelf(ostream& os, vlIndent indent)
{
  vlObject::PrintSelf(os,indent);

  os << indent << "Execute Time: " << this->ExecuteTime.GetMTime() << "\n";
}
