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
#include "QVTKInteractorAdapter.h"
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
  QPalette pal = this->palette();
  pal.setColor(QPalette::Window, QColor(255,255,255,255));
  this->setPalette(pal);
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
    mWin->SetMapped(0);
    mConnect->Disconnect(mWin, vtkCommand::StartEvent, this, SLOT(Start()));
    mConnect->Disconnect(mWin, vtkCommand::WindowMakeCurrentEvent, this, SLOT(MakeCurrent()));
    mConnect->Disconnect(mWin, vtkCommand::EndEvent, this, SLOT(End()));
    mConnect->Disconnect(mWin, vtkCommand::WindowFrameEvent, this, SLOT(Update()));
    mConnect->Disconnect(mWin, vtkCommand::WindowIsCurrentEvent, this, SLOT(IsCurrent(vtkObject*, unsigned long, void*, void*)));
    mConnect->Disconnect(mWin, vtkCommand::WindowIsDirectEvent, this, SLOT(IsDirect(vtkObject*, unsigned long, void*, void*)));
    mConnect->Disconnect(mWin, vtkCommand::WindowSupportsOpenGLEvent, this, SLOT(SupportsOpenGL(vtkObject*, unsigned long, void*, void*)));
  }

  mIren->SetRenderWindow(win);
  mWin = win;
  mIren->Initialize();

  if(mWin)
  {
    mWin->SetMapped(1);
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
    mConnect->Connect(mWin, vtkCommand::WindowIsDirectEvent, this, SLOT(IsDirect(vtkObject*, unsigned long, void*, void*)));
    mConnect->Connect(mWin, vtkCommand::WindowSupportsOpenGLEvent, this, SLOT(SupportsOpenGL(vtkObject*, unsigned long, void*, void*)));
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
  if(this->mWin && this->mFBO)
    {
    this->update(boundingRect());
    }
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

  mWin->PushState();
  mWin->OpenGLInitState();
}

void QVTKGraphicsItem::End()
{
  if(!mFBO)
    return;

  mWin->PopState();

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

void QVTKGraphicsItem::IsDirect(vtkObject*, unsigned long, void*, void* call_data)
{
  int* ptr = reinterpret_cast<int*>(call_data);
  *ptr = 1;
}

void QVTKGraphicsItem::SupportsOpenGL(vtkObject*, unsigned long, void*, void* call_data)
{
  int* ptr = reinterpret_cast<int*>(call_data);
  *ptr = QGLFormat::hasOpenGL();
}


#if QT_VERSION >= 0x040600
void QVTKGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem*, QWidget*)
#else
void QVTKGraphicsItem::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
#endif
{
  if(!mWin)
    return;

#if QT_VERSION >= 0x040600
  // tell Qt we're doing our own GL calls
  // if necessary, it'll put us in an OpenGL 1.x compatible state.
  painter->beginNativePainting();
#endif

  if(!mFBO || this->size().toSize() != mFBO->size() || mWin->GetNeverRendered())
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


  QRectF r = this->rect();

  QColor c = this->palette().color(QPalette::Window);
  glColor4ub(c.red(),c.green(),c.blue(),c.alpha());

  if(c.alpha() < 255)
    {
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
  else
    {
    glDisable(GL_BLEND);
    }

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

  glBindTexture(GL_TEXTURE_2D, 0);

#if QT_VERSION >= 0x040600
  painter->endNativePainting();
#endif
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
    mWin->SetSize(e2.size().width(), e2.size().height());

}

void QVTKGraphicsItem::moveEvent(QGraphicsSceneMoveEvent* e)
{
  e->accept();
  QMoveEvent e2(e->newPos().toPoint(), e->oldPos().toPoint());
  if(mWin)
    mWin->SetPosition(e2.pos().x(), e2.pos().y());
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
