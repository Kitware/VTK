// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

#ifndef Q_VTK_INTERACTOR_INTERNAL_H
#define Q_VTK_INTERACTOR_INTERNAL_H

#include <QtCore/QObject>

#include <map>

class QSignalMapper;
class QTimer;

VTK_ABI_NAMESPACE_BEGIN
class QVTKInteractor;

// internal class, do not use
class QVTKInteractorInternal : public QObject
{
  Q_OBJECT
public:
  QVTKInteractorInternal(QVTKInteractor* p);
  ~QVTKInteractorInternal() override;
public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void TimerEvent(int id);

public: // NOLINT(readability-redundant-access-specifiers)
  QSignalMapper* SignalMapper;
  typedef std::map<int, QTimer*> TimerMap;
  TimerMap Timers;
  QVTKInteractor* Parent;
};

VTK_ABI_NAMESPACE_END
#endif
