/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreePointsGrabber.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreePointsGrabber.h"

#include <assert.h>


//-----------------------------------------------------------------------------
// Default constructor.
vtkHyperOctreePointsGrabber::vtkHyperOctreePointsGrabber()
{
  this->Dimension=3;
}

//-----------------------------------------------------------------------------
// Destructor.
vtkHyperOctreePointsGrabber::~vtkHyperOctreePointsGrabber()
{
}

//-----------------------------------------------------------------------------
// Description:
// Return the dimension of the hyperoctree.
// \post valid_result: (result==2 || result==3)
int vtkHyperOctreePointsGrabber::GetDimension()
{
  assert("post: valid_dim" && (this->Dimension==3 || this->Dimension==2));
  return this->Dimension;
}
 
//-----------------------------------------------------------------------------
void vtkHyperOctreePointsGrabber::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
