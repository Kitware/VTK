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
#include "DataSetF.hh"

vlDataSetFilter::~vlDataSetFilter()
{
}

// Description:
// Specify the input data or filter.
void vlDataSetFilter::SetInput(vlDataSet *input)
{
  if ( this->Input != input )
    {
    this->Input = input;
    this->_Modified();
    }
}

void vlDataSetFilter::_PrintSelf(ostream& os, vlIndent indent)
{
  vlFilter::_PrintSelf(os,indent);
}
