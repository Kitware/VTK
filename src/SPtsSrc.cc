/*=========================================================================

  Program:   Visualization Library
  Module:    SPtsSrc.cc
  Language:  C++
  Date:      7/13/94
  Version:   1.1

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SPtsSrc.hh"

void vlStructuredPointsSource::Update()
{
  vlSource::Update();
}

void vlStructuredPointsSource::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlStructuredPointsSource::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance
    
    vlStructuredPoints::PrintSelf(os,indent);
    vlSource::PrintSelf(os,indent);
    
    this->PrintWatchOff(); // stop worrying about it now
    }
}
