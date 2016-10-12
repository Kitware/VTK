/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTDxQtUnixDevices
 * @brief   Manage a list vtkTDXUnixDevice(s).
 *
 * This class is essentially a map between a X11 window id and a
 * vtkTDXUnixDevice. It is uses internally by QVTKApplication.
 *
 * @sa
 * vtkTDxUnixDevice QVTKWidget QVTKApplication
*/

#ifndef vtkTDxQtUnixDevices_h
#define vtkTDxQtUnixDevices_h

#include "vtkGUISupportQtModule.h" // For export macro
#include <QObject>
#include "QVTKWin32Header.h" // for VTKGUISUPPORTQT_EXPORT
#include "vtkTDxUnixDevice.h" // required for vtkTDxUnixDeviceXEvent

class vtkTDxQtUnixDevicesPrivate;

class VTKGUISUPPORTQT_EXPORT vtkTDxQtUnixDevices : public QObject
{
  Q_OBJECT
public:
  vtkTDxQtUnixDevices();
  ~vtkTDxQtUnixDevices();

  /**
   * Process X11 event `e'. Create a device and emit signal CreateDevice if it
   * does not exist yet.
   * \pre e_exists: e!=0
   */
  void ProcessEvent(vtkTDxUnixDeviceXEvent *e);

signals:
/**
 * This signal should be connected to a slot in the QApplication.
 * The slot in the QApplication is supposed to remit this signal.
 * The QVTKWidget have slot to receive this signal from the QApplication.
 */
   void CreateDevice(vtkTDxDevice *device);

protected:

  vtkTDxQtUnixDevicesPrivate *Private;

private:
  vtkTDxQtUnixDevices(const vtkTDxQtUnixDevices&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTDxQtUnixDevices&) VTK_DELETE_FUNCTION;
};

#endif
