// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkQtInitialization.h"
#include "vtkObjectFactory.h"

#include <QApplication>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQtInitialization);

vtkQtInitialization::vtkQtInitialization()
{
  this->Application = nullptr;
  if (!QApplication::instance())
  {
    int argc = 0;
    this->Application = new QApplication(argc, nullptr);
  }
}

vtkQtInitialization::~vtkQtInitialization()
{
  delete this->Application;
}

void vtkQtInitialization::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "QApplication: " << QApplication::instance() << endl;
}
VTK_ABI_NAMESPACE_END
