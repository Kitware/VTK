// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// this class is deprecated, don't warn about deprecated classes it uses
#define VTK_DEPRECATION_LEVEL 0

#include "QQuickVTKInteractiveWidget.h"

// vtk includes
#include "vtkAbstractWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetRepresentation.h"

//-------------------------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
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
VTK_ABI_NAMESPACE_END
