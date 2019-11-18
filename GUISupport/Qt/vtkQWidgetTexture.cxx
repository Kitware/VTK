/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQWidgetTexture.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLState.h"

#include <QGraphicsProxyWidget>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QWidget>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkQWidgetTexture);

//----------------------------------------------------------------------------
vtkQWidgetTexture::vtkQWidgetTexture()
{
  this->Framebuffer = nullptr;
  this->OffscreenSurface = nullptr;
  this->Scene = nullptr;
  this->Widget = nullptr;
  this->SetMagnificationFilter(vtkTextureObject::Linear);
  this->SetMinificationFilter(vtkTextureObject::Linear);

  this->RedrawMethod = [this]() {
    if (this->Framebuffer)
    {
      this->Context->MakeCurrent();
      auto state = this->Context->GetState();
      state->PushFramebufferBindings();
      this->Framebuffer->bind();

      QOpenGLPaintDevice* device = new QOpenGLPaintDevice(this->Framebuffer->size());
      QPainter* painter = new QPainter(device);

      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
      this->Scene->render(painter);
      this->Framebuffer->release();

      this->AssignToExistingTexture(this->Framebuffer->texture(), GL_TEXTURE_2D);

      delete painter;
      delete device;

      // bring vtk state back in sync with Qt state
      state->PopFramebufferBindings();
      state->ResetEnumState(GL_BLEND);
      state->ResetEnumState(GL_DEPTH_TEST);
      state->ResetEnumState(GL_SCISSOR_TEST);
#ifdef GL_MULTISAMPLE
      state->ResetEnumState(GL_MULTISAMPLE);
#endif
      state->ResetGLScissorState();
      state->ResetGLClearColorState();
      state->ResetGLViewportState();
      state->ResetGLDepthFuncState();
      state->ResetGLBlendFuncState();
      state->ResetFramebufferBindings();
      // reset the depth test to LEQUAL as all vtk classes
      // expect this to be the case when called
      state->vtkglDepthFunc(GL_LEQUAL);
    }
  };
}

//----------------------------------------------------------------------------
vtkQWidgetTexture::~vtkQWidgetTexture()
{
  this->SetWidget(nullptr);
  delete this->Scene;
  this->Scene = nullptr;
  delete this->OffscreenSurface;
  this->OffscreenSurface = nullptr;
  delete this->Framebuffer;
}

//---------------------------------------------------------------------------
void vtkQWidgetTexture::ReleaseGraphicsResources(vtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->Superclass::ReleaseGraphicsResources(win);

  delete this->Framebuffer;
  this->Framebuffer = nullptr;
}

// just hold onto the widget until opengl context is active
void vtkQWidgetTexture::SetWidget(QWidget* w)
{
  if (this->Widget == w)
  {
    return;
  }

  if (w == nullptr && this->Scene && this->Widget->graphicsProxyWidget())
  {
    this->Scene->removeItem(this->Widget->graphicsProxyWidget());
  }

  this->Widget = w;

  this->Modified();
}

// handle any setup required, only call when OpenGL context is active
void vtkQWidgetTexture::AllocateFromWidget()
{
  if (this->OffscreenSurface && this->Framebuffer)
  {
    return;
  }

  // the Qt code can modify a lot of OpenGL State
  // some of which we may want to preserve
  auto state = this->Context->GetState();
  vtkOpenGLState::ScopedglEnableDisable state0(state, GL_BLEND);
  vtkOpenGLState::ScopedglEnableDisable state1(state, GL_DEPTH_TEST);
  vtkOpenGLState::ScopedglEnableDisable state2(state, GL_SCISSOR_TEST);
#ifdef GL_MULTISAMPLE
  vtkOpenGLState::ScopedglEnableDisable state3(state, GL_MULTISAMPLE);
#endif
  vtkOpenGLState::ScopedglBlendFuncSeparate state5(state);
  vtkOpenGLState::ScopedglDepthFunc state6(state);
  vtkOpenGLState::ScopedglViewport state7(state);

  // typically just created once, maybe no OpenGL
  if (!this->OffscreenSurface)
  {
    if (!this->Widget)
    {
      return;
    }

    this->OffscreenSurface = new QOffscreenSurface();
    this->OffscreenSurface->create();

    this->Scene = new QGraphicsScene();

    this->Widget->move(0, 0);
    this->Scene->addWidget(this->Widget);

    QObject::connect(this->Scene, &QGraphicsScene::changed, this->RedrawMethod);
  }

  // Framebuffer gets freed when ReleaseGraphicsResources is called
  // so re setup as needed
  if (!this->Framebuffer)
  {
    this->Framebuffer =
      new QOpenGLFramebufferObject(this->Widget->width(), this->Widget->height(), GL_TEXTURE_2D);
    this->RedrawMethod();
  }
}

void vtkQWidgetTexture::Activate()
{
  // make sure everything is setup in Qt and the texture is created
  this->AllocateFromWidget();
  // do normal activate
  this->Superclass::Activate();
}
