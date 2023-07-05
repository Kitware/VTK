// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStringOutputWindow.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStringOutputWindow);

vtkStringOutputWindow::vtkStringOutputWindow()
{
  this->OStream.str("");
  this->OStream.clear();
}

vtkStringOutputWindow::~vtkStringOutputWindow() = default;

void vtkStringOutputWindow::Initialize()
{
  this->OStream.str("");
  this->OStream.clear();
}

void vtkStringOutputWindow::DisplayText(const char* text)
{
  if (!text)
  {
    return;
  }

  this->OStream << text << endl;
}

void vtkStringOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
