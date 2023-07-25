// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like VTK
 !!! license.
=========================================================================*/

// .SECTION Description
// vtkQtConnection is an internal class.

#ifndef vtkQtConnection_h
#define vtkQtConnection_h

#include "vtkCommand.h" // for event defines
#include <QObject>

VTK_ABI_NAMESPACE_BEGIN
class vtkObject;
class vtkCallbackCommand;
class vtkEventQtSlotConnect;

// class for managing a single VTK/Qt connection
// not to be included in other projects
// only here for moc to process for vtkEventQtSlotConnect
class vtkQtConnection : public QObject
{
  Q_OBJECT

public:
  // constructor
  vtkQtConnection(vtkEventQtSlotConnect* owner);

  // destructor, disconnect if necessary
  ~vtkQtConnection() override;

  // print function
  void PrintSelf(ostream& os, vtkIndent indent);

  // callback from VTK to emit signal
  void Execute(vtkObject* caller, unsigned long event, void* client_data);

  // set the connection
  void SetConnection(vtkObject* vtk_obj, unsigned long event, const QObject* qt_obj,
    const char* slot, void* client_data, float priority = 0.0,
    Qt::ConnectionType type = Qt::AutoConnection);

  // check if a connection matches input parameters
  bool IsConnection(vtkObject* vtk_obj, unsigned long event, const QObject* qt_obj,
    const char* slot, void* client_data);

  static void DoCallback(
    vtkObject* vtk_obj, unsigned long event, void* client_data, void* call_data);

Q_SIGNALS:
  // the qt signal for moc to take care of
  void EmitExecute(vtkObject*, unsigned long, void* client_data, void* call_data, vtkCommand*);

protected Q_SLOTS:
  void deleteConnection();

protected: // NOLINT(readability-redundant-access-specifiers)
  // the connection information
  vtkObject* VTKObject;
  vtkCallbackCommand* Callback;
  const QObject* QtObject;
  void* ClientData;
  unsigned long VTKEvent;
  QString QtSlot;
  vtkEventQtSlotConnect* Owner;

private:
  vtkQtConnection(const vtkQtConnection&);
  void operator=(const vtkQtConnection&);
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkQtConnection.h
