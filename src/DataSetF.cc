/*=========================================================================

  Program:   Visualization Library
  Module:    DataSetF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Abstract class for objects that filter DataSets
//
#include "DataSetF.hh"

vlDataSetFilter::vlDataSetFilter()
{
  this->Input = 0;
}

vlDataSetFilter::~vlDataSetFilter()
{
  if ( this->Input != 0 )
    {
    this->Input->UnRegister((void *)this);
    }
}

void vlDataSetFilter::Update()
{
  // make sure input is available
  if ( !this->Input )
    {
    cerr << "No input available for DataSetFilter\n";
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->GetMTime() || this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}

void vlDataSetFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlDataSetFilter::GetClassName()))
    {
    vlFilter::PrintSelf(os,indent);

    if ( this->Input )
      {
      os << indent << "Input: (" << this->Input << ")\n";
      }
    else
      {
      os << indent << "Input: (none)\n";
      }
   }
}
