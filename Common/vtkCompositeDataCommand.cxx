/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataCommand.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataCommand.h"

vtkCxxRevisionMacro(vtkCompositeDataCommand, "1.1");

//----------------------------------------------------------------------------
vtkCompositeDataCommand::vtkCompositeDataCommand()
{
}

//----------------------------------------------------------------------------
vtkCompositeDataCommand::~vtkCompositeDataCommand()
{
}

//----------------------------------------------------------------------------
void vtkCompositeDataCommand::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

