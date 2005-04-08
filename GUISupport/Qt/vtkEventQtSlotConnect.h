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
// - MySlot(vtkObject* caller, unsigned long vtk_event, void* client_data, vtkCommand*)


#ifndef VTK_EVENT_QT_SLOT_CONNECT
#define VTK_EVENT_QT_SLOT_CONNECT

#include "vtkObject.h"
#include "vtkCommand.h"  // for event defines

class QObject;

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
    // Connect a vtk object's event with a Qt object's slot.
    // Multiple connections which are identical are treated as separate connections.
    virtual void Connect(vtkObject* vtk_obj, unsigned long event, 
                 QObject* qt_obj, const char* slot, void* client_data=NULL, float priority=0.0);
    
    // Description:
    // Disconnect a vtk object from a qt object.
    // Passing in only a vtk object will disconnect all slots from it.
    // Passing only a vtk object and event, will disconnect all slots matching the vtk object and event.
    // Passing all information in will match all information.
    virtual void Disconnect(vtkObject* vtk_obj, unsigned long event=vtkCommand::NoEvent, 
                 QObject* qt_obj=NULL, const char* slot = 0);

  protected:
    vtkQtConnections* Connections;
  
    vtkEventQtSlotConnect();
    ~vtkEventQtSlotConnect();

  private:
    // unimplemented
    vtkEventQtSlotConnect(const vtkEventQtSlotConnect&);
    void operator=(const vtkEventQtSlotConnect&);
};

#endif

