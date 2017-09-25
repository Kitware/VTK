/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

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

#include "vtkCommand.h"  // for event defines
#include <QObject>

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
    void SetConnection(vtkObject* vtk_obj, unsigned long event,
                       const QObject* qt_obj, const char* slot,
                       void* client_data, float priority=0.0
                         ,Qt::ConnectionType type = Qt::AutoConnection);

    // check if a connection matches input parameters
    bool IsConnection(vtkObject* vtk_obj, unsigned long event,
                      const QObject* qt_obj, const char* slot,
                      void* client_data);

    static void DoCallback(vtkObject* vtk_obj, unsigned long event,
                           void* client_data, void* call_data);

  signals:
    // the qt signal for moc to take care of
    void EmitExecute(vtkObject*, unsigned long, void* client_data, void* call_data, vtkCommand*);

  protected slots:
    void deleteConnection();

  protected:

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

#endif
