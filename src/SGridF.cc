/*=========================================================================

  Program:   Visualization Library
  Module:    SGridF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "SGridF.hh"

vlStructuredGridFilter::vlStructuredGridFilter()
{
}

vlStructuredGridFilter::~vlStructuredGridFilter()
{
}

// Description:
// Specify the input Grid or filter.
void vlStructuredGridFilter::SetInput(vlStructuredGrid *input)
{
  if ( this->Input != input )
    {
    vl_DebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vlDataSet *) input;
    this->_Modified();
    }
}

void vlStructuredGridFilter::_PrintSelf(ostream& os, vlIndent indent)
{
  vlFilter::_PrintSelf(os,indent);

}
