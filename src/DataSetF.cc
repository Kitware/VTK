/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DataSetF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "DataSetF.hh"

vtkDataSetFilter::~vtkDataSetFilter()
{
}

// Description:
// Specify the input data or filter.
void vtkDataSetFilter::SetInput(vtkDataSet *input)
{
  if ( this->Input != input )
    {
    vtk_DebugMacro(<<" setting Input to " << (void *)input);
    this->Input = input;
    this->_Modified();
    }
}

void vtkDataSetFilter::_PrintSelf(ostream& os, vtkIndent indent)
{
  vtkFilter::_PrintSelf(os,indent);
}
