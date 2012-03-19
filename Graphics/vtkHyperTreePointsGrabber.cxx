/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreePointsGrabber.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreePointsGrabber.h"

#include <assert.h>


//-----------------------------------------------------------------------------
// Default constructor.
vtkHyperTreePointsGrabber::vtkHyperTreePointsGrabber()
{
  this->Dimension=3;
}

//-----------------------------------------------------------------------------
// Destructor.
vtkHyperTreePointsGrabber::~vtkHyperTreePointsGrabber()
{
}

//-----------------------------------------------------------------------------
// Description:
// Return the dimension of the hyperoctree.
// \post valid_result: (result==2 || result==3)
int vtkHyperTreePointsGrabber::GetDimension()
{
  assert("post: valid_dim" && (this->Dimension==3 || this->Dimension==2));
  return this->Dimension;
}
 
//-----------------------------------------------------------------------------
void vtkHyperTreePointsGrabber::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
