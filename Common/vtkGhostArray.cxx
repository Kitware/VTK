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

// C++ includes
#include <cassert>

// VTK includes
#include "vtkMeshProperty.h"
#include "vtkMeshPropertyEncoder.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkMeshPropertyEncoder );

//------------------------------------------------------------------------------
vtkMeshPropertyEncoder::vtkMeshPropertyEncoder()
{

}

//------------------------------------------------------------------------------
vtkMeshPropertyEncoder::~vtkMeshPropertyEncoder()
{

}

//------------------------------------------------------------------------------
void vtkMeshPropertyEncoder::PrintSelf( std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
}
