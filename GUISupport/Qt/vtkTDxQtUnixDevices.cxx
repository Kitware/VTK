/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkDepthPeelingPass.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTDxQtUnixDevices.h"
#include <vtksys/stl/map>
#include <QApplication> // topLevelWidgets()
#include <QWidget>
#include <X11/Xlib.h> // Needed for X types used in the public interface
#include "vtkSmartPointer.h"

class vtkLessThanWindowId
{
public:
  bool operator()(const vtkTDxUnixDeviceWindow &w1,
                  const vtkTDxUnixDeviceWindow &w2) const
    {
      return w1<w2;
    }
};

typedef std::map<vtkTDxUnixDeviceWindow, vtkSmartPointer<vtkTDxUnixDevice>,
                    vtkLessThanWindowId> vtkWindowIdToDevice;

class vtkTDxQtUnixDevicesPrivate
{
public:
  vtkWindowIdToDevice Map;
};


// ----------------------------------------------------------------------------
vtkTDxQtUnixDevices::vtkTDxQtUnixDevices()
{
  this->Private=new vtkTDxQtUnixDevicesPrivate;
}

// ----------------------------------------------------------------------------
vtkTDxQtUnixDevices::~vtkTDxQtUnixDevices()
{
  delete this->Private;
}

// ----------------------------------------------------------------------------
void vtkTDxQtUnixDevices::ProcessEvent(vtkTDxUnixDeviceXEvent *e)
{
  const XEvent *event=static_cast<const XEvent *>(e);

  // Find the real X11 window id.
  QWidgetList l=static_cast<QApplication *>(QApplication::instance())
    ->topLevelWidgets();
  int winIdLast=0;
  foreach(QWidget *w,l)
    {
    if(!w->isHidden())
      {
      winIdLast=w->winId();
      }
    }

  vtkTDxUnixDeviceWindow winId=static_cast<vtkTDxUnixDeviceWindow>(winIdLast);
    // ;event->xany.window;

  if(winId!=0)
    {
    vtkWindowIdToDevice::iterator it=this->Private->Map.find(winId);
    vtkSmartPointer<vtkTDxUnixDevice> device=0;
    if(it==this->Private->Map.end())
      {
      // not yet created.
      device=vtkSmartPointer<vtkTDxUnixDevice>::New();
      this->Private->Map.insert(
        std::pair<const vtkTDxUnixDeviceWindow ,vtkSmartPointer<vtkTDxUnixDevice> >(
          winId,device));

      device->SetDisplayId(event->xany.display);
      device->SetWindowId(winId);
      device->SetInteractor(0);
      device->Initialize();

      if(!device->GetInitialized())
        {
        vtkGenericWarningMacro("failed to initialize device.");
        }
      else
        {
        cout << "device initialized on window" << winId << hex << winId << dec;
        emit CreateDevice(device);
        }
      }
    else
      {
      device=(*it).second;
      }

    if(event->type==ClientMessage) // 33
      {
      bool caught=false;
      if(device->GetInitialized())
        {
        caught=device->ProcessEvent(e);
        }
      }
    }
}
