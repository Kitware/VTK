/*=========================================================================

  Program:   ParaView
  Module:    vtkSelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelection.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSelection, "1.1");
vtkStandardNewMacro(vtkSelection);

//----------------------------------------------------------------------------
vtkSelection::vtkSelection()
{
}

//----------------------------------------------------------------------------
vtkSelection::~vtkSelection()
{
}

//----------------------------------------------------------------------------
void vtkSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

