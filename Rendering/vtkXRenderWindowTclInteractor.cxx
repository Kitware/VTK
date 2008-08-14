/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowTclInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXRenderWindowTclInteractor.h"

#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkXRenderWindowTclInteractor, "1.54");
vtkStandardNewMacro(vtkXRenderWindowTclInteractor);

//-------------------------------------------------------------------------
vtkXRenderWindowTclInteractor::vtkXRenderWindowTclInteractor()
{
}


//-------------------------------------------------------------------------
vtkXRenderWindowTclInteractor::~vtkXRenderWindowTclInteractor()
{
}


//-------------------------------------------------------------------------
void vtkXRenderWindowTclInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
