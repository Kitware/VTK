/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKInteractiveWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QQuickVTKInteractiveWidget.h"

// vtk includes
#include "vtkAbstractWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetRepresentation.h"

//-------------------------------------------------------------------------------------------------
QQuickVTKInteractiveWidget::QQuickVTKInteractiveWidget(QObject* parent)
  : Superclass(parent)
{
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractiveWidget::setWidget(vtkAbstractWidget* w)
{
  this->m_widget = w;
}

//-------------------------------------------------------------------------------------------------
vtkAbstractWidget* QQuickVTKInteractiveWidget::widget() const
{
  return this->m_widget;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractiveWidget::setEnabled(bool e)
{
  if (this->m_enabled == e)
  {
    return;
  }

  this->m_enabled = e;
  Q_EMIT this->enabledChanged(this->m_enabled);
}

//-------------------------------------------------------------------------------------------------
bool QQuickVTKInteractiveWidget::enabled() const
{
  return this->m_enabled;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractiveWidget::sync(vtkRenderer* ren)
{
  if (!ren || !this->m_widget)
  {
    return;
  }

  auto iren = ren->GetRenderWindow()->GetInteractor();
  this->m_widget->SetInteractor(iren);
  this->m_widget->SetCurrentRenderer(ren);
  this->m_widget->SetEnabled(this->m_enabled);
  this->m_widget->SetProcessEvents(this->m_enabled);
}
