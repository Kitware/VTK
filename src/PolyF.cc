/*=========================================================================

  Program:   Visualization Library
  Module:    PolyF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PolyF.hh"

vlPolyFilter::~vlPolyFilter()
{
}


// Description:
// Specify the input data or filter.
void vlPolyFilter::SetInput(vlPolyData *input)
{
  if ( this->Input != input )
    {
    this->Input = (vlDataSet *) input;
    this->_Modified();
    }
}

void vlPolyFilter::_PrintSelf(ostream& os, vlIndent indent)
{
  vlFilter::_PrintSelf(os,indent);
}
