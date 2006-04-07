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

#include "vtkEventQtSlotConnect.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"

#include "vtkstd/vector"

#include <qobject.h>
#include <qmetaobject.h>

// constructor
vtkQtConnection::vtkQtConnection() 
{
  Callback = vtkCallbackCommand::New();
  Callback->SetCallback(vtkQtConnection::DoCallback);
  this->Callback->SetClientData(this);
}

// destructor, disconnect if necessary
vtkQtConnection::~vtkQtConnection() 
{
  if(VTKObject)
  {
    VTKObject->RemoveObserver(this->Callback);
    //Qt takes care of disconnecting slots
  }
  Callback->Delete();
}

void vtkQtConnection::DoCallback(vtkObject* vtk_obj, unsigned long event,
                                 void* client_data, void* call_data)
{
  vtkQtConnection* conn = static_cast<vtkQtConnection*>(client_data);
  conn->Execute(vtk_obj, event, call_data);
}
    
      
// callback from VTK to emit signal
void vtkQtConnection::Execute(vtkObject* caller, unsigned long e, void* call_data)
{
  if(e != vtkCommand::DeleteEvent || 
     e == vtkCommand::DeleteEvent && VTKEvent == vtkCommand::DeleteEvent)
    {
    emit EmitExecute(caller, e, ClientData, call_data, this->Callback);
    }
  
  if(e == vtkCommand::DeleteEvent)
    {
    VTKObject->RemoveObserver(this->Callback);
    VTKObject = NULL;
    }
}

bool vtkQtConnection::IsConnection(vtkObject* vtk_obj, unsigned long e,
                  const QObject* qt_obj, const char* slot, void* client_data)
{
  if(VTKObject != vtk_obj)
    return false;

  if(e != vtkCommand::NoEvent && e != VTKEvent)
    return false;

  if(qt_obj && qt_obj != QtObject)
    return false;

  if(slot && QtSlot != slot)
    return false;

  if(client_data && ClientData != client_data)
    return false;

  return true;
}
      
// set the connection
void vtkQtConnection::SetConnection(vtkObject* vtk_obj, unsigned long e,
                   const QObject* qt_obj, const char* slot, void* client_data, float priority)
{
  // keep track of what we connected
  VTKObject = vtk_obj;
  QtObject = qt_obj;
  VTKEvent = e;
  ClientData = client_data;
  QtSlot = slot;

  // make a connection between this and the vtk object
  vtk_obj->AddObserver(e, this->Callback, priority);

  if(e != vtkCommand::DeleteEvent)
    {
    vtk_obj->AddObserver(vtkCommand::DeleteEvent, this->Callback);
    }

  // make a connection between this and the Qt object
  qt_obj->connect(this, SIGNAL(EmitExecute(vtkObject*,unsigned long,void*,void*,vtkCommand*)), slot);
}

void vtkQtConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << 
        this->VTKObject->GetClassName() << ":" <<
        vtkCommand::GetStringFromEventId(this->VTKEvent) << "  <---->  " <<
        this->QtObject->metaObject()->className() << "::" <<
#if QT_VERSION < 0x040000
        this->QtSlot << "\n";
#else
        this->QtSlot.toAscii().data() << "\n";
#endif
}
      

// hold all the connections
class vtkQtConnections : public vtkstd::vector< vtkQtConnection* > {};

vtkStandardNewMacro(vtkEventQtSlotConnect)

// constructor
vtkEventQtSlotConnect::vtkEventQtSlotConnect()
{
  Connections = new vtkQtConnections;
}


vtkEventQtSlotConnect::~vtkEventQtSlotConnect()
{
  // clean out connections
  vtkQtConnections::iterator iter;
  for(iter=Connections->begin(); iter!=Connections->end(); ++iter)
    {
    delete (*iter);
    }

  delete Connections;
}

void vtkEventQtSlotConnect::Connect(vtkObject* vtk_obj, unsigned long event,
                 const QObject* qt_obj, const char* slot, void* client_data, float priority)
{
  vtkQtConnection* connection = new vtkQtConnection;
  connection->SetConnection(vtk_obj, event, qt_obj, slot, client_data, priority);
  Connections->push_back(connection);
}


void vtkEventQtSlotConnect::Disconnect(vtkObject* vtk_obj, unsigned long event,
                 const QObject* qt_obj, const char* slot, void* client_data)
{
  bool all_info = true;
  if(slot == NULL || qt_obj == NULL || event == vtkCommand::NoEvent)
    all_info = false;

  vtkQtConnections::iterator iter;
  for(iter=Connections->begin(); iter!=Connections->end();)
    {
      // if information matches, remove the connection
      if((*iter)->IsConnection(vtk_obj, event, qt_obj, slot, client_data))
        {
        delete (*iter);
        iter = Connections->erase(iter);
        // if user passed in all information, only remove one connection and quit
        if(all_info)
          iter = Connections->end();
        }
      else
        ++iter;
    }
}

void vtkEventQtSlotConnect::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if(Connections->empty())
    {
    os << indent << "No Connections\n";
    }
  else
    {
    os << indent << "Connections:\n";
    vtkQtConnections::iterator iter;
    for(iter=Connections->begin(); iter!=Connections->end(); ++iter)
      {
      (*iter)->PrintSelf(os, indent.GetNextIndent());
      }
    }
}


