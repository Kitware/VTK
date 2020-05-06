/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHardwareWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHardwareWindow.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkHardwareWindow);

vtkHardwareWindow::vtkHardwareWindow()
{
  this->Borders = true;
#ifdef VTK_DEFAULT_RENDER_WINDOW_OFFSCREEN
  this->ShowWindow = false;
  this->UseOffScreenBuffers = true;
#else
  this->ShowWindow = true;
#endif
}

//------------------------------------------------------------------------------
vtkHardwareWindow::~vtkHardwareWindow() = default;

//------------------------------------------------------------------------------
void vtkHardwareWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Borders: " << this->Borders << "\n";
}
