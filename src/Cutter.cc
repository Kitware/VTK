/*=========================================================================

  Program:   Visualization Library
  Module:    Cutter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "Cutter.hh"

vlCutter::vlCutter(vlImplicitFunction *cf)
{
  this->CutFunction = cf;
  if ( cf ) cf->Register((void *) this);
}

vlCutter::~vlCutter()
{
  if ( this->CutFunction ) this->CutFunction->UnRegister((void *) this);
}

void vlCutter::Execute()
{

  vlDebugMacro(<< "Executing cutter");
//
// Initialize self; create output objects
//
  this->Initialize();

  if ( !this->CutFunction )
    {
    vlErrorMacro(<<"No cut function specified");
    return;
    }

}
