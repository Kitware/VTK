/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringOutputWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStringOutputWindow.h"
#include "vtkObjectFactory.h"


vtkStandardNewMacro(vtkStringOutputWindow);

vtkStringOutputWindow::vtkStringOutputWindow()
{
  this->OStream.str("");
  this->OStream.clear();
}

vtkStringOutputWindow::~vtkStringOutputWindow()
{
}

void vtkStringOutputWindow::Initialize()
{
  this->OStream.str("");
  this->OStream.clear();
}

void vtkStringOutputWindow::DisplayText(const char* text)
{
  if(!text)
  {
    return;
  }

  this->OStream << text << endl;
}

void vtkStringOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
