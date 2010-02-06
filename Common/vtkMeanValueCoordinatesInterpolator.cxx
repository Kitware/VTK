/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMeanValueCoordinatesInterpolator.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMeanValueCoordinatesInterpolator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMeanValueCoordinatesInterpolator, "$Revision: 1.83 $");
vtkStandardNewMacro(vtkMeanValueCoordinatesInterpolator);

//----------------------------------------------------------------------------
// Construct object with default tuple dimension (number of components) of 1.
vtkMeanValueCoordinatesInterpolator::vtkMeanValueCoordinatesInterpolator()
{
}

//----------------------------------------------------------------------------
vtkMeanValueCoordinatesInterpolator::~vtkMeanValueCoordinatesInterpolator()
{
}

//----------------------------------------------------------------------------
void vtkMeanValueCoordinatesInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
