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
#include "vtkQtConnection.h"

#include "vtkstd/vector"

#include <qobject.h>

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

void vtkEventQtSlotConnect::Connect(
  vtkObject* vtk_obj, unsigned long event,
  const QObject* qt_obj, const char* slot, 
  void* client_data, float priority
  , Qt::ConnectionType type)
{
  if (!vtk_obj || !qt_obj)
    {
    vtkErrorMacro("Cannot connect NULL objects.");
    return;
    }
  vtkQtConnection* connection = new vtkQtConnection(this);
  connection->SetConnection(
    vtk_obj, event, qt_obj, slot, client_data, priority
    , type);
  Connections->push_back(connection);
}


void vtkEventQtSlotConnect::Disconnect(vtkObject* vtk_obj, unsigned long event,
                 const QObject* qt_obj, const char* slot, void* client_data)
{
  if (!vtk_obj)
    {
    vtkQtConnections::iterator iter;
    for(iter=this->Connections->begin(); iter!=this->Connections->end(); ++iter)
      {
      delete (*iter);
      }
    this->Connections->clear();
    return;
    }
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

void vtkEventQtSlotConnect::RemoveConnection(vtkQtConnection* conn)
{
  vtkQtConnections::iterator iter;
  for(iter=this->Connections->begin(); iter!=this->Connections->end(); ++iter)
    {
    if(conn == *iter)
      {
      delete (*iter);
      Connections->erase(iter);
      return;
      }
    }
}

