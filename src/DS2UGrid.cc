/*=========================================================================

  Program:   Visualization Library
  Module:    DS2UGrid.cc
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
#include "DS2UGrid.hh"

void vlDataSetToUnstructuredGridFilter::Update()
{
  vlDataSetFilter::Update();
}

void vlDataSetToUnstructuredGridFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlDataSetToUnstructuredGridFilter::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance
    
    vlUnstructuredGrid::PrintSelf(os,indent);
    vlDataSetFilter::PrintSelf(os,indent);
    
    this->PrintWatchOff(); // stop worrying about it now
    }
}
