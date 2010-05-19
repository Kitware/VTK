/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackball.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleTrackball.h"
#include "vtkMath.h"
#include "vtkPropPicker.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkInteractorStyleTrackball);

//----------------------------------------------------------------------------
vtkInteractorStyleTrackball::vtkInteractorStyleTrackball()
{
  vtkWarningMacro("vtkInteractorStyleTrackball will be deprecated in" << endl 
                  << "the next release after VTK 4.0. Please use" << endl
                  << "vtkInteractorStyleSwitch instead." );
}

//----------------------------------------------------------------------------
vtkInteractorStyleTrackball::~vtkInteractorStyleTrackball() 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTrackball::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
