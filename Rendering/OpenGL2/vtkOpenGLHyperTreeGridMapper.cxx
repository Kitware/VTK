/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLHyperTreeGridMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLHyperTreeGridMapper.h"

#include "vtkCompositePolyDataMapper.h" // For Mapper3D
#include "vtkObjectFactory.h"           // For the macro
#include "vtkOpenGLPolyDataMapper.h"    // For PDMapper

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkOpenGLHyperTreeGridMapper);

//------------------------------------------------------------------------------
vtkOpenGLHyperTreeGridMapper::vtkOpenGLHyperTreeGridMapper()
{
  this->Mapper = vtkSmartPointer<vtkCompositePolyDataMapper>::New();
}

//------------------------------------------------------------------------------
void vtkOpenGLHyperTreeGridMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
