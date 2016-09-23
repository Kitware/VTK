/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLabelRenderStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQtLabelRenderStrategy.h"

#include "vtkCoordinate.h"
#include "vtkImageData.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLabelSizeCalculator.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkQImageToImageSource.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTexturedActor2D.h"
#include "vtkTextureMapToPlane.h"
#include "vtkTimerLog.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QMap>
#include <QPainter>
#include <QPair>
#include <QPixmap>
#include <QTextDocument>
#include <QTextStream>

vtkStandardNewMacro(vtkQtLabelRenderStrategy);

namespace
{
struct vtkQtLabelMapEntry
{
  QString Text;
  QColor Color;
  QFont Font;
};

struct vtkQtLabelMapValue
{
  QImage Image;
  QRectF Bounds;
};

bool operator <(const vtkQtLabelMapEntry& a, const vtkQtLabelMapEntry& other)
{
  if (a.Text != other.Text)
  {
    return a.Text < other.Text;
  }
  if (a.Color.red() != other.Color.red())
  {
    return a.Color.red() < other.Color.red();
  }
  if (a.Color.green() != other.Color.green())
  {
    return a.Color.green() < other.Color.green();
  }
  if (a.Color.blue() != other.Color.blue())
  {
    return a.Color.blue() < other.Color.blue();
  }
  if (a.Color.alpha() != other.Color.alpha())
  {
    return a.Color.alpha() < other.Color.alpha();
  }
  return a.Font < other.Font;
}
} // End of anonymous namespace

class vtkQtLabelRenderStrategy::Internals
{
public:
  QImage* Image;
  QPainter* Painter;
  QMap<vtkQtLabelMapEntry, vtkQtLabelMapValue> Cache;

  QFont TextPropertyToFont(vtkTextProperty* tprop)
  {
    QFont fontSpec(tprop->GetFontFamilyAsString());
    fontSpec.setBold(tprop->GetBold());
    fontSpec.setItalic(tprop->GetItalic());
    fontSpec.setPixelSize(tprop->GetFontSize());
    return fontSpec;
  }

  QColor TextPropertyToColor(double* fc, double opacity)
  {
    QColor textColor(
      static_cast<int>(fc[0]*255),
      static_cast<int>(fc[1]*255),
      static_cast<int>(fc[2]*255),
      static_cast<int>(opacity*255));
    return textColor;
  }
};

//----------------------------------------------------------------------------
vtkQtLabelRenderStrategy::vtkQtLabelRenderStrategy()
{
  this->Implementation = new Internals();
  this->Implementation->Image = new QImage(1, 1, QImage::Format_ARGB32_Premultiplied);
  this->Implementation->Painter = new QPainter(this->Implementation->Image);

  this->QImageToImage = vtkQImageToImageSource::New();
  this->PlaneSource = vtkPlaneSource::New();
  this->TextureMapToPlane = vtkTextureMapToPlane::New();
  this->Texture = vtkTexture::New();
  this->Mapper = vtkPolyDataMapper2D::New();
  this->Actor = vtkTexturedActor2D::New();

  this->QImageToImage->SetQImage(this->Implementation->Image);

  this->PlaneSource->SetOrigin(0, 0, 0);

  this->TextureMapToPlane->SetInputConnection(this->PlaneSource->GetOutputPort());
  this->TextureMapToPlane->AutomaticPlaneGenerationOn();
  this->TextureMapToPlane->SetSRange(0., 1.);
  this->TextureMapToPlane->SetTRange(0., 1.);

  this->Mapper->SetInputConnection(this->TextureMapToPlane->GetOutputPort());
  this->Texture->SetInputConnection(this->QImageToImage->GetOutputPort());
  this->Texture->PremultipliedAlphaOn();
  this->Actor->SetTexture(this->Texture);
  this->Actor->SetMapper(this->Mapper);
}

//----------------------------------------------------------------------------
vtkQtLabelRenderStrategy::~vtkQtLabelRenderStrategy()
{
  delete this->Implementation->Painter;
  delete this->Implementation->Image;
  delete this->Implementation;
  this->QImageToImage->Delete();
  this->PlaneSource->Delete();
  this->TextureMapToPlane->Delete();
  this->Texture->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
}

void vtkQtLabelRenderStrategy::ReleaseGraphicsResources(vtkWindow *window)
{
  this->Texture->ReleaseGraphicsResources(window);
  this->Mapper->ReleaseGraphicsResources(window);
  this->Actor->ReleaseGraphicsResources(window);
}

//double start_frame_time = 0;
//int start_frame_iter = 0;
//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::StartFrame()
{
  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();

  if (!this->Renderer)
  {
    vtkErrorMacro("Renderer must be set.");
    return;
  }

  if (!this->Renderer->GetRenderWindow())
  {
    vtkErrorMacro("RenderWindow must be set.");
    return;
  }

  int *size = this->Renderer->GetRenderWindow()->GetSize();
  int width = size[0];
  int height = size[1];
  // If the render window is not antialiased then the text should not be
  this->AntialiasText = this->Renderer->GetRenderWindow()->GetMultiSamples() > 0;

  if (this->Implementation->Image->width() != width ||
      this->Implementation->Image->height() != height)
  {
    this->Implementation->Painter->end();
    delete this->Implementation->Image;
    this->Implementation->Image = new QImage(width, height,
                                             QImage::Format_ARGB32_Premultiplied);
    this->Implementation->Painter->begin(this->Implementation->Image);
    this->Implementation->Painter->setRenderHint(QPainter::TextAntialiasing,
                                                 this->AntialiasText);
    this->Implementation->Painter->setRenderHint(QPainter::Antialiasing,
                                                 this->AntialiasText);
    this->QImageToImage->SetQImage(this->Implementation->Image);
    this->PlaneSource->SetPoint1(width, 0, 0);
    this->PlaneSource->SetPoint2(0, height, 0);
  }

  this->Implementation->Image->fill(qRgba(0,0,0,0));
  this->QImageToImage->Modified();

  //timer->StopTimer();
  //start_frame_time += timer->GetElapsedTime();
  //start_frame_iter++;
  //if (start_frame_iter % 10 == 0)
  //  {
  //  cerr << "StartFrame time: " << (start_frame_time / start_frame_iter) << endl;
  //  }
}

//double compute_bounds_time = 0;
//int compute_bounds_iter = 0;
//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::ComputeLabelBounds(
  vtkTextProperty* tprop, vtkUnicodeString label, double bds[4])
{
  if (!QApplication::instance())
  {
    vtkErrorMacro("You must initialize a QApplication before using this class.");
    return;
  }

  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();

  if (!tprop)
  {
    tprop = this->DefaultTextProperty;
  }

  QFont fontSpec = this->Implementation->TextPropertyToFont(tprop);

  // This is the recommended Qt way of controlling text antialiasing.
  if (this->AntialiasText)
  {
    fontSpec.setStyleStrategy(QFont::PreferAntialias);
  }
  else
  {
    fontSpec.setStyleStrategy(QFont::NoAntialias);
  }

  QString text = QString::fromUtf8(label.utf8_str());
  QColor textColor = this->Implementation->TextPropertyToColor(tprop->GetColor(),
                                                               tprop->GetOpacity());
  vtkQtLabelMapEntry key;
  key.Font = fontSpec;
  key.Text = text;
  key.Color = textColor;

  QRectF rect;
  if (this->Implementation->Cache.contains(key))
  {
    rect = this->Implementation->Cache[key].Bounds;
  }
  else
  {
    QPainterPath path;
    path.addText(0, 0, fontSpec, text);
    rect = path.boundingRect();
    this->Implementation->Cache[key].Bounds = rect;
  }

  bds[0] = 0;
  bds[1] = rect.width();
  bds[2] = 0;
  bds[3] = rect.height();

  bds[2] -= tprop->GetLineOffset();
  bds[3] -= tprop->GetLineOffset();

  // Take justification into account
  double sz[2] = {bds[1] - bds[0], bds[3] - bds[2]};
  switch (tprop->GetJustification())
  {
    case VTK_TEXT_LEFT:
      break;
    case VTK_TEXT_CENTERED:
      bds[0] -= sz[0]/2;
      bds[1] -= sz[0]/2;
      break;
    case VTK_TEXT_RIGHT:
      bds[0] -= sz[0];
      bds[1] -= sz[0];
      break;
  }
  switch (tprop->GetVerticalJustification())
  {
    case VTK_TEXT_BOTTOM:
      break;
    case VTK_TEXT_CENTERED:
      bds[2] -= sz[1]/2;
      bds[3] -= sz[1]/2;
      break;
    case VTK_TEXT_TOP:
      bds[2] -= sz[1];
      bds[3] -= sz[1];
      break;
  }

  //timer->StopTimer();
  //compute_bounds_time += timer->GetElapsedTime();
  //compute_bounds_iter++;
  //if (compute_bounds_iter % 10000 == 0)
  //  {
  //  cerr << "ComputeLabelBounds time: " << (compute_bounds_time / compute_bounds_iter) << endl;
  //  }
}

//double render_label_time = 0;
//int render_label_iter = 0;
//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::RenderLabel(
  int x[2], vtkTextProperty* tprop, vtkUnicodeString label, int maxWidth)
{
  if (!QApplication::instance())
  {
    vtkErrorMacro("You must initialize a QApplication before using this class.");
    return;
  }

  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();

  // Determine if we can render the label to fit the width
  QString origText = QString::fromUtf8(label.utf8_str());
  QFont fontSpec = this->Implementation->TextPropertyToFont(tprop);

  // This is the recommended Qt way of controlling text antialiasing.
  if (this->AntialiasText)
  {
    fontSpec.setStyleStrategy(QFont::PreferAntialias);
  }
  else
  {
    fontSpec.setStyleStrategy(QFont::NoAntialias);
  }

  QFontMetrics fontMetric(fontSpec);
  QString text = fontMetric.elidedText(origText, Qt::ElideRight, maxWidth);
  if (origText.length() >= 8 && text.length() < 8)
  {
    // Too small to render.
    return;
  }

  // Get properties from text property
  double rotation = -tprop->GetOrientation();
  QColor textColor = this->Implementation->TextPropertyToColor(tprop->GetColor(), tprop->GetOpacity());
  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double h = size[1]-1;
  double line_offset = tprop->GetLineOffset();
  int shOff[2];
  tprop->GetShadowOffset(shOff);
  double sc[3];
  tprop->GetShadowColor(sc);
  QColor shadowColor = this->Implementation->TextPropertyToColor(sc, tprop->GetOpacity());

  // Compute bounds and justification
  QPainterPath path;
  path.addText(0, 0, fontSpec, text);
  QRectF bounds = path.boundingRect();
  double delta_x = 0., delta_y = 0.;

  switch( tprop->GetJustification() )
  {
    case VTK_TEXT_LEFT:
      break;
    case VTK_TEXT_CENTERED:
      delta_x = -bounds.width()/2.0;
      break;
    case VTK_TEXT_RIGHT:
      delta_x = -bounds.width();
      break;
  }
  switch (tprop->GetVerticalJustification())
  {
    case VTK_TEXT_TOP:
      delta_y = bounds.height() - bounds.bottom();
      break;
    case VTK_TEXT_CENTERED:
      delta_y = bounds.height()/2.0 - bounds.bottom();
      break;
    case VTK_TEXT_BOTTOM:
      delta_y = -bounds.bottom();
      break;
  }

  QPainter* painter = this->Implementation->Painter;
  painter->save();
  painter->translate(x[0], h-x[1]);
  painter->rotate(rotation);
  painter->translate(delta_x, delta_y);
  painter->translate(0., line_offset);

  if (tprop->GetShadow())
  {
    painter->save();
    painter->translate(shOff[0], -shOff[1]);
    painter->fillPath(path, shadowColor);
    painter->restore();
  }

  painter->fillPath(path, textColor);
  painter->restore();

  //timer->StopTimer();
  //render_label_time += timer->GetElapsedTime();
  //render_label_iter++;
  //if (render_label_iter % 100 == 0)
  //  {
  //  cerr << "RenderLabel time: " << (render_label_time / render_label_iter) << endl;
  //  }
}

//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::RenderLabel(
  int x[2], vtkTextProperty* tprop, vtkUnicodeString label)
{
  if (!QApplication::instance())
  {
    vtkErrorMacro("You must initialize a QApplication before using this class.");
    return;
  }

  if (!this->Renderer)
  {
    vtkErrorMacro("Renderer must be set.");
    return;
  }

  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();

  QString text = QString::fromUtf8(label.utf8_str());
  QFont fontSpec = this->Implementation->TextPropertyToFont(tprop);

  // This is the recommended Qt way of controlling text antialiasing.
  if (this->AntialiasText)
  {
    fontSpec.setStyleStrategy(QFont::PreferAntialias);
  }
  else
  {
    fontSpec.setStyleStrategy(QFont::NoAntialias);
  }

  double rotation = -tprop->GetOrientation();
  QColor textColor = this->Implementation->TextPropertyToColor(tprop->GetColor(), tprop->GetOpacity());

  int shOff[2];
  tprop->GetShadowOffset(shOff);
  double pixelPadding = 2;
  double pixelPaddingX = pixelPadding + shOff[0];
  double pixelPaddingY = pixelPadding - shOff[1];

  // Get image from cache
  QImage* img = 0;
  QRectF bounds;
  vtkQtLabelMapEntry key;
  key.Font = fontSpec;
  key.Text = text;
  key.Color = textColor;
  if (this->Implementation->Cache.contains(key) && this->Implementation->Cache[key].Image.width() > 0)
  {
    img = &this->Implementation->Cache[key].Image;
    bounds = this->Implementation->Cache[key].Bounds;
  }
  else
  {
    QPainterPath path;
    path.addText(0, 0, fontSpec, text);
    bounds = path.boundingRect();
    this->Implementation->Cache[key].Bounds = bounds;
    bounds.setWidth(bounds.width() + pixelPaddingX);
    bounds.setHeight(bounds.height() + pixelPaddingY);
    QTransform trans;
    trans.rotate(rotation);
    QRectF rotBounds = trans.mapRect(bounds);
    this->Implementation->Cache[key].Image = QImage(static_cast<int>(rotBounds.width()), static_cast<int>(rotBounds.height()), QImage::Format_ARGB32_Premultiplied);
    img = &this->Implementation->Cache[key].Image;
    img->fill(qRgba(0,0,0,0));
    QPainter p(img);
    p.translate(-rotBounds.left(), -rotBounds.top());
    p.rotate(rotation);
    p.setRenderHint(QPainter::TextAntialiasing, this->AntialiasText);
    p.setRenderHint(QPainter::Antialiasing, this->AntialiasText);

    if (tprop->GetShadow())
    {
      p.save();
      p.translate(shOff[0], -shOff[1]);
      double sc[3];
      tprop->GetShadowColor(sc);
      QColor shadowColor = this->Implementation->TextPropertyToColor(sc, tprop->GetOpacity());
      p.fillPath(path, shadowColor);
      p.restore();
    }

    p.fillPath(path, textColor);
  }

  QPainter* painter = this->Implementation->Painter;

  double delta_x = 0.;
  switch( tprop->GetJustification() )
  {
    case VTK_TEXT_LEFT:
      delta_x = bounds.width()/2.0;
      break;
    case VTK_TEXT_CENTERED:
      break;
    case VTK_TEXT_RIGHT:
      delta_x = -bounds.width()/2.0;
      break;
  }

  double delta_y = pixelPadding / 2.0;
  switch (tprop->GetVerticalJustification())
  {
    case VTK_TEXT_TOP:
      delta_y += bounds.height()/2.0;
      break;
    case VTK_TEXT_CENTERED:
      break;
    case VTK_TEXT_BOTTOM:
      delta_y += -bounds.height()/2.0;
      break;
  }

  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double h = size[1]-1;
  double line_offset = tprop->GetLineOffset();

  QRectF imgRect;
  imgRect.setSize(img->size());

  painter->save();
  painter->translate(x[0], h-x[1]);
  painter->translate(-imgRect.width()/2.0, -imgRect.height()/2.0);
  painter->rotate(rotation);
  painter->translate(delta_x, delta_y);
  painter->rotate(-rotation);
  painter->translate(0., line_offset);
  painter->drawImage(imgRect, *img, imgRect);
  painter->restore();

  //timer->StopTimer();
  //render_label_time += timer->GetElapsedTime();
  //render_label_iter++;
  //if (render_label_iter % 100 == 0)
  //  {
  //  cerr << "RenderLabel time: " << (render_label_time / render_label_iter) << endl;
  //  }
}

//double end_frame_time = 0;
//int end_frame_iter = 0;
//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::EndFrame()
{
  //vtkTimerLog* timer = vtkTimerLog::New();
  //timer->StartTimer();
  this->Actor->RenderOverlay(this->Renderer);
  //timer->StopTimer();
  //end_frame_time += timer->GetElapsedTime();
  //end_frame_iter++;
  //if (end_frame_iter % 10 == 0)
  //  {
  //  cerr << "EndFrame time: " << (end_frame_time / end_frame_iter) << endl;
  //  }
}

//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
