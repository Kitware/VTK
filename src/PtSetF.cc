/*=========================================================================

  Program:   Visualization Library
  Module:    PtSetF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PtSetF.hh"

vlPointSetFilter::~vlPointSetFilter()
{
}

// Description:
// Specify the input data or filter.
void vlPointSetFilter::SetInput(vlPointSet *input)
{
  if ( this->Input != input )
    {
    this->Input = (vlDataSet *) input;
    this->_Modified();
    }
}

void vlPointSetFilter::_PrintSelf(ostream& os, vlIndent indent)
{
  vlFilter::_PrintSelf(os,indent);
}
