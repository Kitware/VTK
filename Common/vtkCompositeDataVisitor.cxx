/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataVisitor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataVisitor.h"

#include "vtkCompositeDataCommand.h"

vtkCxxRevisionMacro(vtkCompositeDataVisitor, "1.3");

vtkCxxSetObjectMacro(vtkCompositeDataVisitor,
                     Command, 
                     vtkCompositeDataCommand);

//----------------------------------------------------------------------------
vtkCompositeDataVisitor::vtkCompositeDataVisitor()
{
  this->Command = 0;
  this->CreateTransitionElements = 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataVisitor::~vtkCompositeDataVisitor()
{
  this->SetCommand(0);
}

//----------------------------------------------------------------------------
void vtkCompositeDataVisitor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Command: ";
  if (this->Command)
    {
    os << endl;
    this->Command->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "CreateTransitionElements: " << this->CreateTransitionElements
     << endl;
}

