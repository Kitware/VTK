/*=========================================================================

  Program:   Visualization Library
  Module:    StrPtsF.cc
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
#include "StrPtsF.hh"

vlStructuredPointsFilter::~vlStructuredPointsFilter()
{
}

// Description:
// Specify the input data or filter.
void vlStructuredPointsFilter::SetInput(vlStructuredPoints *input)
{
  if ( this->Input != input )
    {
    vl_DebugMacro(<<" setting Input to " << (void *)input);
    this->Input = (vlDataSet *) input;
    this->_Modified();
    }
}

void vlStructuredPointsFilter::_PrintSelf(ostream& os, vlIndent indent)
{
  vlFilter::_PrintSelf(os,indent);

}
