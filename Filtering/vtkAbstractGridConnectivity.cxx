/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAbstractGridConnectivity.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAbstractGridConnectivity.h"

vtkAbstractGridConnectivity::vtkAbstractGridConnectivity()
{
  this->NumberOfGrids                = 0;
  this->NumberOfGhostLayers          = 0;
  this->AllocatedGhostDataStructures = false;
}

//------------------------------------------------------------------------------
vtkAbstractGridConnectivity::~vtkAbstractGridConnectivity()
{
  this->DeAllocateUserRegisterDataStructures();
  this->DeAllocateInternalDataStructures();
}

//------------------------------------------------------------------------------
void vtkAbstractGridConnectivity::PrintSelf(std::ostream &os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << "NumberOfGrids: " << this->NumberOfGrids << std::endl;
  os << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
}
