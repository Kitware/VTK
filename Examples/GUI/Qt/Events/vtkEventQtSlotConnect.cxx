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

#include <vector>

#include "qobject.h"


class vtkQtConnection::vtkQtConnectionString : public vtkstd::string 
{
  public:
    vtkQtConnectionString(const char* str) : vtkstd::string(str) {}
    vtkQtConnectionString() : vtkstd::string() {}
};

// standard new
vtkQtConnection* vtkQtConnection::New()
{
  return new vtkQtConnection;
}

// constructor
vtkQtConnection::vtkQtConnection() 
{
  mQtSlot = new vtkQtConnectionString;
}

// destructor, disconnect if necessary
vtkQtConnection::~vtkQtConnection() 
{
  if(mVTKObject)
  {
    mVTKObject->RemoveObserver(this);
    //Qt takes care of disconnecting slots
  }
  delete mQtSlot;
}
      
// callback from VTK to emit signal
void vtkQtConnection::Execute(vtkObject* caller, unsigned long event, void*)
{
  if(event != vtkCommand::DeleteEvent || 
     event == vtkCommand::DeleteEvent && mVTKEvent == vtkCommand::DeleteEvent)
    {
    emit emitExecute(caller, event, mClientData, this);
    }
  
  if(event == vtkCommand::DeleteEvent)
    {
    mVTKObject->RemoveObserver(this);
    mVTKObject = NULL;
    }
}

bool vtkQtConnection::IsConnection(vtkObject* vtk_obj, unsigned long event,
                  QObject* qt_obj, const char* slot)
{
  if(mVTKObject != vtk_obj)
    return false;

  if(event != vtkCommand::NoEvent && event != mVTKEvent)
    return false;

  if(qt_obj && qt_obj != mQtObject)
    return false;

  if(slot && strcmp(mQtSlot->c_str(), slot) != 0 )
    return false;

  return true;
}
      
// set the connection
void vtkQtConnection::SetConnection(vtkObject* vtk_obj, unsigned long event,
                   QObject* qt_obj, const char* slot, void* client_data, float priority)
{
  // keep track of what we connected
  mVTKObject = vtk_obj;
  mQtObject = qt_obj;
  mVTKEvent = event;
  mClientData = client_data;
  (*mQtSlot) = vtkQtConnectionString(slot);

  // make a connection between this and the vtk object
  vtk_obj->AddObserver(event, this, priority);
   
  if(event != vtkCommand::DeleteEvent)
    {
    vtk_obj->AddObserver(vtkCommand::DeleteEvent, this);
    }

  // make a connection between this and the Qt object
  qt_obj->connect(this, SIGNAL(emitExecute(vtkObject*,unsigned long,void*,vtkCommand*)), slot);
}

      

// hold all the connections
class vtkEventQtSlotConnect::vtkEventQtSlotConnectPrivate : public vtkstd::vector< vtkQtConnection* > {};

vtkStandardNewMacro(vtkEventQtSlotConnect)

// constructor
vtkEventQtSlotConnect::vtkEventQtSlotConnect()
{
  mPrivate = new vtkEventQtSlotConnect::vtkEventQtSlotConnectPrivate;
}


vtkEventQtSlotConnect::~vtkEventQtSlotConnect()
{
  // clean out connections
  vtkEventQtSlotConnect::vtkEventQtSlotConnectPrivate::iterator iter;
  for(iter=mPrivate->begin(); iter!=mPrivate->end(); ++iter)
    {
    (*iter)->Delete();
    }

  delete mPrivate;
}

void vtkEventQtSlotConnect::Connect(vtkObject* vtk_obj, unsigned long event,
                 QObject* qt_obj, const char* slot, void* client_data, float priority)
{
  vtkQtConnection* connection = vtkQtConnection::New();
  connection->SetConnection(vtk_obj, event, qt_obj, slot, client_data, priority);
}


void vtkEventQtSlotConnect::Disconnect(vtkObject* vtk_obj, unsigned long event,
                 QObject* qt_obj, const char* slot)
{
  bool all_info = true;
  if(slot == NULL || qt_obj == NULL || event == vtkCommand::NoEvent)
    all_info = false;

  vtkEventQtSlotConnect::vtkEventQtSlotConnectPrivate::iterator iter;
  for(iter=mPrivate->begin(); iter!=mPrivate->end(); ++iter)
    {
      // if information matches, remove the connection
      if((*iter)->IsConnection(vtk_obj, event, qt_obj, slot))
        {
        (*iter)->Delete();
        iter = mPrivate->erase(iter);
        // if user passed in all information, only remove one connection and quit
        if(all_info)
          iter = mPrivate->end();
        }
    }
}


