/*=========================================================================

  Program:   Visualization Library
  Module:    SG2PolyF.cc
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
#include "SD2PolyF.hh"

void vlStructuredDataToPolyFilter::Update()
{
  vlStructuredDataFilter::Update();
}

void vlStructuredDataToPolyFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStructuredDataToPolyFilter::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance
    
    vlPolyData::PrintSelf(os,indent);
    vlStructuredDataFilter::PrintSelf(os,indent);
    
    this->PrintWatchOff(); // stop worrying about it now
    }
}
