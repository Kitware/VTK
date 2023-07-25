// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkQWidgetTexture.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLResourceFreeCallback.h"
#include "vtkOpenGLState.h"

#include "vtkOpenGLError.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QOpenGLPaintDevice>
#include <QPainter>
#include <QWidget>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkQWidgetTexture);

//------------------------------------------------------------------------------
void vtkQWidgetTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkQWidgetTexture::vtkQWidgetTexture()
  : Scene(nullptr)
  , Widget(nullptr)
  , ImageBuffer(nullptr)
  , ImageBufferDimensions({ 0, 0 })
{
  this->SetMagnificationFilter(vtkTextureObject::Linear);
  this->SetMinificationFilter(vtkTextureObject::LinearMipmapLinear);
  this->GenerateMipmap = true;

  this->RedrawMethod = [this]() {
    if (this->Scene && this->Widget)
    {
      if (!this->Context)
      {
        return;
      }

      auto size = this->Widget->size();
      int width = size.width();
      int height = size.height();

      this->Context->MakeCurrent();
      auto state = this->Context->GetState();

      state->Reset();
      state->Push();

      auto ImageData = QImage(size, QImage::Format_ARGB32);
      QPainter* painter = new QPainter(&ImageData);
      this->Scene->render(painter);
      painter->end();
      delete painter;

      this->Context->MakeCurrent();
      state->Reset();
      state->Pop();

      // copy the data
      if (this->ImageBufferDimensions[0] != width || this->ImageBufferDimensions[1] != height)
      {
        delete[] this->ImageBuffer;
        this->ImageBuffer = new unsigned char[width * height * 4];
        this->ImageBufferDimensions[0] = width;
        this->ImageBufferDimensions[1] = height;
      }
      unsigned char* imgPtr = this->ImageBuffer;
      for (int j = 0; j < height; j++)
      {
        unsigned char const* line = ImageData.scanLine(height - j - 1);
        unsigned char const* linePtr = line;
        for (int i = 0; i < width; i++)
        {
          *imgPtr++ = linePtr[2];
          *imgPtr++ = linePtr[1];
          *imgPtr++ = linePtr[0];
          *imgPtr++ = 0xff;
          linePtr += 4;
        }
      }

      this->Create2DFromRaw(width, height, 4, VTK_UNSIGNED_CHAR, this->ImageBuffer);
      vtkOpenGLCheckErrorMacro("failed after QWidgeTexture repaint and draw");
    }
  };
}

//------------------------------------------------------------------------------
vtkQWidgetTexture::~vtkQWidgetTexture()
{
  this->SetWidget(nullptr);
  delete this->Scene;
  this->Scene = nullptr;
  delete[] this->ImageBuffer;
}

//------------------------------------------------------------------------------
void vtkQWidgetTexture::ReleaseGraphicsResources(vtkWindow* win)
{
  if (!this->ResourceCallback->IsReleasing())
  {
    this->ResourceCallback->Release();
    return;
  }

  this->Superclass::ReleaseGraphicsResources(win);
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
  if (!this->Widget)
  {
    return;
  }

  if (!this->Scene)
  {
    // the Qt code can modify a lot of OpenGL State
    // some of which we may want to preserve
    auto state = this->Context->GetState();
    state->Reset();
    state->Push();

    // typically just created once, maybe no OpenGL
    this->Scene = new QGraphicsScene();

    this->Widget->move(0, 0);
    this->Scene->addWidget(this->Widget);
    this->Widget->show();

    QObject::connect(this->Scene, &QGraphicsScene::changed, this->RedrawMethod);
    state->Pop();
  }

  if (!this->Handle)
  {
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
VTK_ABI_NAMESPACE_END
