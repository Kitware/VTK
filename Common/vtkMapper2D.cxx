/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper2D.cxx
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
#include "vtkMapper2D.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkMapper2D, "1.16");
vtkStandardNewMacro(vtkMapper2D);

void vtkMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

