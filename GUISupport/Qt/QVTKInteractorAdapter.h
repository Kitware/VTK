// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

// .NAME QVTKInteractorAdapter - Handle Qt events.
// .SECTION Description
// QVTKInteractor handles relaying Qt events to VTK.

#ifndef Q_VTK_INTERACTOR_ADAPTER_H
#define Q_VTK_INTERACTOR_ADAPTER_H

#include "QVTKWin32Header.h"
#include "vtkGUISupportQtModule.h" // For export macro
#include <QtCore/QObject>

class QEvent;

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderWindowInteractor;

// .NAME QVTKInteractorAdapter - A QEvent translator.
// .SECTION Description
// QVTKInteractorAdapter translates QEvents and send them to a
// vtkRenderWindowInteractor.
class VTKGUISUPPORTQT_EXPORT QVTKInteractorAdapter : public QObject
{
  Q_OBJECT
public:
  // Description:
  // Constructor: takes QObject parent
  QVTKInteractorAdapter(QObject* parent = nullptr);

  // Description:
  // Destructor
  ~QVTKInteractorAdapter() override;

  // Description:
  // Enable/disable the touch event processing
  void SetEnableTouchEventProcessing(bool val);
  bool GetEnableTouchEventProcessing() const { return this->EnableTouchEventProcessing; }

  // Description:
  // Set the device pixel ratio, this defaults to 1.0, but in Qt 5 can be != 1.0.
  void SetDevicePixelRatio(float ratio, vtkRenderWindowInteractor* iren = nullptr);
  float GetDevicePixelRatio() { return this->DevicePixelRatio; }

  // Description:
  // Process a QEvent and send it to the interactor
  // returns whether the event was recognized and processed
  virtual bool ProcessEvent(QEvent* e, vtkRenderWindowInteractor* iren);

protected:
  int AccumulatedDelta;
  bool EnableTouchEventProcessing = true;
  float DevicePixelRatio;
  static const double DevicePixelRatioTolerance;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: QVTKInteractorAdapter.h
