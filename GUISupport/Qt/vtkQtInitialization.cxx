/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtInitialization.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkObjectFactory.h"
#include "vtkQtInitialization.h"

#include <QCoreApplication>

vtkCxxRevisionMacro(vtkQtInitialization, "1.1");
vtkStandardNewMacro(vtkQtInitialization);

vtkQtInitialization::vtkQtInitialization()
{
  if(!QCoreApplication::instance())
    {
    int argc = 0;
    new QCoreApplication(argc, 0);
    }
}

vtkQtInitialization::~vtkQtInitialization()
{
}

void vtkQtInitialization::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "QCoreApplication: " << QCoreApplication::instance() << endl;
}
