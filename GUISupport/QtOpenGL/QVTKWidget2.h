/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKWidget2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME QVTKWidget2 - Display a vtkRenderWindow in a Qt's QGLWidget.
// .SECTION Description
// QVTKWidget2 provides a way to display VTK data in a Qt OpenGL widget.

#ifndef Q_VTK_WIDGET2_H
#define Q_VTK_WIDGET2_H

#include "vtkGUISupportQtOpenGLModule.h" // For export macro
#include <QtOpenGL/QGLWidget>
#include "vtkSmartPointer.h"
#include "QVTKWin32Header.h"

class vtkGenericOpenGLRenderWindow;
class vtkEventQtSlotConnect;
class QVTKInteractorAdapter;
class QVTKInteractor;
class vtkObject;

#include "vtkTDxConfigure.h" // defines VTK_USE_TDX
#ifdef VTK_USE_TDX
class vtkTDxDevice;
#endif

//! QVTKWidget2 displays a VTK window in a Qt window.
class VTKGUISUPPORTQTOPENGL_EXPORT QVTKWidget2 : public QGLWidget
{
  Q_OBJECT
  typedef QGLWidget Superclass;
public:
  //! constructor
  QVTKWidget2(QWidget* parent = NULL, const QGLWidget* shareWidget=0, Qt::WindowFlags f = 0);
  QVTKWidget2(vtkGenericOpenGLRenderWindow* w, QWidget* parent = NULL, const QGLWidget* shareWidget=0, Qt::WindowFlags f = 0);
  QVTKWidget2(QGLContext* ctx, QWidget* parent = NULL, const QGLWidget* shareWidget=0, Qt::WindowFlags f = 0);
  QVTKWidget2(const QGLFormat& fmt, QWidget* parent = NULL, const QGLWidget* shareWidget=0, Qt::WindowFlags f = 0);
  //! destructor
  virtual ~QVTKWidget2();

  // Description:
  // Set the vtk render window, if you wish to use your own vtkRenderWindow
  virtual void SetRenderWindow(vtkGenericOpenGLRenderWindow*);

  // Description:
  // Get the vtk render window.
  virtual vtkGenericOpenGLRenderWindow* GetRenderWindow();

  // Description:
  // Get the Qt/vtk interactor that was either created by default or set by the user
  virtual QVTKInteractor* GetInteractor();

  // Description:
  // Get the number of multisamples used for antialiasing
  virtual int GetMultiSamples() const;

  // Description:
  // Use a 3DConnexion device. Initial value is false.
  // If VTK is not build with the TDx option, this is no-op.
  // If VTK is build with the TDx option, and a device is not connected,
  // a warning is emitted.
  // It is must be called before the first Render to be effective, otherwise
  // it is ignored.
  void SetUseTDx(bool useTDx);
  bool GetUseTDx() const;

  // Description:
  // Make the swap buffers functions public
  void setAutoBufferSwap(bool);
  bool autoBufferSwap() const;

  static QGLFormat GetDefaultVTKFormat(vtkGenericOpenGLRenderWindow* w = NULL);

signals:
  void mouseEvent(QMouseEvent* e);

public Q_SLOTS:

  // Description:
  // Receive notification of the creation of the TDxDevice.
  // Only relevant for Unix.
#ifdef VTK_USE_TDX
  void setDevice(vtkTDxDevice *device);
#endif

protected Q_SLOTS:
  // slot to make this vtk render window current
  virtual void MakeCurrent();
  // slot called when vtk wants to know if the context is current
  virtual void IsCurrent(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  // slot called when vtk wants to frame the window
  virtual void Frame();
  // slot called when vtk wants to start the render
  virtual void Start();
  // slot called when vtk wants to end the render
  virtual void End();
  // slot called when vtk wants to know if a window is direct
  virtual void IsDirect(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
  // slot called when vtk wants to know if a window supports OpenGL
  virtual void SupportsOpenGL(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);

protected:
  // overloaded initialize handler
  virtual void initializeGL();
  // overloaded resize handler
  virtual void resizeGL(int, int);
  // overloaded paint handler
  virtual void paintGL();
  // overloaded move handler
  virtual void moveEvent(QMoveEvent* event);

  // overloaded touch events
  virtual bool event(QEvent* e);
  // overloaded mouse press handler
  virtual void mousePressEvent(QMouseEvent* event);
  // overloaded mouse move handler
  virtual void mouseMoveEvent(QMouseEvent* event);
  // overloaded mouse release handler
  virtual void mouseReleaseEvent(QMouseEvent* event);
  // overloaded key press handler
  virtual void keyPressEvent(QKeyEvent* event);
  // overloaded key release handler
  virtual void keyReleaseEvent(QKeyEvent* event);
  // overloaded enter event
  virtual void enterEvent(QEvent*);
  // overloaded leave event
  virtual void leaveEvent(QEvent*);
  // overload wheel mouse event
  virtual void wheelEvent(QWheelEvent*);

  // overload context menu event
  virtual void contextMenuEvent(QContextMenuEvent*);
  // overload drag enter event
  virtual void dragEnterEvent(QDragEnterEvent*);
  // overload drag move event
  virtual void dragMoveEvent(QDragMoveEvent*);
  // overload drag leave event
  virtual void dragLeaveEvent(QDragLeaveEvent*);
  // overload drop event
  virtual void dropEvent(QDropEvent*);

  // overload focus handling so tab key is passed to VTK
  virtual bool focusNextPrevChild(bool);

  // the vtk render window
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> mRenWin;
  bool UseTDx;

  QVTKInteractorAdapter* mIrenAdapter;
  vtkSmartPointer<vtkEventQtSlotConnect> mConnect;

private:
  //! unimplemented operator=
  QVTKWidget2 const& operator=(QVTKWidget2 const&);
  //! unimplemented copy
  QVTKWidget2(const QVTKWidget2&);

};

#endif
