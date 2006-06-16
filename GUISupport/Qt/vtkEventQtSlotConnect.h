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

// .NAME vtkEventQtSlotConnect - Manage connections between VTK events and Qt slots.
// .SECTION Description
// vtkEventQtSlotConnect provides a way to manage connections between VTK events
// and Qt slots.
// Qt slots to connect with must have one of the following signatures:
// - MySlot()
// - MySlot(vtkObject* caller)
// - MySlot(vtkObject* caller, unsigned long vtk_event)
// - MySlot(vtkObject* caller, unsigned long vtk_event, void* client_data)
// - MySlot(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data)
// - MySlot(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data, vtkCommand*)


#ifndef VTK_EVENT_QT_SLOT_CONNECT
#define VTK_EVENT_QT_SLOT_CONNECT

#include "vtkObject.h"
#include "vtkCommand.h"  // for event defines
#include "qobject.h"

class QObject;
class vtkCallbackCommand;
class vtkQtConnections;

#if defined(WIN32) && defined(VTK_BUILD_SHARED_LIBS)
#if defined(QVTK_EXPORTS) || defined(QVTKWidgetPlugin_EXPORTS)
#define QVTK_EXPORT __declspec( dllexport )
#else
#define QVTK_EXPORT __declspec( dllimport ) 
#endif
#else
#define QVTK_EXPORT
#endif

// manage connections between VTK object events and Qt slots
class QVTK_EXPORT vtkEventQtSlotConnect : public vtkObject
{
  public:
    static vtkEventQtSlotConnect* New();
    vtkTypeMacro(vtkEventQtSlotConnect, vtkObject)
    
    // Description:
    // Print the current connections between VTK and Qt
    void PrintSelf(ostream& os, vtkIndent indent);
    
    // Description:
    // Connect a vtk object's event with a Qt object's slot.  Multiple
    // connections which are identical are treated as separate connections.
    virtual void Connect(vtkObject* vtk_obj, unsigned long event, 
                         const QObject* qt_obj, const char* slot, 
                         void* client_data=NULL, float priority=0.0
#if QT_VERSION >= 0x04000
                         ,Qt::ConnectionType type = Qt::AutoConnection);
#else
                         );
#endif
    
    // Description:
    // Disconnect a vtk object from a qt object.
    // Passing no arguments will disconnect all slots maintained by this object.
    // Passing in only a vtk object will disconnect all slots from it.
    // Passing only a vtk object and event, will disconnect all slots matching 
    // the vtk object and event.
    // Passing all information in will match all information.
    virtual void Disconnect(
      vtkObject* vtk_obj=NULL, unsigned long event=vtkCommand::NoEvent, 
      const QObject* qt_obj=NULL, const char* slot = 0, void* client_data=NULL);

  protected:
    vtkQtConnections* Connections;
  
    vtkEventQtSlotConnect();
    ~vtkEventQtSlotConnect();

  private:
    // unimplemented
    vtkEventQtSlotConnect(const vtkEventQtSlotConnect&);
    void operator=(const vtkEventQtSlotConnect&);
};



// class for managing a single VTK/Qt connection
// not to be used directly, only here in header file for 
// moc to process
class vtkQtConnection : public QObject
{
  Q_OBJECT
  
  public:

    // constructor
    vtkQtConnection();

    // destructor, disconnect if necessary
    ~vtkQtConnection();
   
    // print function
    void PrintSelf(ostream& os, vtkIndent indent);
    
    // callback from VTK to emit signal
    void Execute(vtkObject* caller, unsigned long event, void* client_data);
    
    // set the connection
    void SetConnection(vtkObject* vtk_obj, unsigned long event,
                       const QObject* qt_obj, const char* slot, 
                       void* client_data, float priority=0.0
#if QT_VERSION >= 0x04000
                         ,Qt::ConnectionType type = Qt::AutoConnection);
#else
                         );
#endif
    
    // check if a connection matches input parameters
    bool IsConnection(vtkObject* vtk_obj, unsigned long event,
                      const QObject* qt_obj, const char* slot,
                      void* client_data);

    static void DoCallback(vtkObject* vtk_obj, unsigned long event,
                           void* client_data, void* call_data);
    
  signals:
    // the qt signal for moc to take care of
    void EmitExecute(vtkObject*, unsigned long, void* client_data, void* call_data, vtkCommand*);

  protected:
    
    // the connection information
    vtkObject* VTKObject;
    vtkCallbackCommand* Callback;
    const QObject* QtObject;
    void* ClientData;
    unsigned long VTKEvent;
    QString QtSlot;

  private:
    vtkQtConnection(const vtkQtConnection&);
    void operator=(const vtkQtConnection&);

};

#endif

