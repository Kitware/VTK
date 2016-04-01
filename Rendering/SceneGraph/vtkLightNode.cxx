/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLightNode.h"
#include "vtkObjectFactory.h"

#include "vtkLight.h"

//============================================================================
vtkStandardNewMacro(vtkLightNode);

//----------------------------------------------------------------------------
vtkLightNode::vtkLightNode()
{
}

//----------------------------------------------------------------------------
vtkLightNode::~vtkLightNode()
{
}

//----------------------------------------------------------------------------
void vtkLightNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
