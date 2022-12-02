/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkToImplicitStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkToImplicitStrategy.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------
void vtkToImplicitStrategy::PrintSelf(std::ostream& os, vtkIndent indent)
{
  os << indent << "vtkToImplicitStrategy: " << std::endl;
  os << indent << "Tolerance: " << this->Tolerance << std::endl;
  Superclass::PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
