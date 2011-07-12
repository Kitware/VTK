/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKGraphicsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME QVTKGraphicsItem - provides a way to view a VTK scene as an item in QGraphicsView
//
// .SECTION Description
//

#ifndef QVTKGraphicsItem_hpp
#define QVTKGraphicsItem_hpp

#include <QtGui/QGraphicsWidget>
#include <QtOpenGL/QGLContext>
#include <vtkSmartPointer.h>
#include "QVTKWin32Header.h"
class vtkEventQtSlotConnect;
class QGLFramebufferObject;
class QVTKInteractorAdapter;
class QVTKInteractor;
class vtkGenericOpenGLRenderWindow;
class vtkObject;

class QVTK_EXPORT QVTKGraphicsItem : public QGraphicsWidget
{
  Q_OBJECT
  public:
    // Description:
    // constructor.  Takes a QGLContext to use which the QGraphicsView is using.
    QVTKGraphicsItem(QGLContext* ctx, QGraphicsItem* p = 0);

    // Description:
    // destructor
    ~QVTKGraphicsItem();

    // Description:
    // set the render window to use with this item
    void SetRenderWindow(vtkGenericOpenGLRenderWindow* win);

    // Description:
    // get the render window used with this item
    vtkGenericOpenGLRenderWindow* GetRenderWindow() const;

    // Description:
    // get the render window interactor used with this item
    // this item enforces its own interactor
    QVTKInteractor* GetInteractor() const;

  public Q_SLOTS:
    // Description:
    // update this item in the view (this does not cause the vtk render window to draw)
    // it just causes the current contents in the window to draw to the QGraphicsScene
    virtual void Update();

  protected Q_SLOTS:
    // slot to make this vtk render window current
    virtual void MakeCurrent();
    // slot called when vtk render window starts to draw
    virtual void Start();
    // slot called when vtk render window is done drawing
    virtual void End();
    // slot called when vtk wants to know if the context is current
    virtual void IsCurrent(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
    // slot called when vtk wants to know if a window is direct
    virtual void IsDirect(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);
    // slot called when vtk wants to know if a window supports OpenGL
    virtual void SupportsOpenGL(vtkObject* caller, unsigned long vtk_event, void* client_data, void* call_data);

  protected:

    // handle item key events
    void keyPressEvent(QKeyEvent* e);
    void keyReleaseEvent(QKeyEvent* e);

    // handle item mouse events
    void mousePressEvent(QGraphicsSceneMouseEvent* e);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* e);
    void mouseMoveEvent(QGraphicsSceneMouseEvent* e);
    void resizeEvent(QGraphicsSceneResizeEvent* e);
    void moveEvent(QGraphicsSceneMoveEvent* e);
    void wheelEvent(QGraphicsSceneWheelEvent* e);
    void hoverEnterEvent(QGraphicsSceneHoverEvent* e);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* e);
    void hoverMoveEvent(QGraphicsSceneHoverEvent* e);

    // handle item paint event
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    QGLContext* mContext;
    QGLFramebufferObject* mFBO;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> mWin;
    vtkSmartPointer<QVTKInteractor> mIren;
    QVTKInteractorAdapter* mIrenAdapter;
    vtkSmartPointer<vtkEventQtSlotConnect> mConnect;

};

#endif
