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

#ifndef VTK_EVENT_QT_SLOT_CONNECT
#define VTK_EVENT_QT_SLOT_CONNECT

#include "vtkObject.h"
#include "vtkCommand.h"

#include "qobject.h"
#include "qstring.h"

class vtkEventQtSlotConnectPrivate;

// manage connections between VTK object events and Qt slots
class vtkEventQtSlotConnect : public vtkObject
{
  public:
    static vtkEventQtSlotConnect* New();
    vtkTypeMacro(vtkEventQtSlotConnect, vtkObject)
    //void PrintSelf(ostream& os, vtkIndent indent);
    
    // connect a vtk object's event with a Qt object's slot
    // multiple connections which are identical are treated as separate connections
    // allowable slot to connect to have the following signatures
    //   slot()
    //   slot(vtkObject* caller)
    //   slot(vtkObject* caller, unsigned long vtk_event)
    //   slot(vtkObject* caller, unsigned long vtk_event, void* client_data)
    //   slot(vtkObject* caller, unsigned long vtk_event, void* client_data, vtkCommand*)
    virtual void Connect(vtkObject* vtk_obj, unsigned long event, 
                 QObject* qt_obj, const char* slot, void* client_data=NULL, float priority=0.0);
    
    // disconnect a vtk object from a qt object
    // passing in only a vtk object will disconnect all slots from it
    // passing only a vtk object and event, will disconnect all slots matching the vtk object and event
    // passing all information in will match all information
    virtual void Disconnect(vtkObject* vtk_obj, unsigned long event=vtkCommand::NoEvent, 
                 QObject* qt_obj=NULL, const char* slot = 0);

  protected:
    vtkEventQtSlotConnectPrivate* mPrivate;
  
    vtkEventQtSlotConnect();
    ~vtkEventQtSlotConnect();
};


// class for managing a single VTK/Qt connection
class vtkQtConnection : public QObject, public vtkCommand
{
  Q_OBJECT
  
  public:
    
    // standard new
    static vtkQtConnection* New();
    vtkTypeMacro(vtkQtConnection, vtkCommand)
    
    // callback from VTK to emit signal
    void Execute(vtkObject* caller, unsigned long event, void* client_data);
    
    // set the connection
    void SetConnection(vtkObject* vtk_obj, unsigned long event,
                       QObject* qt_obj, const char* slot, void* client_data, float priority=0.0);
    
    // check if a connection matches input parameters
    bool IsConnection(vtkObject* vtk_obj, unsigned long event,
                      QObject* qt_obj, const char* slot);
    
  signals:
    // the qt signal for moc to take care of
    void emitExecute(vtkObject*, unsigned long, void* client_data, vtkCommand*);

  protected:
    
    // constructor
    vtkQtConnection();

    // destructor, disconnect if necessary
    ~vtkQtConnection();
    
    // the connection information
    vtkObject* mVTKObject;
    QObject* mQtObject;
    void* mClientData;
    unsigned long mVTKEvent;
    QString mQtSlot;

};

#endif

