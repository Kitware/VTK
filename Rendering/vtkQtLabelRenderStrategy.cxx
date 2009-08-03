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

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QTextDocument>
#include <QTextStream>

vtkCxxRevisionMacro(vtkQtLabelRenderStrategy, "1.1");
vtkStandardNewMacro(vtkQtLabelRenderStrategy);

class vtkQtLabelRenderStrategy::Internals
{
public:
  QImage* Image;
  QPainter* Painter;
};

//----------------------------------------------------------------------------
vtkQtLabelRenderStrategy::vtkQtLabelRenderStrategy()
{
  if(!QApplication::instance())
    {
    int argc = 0;
    new QApplication(argc, 0);
    }
  this->Implementation = new Internals();
  this->Implementation->Image = new QImage(0, 0, QImage::Format_ARGB32);
  this->Implementation->Painter = new QPainter();

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

//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::ComputeLabelBounds(
  vtkTextProperty* tprop, vtkUnicodeString label, double bds[4])
{
  if (!tprop)
    {
    tprop = this->DefaultTextProperty;
    }
  QString textString;
  QTextStream(&textString) << "<span>" << QString::fromUtf8(label.utf8_str()) << "</span>";

  QFont fontSpec(tprop->GetFontFamilyAsString());
  fontSpec.setBold(tprop->GetBold());
  fontSpec.setItalic(tprop->GetItalic());
  fontSpec.setPixelSize(tprop->GetFontSize());

  QTextDocument textDocument;
  textDocument.setDocumentMargin(0);
  textDocument.setDefaultFont(fontSpec);
  textDocument.setHtml(textString);
  QSizeF tsz = textDocument.size();

  bds[0] = 0;
  bds[1] = tsz.width();
  bds[2] = 0;
  bds[3] = tsz.height();

#if 0
  QFontMetrics fontMetric(fontSpec);
  bds[0] = fontMetric.minLeftBearing();
  bds[1] = fontMetric.width(QString::fromUtf8(label.utf8_str()));
  bds[2] = fontMetric.descent();
  bds[3] = fontMetric.height();
#endif

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
}

//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::StartFrame()
{
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

  delete this->Implementation->Painter;

  if (this->Implementation->Image->width() != width ||
      this->Implementation->Image->height() != height)
    {
    delete this->Implementation->Image;
    this->Implementation->Image = new QImage(width, height, QImage::Format_ARGB32);
    this->QImageToImage->SetQImage(this->Implementation->Image);
    this->PlaneSource->SetPoint1(width, 0, 0);
    this->PlaneSource->SetPoint2(0, height, 0);
    }

  this->Implementation->Painter = new QPainter(this->Implementation->Image);

  this->Implementation->Image->fill(qRgba(0,0,0,0));
  this->QImageToImage->Modified();
}

//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::EndFrame()
{
  this->Actor->RenderOverlay(this->Renderer);
}

//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::RenderLabel(
  double pos[3], vtkTextProperty* tprop, vtkUnicodeString label)
{
  vtkSmartPointer<vtkCoordinate> coord = vtkSmartPointer<vtkCoordinate>::New();
  coord->SetCoordinateSystemToWorld();
  coord->SetValue(pos);
  double* x = coord->GetComputedDoubleDisplayValue(this->Renderer);

  //set text properties from LabelTextProperty
  QFont fontSpec(tprop->GetFontFamilyAsString());
  fontSpec.setBold(tprop->GetBold());
  fontSpec.setItalic(tprop->GetItalic());
  fontSpec.setPixelSize(tprop->GetFontSize());
  
  double* fc = tprop->GetColor();

//FIXME - This ensures all label colorings are consistent.  Is this appropriate?
  QString textString, testString;
  QTextStream(&textString) << "<span>" << QString::fromUtf8(label.utf8_str()) << "</span>";
  QTextStream(&testString) << QString::fromUtf8(label.utf8_str());
//end FIXME
  
  //Qt's coordinate system starts at the top left corner of the layout...
  // vtk has been using the text baseline as the starting point, so
  // we need to add a correction factor to account for the difference
  QFontMetrics fontMetric(fontSpec);
  int baseline = fontMetric.ascent();
  
  QTextDocument textDocument;
  textDocument.setDefaultFont(fontSpec);
  QString styleSheet;
  QTextStream(&styleSheet) << "* { color: rgb( " << fc[0]*255 << ", " << fc[1]*255 << ", " << fc[2]*255 << " ) }";
  textDocument.setDefaultStyleSheet(styleSheet);
  textDocument.setHtml(textString);
  QSizeF tsz = textDocument.size();

  double delta_x = 0., delta_y = 0.;
  switch( tprop->GetJustification() )
    {
    case VTK_TEXT_LEFT: 
      break;
    case VTK_TEXT_CENTERED:
//FIXME - The width is not correct for html encodings...
      delta_x = -tsz.width()/2.0;
      break;
    case VTK_TEXT_RIGHT: 
//FIXME - The width is not correct for html encodings...
      delta_x = -tsz.width();
      break;
    }
  
  switch (tprop->GetVerticalJustification())
    {
    case VTK_TEXT_TOP: 
      break;
    case VTK_TEXT_CENTERED:
      delta_y = -tsz.height()/2.0;
      break;
    case VTK_TEXT_BOTTOM: 
      delta_y = -tsz.height();
      break;
    }
  
  //specify the clockwise text rotation angle
  double rotation = -tprop->GetOrientation();
  
  //we need the window height to set the text correctly...  
  int *size = this->Renderer->GetRenderWindow()->GetSize();
  double h = size[1]-1;

  double line_offset = tprop->GetLineOffset();
  QPainter* painter = this->Implementation->Painter;
  if(tprop->GetShadow())
    {
    painter->save();
    
    int shOff[2];
    tprop->GetShadowOffset(shOff);
    
    painter->translate(x[0], h-x[1]);
    painter->rotate(rotation);
    painter->translate(delta_x, delta_y);
    painter->translate(0., line_offset );
    painter->translate(shOff[0], -shOff[1]);
    
    double shadowColor[3];
    tprop->GetShadowColor(shadowColor);
    
    QTextDocument textDocument;
    textDocument.setDefaultFont(fontSpec);
    QString shadowStyleSheet;
    QTextStream(&shadowStyleSheet) << "* { color: rgb( " << shadowColor[0]*255 << ", " << shadowColor[1]*255 << ", " << shadowColor[2]*255 << " ) }";
    textDocument.setDefaultStyleSheet(shadowStyleSheet);
    textDocument.setHtml(textString);
    textDocument.drawContents(painter);
    
    painter->restore();
    }
  
  painter->save();
  painter->translate(x[0], h-x[1]);
  painter->rotate(rotation);
  painter->translate(delta_x, delta_y);
  painter->translate(0., line_offset);

  textDocument.drawContents(painter);
  
  painter->restore();
}

//----------------------------------------------------------------------------
void vtkQtLabelRenderStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
