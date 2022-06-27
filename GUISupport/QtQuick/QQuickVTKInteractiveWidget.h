/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKInteractiveWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class QQuickVTKInteractiveWidget
 * @brief QObject that manages a VTK interactive widget to ensure that it behaves as per the QtQuick
 * threaded render loop.
 *
 * QQuickVTKInteractiveWidget holds a weak reference to the vtk widget it manages.
 */

#ifndef QQuickVTKInteractiveWidget_h
#define QQuickVTKInteractiveWidget_h

// Qt includes
#include <QObject>

// vtk includes
#include "vtkWeakPointer.h" // For vtkWeakPointer

#include "vtkGUISupportQtQuickModule.h" // for export macro

// Forward declarations
class vtkAbstractWidget;
class vtkRenderer;

class VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKInteractiveWidget : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

  Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged);

public:
  QQuickVTKInteractiveWidget(QObject* parent = nullptr);
  ~QQuickVTKInteractiveWidget() override = default;

  ///@{
  /**
   * Set/Get the widget reference
   */
  void setWidget(vtkAbstractWidget* w);
  vtkAbstractWidget* widget() const;
  ///@}

  ///@{
  /**
   * Set/Get whether the widget is enabled.
   */
  void setEnabled(bool e);
  bool enabled() const;
  ///@}

public Q_SLOTS:
  virtual void sync(vtkRenderer* ren);

Q_SIGNALS:
  void enabledChanged(bool e);

protected:
  // Helper members
  vtkWeakPointer<vtkAbstractWidget> m_widget;

  // Enabled/disabled
  bool m_enabled = false;

private:
  QQuickVTKInteractiveWidget(const QQuickVTKInteractiveWidget&) = delete;
  void operator=(const QQuickVTKInteractiveWidget) = delete;
};

#endif // QQuickVTKInteractiveWidget_h
