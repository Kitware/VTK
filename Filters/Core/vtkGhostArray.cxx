/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkGhostDataEncoder.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// VTK includes
#include "vtkGhostArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkGhostArray );

//------------------------------------------------------------------------------
vtkGhostArray::vtkGhostArray()
{

}

//------------------------------------------------------------------------------
vtkGhostArray::~vtkGhostArray()
{

}

//------------------------------------------------------------------------------
void vtkGhostArray::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}
