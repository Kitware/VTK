/*=========================================================================

  Program:   Visualization Library
  Module:    SPt2Poly.cc
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
#include "SPt2Poly.hh"

void vlStructuredPointsToPolyDataFilter::Update()
{
  vlStructuredPointsFilter::Update();
}

void vlStructuredPointsToPolyDataFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStructuredPointsToPolyDataFilter::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance
    
    vlPolyData::PrintSelf(os,indent);
    vlStructuredPointsFilter::PrintSelf(os,indent);
    
    this->PrintWatchOff(); // stop worrying about it now
    }
}
