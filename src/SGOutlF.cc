/*=========================================================================

  Program:   Visualization Library
  Module:    SGOutlF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SGOutlF.hh"

void vlStructuredGridOutlineFilter::Execute()
{
  vlFloatPoints *newPts;
  vlCellArray *newLines;

  vlDebugMacro(<< "Creating structured outline");
  this->Initialize();

//
// Update selves
//
  this->SetPoints(newPts);
  this->SetLines(newLines);

}
