// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkDeprecation.h"
#include "vtkGUISupportQtQuickModule.h" // for export macro
#include "vtkWeakPointer.h"             // For vtkWeakPointer

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractWidget;
class vtkRenderer;

class VTK_DEPRECATED_IN_9_3_0(
  "Use QQuickVTKItem instead") VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKInteractiveWidget
  : public QObject
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

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
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

VTK_ABI_NAMESPACE_END
#endif // QQuickVTKInteractiveWidget_h
