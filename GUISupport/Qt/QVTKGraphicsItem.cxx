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

#include "QVTKGraphicsItem.h"
#include <QGLFramebufferObject>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>

#include "QVTKInteractor.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkgl.h"

QVTKGraphicsItem::QVTKGraphicsItem(QGLContext* ctx, QGraphicsItem* p)
  : QGraphicsWidget(p), mContext(ctx)
{
  mFBO = NULL;
  mIren = vtkSmartPointer<QVTKInteractor>::New();
  mIrenAdapter = new QVTKInteractorAdapter(this);
  mConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  mConnect->Connect(mIren, vtkCommand::RenderEvent, this, SLOT(Update()));
  vtkSmartPointer<vtkGenericOpenGLRenderWindow> win = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  this->SetRenderWindow(win);

  this->setFlag(QGraphicsItem::ItemIsFocusable, true);
  setFocusPolicy(Qt::ClickFocus);
  setAcceptHoverEvents(true);
  this->setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding));
}

QVTKGraphicsItem::~QVTKGraphicsItem()
{
  if(mFBO)
    delete mFBO;
}

void QVTKGraphicsItem::SetRenderWindow(vtkGenericOpenGLRenderWindow* win)
{
  if(mWin)
  {
    mConnect->Disconnect(mWin, vtkCommand::StartEvent, this, SLOT(Start()));
    mConnect->Disconnect(mWin, vtkCommand::WindowMakeCurrentEvent, this, SLOT(MakeCurrent()));
    mConnect->Disconnect(mWin, vtkCommand::EndEvent, this, SLOT(End()));
    mConnect->Disconnect(mWin, vtkCommand::WindowFrameEvent, this, SLOT(Update()));
    mConnect->Disconnect(mWin, vtkCommand::WindowIsCurrentEvent, this, SLOT(IsCurrent(vtkObject*, unsigned long, void*, void*)));
  }

  mIren->SetRenderWindow(win);
  mWin = win;

  if(mWin)
  {
    mWin->SetDoubleBuffer(0);
    mWin->SetFrontBuffer(vtkgl::COLOR_ATTACHMENT0_EXT);
    mWin->SetFrontLeftBuffer(vtkgl::COLOR_ATTACHMENT0_EXT);
    mWin->SetBackBuffer(vtkgl::COLOR_ATTACHMENT0_EXT);
    mWin->SetBackLeftBuffer(vtkgl::COLOR_ATTACHMENT0_EXT);

    mConnect->Connect(mWin, vtkCommand::StartEvent, this, SLOT(Start()));
    mConnect->Connect(mWin, vtkCommand::WindowMakeCurrentEvent, this, SLOT(MakeCurrent()));
    mConnect->Connect(mWin, vtkCommand::EndEvent, this, SLOT(End()));
    mConnect->Connect(mWin, vtkCommand::WindowFrameEvent, this, SLOT(Update()));
    mConnect->Connect(mWin, vtkCommand::WindowIsCurrentEvent, this, SLOT(IsCurrent(vtkObject*, unsigned long, void*, void*)));
  }
}

vtkGenericOpenGLRenderWindow* QVTKGraphicsItem::GetRenderWindow() const
{
  return mWin;
}

QVTKInteractor* QVTKGraphicsItem::GetInteractor() const
{
  return mIren;
}

void QVTKGraphicsItem::Update()
{
  this->update(boundingRect());
};

void QVTKGraphicsItem::MakeCurrent()
{
  mContext->makeCurrent();

  QSize sz = this->size().toSize();
  if(!mFBO || sz != mFBO->size())
  {
    if(mFBO)
      delete mFBO;

    if(!sz.isEmpty())
      mFBO = new QGLFramebufferObject(sz, QGLFramebufferObject::Depth);
  }

  if(mFBO)
    mFBO->bind();
}

void QVTKGraphicsItem::Start()
{
  MakeCurrent();

  if(!mFBO)
  {
    mWin->SetAbortRender(1);
    return;
  }

  glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  mWin->OpenGLInit();
}

void QVTKGraphicsItem::End()
{
  if(!mFBO)
    return;

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopClientAttrib();
  glPopAttrib();

  mFBO->release();
}

void QVTKGraphicsItem::IsCurrent(vtkObject*, unsigned long, void*, void* call_data)
{
  if(mFBO)
  {
    bool* ptr = reinterpret_cast<bool*>(call_data);
    *ptr = QGLContext::currentContext() == mContext && mFBO->isBound();
  }
}

void QVTKGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
{
  if(!mWin)
    return;

  // tell Qt we're doing our own GL calls
  // if necessary, it'll put us in an OpenGL 1.x compatible state.
  painter->beginNativePainting();

  if(!mFBO || this->size().toSize() != mFBO->size())
  {
    // first time or is enabled
    // if its not the first time and it is disabled, don't update the scene
    if(!mFBO || isEnabled())
    {
      mIren->Render();
    }
  }

  if(!mFBO)
    return;

  // simply draw the already existing texture to the scene
  // modifications to the texture is done using the VTK api (e.g. vtkRenderWindow::Render())
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, mFBO->texture());

  glDisable(GL_BLEND);

  QRectF r = this->rect();

  glColor4f(1,1,1,1);
  glBegin(GL_QUADS);
  glTexCoord2i(0,1);
  glVertex2f(r.left(),r.top());
  glTexCoord2i(1,1);
  glVertex2f(r.right(),r.top());
  glTexCoord2i(1,0);
  glVertex2f(r.right(),r.bottom());
  glTexCoord2i(0,0);
  glVertex2f(r.left(),r.bottom());
  glEnd();

  painter->endNativePainting();
}

void QVTKGraphicsItem::keyPressEvent(QKeyEvent* e)
{
  e->accept();
  mIrenAdapter->ProcessEvent(e, mIren);
}

void QVTKGraphicsItem::keyReleaseEvent(QKeyEvent* e)
{
  e->accept();
  mIrenAdapter->ProcessEvent(e, mIren);
}

void QVTKGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
  QPointF pf = e->pos();
  QPoint pi = pf.toPoint();

  e->accept();
  QMouseEvent e2(QEvent::MouseButtonPress, pi, e->button(),
      e->buttons(), e->modifiers());
  mIrenAdapter->ProcessEvent(&e2, mIren);
}

void QVTKGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
  QPointF pf = e->pos();
  QPoint pi = pf.toPoint();
  e->accept();
  QMouseEvent e2(QEvent::MouseButtonRelease, pi, e->button(),
      e->buttons(), e->modifiers());
  mIrenAdapter->ProcessEvent(&e2, mIren);
}

void QVTKGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent* e)
{
  QPointF pf = e->pos();
  QPoint pi = pf.toPoint();
  e->accept();
  QMouseEvent e2(QEvent::MouseMove, pi, e->button(),
      e->buttons(), e->modifiers());
  mIrenAdapter->ProcessEvent(&e2, mIren);
}

void QVTKGraphicsItem::wheelEvent(QGraphicsSceneWheelEvent* e)
{
  e->accept();
  QWheelEvent e2(e->pos().toPoint(), e->scenePos().toPoint(), e->delta(),
      e->buttons(), e->modifiers(), e->orientation());
  mIrenAdapter->ProcessEvent(&e2, mIren);
}

void QVTKGraphicsItem::resizeEvent(QGraphicsSceneResizeEvent* e)
{
  e->accept();
  QResizeEvent e2(e->newSize().toSize(), e->oldSize().toSize());
  mIrenAdapter->ProcessEvent(&e2, mIren);
  if(mWin)
    mWin->SetSize(e->newSize().width(), e->newSize().height());

}

void QVTKGraphicsItem::moveEvent(QGraphicsSceneMoveEvent* e)
{
  e->accept();
  QMoveEvent e2(e->newPos().toPoint(), e->oldPos().toPoint());
  if(mWin)
    mWin->SetPosition(e->newPos().x(), e->newPos().y());
}

void QVTKGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
  e->accept();
  QEvent e2(QEvent::Enter);
  mIrenAdapter->ProcessEvent(&e2, mIren);
}

void QVTKGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
  e->accept();
  QEvent e2(QEvent::Leave);
  mIrenAdapter->ProcessEvent(&e2, mIren);
}

void QVTKGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent* e)
{
  e->accept();
  QPointF pf = e->pos();
  QPoint pi = pf.toPoint();
  QMouseEvent e2(QEvent::MouseMove, pi, Qt::NoButton, Qt::NoButton, e->modifiers());
  mIrenAdapter->ProcessEvent(&e2, mIren);
}
