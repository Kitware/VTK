// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkQtView.h"

#include "vtkObjectFactory.h"
#include <QApplication>
#include <QPixmap>
#include <QWidget>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkQtView::vtkQtView() = default;

//------------------------------------------------------------------------------
vtkQtView::~vtkQtView() = default;

//------------------------------------------------------------------------------
void vtkQtView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkQtView::ProcessQtEvents()
{
  QApplication::processEvents();
}

//------------------------------------------------------------------------------
void vtkQtView::ProcessQtEventsNoUserInput()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

//------------------------------------------------------------------------------
bool vtkQtView::SaveImage(const char* filename)
{
  return this->GetWidget() != nullptr ? this->GetWidget()->grab().save(filename) : false;
}
VTK_ABI_NAMESPACE_END
