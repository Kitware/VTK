/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtChartArea.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/// \file vtkQtChartArea.cxx
/// \date February 1, 2008

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#endif

#include "vtkQtChartArea.h"

#include "vtkQtChartAxisLayer.h"
#include "vtkQtChartAxisOptions.h"
#include "vtkQtChartBasicStyleManager.h"
#include "vtkQtChartColorGenerator.h"
#include "vtkQtChartColorStyleGenerator.h"
#include "vtkQtChartContentsSpace.h"
#include "vtkQtChartGridLayer.h"
#include "vtkQtChartInteractor.h"
#include "vtkQtChartLayer.h"
#include "vtkQtChartMouseBox.h"
#include "vtkQtChartScene.h"
#include "vtkQtChartStyleManager.h"

#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QEvent>
#include <QGraphicsItem>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QToolTip>

// Include OpenGL support if possible
#include <qglobal.h>
#if defined(VTK_USE_QVTK_QTOPENGL) && (QT_EDITION & QT_MODULE_OPENGL)

#include <QGLWidget>

#endif


class vtkQtChartAreaInternal
{
public:
  vtkQtChartAreaInternal();
  ~vtkQtChartAreaInternal() {}

  QList<vtkQtChartLayer *> Layers;
  vtkQtChartAxisLayer *AxisLayer;
  vtkQtChartLayer *GridLayer;
  vtkQtChartMouseBox *MouseBox;
  vtkQtChartContentsSpace *Contents;
  vtkQtChartInteractor *Interactor;
  vtkQtChartBasicStyleManager *StyleDefault;
  vtkQtChartStyleManager *StyleManager;
  bool InResize;           ///< True if the widget is resizing.
  bool InZoom;             ///< True if handling a zoom layout.
  bool SkipContextMenu;    ///< Used for context menu interaction.
  bool DelayContextMenu;   ///< Used for context menu interaction.
  bool ContextMenuBlocked; ///< Used for context menu interaction.
  bool LayoutPending;      ///< Used to delay chart layout.
  bool InteractiveResize;  ///< True when in a resize interaction.
};


//-----------------------------------------------------------------------------
vtkQtChartAreaInternal::vtkQtChartAreaInternal()
  : Layers()
{
  this->AxisLayer = 0;
  this->GridLayer = 0;
  this->MouseBox = 0;
  this->Contents = 0;
  this->Interactor = 0;
  this->StyleDefault = 0;
  this->StyleManager = 0;
  this->InResize = false;
  this->InZoom = false;
  this->SkipContextMenu = false;
  this->DelayContextMenu = false;
  this->ContextMenuBlocked = false;
  this->LayoutPending = false;
  this->InteractiveResize = false;
}


//-----------------------------------------------------------------------------
vtkQtChartArea::vtkQtChartArea(QWidget *widgetParent)
  : QGraphicsView(widgetParent)
{
  this->Internal = new vtkQtChartAreaInternal();
  this->Internal->Contents = new vtkQtChartContentsSpace(this);
  this->Internal->Contents->setObjectName("ContentsSpace");

  // Set up the default style manager.
  this->Internal->StyleDefault = new vtkQtChartBasicStyleManager(this);
  this->Internal->StyleDefault->setObjectName("BasicStyleManager");
  this->Internal->StyleManager = this->Internal->StyleDefault;

  vtkQtChartColorGenerator *seriesBrush = new vtkQtChartColorGenerator(
      this->Internal->StyleDefault);
  seriesBrush->setColors(this->Internal->StyleDefault->getColors());
  this->Internal->StyleDefault->setGenerator("Brush", seriesBrush);

  vtkQtChartColorStyleGenerator *seriesPen = new vtkQtChartColorStyleGenerator(
      this->Internal->StyleDefault);
  seriesPen->setColors(this->Internal->StyleDefault->getColors());
  this->Internal->StyleDefault->setGenerator("Pen", seriesPen);

  // Set up the graphics scene.
  vtkQtChartScene *chartScene = new vtkQtChartScene(this);
  this->setScene(chartScene);

#if defined(VTK_USE_QVTK_QTOPENGL) && (QT_EDITION & QT_MODULE_OPENGL)
  // Use the OpenGL widget if possible
  this->setUseOpenGLIfAvailable(true);
#endif

  // Set up the axis and grid layers.
  this->Internal->AxisLayer = new vtkQtChartAxisLayer();
  this->Internal->AxisLayer->setObjectName("Axis Layer");
  this->Internal->GridLayer = new vtkQtChartGridLayer();
  this->Internal->GridLayer->setObjectName("Grid Layer");
  this->addLayer(this->Internal->GridLayer);
  this->addLayer(this->Internal->AxisLayer);
  this->Internal->AxisLayer->cancelChartRangeChange();

  // Set up the mouse box.
  this->Internal->MouseBox = new vtkQtChartMouseBox(this);
  chartScene->setMouseBox(this->Internal->MouseBox);
  this->connect(this->Internal->MouseBox, SIGNAL(updateNeeded(const QRectF &)),
      chartScene, SLOT(update(const QRectF &)));

  // Hide the scrollbars and the frame.
  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setFrameShadow(QFrame::Plain);
  this->setFrameShape(QFrame::NoFrame);

  // Listen for zoom/pan changes.
  this->connect(this->Internal->Contents, SIGNAL(xOffsetChanged(float)),
      this, SLOT(update()));
  this->connect(this->Internal->Contents, SIGNAL(yOffsetChanged(float)),
      this, SLOT(update()));
  this->connect(this->Internal->Contents, SIGNAL(maximumChanged(float, float)),
      this, SLOT(handleZoomChange()));

  // Link the layout needed signal to the delay mechanism.
  this->connect(this, SIGNAL(delayedLayoutNeeded()),
      this, SLOT(layoutChart()), Qt::QueuedConnection);

  this->setRenderHints(QPainter::Antialiasing);
}

vtkQtChartArea::~vtkQtChartArea()
{
  delete this->Internal->MouseBox;
  delete this->Internal;
}

void vtkQtChartArea::addLayer(vtkQtChartLayer *chart)
{
  this->insertLayer(this->Internal->Layers.size(), chart);
}

void vtkQtChartArea::insertLayer(int index, vtkQtChartLayer *chart)
{
  // Make sure the chart isn't in the list already.
  if(!chart || this->Internal->Layers.contains(chart))
    {
    return;
    }

  // Make sure the index is valid.
  if(index < 0)
    {
    index = 0;
    }
  else if(index > this->Internal->Layers.size())
    {
    index = this->Internal->Layers.size();
    }

  // Add the chart to the scene and set the z-order.
  this->scene()->addItem(chart);
  chart->setZValue(index);

  // Add the chart to the list of layers.
  if(index == this->Internal->Layers.size())
    {
    this->Internal->Layers.append(chart);
    }
  else
    {
    this->Internal->Layers.insert(index, chart);

    // Fix the z-order for the following items.
    for(int i = index + 1; i < this->Internal->Layers.size(); i++)
      {
      this->Internal->Layers[i]->setZValue(i);
      }
    }

  // Listen for the chart update signals.
  this->connect(chart, SIGNAL(layoutNeeded()), this, SLOT(updateLayout()));
  this->connect(chart, SIGNAL(rangeChanged()),
      this->Internal->AxisLayer, SLOT(handleChartRangeChange()));
  this->Internal->AxisLayer->handleChartRangeChange();

  // Set the layer's reference to the chart area.
  chart->setChartArea(this);

  emit this->layerInserted(index, chart);
}

void vtkQtChartArea::removeLayer(vtkQtChartLayer *chart)
{
  // Get the index for the chart layer. If the chart is not in the
  // layer list, ignore the request.
  int index = this->Internal->Layers.indexOf(chart);
  if(index == -1)
    {
    return;
    }

  // Remove the chart layer from the list.
  emit this->removingLayer(index, chart);
  this->Internal->Layers.removeAt(index);

  // Remove the chart from the scene.
  this->scene()->removeItem(chart);

  // Fix the z-order for the following items.
  for(int i = index; i < this->Internal->Layers.size(); i++)
    {
    this->Internal->Layers[i]->setZValue(i);
    }

  this->disconnect(chart, 0, this, 0);
  this->disconnect(chart, 0, this->Internal->AxisLayer, 0);
  this->Internal->AxisLayer->handleChartRangeChange();

  // Remove the layer's reference to the chart area.
  chart->setChartArea(0);

  emit this->layerRemoved(index, chart);
}

int vtkQtChartArea::getNumberOfLayers() const
{
  return this->Internal->Layers.size();
}

vtkQtChartLayer *vtkQtChartArea::getLayer(int index) const
{
  if(index >= 0 && index < this->Internal->Layers.size())
    {
    return this->Internal->Layers[index];
    }

  return 0;
}

vtkQtChartLayer *vtkQtChartArea::getLayer(const QString &chartName) const
{
  QList<vtkQtChartLayer *>::Iterator layer = this->Internal->Layers.begin();
  for( ; layer != this->Internal->Layers.end(); ++layer)
    {
    if((*layer)->objectName() == chartName)
      {
      return *layer;
      }
    }

  return 0;
}

vtkQtChartAxisLayer *vtkQtChartArea::getAxisLayer() const
{
  return this->Internal->AxisLayer;
}

int vtkQtChartArea::getAxisLayerIndex() const
{
  return this->Internal->Layers.indexOf(this->Internal->AxisLayer);
}

int vtkQtChartArea::getGridLayerIndex() const
{
  return this->Internal->Layers.indexOf(this->Internal->GridLayer);
}

vtkQtChartInteractor *vtkQtChartArea::getInteractor() const
{
  return this->Internal->Interactor;
}

void vtkQtChartArea::setInteractor(vtkQtChartInteractor *interactor)
{
  if(this->Internal->Interactor)
    {
    this->Internal->Interactor->setChartArea(0);
    this->disconnect(this->Internal->Interactor, 0, this, 0);
    }

  this->Internal->Interactor = interactor;
  if(this->Internal->Interactor)
    {
    this->Internal->Interactor->setChartArea(this);
    this->connect(this->Internal->Interactor,
        SIGNAL(cursorChangeRequested(const QCursor &)),
        this, SLOT(changeCursor(const QCursor &)));
    }
}

vtkQtChartContentsSpace *vtkQtChartArea::getContentsSpace() const
{
  return this->Internal->Contents;
}

vtkQtChartMouseBox *vtkQtChartArea::getMouseBox() const
{
  return this->Internal->MouseBox;
}

void vtkQtChartArea::startInteractiveResize()
{
  if(!this->Internal->InteractiveResize)
    {
    this->Internal->InteractiveResize = true;
    QList<vtkQtChartLayer *>::Iterator layer = this->Internal->Layers.begin();
    for( ; layer != this->Internal->Layers.end(); ++layer)
      {
      (*layer)->startInteractiveResize();
      }
    }
}

bool vtkQtChartArea::isInteractivelyResizing() const
{
  return this->Internal->InteractiveResize;
}

void vtkQtChartArea::finishInteractiveResize()
{
  if(this->Internal->InteractiveResize)
    {
    this->Internal->InteractiveResize = false;
    QList<vtkQtChartLayer *>::Iterator layer = this->Internal->Layers.begin();
    for( ; layer != this->Internal->Layers.end(); ++layer)
      {
      (*layer)->finishInteractiveResize();
      }
    }
}

vtkQtChartStyleManager *vtkQtChartArea::getStyleManager() const
{
  return this->Internal->StyleManager;
}

void vtkQtChartArea::setStyleManager(vtkQtChartStyleManager *manager)
{
  this->Internal->StyleManager = manager;
  if(this->Internal->StyleManager == 0)
    {
    this->Internal->StyleManager = this->Internal->StyleDefault;
    }
}

void vtkQtChartArea::layoutChart()
{
  if(!(this->Internal->InResize || this->Internal->InZoom))
    {
    this->Internal->LayoutPending = false;
    }

  // Lay out the axis layer first to set up the axis scales.
  this->Internal->AxisLayer->layoutChart(this->rect());

  // Get the chart contents area.
  QRectF chartBounds = this->Internal->AxisLayer->getLayerBounds();
  this->Internal->Contents->setChartLayerBounds(chartBounds);

  // Lay out each of the chart layers.
  QList<vtkQtChartLayer *>::Iterator layer = this->Internal->Layers.begin();
  for( ; layer != this->Internal->Layers.end(); ++layer)
    {
    if(*layer == this->Internal->AxisLayer)
      {
      continue;
      }

    (*layer)->layoutChart(chartBounds);
    }

  this->update();
}

void vtkQtChartArea::updateLayout()
{
  if(!this->Internal->LayoutPending)
    {
    this->Internal->LayoutPending = true;
    emit this->delayedLayoutNeeded();
    }
}

bool vtkQtChartArea::viewportEvent(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    // Set the font for each of the axes. The layout requests will be
    // compressed into one layout.
    vtkQtChartAxis *axis =
        this->Internal->AxisLayer->getAxis(vtkQtChartAxis::Left);
    axis->getOptions()->setLabelFont(this->font());
    axis = this->Internal->AxisLayer->getAxis(vtkQtChartAxis::Bottom);
    axis->getOptions()->setLabelFont(this->font());
    axis = this->Internal->AxisLayer->getAxis(vtkQtChartAxis::Right);
    axis->getOptions()->setLabelFont(this->font());
    axis = this->Internal->AxisLayer->getAxis(vtkQtChartAxis::Top);
    axis->getOptions()->setLabelFont(this->font());
    }
  else if(e->type() == QEvent::ContextMenu)
    {
    QContextMenuEvent *cme = static_cast<QContextMenuEvent *>(e);
    if(cme->reason() == QContextMenuEvent::Mouse &&
        (this->Internal->SkipContextMenu || this->Internal->DelayContextMenu))
      {
      this->Internal->SkipContextMenu = false;
      if(this->Internal->DelayContextMenu)
        {
        this->Internal->ContextMenuBlocked = true;
        }

      e->accept();
      return true;
      }
    }
  else if(e->type() == QEvent::ToolTip)
    {
    QHelpEvent *he = static_cast<QHelpEvent *>(e);
    QPointF location = this->mapToScene(he->pos());
    int i = this->Internal->Layers.size() - 1;
    for( ; i >= 0; i--)
      {
      QString text;
      if(this->Internal->Layers[i]->getHelpText(location, text))
        {
        QToolTip::showText(he->globalPos(), text, this);
        break;
        }
      }

    // Clear the tooltip if no item was found.
    if(i < 0)
      {
      QToolTip::hideText();
      }

    he->accept();
    return true;
    }

  return QGraphicsView::viewportEvent(e);
}

void vtkQtChartArea::resizeEvent(QResizeEvent *e)
{
  this->Internal->InResize = true;
  this->scene()->setSceneRect(0, 0, e->size().width(), e->size().height());
  this->Internal->Contents->setChartSize(e->size().width(), e->size().height());
  this->layoutChart();
  this->Internal->InResize = false;
}

void vtkQtChartArea::keyPressEvent(QKeyEvent *e)
{
  bool handled = false;
  if(this->Internal->Interactor)
    {
    handled = this->Internal->Interactor->keyPressEvent(e);
    }

  if(handled)
    {
    e->accept();
    }
  else
    {
    QGraphicsView::keyPressEvent(e);
    }
}

void vtkQtChartArea::mousePressEvent(QMouseEvent *e)
{
  // Set the mouse box position in scene coordinates.
  this->Internal->MouseBox->setStartingPosition(e->pos());

  // If the mouse button is the right button, delay the context menu.
  if(e->button() == Qt::RightButton)
    {
    this->Internal->DelayContextMenu = true;
    }

  // Let the interactor handle the rest of the event.
  if(this->Internal->Interactor)
    {
    this->Internal->Interactor->mousePressEvent(e);
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartArea::mouseMoveEvent(QMouseEvent *e)
{
  // When the mouse is moved, the context menu should not pop-up.
  if(e->buttons() & Qt::RightButton)
    {
    this->Internal->SkipContextMenu = true;
    this->Internal->DelayContextMenu = false;
    }

  if(this->Internal->Interactor)
    {
    this->Internal->Interactor->mouseMoveEvent(e);
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartArea::mouseReleaseEvent(QMouseEvent *e)
{
  if(this->Internal->Interactor)
    {
    this->Internal->Interactor->mouseReleaseEvent(e);
    }
  else
    {
    e->ignore();
    }

  if(e->button() == Qt::RightButton)
    {
    if(this->Internal->ContextMenuBlocked)
      {
      if(this->Internal->SkipContextMenu)
        {
        this->Internal->SkipContextMenu = false;
        }
      else if(this->Internal->DelayContextMenu)
        {
        // Re-send the context menu event.
        QContextMenuEvent *cme = new QContextMenuEvent(
            QContextMenuEvent::Mouse, e->pos(), e->globalPos());
        QCoreApplication::postEvent(this, cme);
        }
      }

    this->Internal->ContextMenuBlocked = false;
    this->Internal->DelayContextMenu = false;
    }
}

void vtkQtChartArea::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(this->Internal->Interactor)
    {
    this->Internal->Interactor->mouseDoubleClickEvent(e);
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartArea::wheelEvent(QWheelEvent *e)
{
  if(this->Internal->Interactor)
    {
    this->Internal->Interactor->wheelEvent(e);
    }
  else
    {
    e->ignore();
    }
}

void vtkQtChartArea::handleZoomChange()
{
  if(!this->Internal->InResize)
    {
    this->Internal->InZoom = true;
    this->layoutChart();
    this->Internal->InZoom = false;
    }
}

void vtkQtChartArea::changeCursor(const QCursor &newCursor)
{
  this->setCursor(newCursor);
}

#include <iostream>
using namespace std;
void vtkQtChartArea::setUseOpenGLIfAvailable(bool enable)
{
#if defined(VTK_USE_QVTK_QTOPENGL) && (QT_EDITION & QT_MODULE_OPENGL)
  if (QGLFormat::hasOpenGL() && enable)
    {
    if (qobject_cast<QGLWidget*>(this->viewport()) == NULL)
      {
      this->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
      }
    }
  else 
    {
    if (qobject_cast<QGLWidget*>(this->viewport()) != NULL)
      {
      this->setViewport(new QWidget());
      }
    }
#endif
  (void)enable;
  // not compiled with OpenGL, nothing to do.
}


